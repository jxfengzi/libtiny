#include "sample_http_get.h"
#include "esp_common.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define DEMO_WIFI_SSID      "airport-milink"
#define DEMO_WIFI_PASSWORD  "milink123"

/******************************************************************************
 * FunctionName : wifi_event_handler_cb
 * Description  : wifi event callback
 * Parameters   : system event
 * Returns      : none
 *******************************************************************************/
static void wifi_event_handler_cb(System_Event_t * event)
{
    if (event == NULL)
    {
        return;
    }

    switch (event->event_id)
    {
        case EVENT_STAMODE_GOT_IP:
            printf("free heap size %d line %d \n", system_get_free_heap_size(), __LINE__);
            xTaskCreate(sample_http_get, "http-get", 1024 * 4, NULL, 4, NULL);
            break;

        default:
            break;
    }

    return;
}


/******************************************************************************
 * FunctionName : wifi_config
 * Description  : wifi_config task
 * Parameters   : task param
 * Returns      : none
 *******************************************************************************/
static void wifi_config(void *pvParameters)
{
    struct ip_info ip_config;
    struct station_config sta_config;

    memset(&sta_config, 0, sizeof(struct station_config));
    wifi_set_opmode(STATION_MODE);
    memcpy(sta_config.ssid, DEMO_WIFI_SSID, strlen(DEMO_WIFI_SSID));
    memcpy(sta_config.password, DEMO_WIFI_PASSWORD, strlen(DEMO_WIFI_PASSWORD));
    wifi_station_set_config(&sta_config);

    wifi_station_disconnect();
    wifi_station_connect();

    while (1)
    {
        vTaskDelay(10);
    }
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map)
    {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
    printf("SDK version:%s\n", system_get_sdk_version());

    wifi_set_event_handler_cb(wifi_event_handler_cb);
    xTaskCreate(wifi_config, "wfcf", 512, NULL, 4, NULL);
}