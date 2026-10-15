// cw_wifi_portal.c — Wi-Fi 配置门户实现。
#include "cw_wifi_portal.h"
#include "cw_config.h"
#include "cw_data.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "cw_portal_page.h"
#include "cw_photo.h"
#include "cw_app.h"
#include "cw_store.h"

static const char *TAG = "cw_portal";
static bool s_running = false;
static httpd_handle_t s_httpd = NULL;
static esp_netif_t *s_ap_netif = NULL;
static int s_dns_sock = -1;
static char s_ssid[32] = {0};

// AP IP
#define PORTAL_IP "192.168.4.1"
#define DNS_PORT 53

// ---------- DNS 劫持服务器 ----------
// 响应所有 DNS 查询,返回 AP IP,实现 Captive Portal

static void dns_server_task(void *pvParameters) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    uint8_t rx_buf[512];
    uint8_t tx_buf[512];

    s_dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_dns_sock < 0) {
        ESP_LOGE(TAG, "DNS socket failed");
        vTaskDelete(NULL);
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DNS_PORT);

    if (bind(s_dns_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "DNS bind failed");
        close(s_dns_sock);
        s_dns_sock = -1;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "DNS hijack server running");
    while (s_running) {
        int len = recvfrom(s_dns_sock, rx_buf, sizeof(rx_buf), 0,
                          (struct sockaddr *)&client_addr, &client_len);
        if (len < 12) continue; // DNS 头至少 12 字节

        // 构造 DNS 响应:复制查询,设置应答标志,添加 A 记录指向 192.168.4.1
        memcpy(tx_buf, rx_buf, len);
        tx_buf[2] |= 0x80; // QR=1 (响应)
        tx_buf[3] |= 0x80; // RA=1
        // ANCOUNT=1
        tx_buf[6] = 0; tx_buf[7] = 1;

        // 找到查询名结束位置(以 0 结尾)
        int pos = 12;
        while (pos < len && rx_buf[pos] != 0) {
            pos += rx_buf[pos] + 1;
        }
        pos += 5; // 跳过 0 + QTYPE(2) + QCLASS(2)

        if (pos + 16 <= (int)sizeof(tx_buf)) {
            // Answer: NAME(指针 0xC00C), TYPE=A(1), CLASS=IN(1), TTL=60, RDLENGTH=4, RDATA=192.168.4.1
            tx_buf[pos++] = 0xC0; tx_buf[pos++] = 0x0C;
            tx_buf[pos++] = 0x00; tx_buf[pos++] = 0x01;
            tx_buf[pos++] = 0x00; tx_buf[pos++] = 0x01;
            tx_buf[pos++] = 0x00; tx_buf[pos++] = 0x00; tx_buf[pos++] = 0x00; tx_buf[pos++] = 0x3C;
            tx_buf[pos++] = 0x00; tx_buf[pos++] = 0x04;
            tx_buf[pos++] = 192; tx_buf[pos++] = 168; tx_buf[pos++] = 4; tx_buf[pos++] = 1;

            sendto(s_dns_sock, tx_buf, pos, 0,
                   (struct sockaddr *)&client_addr, client_len);
        }
    }

    close(s_dns_sock);
    s_dns_sock = -1;
    vTaskDelete(NULL);
}

// ---------- HTTP 处理 ----------

