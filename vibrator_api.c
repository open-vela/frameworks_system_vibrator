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
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <log/log.h>
#include <netpacket/rpmsg.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "vibrator.h"
#include "./include/vibrator_api.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DBG(format, args...) SLOGD(format, ##args)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vib_commit()
 *
 * Description:
 *   open message queue
 *
 * Input Parameters:
 *   buffer - the type of the vibrator_t
 *
 * Returned Value:
 *   returns the flag that the vibration is send
 *
 ****************************************************************************/

static int vib_commit(vibrator_t buffer)
{
    int fd;
    int ret;
#ifdef CONFIG_VIBRATOR_SERVER
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
#else
    fd = socket(AF_RPMSG, SOCK_STREAM | SOCK_NONBLOCK, 0);
#endif
    if (fd < 0) {
        DBG("socket fail");
        return fd;
    }

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

    ret = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0 && errno == EINPROGRESS) {
        struct pollfd pfd;
        memset(&pfd, 0, sizeof(struct pollfd));
        pfd.fd = fd;
        pfd.events = POLLOUT;

        ret = poll(&pfd, 1, -1);
        if (ret < 0) {
            DBG("client: poll failure: %d", errno);
            goto errout_with_socket;
        }
    } else if (ret < 0) {
        DBG("client: connect failure: %d", errno);
        goto errout_with_socket;
    }

    ret = send(fd, &buffer, sizeof(vibrator_t), 0);
    if (ret < 0) {
        DBG("send fail %d", ret);
    }

errout_with_socket:
    close(fd);
    return ret;
}

/****************************************************************************
 * Public Functions
 *
 * Description:
 *   the file contains five interfaces play_compositions,
 *   play_waveform, play_oneshot, play_predefined and vibrator_cancel,
 *   and the detailed information of each interface has been described
 *
 ****************************************************************************/

/****************************************************************************
 * Name: vibrator_play_compositions()
 *
 * Description:
 *   play the compositions interface for app
 *
 * Input Parameters:
 *   data - the compositions_t of data
 *
 * Returned Value:
 *   returns the flag that the vibration is enabled, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_play_compositions(const compositions_t* data)
{
    vibrator_t buffer;

    buffer.type = VIBRATION_COMPOSITION;
    buffer.comp = *data;

    return vib_commit(buffer);
}

/****************************************************************************
 * Name: vibrator_play_waveform()
 *
 * Description:
 *    play waveform vibration effects
 *
 * Input Parameters:
 *   timings - timings the element of the timings array
 *   is the duration of the vibration or the duration of the wait
 *   amplitudes - amplitudes are the amplitudes of vibrations
 *   length - length is timings and amplitudes
 *   repeat - repeat is the subscript that indicates the start of
 *   the vibration
 *
 * Returned Value:
 *   returns the flag that the vibration is enabled, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_play_waveform(uint32_t timings[], uint8_t amplitudes[],
                           uint8_t repeat, uint8_t length)
{
    waveform_t wave;
    vibrator_t buffer;

    wave.length = length;
    wave.repeat = repeat;
    memcpy(wave.timings, timings, sizeof(uint32_t) * length);
    memcpy(wave.amplitudes, amplitudes, sizeof(uint8_t) * length);

    buffer.type = VIBRATION_WAVEFORM;
    buffer.wave = wave;

    return vib_commit(buffer);
}

/****************************************************************************
 * Name: vibrator_play_oneshot()
 *
 * Description:
 *    play waveform vibration effects
 *
 * Input Parameters:
 *   timing - duration of vibration
 *   amplitude - amplitude of vibration
 *
 * Returned Value:
 *   returns the flag that the vibration is enabled, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_play_oneshot(uint32_t timing, uint8_t amplitude)
{
    uint8_t ret;
    uint32_t timings[] = { timing };
    uint8_t amplitudes[] = { amplitude };
    uint8_t len = 1;
    uint8_t rep = -1;

    ret = vibrator_play_waveform(timings, amplitudes, len, rep);
    return ret;
}

/****************************************************************************
 * Name: vibrator_play_predefined()
 *
 * Description:
 *    play the predefined interface for app
 *
 * Input Parameters:
 *   effectid - effectid of vibrator
 *
 * Returned Value:
 *   returns the flag that the vibration is enabled, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_play_predefined(uint8_t effect_id)
{
    vibrator_t buffer;

    buffer.type = VIBRATION_EFFECT;
    buffer.effectid = effect_id;

    return vib_commit(buffer);
}

/****************************************************************************
 * Name: vibrator_cancel()
 *
 * Description:
 *    cancel the motor of vibration_id vibration
 *
 * Input Parameters:
 *   vibration_id - vibration_id of vibrator
 *
 * Returned Value:
 *   returns the flag that the vibration is closed, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

int vibrator_cancel(void)
{
    vibrator_t buffer;

    buffer.type = VIBRATION_STOP;

    return vib_commit(buffer);
}
