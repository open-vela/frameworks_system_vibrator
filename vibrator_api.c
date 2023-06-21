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

static int g_vibe_id = 0;

static const compositions_t data[VIBRATION_TYPE_COUNT] = {
    { 0 },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // crown (1)
            {
                .patternid = { WAV1, 0, 0, 0, 0, 0, 0, 0 },
                .waveloop = { 0, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 13,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // keyboard (2)
            {
                .patternid = { WAV2, 0, 0, 0, 0, 0, 0, 0 },
                .waveloop = { 0, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 17,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // watch face (3)
            {
                .patternid = { WAV3_3, 0, 0, 0, 0, 0, 0, 0 },
                .waveloop = { 0, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 42,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // success (4)
            {
                .patternid = { WAV_8, 0, 0, 0, 0, 0, 0, 0 },
                .waveloop = { 4, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 63,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // failed (5)
            {
                .patternid = { WAV_8, WAV_9, WAIT(96), WAV_7, 0, 0, 0, 0 },
                .waveloop = { 2, 4, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 287,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // system opration (6)
            {
                .patternid = { WAV3_3, WAIT(32), WAV3_3, WAV_9, 0, 0, 0, 0 },
                .waveloop = { 0, 0, 0, 12, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 386,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // health alert (7)
            {
                .patternid = { WAV_7, WAIT(53.333), WAV_7, WAIT(53.333), WAV_7, WAV_9, WAV_9, 0 },
                .waveloop = { 0, 0, 0, 0, 0, 14, 5, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 692,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // system event (8)
            {
                .patternid = { WAV_9, WAV_9, 0, 0, 0, 0, 0, 0 },
                .waveloop = { 14, 6, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 459,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // notification (9)
            {
                .patternid = { WAV_7, WAIT(48), WAV_8, 0, 0, 0, 0, 0 },
                .waveloop = { 0, 0, 3, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 148,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // target done (10)
            {
                .patternid = { WAV_10, WAIT(122.667), WAV_7, WAIT(74.667), WAV3_3, WAIT(42.667), WAV3_1, 0 },
                .waveloop = { 8, 0, 0, 0, 0, 0, 1, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 580,
            },
        },
    },
    {
        .count = 2,
        .repeatable = 0,
        .pattern = {
            // breathing training (11)
            {
                .patternid = { WAV_8, 0, 0, 0, 0, 0, 0, 0 },
                .waveloop = { 9, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 4008,
            },
            {
                .patternid = { WAV_8, 0, 0, 0, 0, 0, 0, 0 },
                .waveloop = { 9, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 0,
                .strength = 1,
                .duration = 2984,
            },
        },
    },
    {
        .count = 2,
        .repeatable = 0,
        .pattern = {
            // MiRemix, phonecall (12)
            {
                .patternid = { WAV_9, WAIT(362.667), WAV_9, WAIT(346.667), WAV3_3, WAIT(149.333), WAV_7, WAIT(133.333) },
                .waveloop = { 9, 0, 9, 0, 0, 0, 0, 0 },
                .mainloop = 12,
                .strength = 1,
                .duration = 19487,
            },
            {
                .patternid = { WAV_9, WAIT(362.667), WAV_9, WAIT(346.667), WAV3_3, WAIT(149.333), WAV_7, WAIT(133.333) },
                .waveloop = { 9, 0, 9, 0, 0, 0, 0, 0 },
                .mainloop = 12,
                .strength = 1,
                .duration = 19487,
            },
        },
    },
    {
        .count = 1,
        .repeatable = 0,
        .pattern = {
            // sunrise, clockalarm (13)
            {
                .patternid = { WAV_8, WAIT(96), WAV_7, WAIT(213.333), WAV_7, WAIT(90.667), WAV_7, WAIT(325.333) },
                .waveloop = { 12, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 13,
                .strength = 1,
                .duration = 1035,
            },
        },
    },
    {
        .count = 5,
        .repeatable = 0,
        .pattern = {
            // sleep alarm (14)
            {
                .patternid = { WAV_8, WAIT(96), WAV_7, WAIT(213.333), WAV_7, WAIT(90.667), WAV_7, WAIT(325.333) },
                .waveloop = { 12, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 13,
                .strength = 0.2,
                .duration = 1035,
            },
            {
                .patternid = { WAV_8, WAIT(96), WAV_7, WAIT(213.333), WAV_7, WAIT(90.667), WAV_7, WAIT(325.333) },
                .waveloop = { 12, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 13,
                .strength = 0.4,
                .duration = 1035,
            },
            {
                .patternid = { WAV_8, WAIT(96), WAV_7, WAIT(213.333), WAV_7, WAIT(90.667), WAV_7, WAIT(325.333) },
                .waveloop = { 12, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 13,
                .strength = 0.6,
                .duration = 1035,
            },
            {
                .patternid = { WAV_8, WAIT(96), WAV_7, WAIT(213.333), WAV_7, WAIT(90.667), WAV_7, WAIT(325.333) },
                .waveloop = { 12, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 13,
                .strength = 0.8,
                .duration = 1035,
            },
            {
                .patternid = { WAV_8, WAIT(96), WAV_7, WAIT(213.333), WAV_7, WAIT(90.667), WAV_7, WAIT(325.333) },
                .waveloop = { 12, 0, 0, 0, 0, 0, 0, 0 },
                .mainloop = 13,
                .strength = 1,
                .duration = 1035,
            },
        },
    },
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: create_compositions()
 *
 * Description:
 *   create the compositions interface for app
 *
 * Input Parameters:
 *   var_id - the subscript of the data array
 *
 ****************************************************************************/

uint8_t create_compositions(uint8_t var_id)
{
    mqd_t mq;
    uint8_t buffer[MAX_MSG_SIZE];

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(buffer);

    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0660, &attr);
    if (mq == (mqd_t)-1) {
        DBG("mq_open failed");
        return -1;
    }
    buffer[0] = VIBRATION_COMPOSITION;
    memcpy(buffer + 4, &data[var_id], sizeof(compositions_t));

    if (mq_send(mq, (const char*)buffer, sizeof(buffer), 0) == -1) {
        DBG("mq_send failed");
        return -1;
    }

    g_vibe_id++;
    if (g_vibe_id == INT32_MAX) {
        g_vibe_id = 0;
    }

    mq_close(mq);
    return g_vibe_id;
}

/****************************************************************************
 * Name: create_waveform()
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
 ****************************************************************************/

uint8_t create_waveform(uint32_t timings[], uint8_t amplitudes[],
                        uint8_t repeat, u_int8_t length)
{
    mqd_t mq;
    waveform_t wave;
    uint8_t buffer[MAX_MSG_SIZE];

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;

    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0660, &attr);
    if (mq == (mqd_t)-1) {
        DBG("mq_open failed");
        return -1;
    }

    wave.length = length;
    wave.repeat = repeat;
    memcpy(wave.timings, timings, sizeof(uint32_t) * length);
    memcpy(wave.amplitudes, amplitudes, sizeof(uint8_t) * length);

    buffer[0] = VIBRATION_WAVEFORM;
    memcpy(buffer + 4, &wave, sizeof(waveform_t));

    if (mq_send(mq, (const char*)buffer, MAX_MSG_SIZE, 0) == -1) {
        DBG("mq_send failed");
        return -1;
    }

    g_vibe_id++;
    if (g_vibe_id == INT32_MAX) {
        g_vibe_id = 0;
    }

    mq_close(mq);
    return g_vibe_id;
}

/****************************************************************************
 * Name: create_oneshot()
 *
 * Description:
 *    create waveform vibration effects
 *
 * Input Parameters:
 *   timing - duration of vibration
 *   amplitude - amplitude of vibration
 *
 ****************************************************************************/

uint8_t create_oneshot(uint32_t timing, uint8_t amplitude)
{
    uint8_t ret = -1;
    uint32_t timings[] = { timing };
    uint8_t amplitudes[] = { amplitude };
    uint8_t len = 1;
    uint8_t rep = -1;

    ret = create_waveform(timings, amplitudes, len, rep);
    return ret;
}

/****************************************************************************
 * Name: create_predefined()
 *
 * Description:
 *    create the predefined interface for app
 *
 * Input Parameters:
 *   effectid - effectid of vibrator
 *
 ****************************************************************************/

uint8_t create_predefined(uint8_t effectid)
{
    mqd_t mq;
    uint8_t buffer[MAX_MSG_SIZE];

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;

    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0660, &attr);
    if (mq == (mqd_t)-1) {
        DBG("mq_open failed");
        return -1;
    }
    buffer[0] = VIBRATION_EFFECT;
    buffer[1] = effectid;
    if (mq_send(mq, (char*)buffer, MAX_MSG_SIZE, 0) == -1) {
        DBG("mq_send failed");
        return -1;
    }

    g_vibe_id++;
    if (g_vibe_id == INT32_MAX) {
        g_vibe_id = 0;
    }

    mq_close(mq);
    return g_vibe_id;
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
 ****************************************************************************/

void vibrator_cancel(uint8_t vibration_id)
{
    DBG("Current vibration id=[%d], cancel vibration id=[%d]",
        g_vibe_id, vibration_id);
    if (vibration_id == g_vibe_id) {
        mqd_t mq;
        uint8_t buffer = VIBRATION_STOP;

        struct mq_attr attr;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = MAX_MSG_SIZE;

        mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0660, &attr);
        if (mq == (mqd_t)-1) {
            DBG("mq_open failed");
            return;
        }

        if (mq_send(mq, (char*)&buffer, MAX_MSG_SIZE, 0) == -1) {
            DBG("mq_send failed");
            return;
        }
    }
}
