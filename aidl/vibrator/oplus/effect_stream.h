/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

/*
 * One FIFO waveform: `length` signed 8-bit samples played at `play_rate_hz`.
 *
 * The layout is ABI in two directions and must not change:
 *  - the kernel reads it as `struct custom_fifo_data` (qcom-hv-haptics.c) when it is passed as
 *    the custom data of an FF_PERIODIC/FF_CUSTOM effect, and tells it apart from a predefined
 *    effect request purely by its size;
 *  - the tables inside the stock libqtivibratoreffect.so are arrays of it.
 */
struct effect_stream {
    uint32_t effect_id;
    uint32_t length;
    uint32_t play_rate_hz;
    const int8_t* data;
};

static_assert(sizeof(effect_stream) == 24,
              "effect_stream must match the kernel's custom_fifo_data");
