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

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * @brief Public Types
 ****************************************************************************/

/**
 * @brief Vibrator HAL capability
 */
enum {
    CAP_ON_CALLBACK = 1, /**< Callback for on action */
    CAP_PERFORM_CALLBACK = 2, /**< Callback for perform action */
    CAP_AMPLITUDE_CONTROL = 4, /**< Amplitude control capability */
    CAP_EXTERNAL_CONTROL = 8, /**< External control capability */
    CAP_EXTERNAL_AMPLITUDE_CONTROL = 16, /**< External amplitude control capability */
    CAP_COMPOSE_EFFECTS = 32, /**< Compose effects capability */
    CAP_ALWAYS_ON_CONTROL = 64 /**< Always on control capability */
};

/**
 * @brief Vibration effect IDs
 */
typedef enum {
    CLICK = 0, /**< Click effect */
    DOUBLE_CLICK = 1, /**< Double click effect */
    TICK = 2, /**< Tick effect */
    THUD = 3, /**< Thud effect */
    POP = 4, /**< Pop effect */
    HEAVY_CLICK = 5 /**< Heavy click effect */
} vibratior_effect_id_e;

/**
 * @brief Vibration intensity for predefined effects
 */
typedef enum {
    VIBRATION_LIGHT = 0, /**< Light vibration */
    VIBRATION_MEDIUM = 1, /**< Medium vibration */
    VIBRATION_STRONG = 2, /**< Strong vibration */
    VIBRATION_DEFAULTES = 3 /**< Default vibration */
} vibrator_effect_strength_e;

/**
 * @brief Vibration intensity levels
 */
typedef enum {
    VIBRATION_INTENSITY_LOW = 0, /**< Low intensity */
    VIBRATION_INTENSITY_MEDIUM = 1, /**< Medium intensity */
    VIBRATION_INTENSITY_HIGH = 2, /**< High intensity */
} vibrator_intensity_e;

/**
 * @brief Structure representing a composite effect
 *
 * This structure defines the parameters for a vibration composition,
 * including delay time, vibration pattern, and intensity scale.
 */
typedef struct {
    int32_t delay_ms; /**< Period of silence preceding primitive */
    uint32_t primitive; /**< Identifier for the primitive effect */
    float scale; /**< Scale factor for the primitive effect, 0.0-1.0 */
} vibrator_composite_effect_t;

/**
 * @brief Vibration callback function type.
 *
 * This function is called when the async vibration operation is completed.
 *
 * @param handle The handle of the long-term connection to the vibrator.
 * @param cookie The user-defined data passed to the vibration function.
 * @param arg The argument passed to the user-defined callback function.
 * @param ret The result of the vibration operation. 0 means success,
 *            negative value means failure.
 */
typedef void (*vibrator_uv_callback)(void* handle, void* cookie,
    void* arg, int ret);

/****************************************************************************
 * @brief Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Play a waveform vibration.
 *
 * @param timings The pattern of alternating on-off timings, starting with off.
 *                Timing values of 0 will cause the timing/amplitude pair to be ignored.
 * @param amplitudes The amplitude values of the timing/amplitude pairs.
 *                   Amplitude values must be between 0 and 255, or equal to DEFAULT_AMPLITUDE.
 *                   An amplitude value of 0 implies the motor is off.
 * @param repeat The index into the timings array at which to repeat, or -1 if
 *               you don't want to repeat.
 * @param length The length of timings and amplitudes pairs.
 * @return Returns the flag that the vibrator is playing waveform.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_waveform(uint32_t timings[], uint8_t amplitudes[],
    int8_t repeat, uint8_t length);

/**
 * @brief Play an interval vibration with specified duration and interval.
 *
 * @param duration The duration of vibration.
 * @param interval The time interval between two vibrations.
 * @param count The number of vibrations.
 * @return Returns the flag that the vibrator is playing interval.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_interval(int32_t duration, int32_t interval,
    int16_t count);

/**
 * @brief Play a one-shot vibration.
 *
 * @param timing The number of milliseconds to vibrate. Must be positive.
 * @param amplitude The amplitude of vibration; must be a value between 1 and 255, or DEFAULT_AMPLITUDE.
 * @return Returns the flag that the vibrator is playing one shot.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_oneshot(uint32_t timing, uint8_t amplitude);

/**
 * @brief Play a predefined vibration effect.
 *
 * @param effect_id The ID of the effect to perform.
 * @param es The vibration intensity.
 * @param play_length Returned effect play duration.
 * @return Returns the flag that the vibrator is playing the predefined effect.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_predefined(uint8_t effect_id, vibrator_effect_strength_e es,
    int32_t* play_length);

/**
 * @brief Play a predefined vibration effect with the specified amplitude.
 *
 * @param effect_id The ID of the effect to perform.
 * @param amplitude Vibration amplitude (0.0~1.0).
 * @param play_length Returned effect play duration.
 * @return Returns the flag that the vibrator is playing the predefined effect.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_primitive(uint8_t effect_id, float amplitude,
    int32_t* play_length);

/**
 * @brief Play composed primitive effect.
 *
 * @param composite_effects The composition of primitive effects.
 * @param repeat The index into the primitive array at which to repeat, or -1 if
 *               you don't want to repeat.
 * @param length The length of composite effects array.
 * @return Returns the flag that the vibrator is playing the predefined effect.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_compose(vibrator_composite_effect_t* composite_effects,
    int8_t repeat, uint8_t length);

/**
 * @brief Get vibration intensity.
 *
 * @param intensity Buffer that stores intensity.
 * @return Returns the flag indicating success in getting vibrator intensity.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_intensity(vibrator_intensity_e* intensity);

/**
 * @brief Set vibration intensity.
 *
 * @param intensity The vibration intensity.
 * @return Returns the flag indicating whether setting the intensity was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_intensity(vibrator_intensity_e intensity);

/**
 * @brief Get vibration is disabled or not.
 *
 * @param disabled Buffer that stores disabled status.
 * @return Returns the flag indicating success in getting vibrator
 *         disabled status. Greater than or equal to 0 means success;
 *         otherwise, it means failure.
 */
