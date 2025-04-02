#ifndef __BS_BLE__
#define __BS_BLE__

#include <stdint.h>
#include <stdbool.h>

void bs_ble_init();

void bs_ble_send_data(uint8_t *data, uint32_t len);

#endif /* __BS_BLE__ */