#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

#define LED_SERVICE_UUID 0xFF00
#define LED_CHAR_UUID    0xFF01

static uint16_t led_char_handle;
static btstack_packet_callback_registration_t hci_event_callback_registration;

// 書き込みがあった時に呼ばれる
static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
	if (att_handle == led_char_handle) {
		if (buffer_size > 0) {
			bool led_status = buffer[0];
			cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_status);
			printf("LED Status: %s\n", led_status ? "ON" : "OFF");
		}
	}
	return 0;
}

// 許可するクライアントのMACアドレス (例: 00:11:22:33:44:55)
// 実際のクライアントのログに出るアドレスに合わせて書き換えてください
static const bd_addr_t ALLOWED_CLIENT_ADDR = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
	if (packet_type != HCI_EVENT_PACKET) return;

	bd_addr_t event_addr;
	hci_con_handle_t handle;

	switch(hci_event_packet_get_type(packet)){
		case BTSTACK_EVENT_STATE:
			if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
				bd_addr_t null_addr = {0};
				gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
				uint8_t adv_data[] = { 0x02, 0x01, 0x06, 0x07, 0x09, 'P', 'i', 'c', 'o', 'G', 'W' };
				gap_advertisements_set_data(sizeof(adv_data), adv_data);
				gap_advertisements_enable(1);
			}
			break;
		case HCI_EVENT_LE_META:
			if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
				// 接続ハンドルの取得
				handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
				
				// クライアントのアドレスをパケットから取得
				hci_subevent_le_connection_complete_get_peer_address(packet, event_addr);
				
				printf("Connection request from: %s\n", bd_addr_to_str(event_addr));

				// アドレスを照合
				if (memcmp(event_addr, ALLOWED_CLIENT_ADDR, 6) == 0) {
					printf("Client Authorized. Connection Handle: 0x%04x\n", handle);
				} else {
					printf("Unauthorized Client! Disconnecting...\n");
					// 許可されていない場合は切断する
					gap_disconnect(handle);
				}
			}
			break;
		default:
			break;
	}
}

int main() {
	stdio_init_all();
	if (cyw43_arch_init()) return -1;
	l2cap_init(); sm_init();
	att_server_init(NULL, NULL, att_write_callback);
	att_db_util_init();
	att_db_util_add_service_uuid16(LED_SERVICE_UUID);
	led_char_handle = att_db_util_add_characteristic_uuid16(
		LED_CHAR_UUID, 
		ATT_PROPERTY_WRITE | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE, 
		0,	// read_permission (0 は通常「制限なし」を意味します)
		0,	// write_permission
		NULL, 
		0
	);
	hci_event_callback_registration.callback = &packet_handler;
	hci_add_event_handler(&hci_event_callback_registration);
	hci_power_control(HCI_POWER_ON);
	while(1) { cyw43_arch_poll(); sleep_ms(1); }
}
