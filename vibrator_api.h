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

#ifndef __INCLUDE_NUTTX_VIBRATOR_API_H
#define __INCLUDE_NUTTX_VIBRATOR_API_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <mqueue.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define QUEUE_NAME   "/vibratord"
#define MAX_MSG_NUM  10
#define MAX_MSG_SIZE 256

#define MAX_AMPLITUDE     255
#define DEFAULT_AMPLITUDE -1

#define EFFECT_TICK         2
#define EFFECT_CLICK        0
#define EFFECT_HEAVY_CLICK  5
#define EFFECT_DOUBLE_CLICK 1

#define WAV1                1 //12.8ms 360hz 4cyle,crown used
#define WAV2                2 //16.6ms 240hz 3cycle
#define WAV3_1              3 //31.3ms 240hz 4cycle
#define WAV3_3              4 //41.2ms 240hz 5cycle
#define WAV_7               5 //49.2ms 240hz 7cycle
#define WAV_8               6 //12.5ms max long
#define WAV_9               7 //20.83ms mid long
#define WAV_10              8 //20.83ms min long
#define WAIT(code)          (uint8_t)(code / 5.333) | 0x80

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Vibrator types */

enum {
    VIBRATION_WAVEFORM = 1,
    VIBRATION_EFFECT = 2,
    VIBRATION_COMPOSITION = 3,
    VIBRATION_STOP = 4
};

enum {
    VIBRATION_TYPE_NONE,
    VIBRATION_TYPE_CROWN,
    VIBRATION_TYPE_KEY_BOARD,
    VIBRATION_TYPE_WATCH_FACE,
    VIBRATION_TYPE_SUCCESS,
    VIBRATION_TYPE_FAILED,
    VIBRATION_TYPE_SYSTEM_OPRATION,
    VIBRATION_TYPE_HEALTH_ALERT,
    VIBRATION_TYPE_SYSTEM_EVENT,
    VIBRATION_TYPE_NOTIFICATION,
    VIBRATION_TYPE_TARGET_DONE,
    VIBRATION_TYPE_BREATHING_TRAINING,
    VIBRATION_TYPE_INCOMING_CALL,
    VIBRATION_TYPE_CLOCK_ALARM,
    VIBRATION_TYPE_SLEEP_ALARM,
    VIBRATION_TYPE_COUNT,
};

/* struct waveform
 * @repeat: indicates the location of the start of the array vibration
 * @length: the length of arrays timings and amplitudes
 * @timings: the timings array
 * @amplitudes: the amplitudes array
 */

typedef struct {
    uint8_t length;
    uint8_t repeat;
    uint32_t timings[24];
    uint8_t amplitudes[24];
} waveform_t;

/* struct composition
 * @patternid: waveform or wait time
 * @waveloop: number of waveform vibrations
 * @mainloop: the number of vibrations of the patternid array
 * @strength: the number of strength of the patternid array
 * @duration: duration in one set of vibrations
 */

typedef struct {
    uint8_t patternid[8];
    uint8_t waveloop[8];
    uint8_t mainloop;
    float strength;
    uint32_t duration;
} composition_t;

/* struct compositions
 * @count: size of pattern
 * @repeatable: the number of times array pattern is played repeatedly
 * @pattern: the number of vibrations of the patternid array
 */

typedef struct {
    uint32_t count;
    uint8_t repeatable;
    composition_t pattern[5];
} compositions_t;

/****************************************************************************
 * Name: create_compositions()
 *
 * Description:
 *   create the compositions interface for app
 *
 * Input Parameters:
 *   var_id - the subscript of the data array
 ****************************************************************************/

uint8_t create_compositions(uint8_t var_id);

/****************************************************************************
 * Name: create_waveform()
 *
 * Description:
 *    create waveform vibration effects
 *
 * Input Parameters:
 *   timings - timings the element of the timings array
 *   is the duration of the vibration or the duration of the wait
 *   amplitudes - amplitudes are the amplitudes of vibrations
 *   length - length is timings and amplitudes
 *   repeat - repeat is the subscript that indicates the start of
 *   the vibration
 *
 ****************************************************************************/

uint8_t create_waveform(uint32_t timing[], uint8_t amplitudes[],
                        uint8_t repeat, u_int8_t length);

/****************************************************************************
 * Name: create_oneshot()
 *
 * Description:
 *    create waveform vibration effects
 *
 * Input Parameters:
 *   timing - duration of vibration
 *   amplitude - amplitude of vibration
 *
 ****************************************************************************/

uint8_t create_oneshot(uint32_t timing, uint8_t amplitude);

/****************************************************************************
 * Name: create_predefined()
 *
 * Devoidscription:
 *    create the predefined interface for app
 *
 * Input Parameters:
 *   effectid - effectid of vibrator
 *
 ****************************************************************************/

uint8_t create_predefined(uint8_t effectid);

/****************************************************************************
 * Name: vibrator_cancel()
 *
 * Description:
 *    cancel the motor of vibration_id vibration
 *
 * Input Parameters:
 *   vibration_id - vibration_id of vibrator
 *
 ****************************************************************************/

void vibrator_cancel(uint8_t vibration_id);

#endif /* #define __INCLUDE_NUTTX_VIBRATOR_API_H */
