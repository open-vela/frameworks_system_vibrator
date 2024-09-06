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

#include <fcntl.h>
#include <kvdb.h>
#include <log/log.h>
#include <mqueue.h>
#include <netpacket/rpmsg.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include <nuttx/input/ff.h>

#include "vibrator_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIBRATOR_LOCAL 0
#define VIBRATOR_REMOTE 1
#define VIBRATOR_COUNT 2
#define VIBRATOR_MAX_CLIENTS 16
#define VIBRATOR_MAX_AMPLITUDE 255
#define VIBRATOR_DEFAULT_AMPLITUDE -1
#define VIBRATOR_INVALID_VALUE -1
#define VIBRATOR_STRONG_MAGNITUDE 0x7fff
#define VIBRATOR_MEDIUM_MAGNITUDE 0x5fff
#define VIBRATOR_LIGHT_MAGNITUDE 0x3fff
#define VIBRATOR_CUSTOM_DATA_LEN 3
#define VIBRATOR_DEV_FS "/dev/lra0"
#define KVDB_KEY_VIBRATOR_MODE "persist.vibrator_mode"
#define KVDB_KEY_VIBRATOR_ENABLE "persist.vibration_enable"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    int fd;
    int16_t curr_app_id;
    int16_t curr_magnitude;
    uint8_t curr_amplitude;
    int32_t capabilities;
    vibrator_intensity_e intensity;
} ff_dev_t;

typedef struct {
    bool forcestop;
    bool condition_is_met;
    vibrator_waveform_t wave;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    vibrator_msg_t msg;
    uv_timer_t timer;
    ff_dev_t* ff_dev;
} threadargs;

typedef struct vibrator_context_s {
    uv_poll_t poll_handle;
    uv_os_sock_t sock;
    threadargs* thread_args;
} vibrator_context_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ff_play()
 *
 * Description:
 *    operate the driver's interface, vibrator device control
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   effect_id - ID of the predefined effect will be played. If effectId is
 *               valid(non-negative value), the timeout_ms value will be
 *               ignored, and the real playing length will be set in
 *               playLengtMs and returned to VibratorService. If effectId is
 *               invalid, value in param timeout_ms will be used as the play
 *               length for playing a constant effect.
 *   timeout_ms - playing length, non-zero means playing, zero means stop
 *                playing.
 *   play_length_ms - the playing length in ms unit which will be returned to
 *                    VibratorService if the request is playing a predefined
 *                    effect.
 *
 * Returned Value:
 *   return the ret of file system operations
 *
 ****************************************************************************/

