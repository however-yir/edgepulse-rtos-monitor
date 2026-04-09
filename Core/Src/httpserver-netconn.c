/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c
  * @author  MCD Application Team
  * @brief   Basic http server implementation using LwIP netconn API
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "string.h"
#include "httpserver-netconn.h"
#include "cmsis_os.h"
#include "main.h"
#include "edgepulse_config.h"

#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define WEBSERVER_THREAD_PRIO        (osPriorityAboveNormal)
#define HTTP_REQ_BUFFER_LEN          256U
#define HTTP_ROUTE_BUFFER_LEN         96U
#define HTTP_RESPONSE_HEADER_LEN     224U
#define HTTP_PAGE_BUFFER_LEN        4096U
#define HTTP_TASK_TABLE_BUFFER_LEN  1536U

#define HTTP_STATUS_OK               "200 OK"
#define HTTP_STATUS_NOT_FOUND        "404 Not Found"

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static u32_t g_http_request_count = 0U;
static u32_t g_tasks_page_hits = 0U;

extern struct netif gnetif; /* defined in main.c */

/* Private function prototypes -----------------------------------------------*/
static void http_server_netconn_thread(void *arg);

/* Private functions ---------------------------------------------------------*/
static void http_write_raw(struct netconn *conn, const void *data, size_t len)
{
  if ((conn != NULL) && (data != NULL) && (len > 0U))
  {
    netconn_write(conn, data, len, NETCONN_NOCOPY);
  }
}

static void http_send_response(struct netconn *conn,
                               const char *status,
                               const char *content_type,
                               const char *body)
{
  char header[HTTP_RESPONSE_HEADER_LEN];
  int header_len;
  size_t body_len;

  if ((conn == NULL) || (status == NULL) || (content_type == NULL) || (body == NULL))
  {
    return;
  }

  body_len = strlen(body);

  header_len = snprintf(header,
                        sizeof(header),
                        "HTTP/1.1 %s\r\n"
                        "Server: EdgePulse-HTTP/1.0\r\n"
                        "Connection: close\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %lu\r\n\r\n",
                        status,
                        content_type,
                        (unsigned long)body_len);

  if (header_len < 0)
  {
    return;
  }

  if ((size_t)header_len >= sizeof(header))
  {
    header_len = (int)(sizeof(header) - 1U);
  }

  http_write_raw(conn, header, (size_t)header_len);
  http_write_raw(conn, body, body_len);
}

static int http_extract_get_path(const char *request, char *path, size_t path_size)
{
  const char *start;
  const char *end;
  size_t len;
  char *query_sep;

  if ((request == NULL) || (path == NULL) || (path_size == 0U))
  {
    return 0;
  }

  if (strncmp(request, "GET ", 4) != 0)
  {
    return 0;
  }

  start = request + 4;
  end = strchr(start, ' ');
  if (end == NULL)
  {
    return 0;
  }

  len = (size_t)(end - start);
  if (len == 0U)
  {
    return 0;
  }

  if (len >= path_size)
  {
    len = path_size - 1U;
  }

  memcpy(path, start, len);
  path[len] = '\0';

  query_sep = strchr(path, '?');
  if (query_sep != NULL)
  {
    *query_sep = '\0';
  }

  return 1;
}

