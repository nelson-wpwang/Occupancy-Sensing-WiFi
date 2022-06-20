/* Wi-Fi FTM Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#ifndef FTM_MAIN_H
#define FTM_MAIN_H

static uint8_t s_example_broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


typedef struct {
    uint8_t resp_mac[6];
    uint8_t channel;
} csi_ftm_responder_table;

typedef struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} wifi_args_t;

typedef struct {
    struct arg_str *ssid;
    struct arg_end *end;
} wifi_scan_arg_t;

typedef struct {
    // struct arg_lit *mode;
    // struct arg_int *channel;
    // struct arg_int *repeat_count;
    // struct arg_int *frm_count;
    // struct arg_int *burst_period;
    // struct arg_str *ssid;
    // struct arg_end *end;
    struct arg_int *repeat_count;

    struct arg_lit *initiator;
    struct arg_int *frm_count;
    struct arg_int *burst_period;
    struct arg_str *ssid;
    /* FTM Responder */
    struct arg_lit *responder;
    struct arg_lit *enable;
    struct arg_lit *disable;
    struct arg_int *offset;
    struct arg_end *end;
} wifi_ftm_args_t;

wifi_args_t sta_args;
wifi_args_t ap_args;
wifi_scan_arg_t scan_args;
wifi_ftm_args_t ftm_args;

csi_ftm_responder_table csi_ftm_pass;
int csi_collected_channel;

// new commands

// wifi_config_t g_ap_config = {
//     .ap.max_connection = 4,
//     .ap.authmode = WIFI_AUTH_WPA2_PSK,
//     .ap.ftm_responder = true
// };

#define ETH_ALEN 6
#define MAX_CONNECT_RETRY_ATTEMPTS  5

static uint32_t g_rtt_est, g_dist_est;
bool g_ap_started;
uint8_t g_ap_channel;
uint8_t g_ap_bssid[ETH_ALEN];

static int s_retry_num = 0;

static int g_report_lvl =
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_DIAG
    BIT0 |
#endif
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_RTT
    BIT1 |
#endif
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_T1T2T3T4
    BIT2 |
#endif
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_RSSI
    BIT3 |
#endif
0;

//end 
//

static bool s_reconnect = true;
static const char *TAG_STA = "ftm_station";
static const char *TAG_AP = "ftm_ap";

EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_BIT;
extern const int DISCONNECTED_BIT;
// const int CONNECTED_BIT = BIT0;
// const int DISCONNECTED_BIT = BIT1;

EventGroupHandle_t ftm_event_group;
// const int FTM_REPORT_BIT = BIT0;
// const int FTM_FAILURE_BIT = BIT1;
extern const int FTM_REPORT_BIT;
extern const int FTM_FAILURE_BIT;
wifi_ftm_report_entry_t *g_ftm_report;
uint8_t g_ftm_report_num_entries;

uint16_t g_scan_ap_num;
wifi_ap_record_t *g_ap_list_buffer;

uint8_t primary_channel;
wifi_second_chan_t secondary_channel;

// void wifi_connected_handler(void *arg, esp_event_base_t event_base,
//                                    int32_t event_id, void *event_data);

// void disconnect_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data);

void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);

// void ftm_report_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data);

void ftm_process_report(void);

bool wifi_cmd_sta_join(const char *ssid, const char *pass);

int wifi_cmd_sta(int argc, char **argv);

bool wifi_perform_scan(const char *ssid, bool internal);

int wifi_cmd_scan(int argc, char **argv);

bool wifi_cmd_ap_set(const char* ssid, const char* pass);

int wifi_cmd_ap(int argc, char** argv);

int wifi_cmd_query(int argc, char **argv);

wifi_ap_record_t *find_ftm_responder_ap(const char *ssid);

int wifi_cmd_ftm(int argc, char **argv);

void register_wifi(void);

// void wifi_csi_raw_cb(void *ctx, wifi_csi_info_t *info);

// void initialise_wifi(void);

#endif