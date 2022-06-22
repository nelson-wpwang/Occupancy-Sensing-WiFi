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
#include "esp_console.h"
#include "math.h"
#include "esp_private/wifi.h"
#include "esp_now.h"

#include "ftm_main.h"
#include "csi_main.h"
#include "espnow_main.h"

uint8_t primary_channel;
wifi_second_chan_t secondary_channel;


void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;

    if (initialized) {
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ftm_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL) );

    ESP_ERROR_CHECK(esp_wifi_start() );
    initialized = true;

    //specific setting for ESPNOW to disable sleep when in STA mode
    esp_wifi_set_ps(WIFI_PS_NONE);
    // ESP_ERROR_CHECK( esp_wifi_internal_set_fix_rate(ESPNOW_WIFI_IF, 1, WIFI_PHY_RATE_MCS7_SGI));

}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    initialise_wifi();

    initialize_csi();

    initialize_espnow();


    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    repl_config.prompt = "ftm>";
    // init console REPL environment
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
    /* Register commands */
    register_system();
    register_wifi();

    printf("\n ==========================================================\n");
    printf(" |                      Steps to test FTM                 |\n");
    printf(" |                                                        |\n");
    printf(" |  1. Use 'help' to gain overview of commands            |\n");
    printf(" |  2. Use 'scan' command to search for external AP's     |\n");
    printf(" |                          OR                            |\n");
    printf(" |  2. Start SoftAP on another device using 'ap' command  |\n");
    printf(" |  3. Start FTM with command 'ftm -I -s <SSID>'          |\n");
    printf(" |                                                        |\n");
    printf(" ==========================================================\n\n");

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

}