static int ff_play(ff_dev_t* ff_dev, int effect_id, uint32_t timeout_ms,
    long* play_length_ms)
{
    int16_t data[VIBRATOR_CUSTOM_DATA_LEN] = { 0, 0, 0 };
    struct ff_effect effect;
    struct ff_event_s play;
    int ret;

    memset(&play, 0, sizeof(play));
    if (timeout_ms != 0) {

        /* if curr_app_id is valid, then remove the effect from the device
           first */

        if (ff_dev->curr_app_id != VIBRATOR_INVALID_VALUE) {
            ret = ioctl(ff_dev->fd, EVIOCRMFF, ff_dev->curr_app_id);
            if (ret < 0) {
                VIBRATORERR("ioctl EVIOCRMFF failed, errno = %d", errno);
                goto errout;
            }
            ff_dev->curr_app_id = VIBRATOR_INVALID_VALUE;
        }

        /* if effect_id is valid, then upload the predefined effect with
           effect_id, else upload a constant effect with timeout_ms */

        memset(&effect, 0, sizeof(effect));
        if (effect_id != VIBRATOR_INVALID_VALUE) {
            data[0] = effect_id;
            effect.type = FF_PERIODIC;
            effect.u.periodic.waveform = FF_CUSTOM;
            effect.u.periodic.magnitude = ff_dev->curr_magnitude;
            effect.u.periodic.custom_data = data;
            effect.u.periodic.custom_len = sizeof(int16_t) * VIBRATOR_CUSTOM_DATA_LEN;
        } else {
            effect.type = FF_CONSTANT;
            effect.u.constant.level = ff_dev->curr_magnitude;
            effect.replay.length = timeout_ms;
        }

        effect.id = ff_dev->curr_app_id;
        effect.replay.delay = 0;

        ret = ioctl(ff_dev->fd, EVIOCSFF, &effect);
        if (ret < 0) {
            VIBRATORERR("ioctl EVIOCSFF failed, errno = %d", errno);
            goto errout;
        }

        /* update the curr_app_id with the ID obtained from device driver */

        ff_dev->curr_app_id = effect.id;

        /* return the effect play length to vibrator service */

        if (effect_id != VIBRATOR_INVALID_VALUE && play_length_ms != NULL) {
            *play_length_ms = data[1] * 1000 + data[2];
            VIBRATORINFO("*play_length_ms = %ld", *play_length_ms);
        }

        if (ff_dev->curr_app_id == VIBRATOR_INVALID_VALUE)
            return ret;

        /* write the play event to vibrator device */

        play.value = 1;
        play.code = ff_dev->curr_app_id;
        ret = write(ff_dev->fd, (const void*)&play, sizeof(play));
        if (ret < 0) {
            VIBRATORERR("write failed, errno = %d", errno);
            ret = ioctl(ff_dev->fd, EVIOCRMFF, ff_dev->curr_app_id);
            if (ret < 0)
                VIBRATORERR("ioctl EVIOCRMFF failed, errno = %d", errno);
            goto errout;
        }
    } else if (ff_dev->curr_app_id != VIBRATOR_INVALID_VALUE) {

        /* stop vibration if timeout_ms is zero and curr_app_id is valid */

        ret = ioctl(ff_dev->fd, EVIOCRMFF, ff_dev->curr_app_id);
        if (ret < 0) {
            VIBRATORERR("ioctl EVIOCRMFF failed, errno = %d", errno);
            goto errout;
        }
        ff_dev->curr_app_id = VIBRATOR_INVALID_VALUE;
    }
    return 0;

errout:
    ff_dev->curr_app_id = VIBRATOR_INVALID_VALUE;
    return ret;
}

/****************************************************************************
 * Name: ff_set_amplitude()
 *
 * Description:
 *   operate the driver's interface, set gain to vibrator device
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   amplitude - vibration instensity, range[0,255]
 *
 * Returned Value:
 *   return the ret of write
 *
 ****************************************************************************/

static int ff_set_amplitude(ff_dev_t* ff_dev, uint8_t amplitude)
{
    struct ff_event_s gain;
    int tmp;
    int ret;

    memset(&gain, 0, sizeof(gain));

    tmp = amplitude * (VIBRATOR_STRONG_MAGNITUDE - VIBRATOR_LIGHT_MAGNITUDE) / 255;
    tmp += VIBRATOR_LIGHT_MAGNITUDE;

    gain.code = FF_GAIN;
    gain.value = tmp;

    ret = write(ff_dev->fd, &gain, sizeof(gain));
    if (ret < 0) {
        VIBRATORERR("write FF_GAIN failed, errno = %d", errno);
        return ret;
    }

    ff_dev->curr_magnitude = tmp;

    return ret;
}

/****************************************************************************
 * Name: play_effect()
 *
 * Description:
 *    play the predefined effect.
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   effect_id - ID of the predefined effect will be played.
 *   es - effect intensity.
 *   play_length_ms - the playing length in ms unit which will be returned to
 *                    VibratorService if the request is playing a predefined
 *                    effect.
 *
 * Returned Value:
 *   return the ret of ff_play
 *
 ****************************************************************************/

static int play_effect(ff_dev_t* ff_dev, int effect_id,
    vibrator_effect_strength_e es, long* play_length_ms)
{
    switch (es) {
    case VIBRATION_LIGHT: {
        ff_dev->curr_magnitude = VIBRATOR_LIGHT_MAGNITUDE;
        break;
    }
    case VIBRATION_MEDIUM: {
        ff_dev->curr_magnitude = VIBRATOR_MEDIUM_MAGNITUDE;
        break;
    }
    case VIBRATION_STRONG: {
        ff_dev->curr_magnitude = VIBRATOR_STRONG_MAGNITUDE;
        break;
    }
    default: {
        break;
    }
    }

    return ff_play(ff_dev, effect_id, VIBRATOR_INVALID_VALUE,
        play_length_ms);
}

