#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

static hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID;
static bool led_state = false;
static bool trigger_send = false;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) gap_start_scan();
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            gap_stop_scan();
            bd_addr_t addr;
            gap_event_advertising_report_get_address(packet, addr);
            gap_connect(addr, 0);
            break;
        case HCI_EVENT_LE_META:
		    if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
		        connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
		        printf("Connected! Handle: 0x%04x\n", connection_handle);
		    }
		    break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            connection_handle = HCI_CON_HANDLE_INVALID;
            printf("Disconnected. Scanning again...\n");
            gap_start_scan();
            break;
    }
}

int main() {
    stdio_init_all();
    if (cyw43_arch_init()) return -1;

    l2cap_init();
    sm_init();
    gatt_client_init();

    static btstack_packet_callback_registration_t hci_event_callback_registration;
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    hci_power_control(HCI_POWER_ON);

    absolute_time_t next_send_time = get_absolute_time();

    while(1) {
        // BTstackの内部処理（必須）
        cyw43_arch_poll();

        // 接続中かつ、500ms経過したかチェック
        if (connection_handle != HCI_CON_HANDLE_INVALID) {
            if (absolute_time_diff_us(get_absolute_time(), next_send_time) < 0) {
                
                // LEDの状態を反転
                led_state = !led_state;
                uint8_t value = led_state ? 1 : 0;
                
                // サーバーへ書き込み
                // ハンドル番号11は、サーバー側で最初に作ったキャラクタリスティックのデフォルト
                uint8_t status = gatt_client_write_value_of_characteristic_without_response(connection_handle, 11, 1, &value);
                
                if (status == 0) {
                    printf("Loop: Send LED %s\n", led_state ? "ON" : "OFF");
                }

                // 次の実行時間を設定 (500ms後)
                next_send_time = make_timeout_time_ms(500);
            }
        }
        
        // CPU負荷を下げすぎない程度に待機
        sleep_us(100);
    }
}