static void http_send_home_page(struct netconn *conn)
{
  static const char home_page[] =
      "<!doctype html>"
      "<html lang='zh-CN'><head>"
      "<meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>EdgePulse RTOS Monitor</title>"
      "<style>"
      "body{font-family:Arial,Helvetica,sans-serif;background:#f4f8ff;color:#182848;margin:0;padding:24px;}"
      ".card{max-width:900px;margin:0 auto;background:#fff;border-radius:14px;padding:24px;box-shadow:0 10px 30px rgba(24,40,72,.12);}"
      "h1{margin:0 0 8px 0;font-size:30px;}"
      "h2{margin:8px 0 14px 0;font-size:18px;color:#2b5fb8;}"
      "p,li{line-height:1.65;}"
      "a.btn{display:inline-block;margin:8px 10px 0 0;padding:10px 14px;border-radius:8px;text-decoration:none;color:#fff;background:#2159c6;}"
      "code{background:#f0f4ff;padding:2px 6px;border-radius:6px;}"
      "</style></head><body>"
      "<div class='card'>"
      "<h1>边缘脉冲实时监控站 EdgePulse RTOS Monitor</h1>"
      "<h2>STM32 + FreeRTOS + LwIP Embedded Web Backend</h2>"
      "<p>本节点用于在 MCU 侧提供轻量 Web 可视化与状态查询能力，适合 IoT 网关与现场设备调试。</p>"
      "<ul>"
      "<li>实时任务监控 / Real-time RTOS task snapshot</li>"
      "<li>网络状态接口 / Network status API</li>"
      "<li>嵌入式 HTTP 服务 / Embedded HTTP service</li>"
      "</ul>"
      "<a class='btn' href='/tasks'>查看任务监控 / Task Monitor</a>"
      "<a class='btn' href='/api/status'>访问状态接口 / API Status</a>"
      "<p style='margin-top:20px;'>"
      "Device Role: <code>"
      EDGE_DEVICE_ROLE
      "</code><br>"
      "Project: <code>"
      EDGE_PROJECT_NAME
      "</code>"
      "</p>"
      "</div></body></html>";

  http_send_response(conn, HTTP_STATUS_OK, "text/html; charset=utf-8", home_page);
}

static void http_send_not_found(struct netconn *conn)
{
  static const char not_found[] =
      "<!doctype html>"
      "<html lang='zh-CN'><head><meta charset='utf-8'><title>404 Not Found</title>"
      "<style>body{font-family:Arial;background:#f8f9fc;color:#1f2a44;padding:30px;}"
      "a{color:#1f5bd3;text-decoration:none;}</style></head><body>"
      "<h1>404 - 页面不存在</h1>"
      "<p>请求的资源未在当前固件中注册。</p>"
      "<p><a href='/'>返回首页 / Back to Home</a></p>"
      "</body></html>";

  http_send_response(conn, HTTP_STATUS_NOT_FOUND, "text/html; charset=utf-8", not_found);
}

static void http_send_status_json(struct netconn *conn)
{
  char json[512];
  const char *ip_text = "0.0.0.0";
  int json_len;

  if (netif_is_up(&gnetif))
  {
    const char *tmp_ip = ip4addr_ntoa(netif_ip4_addr(&gnetif));
    if (tmp_ip != NULL)
    {
      ip_text = tmp_ip;
    }
  }

  json_len = snprintf(json,
                      sizeof(json),
                      "{\n"
                      "  \"project\": \"%s\",\n"
                      "  \"role\": \"%s\",\n"
                      "  \"request_count\": %lu,\n"
                      "  \"tasks_page_hits\": %lu,\n"
                      "  \"link_up\": %s,\n"
                      "  \"netif_up\": %s,\n"
                      "  \"ip\": \"%s\",\n"
                      "  \"uptime_ms\": %lu\n"
                      "}\n",
                      EDGE_PROJECT_NAME,
                      EDGE_DEVICE_ROLE,
                      (unsigned long)g_http_request_count,
                      (unsigned long)g_tasks_page_hits,
                      netif_is_link_up(&gnetif) ? "true" : "false",
                      netif_is_up(&gnetif) ? "true" : "false",
                      ip_text,
                      (unsigned long)HAL_GetTick());

  if (json_len < 0)
  {
    return;
  }

  http_send_response(conn, HTTP_STATUS_OK, "application/json; charset=utf-8", json);
}

