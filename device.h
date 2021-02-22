//
// Created by dbrent on 2/20/21.
//

#ifndef DBSDR_DEVICE_H
#define DBSDR_DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef void (*device_rx_callback)(void *, size_t, size_t);

bool device_init(void);

bool device_destroy(void);

bool device_set_frequency(uint64_t freq);

bool device_set_sample_rate(uint64_t sample_rate);

bool device_set_lna_gain(uint32_t gain);

bool device_set_vga_gain(uint32_t gain);

bool device_is_alive();

bool device_rx(device_rx_callback callback_function);

#endif //DBSDR_DEVICE_H
