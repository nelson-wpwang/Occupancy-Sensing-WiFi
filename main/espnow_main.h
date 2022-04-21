/* ESPNOW Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef ESPNOW_EXAMPLE_H
#define ESPNOW_EXAMPLE_H

#define ESPNOW_LMK "lmk1234567890123"

#define ESPNOW_PMK "pmk1234567890123"

#define ESPNOW_CHANNEL 1

#define ESPNOW_SEND_COUNT 10000

#define ESPNOW_QUEUE_SIZE           100

#define RESEND_COUNT 10

#define ESP_NOW_ETH_ALEN 6

static const char *TAG = "espnow";

int espnow_send_feedback;
char *send_feedback_result;

uint8_t *filter_ftm_mac[6];

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

typedef enum {
    EXAMPLE_ESPNOW_SEND_CB,
    EXAMPLE_ESPNOW_RECV_CB,
    
    //CUBIC_CALC,
} example_espnow_event_id_t;

typedef enum {
    ESPNOW_SEND_BROADCAST,
    ESPNOW_SEND_UNICAST,
    ESPNOW_RECV_BROADCAST,
    ESPNOW_RECV_UNICAST,
} espnow_comm_status;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} example_espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} example_espnow_event_recv_cb_t;

typedef struct {
    float phase[52];
    int channel;
} cubic_spline_calc;

typedef union {
    example_espnow_event_send_cb_t send_cb;
    example_espnow_event_recv_cb_t recv_cb;
    cubic_spline_calc cubic_info;
} example_espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    example_espnow_event_id_t id;
    espnow_comm_status send_type;
    example_espnow_event_info_t info;
} example_espnow_event_t;

enum {
    EXAMPLE_ESPNOW_DATA_BROADCAST,
    EXAMPLE_ESPNOW_DATA_UNICAST,
    EXAMPLE_ESPNOW_DATA_MAX,
    EXAMPLE_ESPNOW_DATA_UNICAST_RESEND,
    EXAMPLE_ESPNOW_DATA_RECV,
    EXAMPLE_ESPNOW_DATA_RECV_CHEK
};

/* User defined field of ESPNOW data in this example. */
typedef struct {
    uint8_t type;                         //Broadcast or unicast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint16_t seq_num;                     //Sequence number of ESPNOW data.
    uint16_t crc;                         //CRC16 value of ESPNOW data.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    int channel;
    uint8_t payload[0];                   //Real payload of ESPNOW data.
} __attribute__((packed)) example_espnow_data_t;

/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    bool resend_unicast_flag;
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int channel;
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} example_espnow_send_param_t;

esp_err_t initialize_espnow(void);

void example_espnow_deinit(example_espnow_send_param_t *send_param);

esp_err_t espnow_send_command (uint8_t *mac_addr, uint8_t channel);

//utils for searching and storing mac address.
// struct Node 
// { 
//     uint8_t *mac_addr[6]; 
//     struct Node* next; 
// }; 

// extern struct Node* head = NULL; 

// extern void push(struct Node** head_ref, uint8_t *new_mac_addr);

// extern bool search(struct Node* head, uint8_t *search_mac_addr);


#endif