static void http_send_tasks_page(struct netconn *conn)
{
  char task_table[HTTP_TASK_TABLE_BUFFER_LEN];
  char page[HTTP_PAGE_BUFFER_LEN];
  int page_len;

  memset(task_table, 0, sizeof(task_table));
  osThreadList((unsigned char *)task_table);

  g_tasks_page_hits++;

  page_len = snprintf(page,
                      sizeof(page),
                      "<!doctype html>"
                      "<html lang='zh-CN'><head>"
                      "<meta charset='utf-8'>"
                      "<meta http-equiv='refresh' content='1'>"
                      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                      "<title>EdgePulse Task Monitor</title>"
                      "<style>"
                      "body{font-family:Consolas,Monaco,monospace;background:#101724;color:#d6e2ff;margin:0;padding:22px;}"
                      "h1{font-family:Arial,Helvetica,sans-serif;margin:0 0 12px 0;}"
                      "p{font-family:Arial,Helvetica,sans-serif;}"
                      "a{color:#78a6ff;text-decoration:none;}"
                      "pre{background:#16233b;border-radius:10px;padding:14px;overflow:auto;font-size:13px;line-height:1.4;}"
                      "</style></head><body>"
                      "<h1>实时任务监控 Task Runtime Snapshot</h1>"
                      "<p>刷新周期: %lu ms | 页面访问: %lu | 启动时长: %lu ms | <a href='/'>返回首页</a></p>"
                      "<pre>%s</pre>"
                      "<p>状态码: B=Blocked, R=Ready, D=Deleted, S=Suspended</p>"
                      "</body></html>",
                      (unsigned long)EDGE_DASHBOARD_REFRESH_MS,
                      (unsigned long)g_tasks_page_hits,
                      (unsigned long)HAL_GetTick(),
                      task_table);

  if (page_len < 0)
  {
    return;
  }

  http_send_response(conn, HTTP_STATUS_OK, "text/html; charset=utf-8", page);
}

static void http_server_serve(struct netconn *conn)
{
  struct netbuf *inbuf = NULL;
  err_t recv_err;
  char *buf = NULL;
  u16_t buflen = 0;
  char request[HTTP_REQ_BUFFER_LEN];
  char path[HTTP_ROUTE_BUFFER_LEN];

  recv_err = netconn_recv(conn, &inbuf);

  if ((recv_err == ERR_OK) && (inbuf != NULL) && (netconn_err(conn) == ERR_OK))
  {
    netbuf_data(inbuf, (void **)&buf, &buflen);

    if ((buf != NULL) && (buflen > 0U))
    {
      size_t copy_len = (size_t)buflen;
      if (copy_len >= sizeof(request))
      {
        copy_len = sizeof(request) - 1U;
      }

      memcpy(request, buf, copy_len);
      request[copy_len] = '\0';

      if (http_extract_get_path(request, path, sizeof(path)) != 0)
      {
        g_http_request_count++;

        if ((strcmp(path, "/") == 0) ||
            (strcmp(path, "/index.html") == 0) ||
            (strcmp(path, "/dashboard") == 0))
        {
          http_send_home_page(conn);
        }
        else if ((strcmp(path, "/tasks") == 0) ||
                 (strcmp(path, "/tasks.html") == 0))
        {
          http_send_tasks_page(conn);
        }
        else if (strcmp(path, "/api/status") == 0)
        {
          http_send_status_json(conn);
        }
        else
        {
          http_send_not_found(conn);
        }
      }
      else
      {
        http_send_not_found(conn);
      }
    }
  }

  netconn_close(conn);

  if (inbuf != NULL)
  {
    netbuf_delete(inbuf);
  }
}

/**
  * @brief  http server thread
  * @param arg: pointer on argument(not used here)
  * @retval None
  */
static void http_server_netconn_thread(void *arg)
{
  struct netconn *conn;
  struct netconn *newconn;
  err_t err;
  err_t accept_err;

  LWIP_UNUSED_ARG(arg);

  conn = netconn_new(NETCONN_TCP);
  if (conn != NULL)
  {
    err = netconn_bind(conn, NULL, EDGE_HTTP_PORT);
    if (err == ERR_OK)
    {
      netconn_listen(conn);

      while (1)
      {
        accept_err = netconn_accept(conn, &newconn);
        if (accept_err == ERR_OK)
        {
          http_server_serve(newconn);
          netconn_delete(newconn);
        }
      }
    }

    netconn_delete(conn);
  }
}

/**
  * @brief  Initialize the HTTP server (start its thread)
  * @param  none
  * @retval None
  */
void http_server_netconn_init(void)
{
  sys_thread_new("HTTP", http_server_netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE, WEBSERVER_THREAD_PRIO);
}