/****************************************************************************
 * Name: play_primitive()
 *
 * Description:
 *    play the predefined effect with the specified amplitude.
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   effect_id - ID of the predefined effect will be played.
 *   amplitude - effect amplitude(0-1.0).
 *   play_length_ms - the playing length in ms unit which will be returned to
 *                    VibratorService if the request is playing a predefined
 *                    effect.
 *
 * Returned Value:
 *   return the ret of ff_play
 *
 ****************************************************************************/

static int play_primitive(ff_dev_t* ff_dev, int effect_id,
    float amplitude, long* play_length_ms)
{
    int tmp;

    tmp = (uint8_t)(amplitude * VIBRATOR_MAX_AMPLITUDE);
    ff_dev->curr_magnitude = tmp * (VIBRATOR_STRONG_MAGNITUDE - VIBRATOR_LIGHT_MAGNITUDE) / 255;
    ff_dev->curr_magnitude += VIBRATOR_LIGHT_MAGNITUDE;

    return ff_play(ff_dev, effect_id, VIBRATOR_INVALID_VALUE,
        play_length_ms);
}

/****************************************************************************
 * Name: on()
 *
 * Description:
 *    turn on vibrator.
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   timeout_ms - number of milliseconds to vibrate
 *
 * Returned Value:
 *   return the ret of ff_play
 *
 ****************************************************************************/

static int on(ff_dev_t* ff_dev, uint32_t timeout_ms)
{
    return ff_play(ff_dev, VIBRATOR_INVALID_VALUE, timeout_ms, NULL);
}

/****************************************************************************
 * Name: off()
 *
 * Description:
 *    turn off vibrator.
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *
 * Returned Value:
 *   return the ret of ff_play
 *
 ****************************************************************************/

static int off(ff_dev_t* ff_dev)
{
    return ff_play(ff_dev, VIBRATOR_INVALID_VALUE, 0, NULL);
}

/****************************************************************************
 * Name: scale()
 *
 * Description:
 *    scale the amplitude with the given intensity.
 *
 * Input Parameters:
 *   amplitude - vibration amplitude, range 0 - 255
 *   intensity - vibration intensity
 *
 * Returned Value:
 *   return the scaled vibration amplitude
 *
 ****************************************************************************/

static int scale(int amplitude, vibrator_intensity_e intensity)
{
    int scale_amplitude;

    switch (intensity) {
    case VIBRATION_INTENSITY_LOW:
        scale_amplitude = amplitude * 0.3;
        break;
    case VIBRATION_INTENSITY_MEDIUM:
        scale_amplitude = amplitude * 0.6;
        break;
    case VIBRATION_INTENSITY_HIGH:
        scale_amplitude = amplitude * 1;
        break;
    default:
        scale_amplitude = VIBRATOR_MAX_AMPLITUDE;
    }

    return scale_amplitude;
}

/****************************************************************************
 * Name: should_vibrate()
 *
 * Description:
 *    confirm whether vibration is allowed.
 *
 * Input Parameters:
 *   intensity - vibration intensity
 *
 * Returned Value:
 *   true: allowed, false: not allowed
 *
 ****************************************************************************/

static bool should_vibrate(vibrator_intensity_e intensity)
{
    if (intensity == VIBRATION_INTENSITY_OFF)
        return false;
    return true;
}

/****************************************************************************
 * Name: should_repeat()
 *
 * Description:
 *    confirm whether waveform repeat is allowed.
 *
 * Input Parameters:
 *   repeat - the index into the timings array at which to repeat
 *   timings - motor vibration time
 *   len - length of timings array
 *
 * Returned Value:
 *   true: allowed, false: not allowed
 *
 ****************************************************************************/

static bool should_repeat(int repeat, uint32_t timings[], uint8_t len)
{
    int ret = false;

    while (repeat < len) {
        if (timings[repeat] != 0) {
            ret = true;
            break;
        }
        repeat++;
    }

    return ret;
}

/****************************************************************************
 * Name: receive_stop()
 *
 * Description:
 *   stop vibration
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *
 * Returned Value:
 *   return stop ioctl value
 *
 ****************************************************************************/

static int receive_stop(ff_dev_t* ff_dev)
{
    return off(ff_dev);
}

/****************************************************************************
 * Name: receive_start()
 *
 * Description:
 *   start a constant vibration with timeoutms
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   timeoutms - number of milliseconds to vibrate
 *
 * Returned Value:
 *   return stop ioctl value
 *
 ****************************************************************************/

