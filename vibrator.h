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

#ifndef __INCLUDE_NUTTX_VIBRATOR_H
#define __INCLUDE_NUTTX_VIBRATOR_H

#include <pthread.h>
#include "./include/vibrator_api.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_CLIENTS 16
#define VIB_LOCAL   0
#define VIB_REMOTE  1
#define VIB_COUNT   2
#define VIBRATOR_DEV_FS "/dev/lra0"
#define MAX_VIBRATION_STRENGTH_LEVEL (100)
#define MOTO_CALI_PREFIX "ro.factory.motor_calib"

/* Vibrator types */

enum {
    VIBRATION_WAVEFORM = 1,
    VIBRATION_EFFECT = 2,
    VIBRATION_COMPOSITION = 3,
    VIBRATION_STOP = 4
};

/* struct threadargs
 * @fd: file descriptor
 * @forcestop: force stop the vibration of waveform
 * @wave: the waveform_t of above structure
 * @mutex: the pthread_mutex_t of above structure
 * @condition: the pthread_cond_t of above structure
 */

typedef struct {
    int fd;
    bool forcestop;
    waveform_t wave;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} threadargs;

#endif /* #define __INCLUDE_NUTTX_VIBRATOR_H */
