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

#ifndef __INCLUDE_VIBRATOR_API_H
#define __INCLUDE_VIBRATOR_API_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Vibrator hal capabitity*/

enum {
    CAP_ON_CALLBACK = 1,
    CAP_PERFORM_CALLBACK = 2,
    CAP_AMPLITUDE_CONTROL = 4,
    CAP_EXTERNAL_CONTROL = 8,
    CAP_EXTERNAL_AMPLITUDE_CONTROL = 16,
    CAP_COMPOSE_EFFECTS = 32,
    CAP_ALWAYS_ON_CONTROL = 64
};

/* Vibration effect id */

typedef enum {
    CLICK = 0,
    DOUBLE_CLICK = 1,
    TICK = 2,
    THUD = 3,
    POP = 4,
    HEAVY_CLICK = 5
} vibratior_effect_id_e;

/* Vibration intensity for predefined*/

typedef enum {
    VIBRATION_LIGHT = 0,
    VIBRATION_MEDIUM = 1,
    VIBRATION_STRONG = 2
} vibrator_effect_strength_e;

/* Vibration intensity */

typedef enum {
    VIBRATION_INTENSITY_LOW = 0,
    VIBRATION_INTENSITY_MEDIUM = 1,
    VIBRATION_INTENSITY_HIGH = 2,
    VIBRATION_INTENSITY_OFF = 3
} vibrator_intensity_e;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vibrator_play_waveform()
 *
 * Description:
 *   play a waveform vibration
 *
 * Input Parameters:
 *   timings - the pattern of alternating on-off timings, starting with off,
 *             timing values of 0 will cause the timing / amplitude pair to
 *             be ignored
 *   amplitudes - the amplitude values of the timing / amplitude pairs.
 *                Amplitude values must be between 0 and 255, or equal to
 *                DEFAULT_AMPLITUDE. An amplitude value of 0 implies the
 *                motor is off.
 *   repeat - the index into the timings array at which to repeat, or -1 if
 *            you you don't want to repeat.
 *   length - the length of timings and amplitudes pairs
 *
 * Returned Value:
 *   returns the flag that the vibrator playing waveform, greater than or
 *   equal to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_play_waveform(uint32_t timings[], uint8_t amplitudes[],
    int8_t repeat, uint8_t length);

/****************************************************************************
 * Name: vibrator_play_oneshot()
 *
 * Description:
 *   play a one shot vibration
 *
 * Input Parameters:
 *   timing - the number of milliseconds to vibrate, must be positive
 *   amplitude - the amplitude of vibration, must be a value between 1 and
 *               255, or DEFAULT_AMPLITUDE
 *
 * Returned Value:
 *   returns the flag that the vibrator playing oneshot, greater than or
 *   equal to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_play_oneshot(uint32_t timing, uint8_t amplitude);

/****************************************************************************
 * Name: vibrator_play_predefined()
 *
 * Description:
 *   play a predefined vibration effect
 *
 * Input Parameters:
 *   effectid - the ID of the effect to perform
 *   es - vibration intensity
 *   play_length - returned effect play duration
 *
 * Returned Value:
 *   returns the flag that the vibrator playing predefined effect, greater
 *   than or equal to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_play_predefined(uint8_t effect_id, vibrator_effect_strength_e es,
    int32_t* play_length);

/****************************************************************************
 * Name: vibrator_get_intensity()
 *
 * Description:
 *   get vibration intensity
 *
 * Returned Value:
 *   returns the vibration intensity
 *
 ****************************************************************************/

vibrator_intensity_e vibrator_get_intensity(void);

/****************************************************************************
 * Name: vibrator_set_intensity()
 *
 * Description:
 *   set vibration intensity
 *
 * Input Parameters:
 *   intensity - the vibration intensity
 *
 * Returned Value:
 *   returns the flag indicating the intensity whether setting was successful,
 *   greater than or equal to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_set_intensity(vibrator_intensity_e intensity);

/****************************************************************************
 * Name: vibrator_cancel()
 *
 * Description:
 *   cancel the vibration
 *
 * Returned Value:
 *   returns the flag that the vibration is closed, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_cancel(void);

/****************************************************************************
 * Name: vibrator_start()
 *
 * Description:
 *   start the vibrator with vibrate time
 *
 * Input Parameters:
 *   timeoutms - number of milliseconds to vibrate
 *
 * Returned Value:
 *   returns the flag that the vibration start, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_start(int32_t timeoutms);

/****************************************************************************
 * Name: vibrator_set_amplitude()
 *
 * Description:
 *   set vibration amplitude
 *
 * Input Parameters:
 *   amplitude - the amplitude of vibration, must be a value between 1 and
 *               255, or DEFAULT_AMPLITUDE
 *
 * Returned Value:
 *   returns the flag indicating set vibrator amplitude, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_set_amplitude(uint8_t amplitude);

/****************************************************************************
 * Name: vibrator_get_capabilities()
 *
 * Description:
 *   get vibration capabilities
 *
 * Input Parameters:
 *   capabilities - buffer that store capabilities
 *
 * Returned Value:
 *   returns the flag indicating get vibrator capability, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_get_capabilities(int32_t* capabilities);

#ifdef __cplusplus
}
#endif

#endif /* #define __INCLUDE_VIBRATOR_API_H */
