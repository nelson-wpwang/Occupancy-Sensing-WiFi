#include<stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include<stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"

#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "math.h"

#include "espnow_main.h"


#include "lwip/err.h"
#include "lwip/sys.h"


static xQueueHandle s_example_espnow_queue;
static xQueueHandle s_espnow_send_queue;


static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };

// static void example_espnow_deinit(example_espnow_send_param_t *send_param);

int ESPNOW_PACKET_LEN = 12;
int SEND_DELAY = 20000;
uint16_t ESP_CHANNEL = 1;
float csi_allchan[13][52];

esp_interface_t espnow_set_peer_ifidx(void)
{
    wifi_mode_t current_wifi_mode;
    esp_wifi_get_mode(&current_wifi_mode);
    if (current_wifi_mode == WIFI_MODE_STA)
    {
        return ESP_IF_WIFI_STA;
    }
    else
    {
        return ESP_IF_WIFI_AP;
    }
}

esp_err_t espnow_manage_peer(const uint8_t *mac_addr, uint8_t channel)
{
    if (esp_now_is_peer_exist(mac_addr) == false) 
    {
        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
        if (peer == NULL) {
            ESP_LOGE(TAG, "Malloc peer information fail");
        }

        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = channel;
        peer->ifidx = espnow_set_peer_ifidx();
        peer->encrypt = false;
        memcpy(peer->lmk, ESPNOW_LMK, ESP_NOW_KEY_LEN);
        memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_add_peer(peer) );
        free(peer);
        ESP_LOGI(TAG, "Successfully added peer MAC: "MACSTR", channel: %d\n", MAC2STR(mac_addr), peer->channel);
    }

    else
    {
        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
        if (peer == NULL) {
            ESP_LOGE(TAG, "Malloc peer information fail");
        }

        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = channel;
        peer->ifidx = espnow_set_peer_ifidx();
        peer->encrypt = false;
        memcpy(peer->lmk, ESPNOW_LMK, ESP_NOW_KEY_LEN);
        memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_mod_peer(peer) );
        free(peer);
        ESP_LOGI(TAG, "Successfully modded peer MAC: "MACSTR", channel: %d\n", MAC2STR(mac_addr), peer->channel);
    }

    return ESP_OK;
}

esp_err_t espnow_transmit(uint8_t *mac_addr, example_espnow_send_param_t *send_param)
{
    esp_now_send(mac_addr, send_param->buffer, send_param->len);
    send_feedback_result = esp_err_to_name(espnow_send_feedback);
    if (espnow_send_feedback != ESP_OK) {
        ESP_LOGE(TAG, "%s\n", send_feedback_result);
        example_espnow_deinit(send_param);
        vTaskDelete(NULL);
    }

    return ESP_OK;
}

void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    example_espnow_event_t evt;
    example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
    espnow_comm_status *send_type = &evt.send_type;

    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_SEND_CB;
    evt.send_type = IS_BROADCAST_ADDR(mac_addr) ? ESPNOW_SEND_BROADCAST : ESPNOW_SEND_UNICAST;

    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    ESP_LOGI(TAG, "send callback success\n");
}

void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    example_espnow_event_t evt;
    example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

    ESP_LOGI(TAG, "RECV callback called success\n");

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    ESP_LOGI(TAG, "Received from mac: "MACSTR"\n", MAC2STR(recv_cb->mac_addr));
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_example_espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

int example_espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *state, uint16_t *seq, int *magic, int *recv_channel)
{
    example_espnow_data_t *buf = (example_espnow_data_t *)data;
    uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(example_espnow_data_t)) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return -1;
    }

    *state = buf->state;
    *seq = buf->seq_num;
    *magic = buf->magic;
    *recv_channel = buf->channel;
    crc = buf->crc;
    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

    if (crc_cal == crc) {
        return buf->type;
    }

    return -1;
}

void example_espnow_data_prepare(example_espnow_send_param_t *send_param)
{
    example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(example_espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->state = send_param->state;
    buf->seq_num = s_example_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->channel = send_param->channel;
    buf->magic = send_param->magic;
    
    /* Fill all remaining bytes after the data with random values */
    esp_fill_random(buf->payload, send_param->len - sizeof(example_espnow_data_t));
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}


/* common variables between tasks */
static uint8_t received_broadcast_message = 0;
static uint8_t unicast_delivered = 0;


void send_task(void *pvParameter)
{   
    ESP_LOGI(TAG, "Entering Send Task\n");

    example_espnow_event_t evt;
    example_espnow_send_param_t *send_param = (example_espnow_send_param_t *)pvParameter;

    uint8_t recv_state = 0;
    uint16_t recv_seq = 0;
    int recv_channel;
    int recv_magic = 0;
    bool is_broadcast = false;
    int ret;
    int count = 100;
    int current_recv_seq = -1;

    uint8_t primary_channel;
    wifi_second_chan_t secondary_channel;
    int current_channel;
    
    esp_wifi_get_channel(&primary_channel, &secondary_channel);
    current_channel = (int)primary_channel;

    while (xQueueReceive(s_espnow_send_queue, &evt, portMAX_DELAY) == pdTRUE) {

        ESP_LOGI(TAG, "Send Task Handling Send Queue\n");

        example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

        is_broadcast = IS_BROADCAST_ADDR(send_cb->mac_addr);

        esp_wifi_get_channel(&primary_channel, &secondary_channel);
        current_channel = (int)primary_channel;

        switch (evt.send_type) {

            case ESPNOW_SEND_UNICAST:
            {
                if (is_broadcast == false && send_param->unicast == true){
                    if (unicast_delivered == 1){
                        break;
                    }

                    if (send_param->delay > 0) {
                        vTaskDelay(2000/portTICK_RATE_MS);
                    }

                    if (unicast_delivered == 1){
                        break;
                    }

                    ESP_LOGI(TAG, "Resend unicast data to "MACSTR"", MAC2STR(send_cb->mac_addr));

                    if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
                        ESP_LOGE(TAG, "Send error");
                        example_espnow_deinit(send_param);
                        vTaskDelete(NULL);
                    }
                }
                break;
            }

            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;
        }
    }
}



