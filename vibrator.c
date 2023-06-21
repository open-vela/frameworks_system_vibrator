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
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include <log/log.h>
#include <nuttx/motor/motor.h>
#ifdef CONFIG_MOTOR_AW86225
#include <nuttx/motor/aw86225.h>
#endif
#include <sys/ioctl.h>

#include "vibrator_api.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIBRATOR_DEV_FS "/dev/lra0"
#define MOTO_CALI_PREFIX "ro.factory.motor_calib"
#define MAX_VIBRATION_STRENGTH_LEVEL (100)

#define DBG(format, args...) SLOGD(format, ##args)

extern int property_get(FAR const char* key, FAR char* value,
                        FAR const char* default_value);

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
 ****************************************************************************/

static void vibrate_driver_run_waveform(int fd,
                                        FAR struct motor_params_s* params)
{
    int ret = -1;

    ret = ioctl(fd, MTRIOC_SET_MODE, MOTOR_OPMODE_PATTERN);
    if (ret < 0) {
        DBG("failed to set mode.");
        return;
    }

    ret = ioctl(fd, MTRIOC_SET_PARAMS, params);
    if (ret < 0) {
        DBG("failed to set params.");
        return;
    }

    ret = ioctl(fd, MTRIOC_START);
    if (ret < 0) {
        DBG("failed to start vibrator.");
    }
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
 ****************************************************************************/

static void vibrate_driver_stop(int fd)
{
    if (fd < 0) {
        return;
    }
    int ret = ioctl(fd, MTRIOC_STOP);
    if (ret < 0) {
        DBG("failed to stop vibrator");
    }
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
 ****************************************************************************/

static void receive_compositions(compositions_t data, int fd)
{
    vibrate_driver_stop(fd);

#ifdef CONFIG_MOTOR_AW86225
    FAR struct aw86225_patterns_s* patterns = (FAR void*)&data;

    struct motor_params_s params = {
        .privdata = patterns,
    };
#else
    struct motor_params_s params = {
        .privdata = &data,
    };
#endif
    vibrate_driver_run_waveform(fd, &params);
}

/****************************************************************************
 * Name: receive_waveform()
 *
 * Description:
 *   receive waveform from vibrator_upper file
 *
 * Input Parameters:
 *   wave - the waveform of the waveform_t array
 *   fd - the file of description
 *
 ****************************************************************************/

static void receive_waveform(waveform_t wave, int fd)
{
    vibrate_driver_stop(fd);
    struct motor_params_s params = {
        .privdata = &wave,
    };
    vibrate_driver_run_waveform(fd, &params);
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
 ****************************************************************************/

static void receive_predefined(int effectid, int fd)
{
    vibrate_driver_stop(fd);
    struct motor_params_s params = {
        .privdata = &effectid,
    };
    vibrate_driver_run_waveform(fd, &params);
}

/****************************************************************************
 * Name: vibrator_cali()
 *
 * Description:
 *   calibrate of the vibrator
 *
 * Input Parameters:
 *   fd - the file of description
 *
 ****************************************************************************/

static void vibrator_cali(int fd)
{
    int ret = -1;
    static char calibrate_value[32];

    struct motor_limits_s limits = {
        .force = MAX_VIBRATION_STRENGTH_LEVEL / 100.0f,
    };

    ret = ioctl(fd, MTRIOC_SET_LIMITS, &limits);
    if (ret < 0) {
        DBG("failed to motor limits");
        return;
    }

    ret = property_get(MOTO_CALI_PREFIX, calibrate_value, "");
    if (strlen(calibrate_value) == 0) {
        DBG("calibrate value get failed\n");
        return;
    }

    ret = ioctl(fd, MTRIOC_SET_CALIBDATA, calibrate_value);
    if (ret < 0) {
        DBG("ioctl failed %d!", ret);
        return;
    }
}

int main(int argc, FAR char* argv[])
{
    int vib;
    mqd_t mq;
    uint8_t effectid;
    uint8_t buffer[512];
    bool loop = true;
    waveform_t wave;
    int var;
    compositions_t received_data;

    var = open(VIBRATOR_DEV_FS, O_CLOEXEC | O_RDWR);
    if (var < 0) {
        DBG("vibrator open failed");
        return 0;
    }

    vibrator_cali(var);

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(buffer);

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0660, &attr);
    if (mq == (mqd_t)-1) {
        DBG("mq_open fail");
        return 0;
    }

    while (loop) {
        memset(buffer, 0, sizeof(buffer));
        if (mq_receive(mq, (char*)buffer, sizeof(buffer), NULL) == -1) {
            DBG("mq_receive");
            return 0;
        }

        vib = buffer[0];
        switch (vib) {
        case VIBRATION_WAVEFORM: {
            memcpy(&wave, buffer + 4, sizeof(waveform_t));
            receive_waveform(wave, var);
            break;
        }
        case VIBRATION_EFFECT: {
            memcpy(&effectid, buffer + 4, sizeof(uint8_t));
            receive_predefined(effectid, var);
            break;
        }
        case VIBRATION_COMPOSITION: {
            memcpy(&received_data, buffer + 4, sizeof(compositions_t));
            receive_compositions(received_data, var);
            break;
        }
        case VIBRATION_STOP: {
            vibrate_driver_stop(var);
            break;
        }
        default: {
            break;
        }
        }
    }

    mq_close(mq);
    return 0;
}
