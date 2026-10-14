// cw_weather.c — Open-Meteo 天气客户端实现。
#include "cw_weather.h"

#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "cw_weather";

// 内置城市经纬度表
static const cw_city_geo_t CITY_GEO[] = {
    {"Vancouver", 49.2827, -123.1207},
    {"Toronto", 43.6532, -79.3832},
    {"Montreal", 45.5017, -73.5673},
    {"Calgary", 51.0447, -114.0719},
    {"Ottawa", 45.4215, -75.6972},
    {"Beijing", 39.9042, 116.4074},
    {"Shanghai", 31.2304, 121.4737},
    {"Guangzhou", 23.1291, 113.2644},
    {"Shenzhen", 22.5431, 114.0579},
    {"Tokyo", 35.6762, 139.6503},
    {"New York", 40.7128, -74.0060},
    {"London", 51.5074, -0.1278},
};

static const cw_city_geo_t *find_city(const char *query) {
    if (!query) return NULL;
    for (int i = 0; i < (int)(sizeof(CITY_GEO) / sizeof(CITY_GEO[0])); i++) {
        if (strcasecmp(CITY_GEO[i].query, query) == 0) {
            return &CITY_GEO[i];
        }
    }
    return NULL;
}

cw_wx_cond_t cw_weather_code_to_cond(int wmo_code) {
    if (wmo_code == 0) return CW_WX_CLEAR;
    if (wmo_code <= 3) return CW_WX_PARTLY_CLOUDY;
    if (wmo_code <= 48) return CW_WX_FOG;
    if (wmo_code <= 57) return CW_WX_DRIZZLE;
    if (wmo_code <= 67) return CW_WX_RAIN;
    if (wmo_code <= 77) return CW_WX_SNOW;
    if (wmo_code <= 82) return CW_WX_RAIN;
    if (wmo_code <= 86) return CW_WX_SNOW;
    if (wmo_code <= 99) return CW_WX_THUNDERSTORM;
    return CW_WX_UNKNOWN;
}

bool cw_weather_fetch(const char *city_query, cw_weather_data_t *data) {
    if (!city_query || !data) return false;

    const cw_city_geo_t *geo = find_city(city_query);
    if (!geo) {
        ESP_LOGW(TAG, "Unknown city: %s", city_query);
        return false;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "%s?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m&daily=temperature_2m_max,temperature_2m_min&timezone=auto",
             CW_WEATHER_API_BASE, geo->lat, geo->lon);

    ESP_LOGI(TAG, "Fetching weather: %s", url);

    char resp_buf[2048];
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    esp_err_t err = esp_http_client_perform(client);
    bool ok = false;

    if (err == ESP_OK) {
        int len = esp_http_client_get_content_length(client);
        if (len > 0 && len < (int)sizeof(resp_buf)) {
            int read = esp_http_client_read(client, resp_buf, sizeof(resp_buf) - 1);
            if (read > 0) {
                resp_buf[read] = '\0';
                // 解析 JSON
                cJSON *root = cJSON_Parse(resp_buf);
                if (root) {
                    cJSON *current = cJSON_GetObjectItem(root, "current");
                    cJSON *daily = cJSON_GetObjectItem(root, "daily");
                    if (current && daily) {
                        cJSON *t = cJSON_GetObjectItem(current, "temperature_2m");
                        cJSON *h = cJSON_GetObjectItem(current, "relative_humidity_2m");
                        cJSON *w = cJSON_GetObjectItem(current, "weather_code");
                        cJSON *ws = cJSON_GetObjectItem(current, "wind_speed_10m");
                        cJSON *hi = cJSON_GetObjectItem(daily, "temperature_2m_max");
                        cJSON *lo = cJSON_GetObjectItem(daily, "temperature_2m_min");

                        if (cJSON_IsNumber(t)) data->temp_c = (int)t->valuedouble;
                        if (cJSON_IsNumber(h)) data->humidity = (int)h->valuedouble;
                        if (cJSON_IsNumber(w)) data->cond = cw_weather_code_to_cond(w->valueint);
                        if (cJSON_IsNumber(ws)) data->wind_kph = (int)ws->valuedouble;
                        if (cJSON_IsArray(hi) && cJSON_GetArraySize(hi) > 0) {
                            data->temp_hi_c = (int)cJSON_GetArrayItem(hi, 0)->valuedouble;
                        }
                        if (cJSON_IsArray(lo) && cJSON_GetArraySize(lo) > 0) {
                            data->temp_lo_c = (int)cJSON_GetArrayItem(lo, 0)->valuedouble;
                        }
                        ok = true;
                    }
                    cJSON_Delete(root);
                }
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return ok;
}
