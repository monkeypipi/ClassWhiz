// cw_wifi_portal.h — Wi-Fi 配置门户:AP 模式 + Captive Portal + HTTP 服务器。
//
// 用户连上 "ClassWhiz-XXXX" 热点后,任意网址都会跳转到 192.168.4.1 的配置页。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 启动配置门户(AP + DNS 劫持 + HTTP)
// 返回 true 表示启动成功
bool cw_portal_start(void);

// 停止配置门户,恢复正常模式
void cw_portal_stop(void);

// 门户是否运行中
bool cw_portal_is_running(void);

// AP 的 SSID(供 UI 显示)
const char *cw_portal_ssid(void);

// AP 的 IP 地址(固定 192.168.4.1)
const char *cw_portal_ip(void);

#ifdef __cplusplus
}
#endif
