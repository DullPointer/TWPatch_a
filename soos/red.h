#pragma once
#include <stdint.h>

typedef struct
{
	int temperature;
	float gamma[3];
	float brightness;
} color_setting_t;

void colorramp_fill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b,
	int size, const color_setting_t *setting);
void colorramp_fill_float(float *gamma_r, float *gamma_g, float *gamma_b,
    int size, const color_setting_t *setting);
