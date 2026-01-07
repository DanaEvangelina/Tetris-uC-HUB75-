#include "led.h"

#include <stdint.h>
#include <math.h>

#include "main.h"
#include "gamma.h"


RGB_t frame[WIDTH*HEIGHT];
RGB_t static_frame[WIDTH*HEIGHT];
uint8_t buffer1[WIDTH*BITS_PER_CHANNEL*SCAN_RATE];
uint8_t buffer2[WIDTH*BITS_PER_CHANNEL*SCAN_RATE];

float mapf(float value, float c_min, float c_max, float t_min, float t_max) {
	float res = (value - c_min) / (c_max - c_min) * (t_max - t_min) + t_min;
    return res;
}

float bound(float value, float max, float min) {
    return fmaxf(fminf(value, max), min);
}


void LED_fillBuffer(RGB_t *frame, uint8_t *buffer) {
    uint32_t i = 0, p1, p2;
    uint8_t bit, mask;
    for(uint8_t row = 0; row < SCAN_RATE; row++) {
        p1 = row * WIDTH;
        p2 = p1 + WIDTH * SCAN_RATE;
        for(bit = 0; bit < BITS_PER_CHANNEL; bit++) {
            mask = 1<<bit;
            for(uint8_t col = 0; col < WIDTH; col++) {
                buffer[i] =
                    ((((gammaR[frame[p2+col].R]) & mask) >> bit) << 3) |
                    ((((gammaG[frame[p2+col].G]) & mask) >> bit) << 4) |
                    ((((gammaB[frame[p2+col].B]) & mask) >> bit) << 5) |
                    ((((gammaR[frame[p1+col].R]) & mask) >> bit) << 0) |
                    ((((gammaG[frame[p1+col].G]) & mask) >> bit) << 1) |
                    ((((gammaB[frame[p1+col].B]) & mask) >> bit) << 2);
                i++;
            }
        }
    }
}


