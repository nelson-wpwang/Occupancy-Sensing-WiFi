
#include <errno.h>
#include <string.h>
#include <stdio.h>
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


void push(struct Node** head_ref, uint8_t *new_mac_addr) 
{ 
    /* allocate node */
    struct Node* new_node = 
            (struct Node*) malloc(sizeof(struct Node)); 

    /* put in the key  */
    memcpy(new_node->mac_addr, new_mac_addr, 6);

    /* link the old list off the new node */
    new_node->next = (*head_ref); 

    /* move the head to point to the new node */
    (*head_ref) = new_node; 
} 

bool search(struct Node* head, uint8_t *search_mac_addr) 
{ 
    struct Node* current = head;  // Initialize current 
    if (current == NULL){
        return false;
    }
    
    while (current != NULL) 
    { 
        if (memcmp(current->mac_addr, search_mac_addr, 6) == 0) 
            return true; 
        current = current->next; 
    } 
    return false; 
} 