void data_process_task(void *pvParameter)
{

    ESP_LOGI(TAG, "Entering Data Process Task\n");

    example_espnow_event_t evt;
    example_espnow_send_param_t *send_param = (example_espnow_send_param_t *)pvParameter;

    uint8_t recv_state = 0;
    uint16_t recv_seq = 0;
    int recv_channel;
    int recv_magic = 0;
    bool is_broadcast = false;
    int ret;
    
    int current_recv_seq = -1;

    uint8_t primary_channel;
    wifi_second_chan_t secondary_channel;
    int current_channel;

    while (xQueueReceive(s_example_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {

        example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

        esp_wifi_get_channel(&primary_channel, &secondary_channel);
        current_channel = (int)primary_channel;

        ret = example_espnow_data_parse(recv_cb->data, recv_cb->data_len, &recv_state, &recv_seq, &recv_magic, &recv_channel);
        free(recv_cb->data);

        
        switch(ret){

            case (EXAMPLE_ESPNOW_DATA_UNICAST):
            {

                ESP_LOGI(TAG, "Receive %dth unicast data from: "MACSTR", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

                received_broadcast_message = 1;
                unicast_delivered = 1;
                current_recv_seq = recv_seq;

                //add filter mac to csi to print out csi data from ftm initiator
                memcpy(filter_ftm_mac, recv_cb->mac_addr, 6);

                /* If receive unicast ESPNOW data, also stop sending broadcast ESPNOW data. */
                send_param->broadcast = false;
                send_param->unicast = true;

                
                ESP_LOGI(TAG, "Current Channel: %d, received channel: %d\n", current_channel, recv_channel);

                /* decide if need to change channel  */
                if (current_channel <= 13){
                    send_param->channel = recv_channel;
                }
                else{
                    send_param->channel = 1;
                }

                if (true)
                {
                    esp_wifi_set_channel(send_param->channel, secondary_channel);
                    ESP_LOGI(TAG,"Setting into new channel: %d\n", send_param->channel);
                
                    espnow_manage_peer(recv_cb->mac_addr, send_param->channel);
                }
                break;
            }

            default:
            {
                ESP_LOGI(TAG, "Received error message, %d\n", ret);
                break;
            }
        }
    }
}


esp_err_t initialize_espnow(void)
{
    example_espnow_send_param_t *send_param;

    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    s_espnow_send_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    if (s_espnow_send_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)ESPNOW_PMK) );

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(example_espnow_send_param_t));
    memset(send_param, 0, sizeof(example_espnow_send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_example_espnow_queue);
        vSemaphoreDelete(s_espnow_send_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    send_param->unicast = true;
    send_param->broadcast = false;
    send_param->state = 0;
    send_param->magic = esp_random();
    send_param->count = ESPNOW_SEND_COUNT;
    send_param->delay = 2000;
    send_param->len = 20;
    send_param->channel = ESP_CHANNEL;
    send_param->buffer = malloc(20);

    if (send_param->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_example_espnow_queue);
        vSemaphoreDelete(s_espnow_send_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    xTaskCreate(send_task, "espnow_send_task", 4096, send_param, 5, NULL);
    xTaskCreate(data_process_task, "data_process_task", 4096, send_param, 2, NULL);

    return ESP_OK;
}

void example_espnow_deinit(example_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_example_espnow_queue);
    vSemaphoreDelete(s_espnow_send_queue);
    esp_now_deinit();
}

esp_err_t espnow_send_command (uint8_t *mac_addr, uint8_t channel)
{
    example_espnow_send_param_t *send_param;

    uint8_t primary_channel;
    wifi_second_chan_t secondary_channel;
    int current_channel;

    esp_wifi_get_channel(&primary_channel, &secondary_channel);

    espnow_manage_peer(mac_addr, primary_channel);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(example_espnow_send_param_t));
    memset(send_param, 0, sizeof(example_espnow_send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_example_espnow_queue);
        vSemaphoreDelete(s_espnow_send_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    send_param->unicast = true;
    send_param->broadcast = false;
    send_param->state = 0;
    send_param->magic = esp_random();
    send_param->count = ESPNOW_SEND_COUNT;
    send_param->delay = 2000;
    send_param->len = 20;
    send_param->channel = channel;
    send_param->buffer = malloc(20);

    if (send_param->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_example_espnow_queue);
        vSemaphoreDelete(s_espnow_send_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    memcpy(send_param->dest_mac, mac_addr, ESP_NOW_ETH_ALEN);
    example_espnow_data_prepare(send_param);

    espnow_transmit(mac_addr, send_param);

    return ESP_OK;
}