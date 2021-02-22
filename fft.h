//
// Created by dbrent on 2/20/21.
//

#ifndef DBSDR_FFT_H
#define DBSDR_FFT_H

#include <stddef.h>
#include <stdint.h>

float *fft(const int8_t *samples);

void fft_destroy();

void fft_init(size_t size);

#endif //DBSDR_FFT_H
