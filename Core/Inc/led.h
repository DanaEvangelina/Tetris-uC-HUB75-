#pragma once

#include <stdint.h>

struct RGB {
	uint8_t R;
	uint8_t G;
	uint8_t B;
};

typedef struct RGB RGB_t;

extern RGB_t frame[];
extern RGB_t static_frame[];
extern uint8_t buffer1[];
extern uint8_t buffer2[];


float mapf(float value, float c_min, float c_max, float t_min, float t_max);
float bound(float value, float max, float min);
void LED_fillBuffer(RGB_t *frame, uint8_t *buffer);

