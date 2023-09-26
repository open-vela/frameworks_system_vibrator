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
#include <mqueue.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <log/log.h>
#include <netpacket/rpmsg.h>
#include <nuttx/motor/motor.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "vibrator.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DBG(format, args...) SLOGD(format, ##args)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vibrate_driver_run_waveform()
 *
 * Description:
 *    motor driver running the waveform
 *
 * Input Parameters:
 *   fd - driver file descriptors
 *   params - vibrator waveform
 *
 * Returned Value:
 *   return the ret of ioctl
 *
 ****************************************************************************/

static int vibrate_driver_run_waveform(int fd,
                                       FAR struct motor_params_s* params)
{
    int ret;

    if (fd < 0) {
        DBG("failed to run waveform fd");
        return fd;
    }

    ret = ioctl(fd, MTRIOC_SET_MODE, MOTOR_OPMODE_PATTERN);
    if (ret < 0) {
        DBG("failed to set mode. errno = %d", errno);
        return ret;
    }

    ret = ioctl(fd, MTRIOC_SET_PARAMS, params);
    if (ret < 0) {
        DBG("failed to set params. errno = %d", errno);
        return ret;
    }

    ret = ioctl(fd, MTRIOC_START);
    if (ret < 0) {
        DBG("failed to start vibrator. errno = %d", errno);
    }

    return ret;
}

/****************************************************************************
 * Name: vibrate_driver_stop()
 *
 * Description:
 *   stop driver vibration
 *
 * Input Parameters:
 *   fd - the file of description
 *
 * Returned Value:
 *   return stop ioctl value
 *
 ****************************************************************************/

static int vibrate_driver_stop(int fd)
{
    int ret;

    if (fd < 0) {
        DBG("failed to open fd to stop");
        return fd;
    }
    ret = ioctl(fd, MTRIOC_STOP);
    if (ret < 0) {
        DBG("failed to stop vibrator");
    }

    return ret;
}

/****************************************************************************
 * Name: receive_compositions()
 *
 * Description:
 *   receive waveform from vibrator_upper file
 *
 * Input Parameters:
 *   data - the waveform of the compositions array
 *   fd - the file of description
 *
 * Returned Value:
 *   return the run waveform value
 *
 ****************************************************************************/

static int receive_compositions(compositions_t data, int fd)
{
    int ret;
    struct motor_params_s params = {
        .privdata = &data,
    };

    ret = vibrate_driver_stop(fd);
    if (ret < 0) {
        return ret;
    }

    return vibrate_driver_run_waveform(fd, &params);
}

/****************************************************************************
 * Name: do_vibrator_set_amplitude()
 *
 * Description:
 *   operate the driver's interface, just retain the interface
 *
 * Input Parameters:
 *   fd - the file of description
 *   amplitude - amplitude of the vibrator
 *
 ****************************************************************************/

void do_vibrator_set_amplitude(int fd, uint8_t amplitude)
{
    // ioctl(fd, amplitude);
}

/****************************************************************************
 * Name: do_vibrator_on()
 *
 * Description:
 *   operate the driver's interface, just retain the interface
 *
 * Input Parameters:
 *   fd - the file of description
 *   on_duration - motor on duration
 *   amplitude - amplitude of the vibrator
 *
 ****************************************************************************/

