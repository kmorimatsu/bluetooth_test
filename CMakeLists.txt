cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_ble_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# --- Server Target ---
add_executable(ble_server
	server.c
)
target_link_libraries(ble_server
	pico_cyw43_arch_none
	pico_stdlib pico_btstack_ble
	pico_btstack_cyw43
)
pico_enable_stdio_usb(ble_server 1)
pico_enable_stdio_uart(ble_server 0)
target_include_directories(ble_server PRIVATE ${CMAKE_CURRENT_LIST_DIR})
pico_add_extra_outputs(ble_server)

# --- Client Target ---
add_executable(ble_client
	client.c
)
target_link_libraries(ble_client
	pico_cyw43_arch_none
	pico_stdlib pico_btstack_ble
	pico_btstack_cyw43
)
pico_enable_stdio_usb(ble_client 1)
pico_enable_stdio_uart(ble_client 0)
target_include_directories(ble_client PRIVATE ${CMAKE_CURRENT_LIST_DIR})
pico_add_extra_outputs(ble_client)