static int receive_start(ff_dev_t* ff_dev, uint32_t timeoutms)
{
    int scale_amplitude;
    int ret;

    if (!should_vibrate(ff_dev->intensity))
        return -ENOTSUP;

    scale_amplitude = scale(ff_dev->curr_amplitude, ff_dev->intensity);

    /* Note: ordering is important here! Many haptic drivers will reset their
       amplitude when enabled, so we always have to enable first, then set
       the amplitude. */

    ret = on(ff_dev, timeoutms);
    if (ret < 0) {
        VIBRATORERR("Error: ioctl failed, errno = %d", errno);
    }

    return ff_set_amplitude(ff_dev, scale_amplitude);
}

/****************************************************************************
 * Name: get_total_on_duration()
 *
 * Description:
 *   calculate the time when the continuous amplitude value of
 *   the elements in the array is not 0
 *
 * Input Parameters:
 *   timings - motor vibration time
 *   amplitudes - motor of amplitudes
 *   start_index - starting subscript
 *   repeat_index - cyclic repeat subscript
 *   len - length of timings and amplitudes array
 *
 * Returned Value:
 *   return the on_duration time
 *
 ****************************************************************************/

static int32_t get_total_on_duration(uint32_t timings[], uint8_t amplitudes[],
    uint8_t start_index, int8_t repeat_index, uint8_t len)
{
    uint8_t i = start_index;
    uint32_t timing = 0;

    while (amplitudes[i] != 0) {
        timing += timings[i++];
        if (i >= len) {
            if (repeat_index >= 0) {
                i = repeat_index;
                repeat_index = -1;
            } else {
                break;
            }
        }
        if (i == start_index) {
            return 1000;
        }
    }
    return timing;
}

/****************************************************************************
 * Name: custom_wait()
 *
 * Description:
 *   used to wait for a given time interval
 *
 * Input Parameters:
 *   milliseconds - milliseconds to wait
 *
 ****************************************************************************/