int vibrator_is_disabled(uint8_t* disabled);

/**
 * @brief Set vibration disable or not.
 *
 * @param disable The vibration disable flag.
 * @return Returns the flag indicating success in setting vibrator
 *         disabled status. Greater than or equal to 0 means success;
 *         otherwise, it means failure.
 */
int vibrator_set_disable(uint8_t disable);

/**
 * @brief Cancel the vibration.
 *
 * @return Returns the flag that the vibration is stopped.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_cancel(void);

/**
 * @brief Start the vibrator with vibrate time.
 *
 * @param timeoutms Number of milliseconds to vibrate.
 * @return Returns the flag that the vibration has started.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_start(int32_t timeoutms);

/**
 * @brief Set vibration amplitude.
 *
 * @param amplitude The amplitude of vibration; must be a value between 1 and 255, or DEFAULT_AMPLITUDE.
 * @return Returns the flag indicating whether setting the vibrator amplitude was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_amplitude(uint8_t amplitude);

/**
 * @brief Get vibration capabilities.
 *
 * @param capabilities Buffer that stores capabilities.
 * @return Returns the flag indicating success in getting vibrator capability.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_capabilities(int32_t* capabilities);

/**
 * @brief Get vibration primitive effect duration.
 *
 * @param effect_id The ID of the effect.
 * @param duration BUffer that stores the effect duration.
 * @return Returns the flag indicating success in getting vibrator duration.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_primitive_duration(uint8_t effect_id, int32_t* duration);

/**
 * @brief Calibrate vibrator when it is not calibrated, Generally at the time of leaving the factory.
 *
 * @param data Buffer that stores the calibration result data.
 * @return Returns the flag indicating whether the vibrator calibration was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_calibrate(uint8_t* data);

/**
 * @brief Get vibration calibration data.
 *
 * @param data Buffer that stores calibration data.
 * @return Returns the flag indicating success in setting vibrator calibration data.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_calibvalue(uint8_t* data);

/**
 * @brief Send custom control command to the vibrator device driver.
 *
 * @param cmd Custom ioctl command code.
 * @param arg The control data send or received.
 * @param len The length of the control data.
 * @return Returns 0 on success, or a negative error code on failure.
 */
int vibrator_control(uint32_t cmd, uint8_t* arg, uint32_t len);

#ifdef CONFIG_VIBRATOR_UV_API
/**
 * @brief Asynchronously sends a request to play a predefined vibration effect to
 *        the vibrator server over an established long connection.
 *
 * @param handle The handle to use for the connection.
 * @param effect_id The ID of the predefined vibration effect to be played.
 * @param es The strength of the vibration effect.
 * @param cb The callback function to be called when the request is sent.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
int vibrator_uv_play_predefined(void* handle, uint8_t effect_id,
    vibrator_effect_strength_e es, vibrator_uv_callback cb);

/**
 * @brief Build a long-term connection with the vibrator server async.
 *
 * @param on_connect The callback function to be called when the connection
 *                   is established.
 * @param cookie     Long-term context, for on_connect.
 * @return Returns handle to the pipe on success, or NULL on failure.
 */
void* vibrator_uv_connect(vibrator_uv_callback on_connect, void* cookie);

/**
 * @brief Disconnect the connection.
 *
 * @param handle The handle to the pipe to be disconnected.
 */
void vibrator_uv_disconnect(void* handle);
#endif

#ifdef __cplusplus
}
#endif

#endif /* #define __INCLUDE_VIBRATOR_API_H */
