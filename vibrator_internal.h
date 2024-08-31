/****************************************************************************
 *
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#ifndef __INCLUDE_VIBRATOR_H
#define __INCLUDE_VIBRATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "vibrator_api.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PROP_SERVER_PATH "vibratord"
#define WAVEFORM_MAXNUM 24
#define VIBRATOR_MSG_HEADER 8
#define VIBRATOR_MSG_RESULT 4

#ifdef CONFIG_VIBRATOR_ERROR
#define VIBRATORERR(format, args...) SLOGE(format, ##args)
#else
#define VIBRATORERR(format, args...)
#endif

#ifdef CONFIG_VIBRATOR_WARN
#define VIBRATORWARN(format, args...) SLOGW(format, ##args)
#else
#define VIBRATORWARN(format, args...)
#endif

#ifdef CONFIG_VIBRATOR_INFO
#define VIBRATORINFO(format, args...) SLOGI(format, ##args)
#else
#define VIBRATORINFO(format, args...)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Vibrator operation types */

enum {
    VIBRATION_WAVEFORM = 1,
    VIBRATION_EFFECT,
    VIBRATION_COMPOSITION,
    VIBRATION_START,
    VIBRATION_STOP,
    VIBRATION_PRIMITIVE,
    VIBRATION_INTERVAL,
    VIBRATION_SET_AMPLITUDE,
    VIBRATION_GET_CAPABLITY,
    VIBRATION_SET_INTENSITY,
    VIBRATION_GET_INTENSITY
};

/* struct vibrator_waveform_t
 * @repeat: indicates the location of the start of the array vibration
 * @length: the length of arrays timings and amplitudes
 * @timings: the timings array
 * @amplitudes: the amplitudes array
 */

begin_packed_struct typedef struct {
    int8_t repeat;
    uint8_t length;
    int16_t count;
    uint8_t amplitudes[WAVEFORM_MAXNUM];
    uint32_t timings[WAVEFORM_MAXNUM];
} vibrator_waveform_t end_packed_struct;

/* struct vibrator_effect_t
 * @effect_id: the id of effect
 * @es: the intensity of vibration
 * @play_length: returned effect play length
 */

begin_packed_struct typedef struct {
    int32_t effect_id;
    int32_t play_length;
    union {
        uint8_t es;
        float amplitude;
    };
} vibrator_effect_t end_packed_struct;

/* struct vibrator_msg_t
 * @type: vibrator of type
 * @effect: the vibrator_effect_t of above structure
 * @wave: the vibrator_waveform_t of above structure
 * @intensity: the intensity of vibration
 * @comp: the vibrator_compositions_t of above structure
 * @timeoutms: the number of milliseconds to vibrate
 * @amplitude: the amplitude of vibration
 * @capabilities: the capabilities of vibrator
 */

begin_packed_struct typedef struct {
    int32_t result;
    uint8_t type;
    uint8_t request_len;
    uint8_t response_len;
    union {
        uint8_t intensity;
        uint8_t amplitude;
        uint32_t timeoutms;
        int32_t capabilities;
        vibrator_waveform_t wave;
        vibrator_effect_t effect;
    };
} vibrator_msg_t end_packed_struct;

#endif /* #define __INCLUDE_VIBRATOR_H */
