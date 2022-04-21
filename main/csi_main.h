/* CSI header
*/
#pragma once

#ifndef CSI_MAIN_H
#define CSI_MAIN_H

// extern int csi_collected_channel;


// extern struct Node* head = NULL; 

// extern void push(struct Node** head_ref, uint8_t *new_mac_addr);

// extern bool search(struct Node* head, uint8_t *search_mac_addr);


void cmd_register_ping(void);
void cmd_register_system(void);
void cmd_register_wifi(void);
void cmd_register_csi(void);
void cmd_register_detect(void);

void wifi_csi_raw_cb(void *ctx, wifi_csi_info_t *info);

void initialize_csi(void);
#endif