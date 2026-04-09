#ifndef __EDGEPULSE_CONFIG_H__
#define __EDGEPULSE_CONFIG_H__

/*
 * Project identity
 */
#define EDGE_PROJECT_NAME              "EdgePulse RTOS Monitor"
#define EDGE_PROJECT_TITLE_CN          "边缘脉冲实时监控站"
#define EDGE_DEVICE_ROLE               "iot-gateway"

/*
 * Network profile
 */
#define EDGE_USE_DHCP                  1
#define EDGE_STATIC_IP0                192
#define EDGE_STATIC_IP1                168
#define EDGE_STATIC_IP2                10
#define EDGE_STATIC_IP3                40

#define EDGE_NETMASK0                  255
#define EDGE_NETMASK1                  255
#define EDGE_NETMASK2                  255
#define EDGE_NETMASK3                  0

#define EDGE_GATEWAY0                  192
#define EDGE_GATEWAY1                  168
#define EDGE_GATEWAY2                  10
#define EDGE_GATEWAY3                  1

/*
 * Web profile
 */
#define EDGE_HTTP_PORT                 80
#define EDGE_DASHBOARD_REFRESH_MS      1000U

/*
 * External service placeholders
 * Update these values when integrating with backend services.
 */
#define EDGE_DB_HOST                   "192.168.10.20"
#define EDGE_DB_PORT                   5432
#define EDGE_REDIS_HOST                "192.168.10.21"
#define EDGE_REDIS_PORT                6379
#define EDGE_OLLAMA_API_URL            "http://192.168.10.30:11434"
#define EDGE_TELEMETRY_API_URL         "http://192.168.10.40:9000/api/v1/telemetry"

#endif /* __EDGEPULSE_CONFIG_H__ */
