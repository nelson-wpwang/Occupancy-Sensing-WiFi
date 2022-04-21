/* Wi-Fi CSI Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "cmd_system.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "esp_console.h"
#include "esp_now.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "csi_main.h"
#include "ftm_main.h"
#include "espnow_main.h"
// #include "utils.h"

static const char *TAG_CSI  = "csi_cmd";

extern float g_move_absolute_threshold;
extern float g_move_relative_threshold;

static struct {
    struct arg_int *len;
    struct arg_str *mac;
    struct arg_lit *output_raw_data;
    struct arg_end *end;
} csi_args;

static int wifi_cmd_csi(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &csi_args) != ESP_OK) {
        arg_print_errors(stderr, csi_args.end, argv[0]);
        return ESP_FAIL;
    }

    // wifi_radar_config_t radar_config = {0};
    // esp_wifi_radar_get_config(&radar_config);

    if (csi_args.len->count) {
        if (csi_args.len->ival[0] != 0 && csi_args.len->ival[0] != 128
                && csi_args.len->ival[0] != 256 && csi_args.len->ival[0] != 376
                && csi_args.len->ival[0] != 384 && csi_args.len->ival[0] != 612) {
            ESP_LOGE(TAG_CSI, "FAIL Filter CSI total bytes, 128 or 256 or 384");
            return ESP_FAIL;
        }

        // radar_config.filter_len = csi_args.len->ival[0];
    }

    // if (csi_args.mac->count) {
    //     if (!mac_str2hex(csi_args.mac->sval[0], radar_config.filter_mac)) {
    //         ESP_LOGE(TAG, "The format of the address is incorrect."
    //                  "Please enter the format as xx:xx:xx:xx:xx:xx");
    //         return ESP_FAIL;
    //     }
    // }

    // if (csi_args.output_raw_data->count) {
    //     radar_config.wifi_csi_raw_cb = wifi_csi_raw_cb;
    // }

    return 0;
}

void cmd_register_csi(void)
{
    csi_args.len = arg_int0("l", "len", "len (0/128/256/376/384/612)>", "Filter CSI total bytes");
    csi_args.mac = arg_str0("m", "mac", "mac (xx:xx:xx:xx:xx:xx)>", "Filter device mac");
    csi_args.output_raw_data = arg_lit0("o", "output_raw_data", "Enable the output of raw csi data");
    csi_args.end = arg_end(1);

    const esp_console_cmd_t csi_cmd = {
        .command = "csi",
        .help = "CSI command",
        .hint = NULL,
        .func = &wifi_cmd_csi,
        .argtable = &csi_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&csi_cmd));
}

static struct {
    struct arg_str *move_absolute_threshold;
    struct arg_str *move_relative_threshold;
    struct arg_end *end;
} detect_args;

static int wifi_cmd_detect(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &detect_args) != ESP_OK) {
        arg_print_errors(stderr, detect_args.end, argv[0]);
        return ESP_FAIL;
    }

    if (detect_args.move_absolute_threshold->count) {
        float threshold = atof(detect_args.move_absolute_threshold->sval[0]);

        if (threshold > 0.0 && threshold < 1.0) {
            g_move_absolute_threshold = threshold;
        } else {
            ESP_LOGE(TAG_CSI, "If the setting fails, the absolute threshold range of human movement is: 0.0 ~ 1.0");
        }
    }

    if (detect_args.move_relative_threshold->count) {
        float threshold = atof(detect_args.move_relative_threshold->sval[0]);

        if (threshold > 0.0 && threshold < 1.0) {
            g_move_relative_threshold = threshold;
        } else {
            ESP_LOGE(TAG_CSI, "If the setting fails, the absolute threshold range of human movement is: 1.0 ~ 5.0");
        }
    }

    return ESP_OK;
}

void cmd_register_detect(void)
{
    detect_args.move_absolute_threshold = arg_str0("a", "move_absolute_threshold", "<0.0 ~ 1.0>", "Set the absolute threshold of human movement");
    detect_args.move_relative_threshold = arg_str0("r", "move_relative_threshold", "<1.0 ~ 5.0>", "Set the relative threshold of human movement");
    detect_args.end = arg_end(1);

    const esp_console_cmd_t detect_cmd = {
        .command = "detect",
        .help = "Detect command",
        .hint = NULL,
        .func = &wifi_cmd_detect,
        .argtable = &detect_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&detect_cmd));
}

void wifi_csi_raw_cb(void *ctx, wifi_csi_info_t *info)
{
    static char buff[2048];
    size_t len = 0;
    wifi_pkt_rx_ctrl_t *rx_ctrl = &info->rx_ctrl;
    static uint32_t s_count = 0;

    uint8_t primary_channel;
    wifi_second_chan_t secondary_channel;
    int current_channel;

    esp_wifi_get_channel(&primary_channel, &secondary_channel);
    current_channel = (int)primary_channel;

    
    // only print out the MAC which ftm is connecting
    // if ((memcmp(info->mac, csi_ftm_pass.resp_mac, sizeof(info->mac)) == 0 && csi_collected_channel < csi_ftm_pass.channel) || search(head, csi_ftm_pass.resp_mac) == true)
    if ((memcmp(info->mac, csi_ftm_pass.resp_mac, sizeof(info->mac)) == 0 )&& ((csi_collected_channel < current_channel) || (csi_collected_channel == 10 && current_channel == 1)))
    {
        ESP_LOGI(TAG_CSI, "Collecting CSI from " MACSTR "on channel %d\n", MAC2STR(info->mac), csi_ftm_pass.channel);
        csi_collected_channel = current_channel;
        // if (csi_collected_channel >= 10){
        //     csi_collected_channel = 0;
        // }
    }
    else
    {
        return;
    }

    // if (!s_count) {   
    //     // ets_printf("type,id,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data\n");
    //     len += snprintf(buff, sizeof(buff),"type,id,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data\n");
    // }

    len += snprintf(buff + len, sizeof(buff) - len,"CSI_DATA,%d," MACSTR ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               current_channel, MAC2STR(info->mac), rx_ctrl->rssi, rx_ctrl->rate, rx_ctrl->sig_mode,
               rx_ctrl->mcs, rx_ctrl->cwb, rx_ctrl->smoothing, rx_ctrl->not_sounding,
               rx_ctrl->aggregation, rx_ctrl->stbc, rx_ctrl->fec_coding, rx_ctrl->sgi,
               rx_ctrl->noise_floor, rx_ctrl->ampdu_cnt, rx_ctrl->channel, rx_ctrl->secondary_channel,
               rx_ctrl->timestamp, rx_ctrl->ant, rx_ctrl->sig_len, rx_ctrl->rx_state);

    len += snprintf(buff + len, sizeof(buff) - len, ",%d,%d,\"[", info->len, info->first_word_invalid);

    int i = 0;
    for (; i < info->len - 1; i++) {
        len += snprintf(buff + len, sizeof(buff) - len, "%d,", info->buf[i]);
    }
    len += snprintf(buff + len, sizeof(buff) - len, "%d",info->buf[i]);

    len += snprintf(buff + len, sizeof(buff) - len, "]\"\n");
    ets_printf("%s",buff);
    // fputs(buff, fp);

    
}

void initialize_csi(void)
{
    // init csi and its callback function
    ESP_ERROR_CHECK(esp_wifi_set_csi(1));

    wifi_csi_config_t CSI_Config; // CSI = Channel State Information
	CSI_Config.lltf_en = 1;//1
	CSI_Config.htltf_en = 1;//1
	CSI_Config.stbc_htltf2_en = 1;//1
	CSI_Config.ltf_merge_en = 1;//1
	CSI_Config.channel_filter_en = 0;//1
	CSI_Config.manu_scale = 0; // Automatic scalling
	//CSI_Config.shift=15; // 0->15
	ESP_ERROR_CHECK(esp_wifi_set_csi_config(&CSI_Config));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(&wifi_csi_raw_cb, NULL));
}