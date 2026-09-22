/*
VoidFX
Copyright (C) 2026 Voidscape Development

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <math.h>
#include <stdint.h>

/* Deterministic hash of an integer to [0, 1). */
static inline float fx_hash(int32_t n)
{
	uint32_t x = (uint32_t)n;
	x ^= x >> 16;
	x *= 0x7feb352dU;
	x ^= x >> 15;
	x *= 0x846ca68bU;
	x ^= x >> 16;
	return (float)(x & 0xffffffU) / (float)0x1000000;
}

/* Smooth 1D value noise in [0, 1). */
static inline float fx_value_noise(float t)
{
	float i = floorf(t);
	float f = t - i;
	float u = f * f * (3.0f - 2.0f * f);
	float a = fx_hash((int32_t)i);
	float b = fx_hash((int32_t)i + 1);
	return a + (b - a) * u;
}
