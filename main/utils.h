
#pragma once

#ifndef UTILS_H
#define UTILS_H


//handles sender mac from espnow_main
struct Node 
{ 
    uint8_t *mac_addr[6]; 
    struct Node* next; 
}; 

struct Node* head; 

void push(struct Node** head_ref, uint8_t *new_mac_addr);

bool search(struct Node* head, uint8_t *search_mac_addr);

#endif