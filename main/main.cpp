//
// Created by Braden Nicholson on 6/13/22.
//

#include <esp_timer.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include "server.h"
#include "wifi.h"

extern "C" void app_main(void) {
    // Initialize the onboard flashmo
    ESP_ERROR_CHECK(nvs_flash_init());
    // Initialize TCP/IP stack for the runtime
    ESP_ERROR_CHECK(esp_netif_init());
    // Initialize the event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifiInit();

    auto s = Server::instance();
    s.start_webserver();
}
