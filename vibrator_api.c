/****************************************************************************
 * Copyright (C) 2023 Xiaomi Corperation
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

/****************************************************************************
 * @brief Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <netpacket/rpmsg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "vibrator_internal.h"

/****************************************************************************
 * @brief Private Functions
 ****************************************************************************/

/**
 * @brief Fill the vibrator message header
 *
 * @details This function fills the vibrator message header using the specified type.
 *
 * @param buffer The buffer of the vibrator_msg_tS.
 */
static void vibrator_msg_packet(vibrator_msg_t* buffer)
{
    switch (buffer->type) {
    case VIBRATION_WAVEFORM:
    case VIBRATION_INTERVAL:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(vibrator_waveform_t);
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_EFFECT:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(vibrator_effect_t);
        buffer->response_len = VIBRATOR_MSG_HEADER + sizeof(vibrator_effect_t);
        break;
    case VIBRATION_START:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(uint32_t);
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_STOP:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_SET_AMPLITUDE:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(uint8_t);
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_GET_CAPABLITY:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_HEADER + sizeof(int32_t);
        break;
    case VIBRATION_GET_INTENSITY:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_HEADER + sizeof(int32_t);
        break;
    case VIBRATION_SET_INTENSITY:
        buffer->request_len = sizeof(vibrator_intensity_e) + VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_CALIBRATE:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_HEADER + VIBRATOR_CALIBVALUE_MAX;
        break;
    case VIBRATION_SET_CALIBVALUE:
        buffer->request_len = VIBRATOR_MSG_HEADER + VIBRATOR_CALIBVALUE_MAX;
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    default:
        VIBRATORERR("unknown message type %d", buffer->type);
        buffer->request_len = sizeof(vibrator_msg_t);
        buffer->response_len = sizeof(vibrator_msg_t);
        break;
    }
}

/**
 * @brief Open message queue
 *
 * @details This function opens the message queue for vibrator messages.
 *
 * @param buffer The type of the vibrator_msg_t.
 *
 * @return Returns a flag indicating whether the vibration is sent.
 */
static int vibrator_commit(vibrator_msg_t* buffer)
{
    int fd;
    int ret;

#ifdef CONFIG_VIBRATOR_SERVER
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = PROP_SERVER_PATH,
    };
#else
    struct sockaddr_rpmsg addr = {
        .rp_family = AF_RPMSG,
        .rp_name = PROP_SERVER_PATH,
        .rp_cpu = CONFIG_VIBRATOR_SERVER_CPUNAME,
    };
#endif

#ifdef CONFIG_VIBRATOR_SERVER
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    fd = socket(AF_RPMSG, SOCK_STREAM | SOCK_CLOEXEC, 0);
#endif
    if (fd < 0) {
        VIBRATORERR("socket fail, errno = %d", errno);
        return fd;
    }

    ret = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        VIBRATORERR("client: connect failure, errno = %d", errno);
        ret = -errno;
        goto errout;
    }

    vibrator_msg_packet(buffer);

    ret = send(fd, buffer, buffer->request_len, 0);
    if (ret < 0) {
        VIBRATORERR("send fail, errno = %d", errno);
        ret = -errno;
        goto errout;
    }

    ret = recv(fd, buffer, sizeof(vibrator_msg_t), 0);
    if (ret < buffer->response_len) {
        VIBRATORERR("recv fail, errno = %d", errno);
        ret = ret < 0 ? -errno : -EINVAL;
        goto errout;
    }
    VIBRATORINFO("recv len = %d, result = %" PRIi32, ret, buffer->result);
    ret = buffer->result;

errout:
    close(fd);
    return ret;
}