static esp_err_t root_handler(httpd_req_t *req) {
    extern const char CW_PORTAL_HTML[];
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, CW_PORTAL_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t api_config_get_handler(httpd_req_t *req) {
    static char json_buf[8192];
    if (!cw_config_to_json(json_buf, sizeof(json_buf))) {
        httpd_resp_set_status(req, "500 Internal Error");
        httpd_resp_send(req, "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    // 注入 settings
    cJSON *root = cJSON_Parse(json_buf);
    if (root) {
        cw_settings_t *s = cw_app_settings();
        cJSON *st = cJSON_CreateObject();
        cJSON_AddNumberToObject(st, "lang", s->lang);
        cJSON_AddNumberToObject(st, "time24", s->time_24h ? 1 : 0);
        cJSON_AddNumberToObject(st, "tempC", s->temp_celsius ? 1 : 0);
        cJSON_AddItemToObject(root, "settings", st);
        char *out = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        if (out) {
            httpd_resp_set_type(req, "application/json; charset=utf-8");
            httpd_resp_send(req, out, HTTPD_RESP_USE_STRLEN);
            cJSON_free(out);
            return ESP_OK;
        }
    }
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_send(req, json_buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t api_config_post_handler(httpd_req_t *req) {
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_set_status(req, "500 Internal Error");
        httpd_resp_send(req, "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    buf[ret] = '\0';

    bool ok = cw_config_from_json(buf);

    // 解析 settings 并应用
    if (ok) {
        cJSON *root = cJSON_Parse(buf);
        if (root) {
            cJSON *st = cJSON_GetObjectItem(root, "settings");
            if (cJSON_IsObject(st)) {
                cw_settings_t *s = cw_app_settings();
                cJSON *lang = cJSON_GetObjectItem(st, "lang");
                cJSON *time24 = cJSON_GetObjectItem(st, "time24");
                cJSON *tempC = cJSON_GetObjectItem(st, "tempC");
                if (cJSON_IsNumber(lang)) s->lang = (cw_lang_t)lang->valueint;
                if (cJSON_IsNumber(time24)) s->time_24h = time24->valueint ? true : false;
                if (cJSON_IsNumber(tempC)) s->temp_celsius = tempC->valueint ? true : false;
                cw_set_lang(s->lang);
                cw_app_mark_dirty();
            }
            cJSON_Delete(root);
        }
    }
    free(buf);

    if (ok) {
        cw_config_save();
        cw_data_sync_views();
        ESP_LOGI(TAG, "Config updated via web, saved to NVS");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, ok ? "{\"ok\":true}" : "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 恢复出厂:POST /api/reset
static esp_err_t api_reset_handler(httpd_req_t *req) {
    (void)req;
    cw_config_reset_defaults();
    cw_data_sync_views();
    ESP_LOGI(TAG, "Config reset to defaults via web");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 照片上传:POST /api/photo, Content-Type: application/octet-stream
// body 为 96x96 RGB565 raw (18432 字节,由网页端转换)
static esp_err_t api_photo_post_handler(httpd_req_t *req) {
    if (req->content_len != CW_PHOTO_SIZE) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "{\"ok\":false,\"err\":\"bad size\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    uint8_t *buf = malloc(req->content_len);
    if (!buf) {
        httpd_resp_set_status(req, "500 Internal Error");
        httpd_resp_send(req, "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    int received = 0;
    while (received < req->content_len) {
        int ret = httpd_req_recv(req, (char *)buf + received, req->content_len - received);
        if (ret <= 0) break;
        received += ret;
    }

    bool ok = false;
    if (received == req->content_len) {
        ok = cw_photo_save(buf, received);
        if (ok) {
            // 更新学生证 has_photo 标记
            cw_student_mut_t *s = cw_config_student_mut();
            s->has_photo = true;
            cw_config_save();
        }
    }
    free(buf);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, ok ? "{\"ok\":true}" : "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Captive Portal 检测:各系统的连通性检查 URL 都重定向到配置页
static esp_err_t captive_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://" PORTAL_IP "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static void start_httpd(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 12;

    if (httpd_start(&s_httpd, &config) != ESP_OK) {
        ESP_LOGE(TAG, "HTTPD start failed");
        return;
    }

    httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_handler};
    httpd_register_uri_handler(s_httpd, &root);

    httpd_uri_t api_get = {.uri = "/api/config", .method = HTTP_GET, .handler = api_config_get_handler};
    httpd_register_uri_handler(s_httpd, &api_get);

    httpd_uri_t api_post = {.uri = "/api/config", .method = HTTP_POST, .handler = api_config_post_handler};
    httpd_register_uri_handler(s_httpd, &api_post);

    httpd_uri_t api_photo = {.uri = "/api/photo", .method = HTTP_POST, .handler = api_photo_post_handler};
    httpd_register_uri_handler(s_httpd, &api_photo);

    httpd_uri_t api_reset = {.uri = "/api/reset", .method = HTTP_POST, .handler = api_reset_handler};
    httpd_register_uri_handler(s_httpd, &api_reset);

    // 常见 Captive Portal 检测 URL
    const char *captive_urls[] = {
        "/generate_204",           // Android
        "/gen_204",                // Android
        "/hotspot-detect.html",    // iOS
        "/library/test/success.html", // iOS
        "/ncsi.txt",               // Windows
        "/connecttest.txt",        // Windows
    };
    for (int i = 0; i < 6; i++) {
        httpd_uri_t cu = {.uri = captive_urls[i], .method = HTTP_GET, .handler = captive_handler};
        httpd_register_uri_handler(s_httpd, &cu);
    }

    ESP_LOGI(TAG, "HTTP server started on port 80");
}

static void stop_httpd(void) {
    if (s_httpd) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }
}

// ---------- 公共 API ----------

bool cw_portal_start(void) {
    if (s_running) return true;

    // esp_wifi_init 依赖 netif 与默认事件循环;main.c 没有建过,这里补上。
    // 若其它模块已建过则返回 ESP_ERR_INVALID_STATE,属正常,忽略即可。
    // 绝不能用 ESP_ERROR_CHECK 包 Wi-Fi 调用:一次失败就 abort 重启,
    // 表现为“按 OK 回到主页”。所有失败都要记日志并优雅返回 false。
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "event loop create failed: %s", esp_err_to_name(err));
        return false;
    }

    // Wi-Fi AP(netif 只建一次,stop 时销毁,避免反复开关泄漏)
    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        if (s_ap_netif) {
            esp_netif_destroy(s_ap_netif);
            s_ap_netif = NULL;
        }
        return false;
    }

    // 生成 SSID: ClassWhiz-XXXX (后4位用 MAC),须在 esp_wifi_init 之后取 MAC
    uint8_t mac[6] = {0};
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    snprintf(s_ssid, sizeof(s_ssid), "ClassWhiz-%02X%02X", mac[4], mac[5]);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, s_ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(s_ssid);
    wifi_config.ap.channel = 6;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        esp_wifi_deinit();
        if (s_ap_netif) {
            esp_netif_destroy(s_ap_netif);
            s_ap_netif = NULL;
        }
        return false;
    }
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
        esp_wifi_deinit();
        if (s_ap_netif) {
            esp_netif_destroy(s_ap_netif);
            s_ap_netif = NULL;
        }
        return false;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
        esp_wifi_deinit();
        if (s_ap_netif) {
            esp_netif_destroy(s_ap_netif);
            s_ap_netif = NULL;
        }
        return false;
    }

    ESP_LOGI(TAG, "AP started: SSID=%s", s_ssid);

    s_running = true;

    // DNS 劫持
    // dns_server_task 在栈上放了 rx_buf[512]+tx_buf[512]=1KB,再加 esp_log 的
    // vfprintf 开销,2KB 会触发栈保护中断(实测 Guru Meditation),给 4KB。
    xTaskCreate(dns_server_task, "dns_hijack", 4096, NULL, 5, NULL);

    // HTTP 服务器
    start_httpd();

    return true;
}

void cw_portal_stop(void) {
    if (!s_running) return;
    s_running = false;

    stop_httpd();

    // DNS 任务会自己退出(检查 s_running)
    // 给一点时间让任务退出
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_wifi_stop();
    esp_wifi_deinit();

    if (s_ap_netif) {
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
    }

    ESP_LOGI(TAG, "Portal stopped");
}

bool cw_portal_is_running(void) {
    return s_running;
}

const char *cw_portal_ssid(void) {
    return s_ssid;
}

const char *cw_portal_ip(void) {
    return PORTAL_IP;
}
