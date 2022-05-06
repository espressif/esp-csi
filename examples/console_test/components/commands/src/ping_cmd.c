// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_console.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "ping/ping_sock.h"

static const char *TAG  = "ping_cmd";

static esp_ping_handle_t g_ping_handle = NULL;

static void ping_cmd_on_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGD(TAG, "%d bytes from %s icmp_seq=%d ttl=%d time=%d ms",
             recv_len, ipaddr_ntoa((ip_addr_t *)&target_addr), seqno, ttl, elapsed_time);
}

static void ping_cmd_on_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    ESP_LOGW(TAG, "From %s icmp_seq=%d timeout", ipaddr_ntoa((ip_addr_t *)&target_addr), seqno);
}

static void ping_cmd_on_end(esp_ping_handle_t hdl, void *args)
{
    ip_addr_t target_addr;
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    uint32_t loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);

    if (IP_IS_V4(&target_addr)) {
        ESP_LOGI(TAG, "\n--- %s ping statistics ---", inet_ntoa(*ip_2_ip4(&target_addr)));
    } else {
        ESP_LOGI(TAG, "\n--- %s ping statistics ---", inet6_ntoa(*ip_2_ip6(&target_addr)));
    }

    ESP_LOGI(TAG, "%d packets transmitted, %d received, %d%% packet loss, time %dms",
             transmitted, received, loss, total_time_ms);
    // delete the ping sessions, so that we clean up all resources and can create a new ping session
    // we don't have to call delete function in the callback, instead we can call delete function from other tasks
    esp_ping_delete_session(hdl);
    g_ping_handle = NULL;
}

static struct {
    struct arg_int *timeout;
    struct arg_int *interval;
    struct arg_int *data_size;
    struct arg_int *count;
    struct arg_int *tos;
    struct arg_str *host;
    struct arg_lit *abort;
    struct arg_end *end;
} ping_args;

static int wifi_cmd_ping(int argc, char **argv)
{
    esp_ping_config_t config = {
        .count       = 0,
        .interval_ms = 10,
        .timeout_ms  = 1000,
        .data_size   = 1,
        .tos         = 0,
        .task_stack_size = 4096,
        .task_prio = 0,
    };

    if (arg_parse(argc, argv, (void **)&ping_args) != ESP_OK) {
        arg_print_errors(stderr, ping_args.end, argv[0]);
        return ESP_FAIL;
    }

    if (ping_args.timeout->count > 0) {
        config.timeout_ms = ping_args.timeout->ival[0];
    }

    if (ping_args.interval->count > 0) {
        config.interval_ms = ping_args.interval->ival[0];
    }

    if (ping_args.data_size->count > 0) {
        config.data_size = ping_args.data_size->ival[0];
    }

    if (ping_args.count->count > 0) {
        config.count = ping_args.count->ival[0];
    }

    if (ping_args.tos->count > 0) {
        config.tos = ping_args.tos->ival[0];
    }

    if (ping_args.abort->count > 0) {
        esp_ping_stop(g_ping_handle);
        esp_ping_delete_session(g_ping_handle);

        ESP_LOGW(TAG, "esp_ping stop");
        return ESP_OK;
    }

    char host[32] = {0};

    if (ping_args.host->count > 0) {
        strcpy(host, ping_args.host->sval[0]);
    } else {
        esp_netif_ip_info_t local_ip;
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &local_ip);
        sprintf(host, IPSTR, IP2STR(&local_ip.gw));
        ESP_LOGI(TAG, "ping router ip:" IPSTR ", gw: " IPSTR, IP2STR(&local_ip.ip), IP2STR(&local_ip.gw));
    }

    // parse IP address
    struct sockaddr_in6 sock_addr6;
    ip_addr_t target_addr;
    memset(&target_addr, 0, sizeof(target_addr));

    if (inet_pton(AF_INET6, host, &sock_addr6.sin6_addr) == 1) {
        /* convert ip6 string to ip6 address */
        ipaddr_aton(host, &target_addr);
    } else {
        struct addrinfo hint;
        struct addrinfo *res = NULL;
        memset(&hint, 0, sizeof(hint));

        /* convert ip4 string or hostname to ip4 or ip6 address */
        if (getaddrinfo(host, NULL, &hint, &res) != 0) {
            printf("ping: unknown host %s\n", host);
            return ESP_FAIL;
        }

        if (res->ai_family == AF_INET) {
            struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
            inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
        } else {
            struct in6_addr addr6 = ((struct sockaddr_in6 *)(res->ai_addr))->sin6_addr;
            inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
        }

        freeaddrinfo(res);
    }

    config.target_addr = target_addr;

    /* set callback functions */
    esp_ping_callbacks_t cbs = {
        .on_ping_success = ping_cmd_on_success,
        .on_ping_timeout = ping_cmd_on_timeout,
        .on_ping_end = ping_cmd_on_end,
        .cb_args = NULL
    };

    esp_ping_new_session(&config, &cbs, &g_ping_handle);
    esp_ping_start(g_ping_handle);

    return ESP_OK;
}

void cmd_register_ping(void)
{
    ping_args.timeout   = arg_int0("W", "timeout", "<t>", "Time to wait for a response, in seconds");
    ping_args.interval  = arg_int0("i", "interval", "<t>", "Wait interval seconds between sending each packet");
    ping_args.data_size = arg_int0("s", "size", "<n>", "Specify the number of data bytes to be sent");
    ping_args.count     = arg_int0("c", "count", "<n>", "Stop after sending count packets");
    ping_args.tos       = arg_int0("Q", "tos", "<n>", "Set Type of Service related bits in IP datagrams");
    ping_args.host      = arg_str0("h", "host", "<host>", "Host address");
    ping_args.abort     = arg_lit0("a", "abort", "Abort running ping");
    ping_args.end       = arg_end(10);
    const esp_console_cmd_t ping_cmd = {
        .command = "ping",
        .help = "Send ICMP ECHO_REQUEST to network hosts",
        .hint = NULL,
        .func = &wifi_cmd_ping,
        .argtable = &ping_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&ping_cmd));
}
