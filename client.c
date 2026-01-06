#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

static btstack_packet_callback_registration_t hci_event_callback_registration;
static bd_addr_t server_addr;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            printf("Client searching for server...\n");
            gap_set_scan_parameters(0, 0x0030, 0x0030);
            gap_start_scan();
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
            // 本来はアドバタイズデータを見てサーバーを特定
            gap_event_advertising_report_get_address(packet, server_addr);
            printf("Server found! Connecting...\n");
            gap_stop_scan();
            gap_connect(server_addr, 0);
            break;

        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                printf("Connected to server successfully!\n");
            }
            break;
    }
}

int main() {
    stdio_init_all();
    if (cyw43_arch_init()) return -1;

    l2cap_init();
    sm_init();
    gatt_client_init();

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    hci_power_control(HCI_POWER_ON);

    while(1) {
        cyw43_arch_poll();
        sleep_ms(1);
    }
}
