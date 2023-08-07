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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <log/log.h>

#include "vibrator_api.h"

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
    mqd_t mq;

    mq = mq_open(QUEUE_NAME, O_RDWR, 0666, NULL);
    if (mq == (mqd_t)-1) {
        DBG("mq_open failed errno = %d", -errno);
        return errno;
    }

    if (mq_send(mq, (const char*)&buffer, sizeof(buffer), 0) == -1) {
        DBG("mq_send failed errno = %d", -errno);
        return errno;
    }

    mq_close(mq);
    return 0;
}

/****************************************************************************
 * Public Functions
 *
 * Description:
 *   the file contains five interfaces create_compositions,
 *   create_waveform, create_oneshot, create_predefined and vibrator_cancel,
 *   and the detailed information of each interface has been described
 *
 ****************************************************************************/

/****************************************************************************
 * Name: vibrator_create_compositions()
 *
 * Description:
 *   create the compositions interface for app
 *
 * Input Parameters:
 *   data - the compositions_t of data
 *
 * Returned Value:
 *   returns the flag that the vibration is enabled, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

uint8_t vibrator_create_compositions(compositions_t data)
{
    uint8_t ret;
    vibrator_t buffer;

    buffer.type = VIBRATION_COMPOSITION;
    buffer.comp = data;

    if ((ret = vib_commit(buffer)) < 0) {
        return ret;
    }

    return ret;
}

/****************************************************************************
 * Name: vibrator_create_waveform()
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
 * Returned Value:
 *   returns the flag that the vibration is enabled, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

uint8_t vibrator_create_waveform(uint32_t timings[], uint8_t amplitudes[],
                                 uint8_t repeat, u_int8_t length)
{
    uint8_t ret;
    waveform_t wave;
    vibrator_t buffer;

    wave.length = length;
    wave.repeat = repeat;
    memcpy(wave.timings, timings, sizeof(uint32_t) * length);
    memcpy(wave.amplitudes, amplitudes, sizeof(uint8_t) * length);

    buffer.type = VIBRATION_WAVEFORM;
    buffer.wave = wave;

    if ((ret = vib_commit(buffer)) < 0) {
        return ret;
    }

    return ret;
}

/****************************************************************************
 * Name: vibrator_create_oneshot()
 *
 * Description:
 *    create waveform vibration effects
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

uint8_t vibrator_create_oneshot(uint32_t timing, uint8_t amplitude)
{
    uint8_t ret;
    uint32_t timings[] = { timing };
    uint8_t amplitudes[] = { amplitude };
    uint8_t len = 1;
    uint8_t rep = -1;

    ret = vibrator_create_waveform(timings, amplitudes, len, rep);
    return ret;
}

/****************************************************************************
 * Name: vibrator_create_predefined()
 *
 * Description:
 *    create the predefined interface for app
 *
 * Input Parameters:
 *   effectid - effectid of vibrator
 *
 * Returned Value:
 *   returns the flag that the vibration is enabled, greater than or equal
 *   to 0 means success, otherwise it means failure
 *
 ****************************************************************************/

uint8_t vibrator_create_predefined(uint8_t effect_id)
{
    int ret;
    vibrator_t buffer;

    buffer.type = VIBRATION_EFFECT;
    buffer.effectid = effect_id;

    if ((ret = vib_commit(buffer)) < 0) {
        return ret;
    }

    return ret;
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

int vibrator_cancel(uint8_t vibration_id)
{
    int ret;
    vibrator_t buffer;

    buffer.type = VIBRATION_STOP;

    ret = vib_commit(buffer);
    return ret;
}