/****************************************************************************
 * @brief Public Functions
 *
 * @details This file contains nine interfaces vibrator_play_waveform,
 *   vibrator_play_oneshot, vibrator_play_predefined, vibrator_get_intensity,
 *   vibrator_set_intensity, vibrator_cancel, vibrator_start,
 *   vibrator_set_amplitude, and vibrator_get_capabilities
 *   and the detailed information of each interface has been described
 *
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
    int8_t repeat, uint8_t length)
{
    vibrator_waveform_t wave;
    vibrator_msg_t buffer;

    if (repeat < -1 || repeat >= length)
        return -EINVAL;

    wave.length = length;
    wave.repeat = repeat;
    memcpy(wave.timings, timings, sizeof(uint32_t) * length);
    memcpy(wave.amplitudes, amplitudes, sizeof(uint8_t) * length);

    buffer.type = VIBRATION_WAVEFORM;
    buffer.wave = wave;

    return vibrator_commit(&buffer);
}

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
    int8_t repeat, uint8_t length)
{
    vibrator_msg_t buffer;

    if (repeat < -1 || repeat >= length)
        return -EINVAL;

    buffer.type = VIBRATION_COMPOSITION;
    buffer.composition.length = length;
    buffer.composition.repeat = repeat;
    buffer.composition.index = 0;
    memcpy(buffer.composition.composite_effect, composite_effects,
        sizeof(vibrator_composite_effect_t) * length);

    return vibrator_commit(&buffer);
}

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
    int16_t count)
{
    vibrator_msg_t buffer;

    if (duration <= 0 || interval < 0 || count < 0)
        return -EINVAL;

    buffer.type = VIBRATION_INTERVAL;
    buffer.wave.timings[0] = duration;
    buffer.wave.timings[1] = interval;
    buffer.wave.count = count;

    return vibrator_commit(&buffer);
}

/**
 * @brief Play a one-shot vibration.
 *
 * @param timing The number of milliseconds to vibrate. Must be positive.
 * @param amplitude The amplitude of vibration; must be a value between 1 and 255, or DEFAULT_AMPLITUDE.
 * @return Returns the flag that the vibrator is playing one shot.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_oneshot(uint32_t timing, uint8_t amplitude)
{
    return vibrator_play_waveform(&timing, &amplitude, -1, 1);
}

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
    int32_t* play_length)
{
    vibrator_msg_t buffer;
    int ret;

    if (es < VIBRATION_LIGHT || es > VIBRATION_DEFAULTES)
        return -EINVAL;

    buffer.type = VIBRATION_EFFECT;
    buffer.effect.effect_id = effect_id;
    buffer.effect.es = es;

    ret = vibrator_commit(&buffer);
    if (ret >= 0) {
        if (play_length != NULL)
            *play_length = buffer.effect.play_length;
    }

    return ret;
}

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
    int32_t* play_length)
{
    vibrator_msg_t buffer;
    int ret;

    if (amplitude < 0.0 || amplitude > 1.0)
        return -EINVAL;

    buffer.type = VIBRATION_PRIMITIVE;
    buffer.effect.effect_id = effect_id;
    buffer.effect.amplitude = amplitude;

    ret = vibrator_commit(&buffer);
    if (ret >= 0) {
        if (play_length != NULL)
            *play_length = buffer.effect.play_length;
    }

    return ret;
}

/**
 * @brief Get vibration intensity.
 *
 * @param intensity Buffer that stores intensity.
 * @return Returns the flag indicating success in getting vibrator intensity.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_intensity(vibrator_intensity_e* intensity)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_GET_INTENSITY;

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        *intensity = buffer.intensity;

    return ret;
}

/**
 * @brief Set vibration intensity.
 *
 * @param intensity The vibration intensity.
 * @return Returns the flag indicating whether setting the intensity was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_intensity(vibrator_intensity_e intensity)
{
    vibrator_msg_t buffer;

    if (intensity < VIBRATION_INTENSITY_LOW || intensity > VIBRATION_INTENSITY_OFF)
        return -EINVAL;

    buffer.type = VIBRATION_SET_INTENSITY;
    buffer.intensity = intensity;

    return vibrator_commit(&buffer);
}

/**
 * @brief Cancel the vibration.
 *
 * @return Returns the flag that the vibration is stopped.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_cancel(void)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_STOP;

    return vibrator_commit(&buffer);
}

/**
 * @brief Start the vibrator with vibrate time.
 *
 * @param timeoutms Number of milliseconds to vibrate.
 * @return Returns the flag that the vibration has started.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_start(int32_t timeoutms)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_START;
    buffer.timeoutms = timeoutms;

    return vibrator_commit(&buffer);
}

/**
 * @brief Set vibration amplitude.
 *
 * @param amplitude The amplitude of vibration; must be a value between 1 and 255, or DEFAULT_AMPLITUDE.
 * @return Returns the flag indicating whether setting the vibrator amplitude was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_amplitude(uint8_t amplitude)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_SET_AMPLITUDE;
    buffer.amplitude = amplitude;

    return vibrator_commit(&buffer);
}

/**
 * @brief Get vibration capabilities.
 *
 * @param capabilities Buffer that stores capabilities.
 * @return Returns the flag indicating success in getting vibrator capability.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_capabilities(int32_t* capabilities)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_GET_CAPABLITY;
    buffer.capabilities = 0;

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        *capabilities = buffer.capabilities;

    return ret;
}

/**
 * @brief Calibrate vibrator when it is not calibrated, Generally at the time of leaving the factory.
 *
 * @param data Buffer that stores the calibration result data.
 * @return Returns the flag indicating whether the vibrator calibration was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_calibrate(uint8_t* data)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_CALIBRATE;
    memset(buffer.calibvalue, 0, VIBRATOR_CALIBVALUE_MAX);

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        memcpy(data, buffer.calibvalue, VIBRATOR_CALIBVALUE_MAX);

    return ret;
}

/**
 * @brief Get vibration calibration data.
 *
 * @param data Buffer that stores calibration data.
 * @return Returns the flag indicating success in setting vibrator calibration data.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_calibvalue(uint8_t* data)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_SET_CALIBVALUE;
    memcpy(buffer.calibvalue, data, VIBRATOR_CALIBVALUE_MAX);

    return vibrator_commit(&buffer);
}