void do_vibrator_on(int fd, uint32_t on_duration, uint8_t amplitude)
{
    // ioctl(fd, on_duration);
    do_vibrator_set_amplitude(fd, amplitude);
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

int32_t get_total_on_duration(uint32_t timings[], uint8_t amplitudes[],
                              uint8_t start_index, uint8_t repeat_index,
                              uint8_t len)
{
    uint8_t i;
    uint32_t timing;

    timing = 0;
    i = start_index;

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

void custom_wait(uint32_t milliseconds)
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

uint32_t time_millis(void)
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
 *   forcestop - motor vibration time
 *   duration - blocks program execution for a period of time
 *
 * Returned Value:
 *   returns the actual waiting time
 *
 ****************************************************************************/

uint32_t delay_locked(bool forcestop, uint32_t duration)
{
    uint32_t duration_remaining = duration;

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
    int fd;
    int index;
    bool forcestop;
    waveform_t wave;
    uint8_t amplitude;
    uint32_t duration;
    uint32_t wait_time;
    uint32_t on_duration;
    threadargs* thread_args;

    thread_args = (threadargs*)args;
    fd = thread_args->fd;
    wave = thread_args->wave;

    uint32_t timings[wave.length];
    uint8_t amplitudes[wave.length];

    index = 0;
    duration = 0;
    amplitude = 0;
    on_duration = 0;

    for (uint8_t i = 0; i < wave.length; i++) {
        timings[i] = wave.timings[i];
        amplitudes[i] = wave.amplitudes[i];
    }

    while (1) {
        pthread_mutex_lock(&thread_args->mutex);
        forcestop = thread_args->forcestop;
        if (thread_args->forcestop) {
            pthread_mutex_unlock(&thread_args->mutex);
            break;
        }
        pthread_mutex_unlock(&thread_args->mutex);

        if (index < wave.length) {
            amplitude = amplitudes[index];
            duration = timings[index++];
            if (duration <= 0) {
                continue;
            }
            if (amplitude != 0) {
                if (on_duration <= 0) {
                    on_duration = get_total_on_duration(timings, amplitudes, index - 1, wave.repeat, wave.length);
                    do_vibrator_on(fd, on_duration, amplitude);
                } else {
                    do_vibrator_set_amplitude(fd, amplitude);
                }
            }
            wait_time = delay_locked(forcestop, duration);
            if (amplitude != 0) {
                on_duration -= wait_time;
            }
        } else if (wave.repeat < 0) {
            break;
        } else {
            index = wave.repeat;
        }
    }

    pthread_cond_signal(&thread_args->condition);

    return NULL;
}

/****************************************************************************
 * Name: receive_predefined()
 *
 * Description:
 *   receive predefined from vibrator_upper file
 *
 * Input Parameters:
 *   effectid - the wave id
 *   fd - the file of description
 *
 * Returned Value:
 *   return the run waveform value
 *
 ****************************************************************************/

static int receive_predefined(int effectid, int fd)
{
    int ret;
    struct motor_params_s params = {
        .privdata = &effectid,
    };

    ret = vibrate_driver_stop(fd);
    if (ret < 0) {
        return ret;
    }

    return vibrate_driver_run_waveform(fd, &params);
}

/****************************************************************************
 * Name: vibrator_init()
 *
 * Description:
 *   calibrate of the vibrator and open the file descriptor
 *
 * Returned Value:
 *   return file descriptor
 *
 ****************************************************************************/

static int vibrator_init(void)
{
    int fd;
    int ret;
    char calibrate_value[32];

    fd = open(VIBRATOR_DEV_FS, O_CLOEXEC | O_RDWR);
    if (fd < 0) {
        DBG("vibrator open failed, errno = %d", errno);
        return fd;
    }

    struct motor_limits_s limits = {
        .force = MAX_VIBRATION_STRENGTH_LEVEL / 100.0f,
    };

    ret = ioctl(fd, MTRIOC_SET_LIMITS, &limits);
    if (ret < 0) {
        DBG("failed to motor limits, errno = %d", errno);
    }

    ret = property_get(MOTO_CALI_PREFIX, calibrate_value, "");
    if (ret > 0) {
        if (strlen(calibrate_value) == 0) {
            DBG("calibrate value get failed, errno = %d", errno);
        }

        ret = ioctl(fd, MTRIOC_SET_CALIBDATA, calibrate_value);
        if (ret < 0) {
            DBG("ioctl failed %d!", ret);
        }
    }

    return fd;
}

/****************************************************************************
 * Name: vibrator_mode_select()
 *
 * Description:
 *   choose vibration mode
 *
 * Input Parameters:
 *   vibra - the wave of vibrator_t
 *   args - the threadargs
 *
 ****************************************************************************/

void vibrator_mode_select(vibrator_t vibra, void* args)
{
    int ret;
    pthread_attr_t vibattr;
    pthread_t vibra_thread;

    pthread_attr_init(&vibattr);
    pthread_attr_setstacksize(&vibattr, 256);
    threadargs* thread_args;
    thread_args = (threadargs*)args;

    if (args == NULL) {
        DBG("mode select args is NULL");
        return;
    }

    switch (vibra.type) {
    case VIBRATION_WAVEFORM: {
        pthread_mutex_lock(&thread_args->mutex);
        if (thread_args->forcestop == false) {
            thread_args->forcestop = true;
            pthread_cond_wait(&thread_args->condition, &thread_args->mutex);
            thread_args->forcestop = false;
        } else {
            thread_args->forcestop = false;
        }
        pthread_mutex_unlock(&thread_args->mutex);
        thread_args->wave = vibra.wave;
        pthread_create(&vibra_thread, &vibattr, receive_waveform_thread, thread_args);
        break;
    }
    case VIBRATION_EFFECT: {
        thread_args->forcestop = true;
        ret = receive_predefined(vibra.effectid, thread_args->fd);
        DBG("receive_predefined ret = %d", ret);
        break;
    }
    case VIBRATION_COMPOSITION: {
        thread_args->forcestop = true;
        ret = receive_compositions(vibra.comp, thread_args->fd);
        DBG("receive_compositions ret = %d", ret);
        break;
    }
    case VIBRATION_STOP: {
        thread_args->forcestop = true;
        ret = vibrate_driver_stop(thread_args->fd);
        DBG("vibrate driver stop ret = %d", ret);
        break;
    }
    default: {
        break;
    }
    }
}

int main(int argc, FAR char* argv[])
{
    int fd;
    int ret;
    vibrator_t vibra_t;
    threadargs thread_args;
    int sock_fd[VIB_COUNT];
    struct pollfd pfd[VIB_COUNT];

    const int family[] = {
        [VIB_LOCAL] = AF_UNIX,
        [VIB_REMOTE] = AF_RPMSG,
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
        [VIB_LOCAL] = (const struct sockaddr*)&addr0,
        [VIB_REMOTE] = (const struct sockaddr*)&addr1,
    };

    const socklen_t addrlen[] = {
        [VIB_LOCAL] = sizeof(struct sockaddr_un),
        [VIB_REMOTE] = sizeof(struct sockaddr_rpmsg),
    };

    fd = vibrator_init();
    if (fd < 0) {
        return fd;
    }

    thread_args.fd = fd;
    thread_args.forcestop = true;
    thread_args.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    thread_args.condition = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    memset(sock_fd, 0, sizeof(sock_fd));

    for (int i = 0; i < VIB_COUNT; i++) {
        sock_fd[i] = socket(family[i], SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sock_fd[i] < 0) {
            DBG("socket failure %d: %d", i, errno);
            continue;
        }

        ret = bind(sock_fd[i], addr[i], addrlen[i]);
        if (ret < 0) {
            return ret;
        }

        ret = listen(sock_fd[i], MAX_CLIENTS);
        if (ret < 0) {
            return ret;
        }
    }

    memset(pfd, 0, sizeof(pfd));

    for (int i = 0; i < VIB_COUNT; i++) {
        pfd[i].fd = sock_fd[i];
        pfd[i].events = POLLIN;
    }

    DBG("listen success");

    while (1) {
        ret = poll(pfd, VIB_COUNT, -1);
        if (ret < 0) {
            DBG("poll failed");
            break;
        }

        for (int i = 0; i < VIB_COUNT; i++) {
            if (pfd[i].revents & POLLIN) {
                int client_fd = accept(sock_fd[i], NULL, NULL);
                if (client_fd < 0) {
                    DBG("accept failed %d: %d", i, errno);
                    continue;
                }

                ret = recv(client_fd, &vibra_t, sizeof(vibrator_t), 0);
                if (ret == -1) {
                    DBG("recv failed %d: %d", i, errno);
                } else if (ret == 0) {
                    DBG("client disconnected %d: %d", i, errno);
                } else {
                    thread_args.forcestop = true;
                    vibrator_mode_select(vibra_t, &thread_args);
                }

                close(client_fd);
            }
        }
    }

    for (int i = 0; i < VIB_COUNT; i++) {
        if (sock_fd[i] > 0) {
            close(sock_fd[i]);
        }
    }

    close(fd);
    return ret;
}