static void custom_wait(uint32_t milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/****************************************************************************
 * Name: time_millis()
 *
 * Description:
 *   get the current time in milliseconds
 *
 * Returned Value:
 *   return the current time in milliseconds
 *
 ****************************************************************************/

static uint32_t time_millis(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

/****************************************************************************
 * Name: delay_locked()
 *
 * Description:
 *   blocks program execution for a period of time
 *
 * Input Parameters:
 *   forcestop - force stop vibration flag
 *   duration - blocks program execution for a period of time
 *
 * Returned Value:
 *   returns the actual waiting time
 *
 ****************************************************************************/

static uint32_t delay_locked(bool forcestop, int32_t duration)
{
    int32_t duration_remaining = duration;

    if (duration > 0) {
        uint32_t bedtime = duration + time_millis();
        do {
            custom_wait(duration_remaining);
            if (forcestop) {
                break;
            }
            duration_remaining = bedtime - time_millis();
        } while (duration_remaining > 0);

        return duration - duration_remaining;
    }

    return 0;
}

/****************************************************************************
 * Name: receive_waveform_thread()
 *
 * Description:
 *   receive waveform from vibrator_upper file
 *
 * Input Parameters:
 *   args - the args of threadargs
 *
 ****************************************************************************/

static void* receive_waveform_thread(void* args)
{
    threadargs* thread_args = (threadargs*)args;
    vibrator_waveform_t wave = thread_args->wave;
    ff_dev_t* ff_dev = thread_args->ff_dev;
    int32_t on_duration = 0;
    uint8_t amplitude = 0;
    int32_t duration = 0;
    uint32_t wait_time;
    bool forcestop;
    int index = 0;

    if (!should_vibrate(ff_dev->intensity))
        return NULL;

    if (!should_repeat(wave.repeat, wave.timings, wave.length))
        wave.repeat = -1;

    while (1) {
        pthread_mutex_lock(&thread_args->mutex);
        forcestop = thread_args->forcestop;
        if (thread_args->forcestop) {
            pthread_mutex_unlock(&thread_args->mutex);
            break;
        }
        pthread_mutex_unlock(&thread_args->mutex);
        VIBRATORINFO("index = %d", index);

        if (index < wave.length) {
            amplitude = scale(wave.amplitudes[index], ff_dev->intensity);
            duration = wave.timings[index++];
            if (duration <= 0) {
                continue;
            }
            if (amplitude != 0) {
                if (on_duration <= 0) {
                    on_duration = get_total_on_duration(wave.timings, wave.amplitudes,
                        index - 1, wave.repeat, wave.length);
                    on(ff_dev, on_duration);
                }
                ff_set_amplitude(ff_dev, amplitude);
            }
            wait_time = delay_locked(forcestop, duration);
            if (amplitude != 0) {
                on_duration -= wait_time;
            }
        } else if (wave.repeat < 0) {
            VIBRATORINFO("repeat < 0, play waveform exit");
            break;
        } else {
            index = wave.repeat;
        }
    }

    pthread_mutex_lock(&thread_args->mutex);
    thread_args->condition_is_met = true;
    pthread_mutex_unlock(&thread_args->mutex);
    pthread_cond_signal(&thread_args->condition);

    VIBRATORINFO("receive_waveform_thread exit");

    return NULL;
}

/****************************************************************************
 * Name: interval_timer_cb()
 *
 * Description:
 *   callback function to wait for a given time interval
 *
 * Input Parameters:
 *   timer - the handle of the uv timer
 *
 ****************************************************************************/

static void interval_timer_cb(uv_timer_t* timer)
{
    threadargs* thread_args = (threadargs*)timer->data;
    vibrator_waveform_t* wave = &thread_args->wave;
    ff_dev_t* ff_dev = thread_args->ff_dev;
    int32_t duration = wave->timings[0];
    int32_t interval = wave->timings[1];

    if (wave->count-- == 0) {
        uv_timer_stop(timer);
        return;
    }

    VIBRATORINFO("%s: call on, duration %d, interval %d, count %d",
        __func__, duration, interval, wave->count);
    on(ff_dev, duration);
}

/****************************************************************************
 * Name: receive_interval()
 *
 * Description:
 *   receive receive_interval from vibrator_upper file
 *
 * Input Parameters:
 *   args - the args of threadargs
 *
 ****************************************************************************/

static int receive_interval(void* args)
{
    threadargs* thread_args = (threadargs*)args;

    return uv_timer_start(&thread_args->timer, interval_timer_cb, 0,
        thread_args->wave.timings[0] + thread_args->wave.timings[1]);
}

/****************************************************************************
 * Name: receive_predefined()
 *
 * Description:
 *   receive predefined from vibrator_upper file
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   eff - effect struct, including effect id and effect strength
 *
 * Returned Value:
 *   return the play_effect value
 *
 ****************************************************************************/

static int receive_predefined(ff_dev_t* ff_dev, vibrator_effect_t* eff)
{
    int32_t play_length;
    int ret;

    if (!should_vibrate(ff_dev->intensity))
        return -ENOTSUP;

    ret = play_effect(ff_dev, eff->effect_id, eff->es, (long*)&play_length);

    if (ret >= 0)
        eff->play_length = play_length;

    return ret;
}

/****************************************************************************
 * Name: receive_primitive()
 *
 * Description:
 *   receive play primitive from vibrator_upper file
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   eff - effect struct, including effect id and effect strength
 *
 * Returned Value:
 *   return the play_effect value
 *
 ****************************************************************************/

static int receive_primitive(ff_dev_t* ff_dev, vibrator_effect_t* eff)
{
    int32_t play_length;
    int ret;

    if (!should_vibrate(ff_dev->intensity))
        return -ENOTSUP;

    ret = play_primitive(ff_dev, eff->effect_id, eff->amplitude, (long*)&play_length);

    if (ret >= 0)
        eff->play_length = play_length;

    return ret;
}

/****************************************************************************
 * Name: receive_set_intensity()
 *
 * Description:
 *   recevice set vibrator intensity operation from vibrator_upper file
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   intensity - vibration intensity
 *
 * Returned Value:
 *   return the ret of write
 *
 ****************************************************************************/

static int receive_set_intensity(ff_dev_t* ff_dev,
    vibrator_intensity_e intensity)
{
    int ret;

    ff_dev->intensity = intensity;
    ret = property_set_int32(KVDB_KEY_VIBRATOR_MODE, ff_dev->intensity);
    if (ret < 0) {
        return ret;
    }

    return OK;
}

/****************************************************************************
 * Name: receive_get_intensity()
 *
 * Description:
 *   recevice get vibrator intensity operation from vibrator_upper file
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   intensity - vibration intensity
 *
 * Returned Value:
 *   return OK(success)
 *
 ****************************************************************************/

static int receive_get_intensity(ff_dev_t* ff_dev,
    vibrator_intensity_e* intensity)
{
    ff_dev->intensity = property_get_int32(KVDB_KEY_VIBRATOR_MODE,
        ff_dev->intensity);
    *intensity = ff_dev->intensity;
    return OK;
}

/****************************************************************************
 * Name: receive_set_amplitude()
 *
 * Description:
 *   recevice set vibrator amplitude operation from vibrator_upper file
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   amplitude - vibration amplitude, range 0 - 255
 *
 * Returned Value:
 *   return the ret of write
 *
 ****************************************************************************/

static int receive_set_amplitude(ff_dev_t* ff_dev, uint8_t amplitude)
{
    ff_dev->curr_amplitude = amplitude;
    return ff_set_amplitude(ff_dev, amplitude);
}

/****************************************************************************
 * Name: receive_get_capabilities()
 *
 * Description:
 *   recevice get vibrator capabilities operation from vibrator_upper file
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *   capabilities - buffer that store capabilities
 *
 * Returned Value:
 *   0 means success
 *
 ****************************************************************************/

static int receive_get_capabilities(ff_dev_t* ff_dev, int32_t* capabilities)
{
    *capabilities = ff_dev->capabilities;
    return OK;
}

/****************************************************************************
 * Name: vibrator_init()
 *
 * Description:
 *   open the vibrator file descriptor
 *
 * Input Parameters:
 *   ff_dev - structure for operating the ff device driver
 *
 * Returned Value:
 *   0 means success, otherwise it means failure
 *
 ****************************************************************************/

static int vibrator_init(ff_dev_t* ff_dev)
{
    unsigned char ffbitmask[1 + FF_MAX / 8 / sizeof(unsigned char)];
    int ret;

    ff_dev->curr_app_id = VIBRATOR_INVALID_VALUE;
    ff_dev->curr_magnitude = VIBRATOR_STRONG_MAGNITUDE;
    ff_dev->intensity = VIBRATION_INTENSITY_OFF;
    ff_dev->curr_amplitude = VIBRATOR_MAX_AMPLITUDE;
    ff_dev->capabilities = 0;

    ff_dev->fd = open(VIBRATOR_DEV_FS, O_CLOEXEC | O_RDWR);
    if (ff_dev->fd < 0) {
        VIBRATORERR("vibrator open failed, errno = %d", errno);
        return -ENODEV;
    }

    memset(ffbitmask, 0, sizeof(ffbitmask));
    ret = ioctl(ff_dev->fd, EVIOCGBIT, ffbitmask);
    if (ret < 0) {
        VIBRATORERR("ioctl EVIOCGBIT failed, errno = %d", errno);
        return ret;
    }

    if (test_bit(FF_CONSTANT, ffbitmask) || test_bit(FF_PERIODIC, ffbitmask)) {
        if (test_bit(FF_CUSTOM, ffbitmask))
            ff_dev->capabilities |= CAP_AMPLITUDE_CONTROL;
        if (test_bit(FF_GAIN, ffbitmask)) {
            ff_dev->capabilities |= CAP_PERFORM_CALLBACK;
            ff_dev->capabilities |= CAP_COMPOSE_EFFECTS;
        }
    } else {
        return -ENODEV;
    }

    ff_dev->intensity = property_get_int32(KVDB_KEY_VIBRATOR_MODE,
        ff_dev->intensity);
    return OK;
}

/****************************************************************************
 * Name: vibrator_mode_select()
 *
 * Description:
 *   choose vibrator operation mode
 *
 * Input Parameters:
 *   vibrator - structure for operating the vibrator
 *   args - the threadargs
 *
 ****************************************************************************/

static int vibrator_mode_select(vibrator_msg_t* msg, void* args)
{
    pthread_attr_t vibattr;
    pthread_t vibra_thread;
    threadargs* thread_args;
    ff_dev_t* ff_dev;
    uv_timer_t* timer;
    int ret;

    if (args == NULL) {
        VIBRATORERR("mode select args is NULL");
        return -EINVAL;
    }

    pthread_attr_init(&vibattr);
    pthread_attr_setstacksize(&vibattr, CONFIG_VIBRATOR_STACKSIZE);

    thread_args = (threadargs*)args;
    ff_dev = thread_args->ff_dev;
    timer = &thread_args->timer;

    switch (msg->type) {
    case VIBRATION_WAVEFORM: {
        uv_timer_stop(timer);
        pthread_mutex_lock(&thread_args->mutex);
        if (thread_args->forcestop == false) {
            thread_args->forcestop = true;
            while (!thread_args->condition_is_met) {
                pthread_cond_wait(&thread_args->condition, &thread_args->mutex);
            }
            thread_args->condition_is_met = false;
            thread_args->forcestop = false;
        } else {
            thread_args->forcestop = false;
        }
        pthread_mutex_unlock(&thread_args->mutex);
        thread_args->wave = msg->wave;
        ret = pthread_create(&vibra_thread, &vibattr, receive_waveform_thread, thread_args);
        break;
    }
    case VIBRATION_INTERVAL: {
        thread_args->forcestop = true;
        uv_timer_stop(timer);
        thread_args->wave = msg->wave;
        ret = receive_interval(thread_args);
        VIBRATORINFO("receive interval ret = %d", ret);
        break;
    }
    case VIBRATION_EFFECT: {
        thread_args->forcestop = true;
        uv_timer_stop(timer);
        ret = receive_predefined(ff_dev, &msg->effect);
        VIBRATORINFO("receive predefined ret = %d", ret);
        break;
    }
    case VIBRATION_STOP: {
        thread_args->forcestop = true;
        uv_timer_stop(timer);
        ret = receive_stop(ff_dev);
        VIBRATORINFO("receive stop ret = %d", ret);
        break;
    }
    case VIBRATION_START: {
        thread_args->forcestop = true;
        uv_timer_stop(timer);
        ret = receive_start(ff_dev, msg->timeoutms);
        VIBRATORINFO("receive start ret = %d", ret);
        break;
    }
    case VIBRATION_PRIMITIVE: {
        thread_args->forcestop = true;
        uv_timer_stop(timer);
        ret = receive_primitive(ff_dev, &msg->effect);
        VIBRATORINFO("receive primitive ret = %d", ret);
        break;
    }
    case VIBRATION_SET_INTENSITY: {
        ret = receive_set_intensity(ff_dev, msg->intensity);
        VIBRATORINFO("receive set intensity = %d", ret);
        break;
    }
    case VIBRATION_GET_INTENSITY: {
        ret = receive_get_intensity(ff_dev, (vibrator_intensity_e*)&msg->intensity);
        VIBRATORINFO("receive get intensity = %d", msg->intensity);
        break;
    }
    case VIBRATION_SET_AMPLITUDE: {
        ret = receive_set_amplitude(ff_dev, msg->amplitude);
        VIBRATORINFO("receive set amplitude = %d", ret);
        break;
    }
    case VIBRATION_GET_CAPABLITY: {
        ret = receive_get_capabilities(ff_dev, &msg->capabilities);
        VIBRATORINFO("receive get capabilities = %d", (int)msg->capabilities);
        break;
    }
    default: {
        ret = -EINVAL;
        break;
    }
    }

    return ret;
}

static void connection_close_cb(uv_handle_t* handle)
{
    vibrator_context_t* ctx = handle->data;
    free(ctx);
}

static void connection_poll_cb(uv_poll_t* handle, int status, int events)
{
    vibrator_context_t* ctx = handle->data;
    vibrator_msg_t* msg = &ctx->thread_args->msg;

    if (events & UV_READABLE) {
        int ret = recv(ctx->sock, msg, sizeof(vibrator_msg_t), 0);
        if (ret >= msg->request_len) {
            VIBRATORINFO("recv client: recv len = %d, type = %d", ret, msg->type);
            msg->result = vibrator_mode_select(msg, ctx->thread_args);
            ret = send(ctx->sock, msg, msg->response_len, 0);
            if (ret < 0) {
                VIBRATORERR("send fail, errno = %d", errno);
            }
        }
    }

    if (events & UV_DISCONNECT) {
        VIBRATORINFO("client disconnect");
        uv_poll_stop(handle);
        close(ctx->sock);
        uv_close((uv_handle_t*)&ctx->poll_handle, connection_close_cb);
    }
}

static void server_poll_cb(uv_poll_t* handle, int status, int events)
{
    vibrator_context_t* server_ctx = handle->data;
    vibrator_context_t* client_ctx;
    uv_os_sock_t client_fd;

    client_fd = accept(server_ctx->sock, NULL, NULL);
    if (client_fd < 0) {
        VIBRATORERR("accept failed %d: %d", client_fd, errno);
        return;
    }

    client_ctx = malloc(sizeof *client_ctx);
    int ret = uv_poll_init_socket(uv_default_loop(), &client_ctx->poll_handle, client_fd);
    if (ret < 0) {
        VIBRATORERR("uv poll init socket failed: %d\n", ret);
        close(client_fd);
        free(client_ctx);
        return;
    }

    client_ctx->sock = client_fd;
    client_ctx->thread_args = server_ctx->thread_args;
    client_ctx->poll_handle.data = client_ctx;
    ret = uv_poll_start(&client_ctx->poll_handle, UV_READABLE | UV_DISCONNECT,
        connection_poll_cb);
    if (ret < 0) {
        VIBRATORERR("uv poll start socket failed: %d\n", ret);
        close(client_fd);
        free(client_ctx);
        return;
    }
}

int main(int argc, char* argv[])
{
    vibrator_context_t server_context[VIBRATOR_COUNT];
    threadargs thread_args;
    ff_dev_t ff_dev;
    int ret;

    const int family[] = {
        [VIBRATOR_LOCAL] = AF_UNIX,
        [VIBRATOR_REMOTE] = AF_RPMSG,
    };

    const struct sockaddr_un addr0 = {
        .sun_family = AF_UNIX,
        .sun_path = PROP_SERVER_PATH,
    };

    const struct sockaddr_rpmsg addr1 = {
        .rp_family = AF_RPMSG,
        .rp_cpu = "",
        .rp_name = PROP_SERVER_PATH,
    };

    const struct sockaddr* addr[] = {
        [VIBRATOR_LOCAL] = (const struct sockaddr*)&addr0,
        [VIBRATOR_REMOTE] = (const struct sockaddr*)&addr1,
    };

    const socklen_t addrlen[] = {
        [VIBRATOR_LOCAL] = sizeof(struct sockaddr_un),
        [VIBRATOR_REMOTE] = sizeof(struct sockaddr_rpmsg),
    };

    ret = vibrator_init(&ff_dev);
    if (ret < 0) {
        VIBRATORERR("vibrator init failed: %d", ret);
        return ret;
    }

    thread_args.forcestop = true;
    thread_args.condition_is_met = false;
    thread_args.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    thread_args.condition = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    thread_args.ff_dev = &ff_dev;
    thread_args.timer.data = &thread_args;

    for (int i = 0; i < VIBRATOR_COUNT; i++) {
        server_context[i].thread_args = &thread_args;

        server_context[i].sock = socket(family[i], SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (server_context[i].sock < 0) {
            VIBRATORERR("socket failure %d: %d", i, errno);
            continue;
        }

        ret = bind(server_context[i].sock, addr[i], addrlen[i]);
        if (ret < 0) {
            goto errout;
        }

        ret = uv_poll_init_socket(uv_default_loop(), &server_context[i].poll_handle, server_context[i].sock);
        if (ret < 0) {
            goto errout;
        }
        server_context[i].poll_handle.data = &server_context[i];

        ret = listen(server_context[i].sock, VIBRATOR_MAX_CLIENTS);
        if (ret < 0) {
            goto errout;
        }

        ret = uv_poll_start(&server_context[i].poll_handle, UV_READABLE, server_poll_cb);
        if (ret < 0) {
            goto errout;
        }
    }

    uv_timer_init(uv_default_loop(), &thread_args.timer);

    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    if (ret < 0) {
        VIBRATORERR("uv_run failed: %d", ret);
    }

errout:
    for (int i = 0; i < VIBRATOR_COUNT; i++) {
        if (server_context[i].sock > 0) {
            close(server_context[i].sock);
        }
    }

    close(ff_dev.fd);
    return ret;
}
