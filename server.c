#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

// UUIDの定義 (16-bit)
#define HEART_RATE_SERVICE_UUID 0x180D
#define HEART_RATE_VALUE_UUID   0x2A37

static btstack_packet_callback_registration_t hci_event_callback_registration;

// アドバタイズデータの作成
const uint8_t adv_data[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    0x07, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', 'G', 'W',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x0d, 0x18,
};

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;
    
    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            printf("Server is up and running...\n");
            
            // アドバタイズ設定
            bd_addr_t null_addr = {0};
            uint16_t adv_int_min = 0x0030;
            uint16_t adv_int_max = 0x0030;
            gap_advertisements_set_params(adv_int_min, adv_int_max, 0, 0, null_addr, 0x07, 0x00);
            gap_advertisements_set_data(sizeof(adv_data), (uint8_t*)adv_data);
            gap_advertisements_enable(1);
            break;
        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                printf("Client Connected!\n");
            }
            break;
        default:
            break;
    }
}

int main() {
    stdio_init_all();
    if (cyw43_arch_init()) return -1;

    l2cap_init();
    sm_init();
    
    // GATTサービスの設定（本来は.gattファイルから生成しますが、簡易的に初期化）
    att_server_init(NULL, NULL, NULL); 

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    hci_power_control(HCI_POWER_ON);
    
    while(1) {
        cyw43_arch_poll();
        sleep_ms(1);
    }
}
