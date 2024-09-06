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
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <log/log.h>
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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vibrator_msg_packet()
 *
 * Description:
 *   fill the vibrator message header using the type
 *
 * Input Parameters:
 *   buffer - the buffer of the vibrator_msg_tS
 *
 ****************************************************************************/

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
    default:
        VIBRATORERR("unknown message type %d", buffer->type);
        buffer->request_len = sizeof(vibrator_msg_t);
        buffer->response_len = sizeof(vibrator_msg_t);
        break;
    }
}

/****************************************************************************
 * Name: vibrator_commit()
 *
 * Description:
 *   open message queue
 *
 * Input Parameters:
 *   buffer - the type of the vibrator_msg_t
 *
 * Returned Value:
 *   returns the flag that the vibration is send
 *
 ****************************************************************************/

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
    VIBRATORINFO("recv len = %d, result = %d", ret, buffer->result);
    ret = buffer->result;

errout:
    close(fd);
    return ret;
}

/****************************************************************************
 * Public Functions
 *
 * Description:
 *   the file contains nine interfaces vibrator_play_waveform,
 *   vibrator_play_oneshot, vibrator_play_predefined, vibrator_get_intensity,
 *   vibrator_set_intensity, vibrator_cancel, vibrator_start,
 *   vibrator_set_amplitude, and vibrator_get_capabilities
 *   and the detailed information of each interface has been described
 *
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

/****************************************************************************
 * Name: vibrator_play_interval()
 *
 * Description:
 *   play a interval vibration with specified duration and interval
 *
 * Input Parameters:
 *   duration - the duration of vibration.
 *   interval - the time interval between two vibration.
 *   count - the times of vibration.
 *
 * Returned Value:
 *   returns the flag that the vibrator playing waveform, greater than or
 *   equal to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

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

int vibrator_play_oneshot(uint32_t timing, uint8_t amplitude)
{
    return vibrator_play_waveform(&timing, &amplitude, -1, 1);
}

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

/****************************************************************************
 * Name: vibrator_play_primitive()
 *
 * Description:
 *   play a predefined vibration effect with specified amplitude
 *
 * Input Parameters:
 *   effectid - the ID of the effect to perform
 *   amplitude - vibration amplitude(0.0~1.0)
 *   play_length - returned effect play duration
 *
 * Returned Value:
 *   returns the flag that the vibrator playing predefined effect, greater
 *   than or equal to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: vibrator_get_intensity()
 *
 * Description:
 *   get vibration intensity
 *
 * Input Parameters:
 *   intensity - buffer that store intensity
 *
 * Returned Value:
 *   returns the flag indicating get vibrator intensity, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

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

int vibrator_set_intensity(vibrator_intensity_e intensity)
{
    vibrator_msg_t buffer;

    if (intensity < VIBRATION_INTENSITY_LOW || intensity > VIBRATION_INTENSITY_OFF)
        return -EINVAL;

    buffer.type = VIBRATION_SET_INTENSITY;
    buffer.intensity = intensity;

    return vibrator_commit(&buffer);
}

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

int vibrator_cancel(void)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_STOP;

    return vibrator_commit(&buffer);
}

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

int vibrator_start(int32_t timeoutms)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_START;
    buffer.timeoutms = timeoutms;

    return vibrator_commit(&buffer);
}

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

int vibrator_set_amplitude(uint8_t amplitude)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_SET_AMPLITUDE;
    buffer.amplitude = amplitude;

    return vibrator_commit(&buffer);
}

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
