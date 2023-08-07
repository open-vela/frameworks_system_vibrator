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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <log/log.h>
#include <nuttx/motor/motor.h>
#include <sys/ioctl.h>

#include "vibrator_api.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIBRATOR_DEV_FS "/dev/lra0"
#define MOTO_CALI_PREFIX "ro.factory.motor_calib"
#define MAX_VIBRATION_STRENGTH_LEVEL (100)

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
 ****************************************************************************/

static int vibrate_driver_stop(int fd)
{
    int ret;

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
 * Name: receive_waveform()
 *
 * Description:
 *   receive waveform from vibrator_upper file
 *
 * Input Parameters:
 *   wave - the waveform of the waveform_t array
 *   fd - the file of description
 *
 * Returned Value:
 *   return the run waveform value
 *
 ****************************************************************************/

static int receive_waveform(waveform_t wave, int fd)
{
    int ret;
    struct motor_params_s params = {
        .privdata = &wave,
    };

    ret = vibrate_driver_stop(fd);
    if (ret < 0) {
        return ret;
    }

    return vibrate_driver_run_waveform(fd, &params);
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
 * Name: vibrator_cali()
 *
 * Description:
 *   calibrate of the vibrator
 *
 * Returned Value:
 *   return file descriptor
 *
 ****************************************************************************/

static int vibrator_init(void)
{
    int fd;
    int ret;
    static char calibrate_value[32];


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

int main(int argc, FAR char* argv[])
{
    int fd;
    int ret;
    mqd_t mq;
    vibrator_t vibra_t;
    struct mq_attr attr;

    attr.mq_maxmsg = MAX_MSG_NUM;
    attr.mq_msgsize = MAX_MSG_SIZE;

    fd = vibrator_init();
    if (fd < 0) {
        return fd;
    }

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0660, &attr);
    if (mq == (mqd_t)-1) {
        DBG("mq_open fail");
        close(fd);
        return mq;
    }

    while (1) {
        memset(&vibra_t, 0, sizeof(vibrator_t));
        if (mq_receive(mq, (char*)&vibra_t, MAX_MSG_SIZE, NULL) == -1) {
            DBG("mq_receive");
            close(fd);
            mq_close(mq);
            return 0;
        }

        switch (vibra_t.type) {
        case VIBRATION_WAVEFORM: {
            ret = receive_waveform(vibra_t.wave, fd);
            DBG("receive_waveform ret = %d", ret);
            break;
        }
        case VIBRATION_EFFECT: {
            ret = receive_predefined(vibra_t.effectid, fd);
            DBG("receive_predefined ret = %d", ret);
            break;
        }
        case VIBRATION_COMPOSITION: {
            ret = receive_compositions(vibra_t.comp, fd);
            DBG("receive_compositions ret = %d", ret);
            break;
        }
        case VIBRATION_STOP: {
            ret = vibrate_driver_stop(fd);
            DBG("vibrate driver stop ret = %d", ret);
            break;
        }
        default: {
            break;
        }
        }
    }

    close(fd);
    mq_close(mq);
    return ret;
}
