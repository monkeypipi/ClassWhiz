// cw_weather.h — 天气服务:Open-Meteo API 客户端。
//
// 服务地址: https://api.open-meteo.com/v1/forecast
// 免费,无需 API Key,支持经纬度查询。
// 注意:需要设备处于 STA 模式联网才能调用;AP 配置模式下使用演示数据。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Open-Meteo API 基础地址
#define CW_WEATHER_API_BASE "https://api.open-meteo.com/v1/forecast"

// 天气状况
typedef enum {
    CW_WX_CLEAR = 0,
    CW_WX_PARTLY_CLOUDY,
    CW_WX_CLOUDY,
    CW_WX_FOG,
    CW_WX_DRIZZLE,
    CW_WX_RAIN,
    CW_WX_SNOW,
    CW_WX_THUNDERSTORM,
    CW_WX_UNKNOWN,
} cw_wx_cond_t;

// 天气数据
typedef struct {
    int temp_c;           // 当前温度(摄氏)
    int temp_hi_c;        // 今日最高
    int temp_lo_c;        // 今日最低
    cw_wx_cond_t cond;    // 天气状况
    int humidity;         // 湿度 %
    int wind_kph;         // 风速
} cw_weather_data_t;

// 城市经纬度(内置常用城市,网页配置的城市名在此查表)
typedef struct {
    const char *query;    // 英文名(网页配置用)
    double lat;
    double lon;
} cw_city_geo_t;

// 从 Open-Meteo 获取天气(需要联网)
// city_query: 城市英文名,如 "Vancouver"
// 返回 true 表示成功,data 有效
bool cw_weather_fetch(const char *city_query, cw_weather_data_t *data);

// WMO 天气代码转状况
cw_wx_cond_t cw_weather_code_to_cond(int wmo_code);

#ifdef __cplusplus
}
#endif
