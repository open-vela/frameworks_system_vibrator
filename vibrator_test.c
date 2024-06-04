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

#include <stdint.h>
#include <stdio.h>

#include <vibrator_api.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIBRATOR_TEST_DEFAULT_AMPLITUDE 255
#define VIBRATOR_TEST_DEFAULT_INTENSITY 2
#define VIBRATOR_TEST_DEFAULT_EFFECT_ID 5
#define VIBRATOR_TEST_DEFAULT_TIME 3000
#define VIBRATOR_TEST_DEFAULT_REPEAT -1
#define VIBRATOR_TEST_DEFAULT_API 1

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct vibrator_test_s {
    int api;
    int time;
    int amplitude;
    int intensity;
    int effectid;
    int repeat;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(void)
{
    printf("Utility to test vibrator API.\n"
           "Usage: vibrator_test [arguments...] <apino>\n"
           "\t<apino>     Which api to be tested\n"
           "\tArguments:\n"
           "\t[-h       ] Commands help\n"
           "\t[-t <val> ] The number of milliseconds to vibrate, default: 3000\n"
           "\t[-a <val> ] The amplitude of vibration, [0,255], default: 255\n"
           "\t[-e <val> ] Effect id, default: 5\n"
           "\t[-r <val> ] The index into the timings array at which to repeat,\n"
           "\t            -1 means no repeat, default: -1\n"
           "\t[-i <val> ] The intensity of vibration, default: 2\n");
}

static int test_play_predefined(uint8_t id)
{
    int play_length_ms;
    int ret;

    ret = vibrator_play_predefined(id, VIBRATION_STRONG, &play_length_ms);
    printf("Effect play length: %d\n", play_length_ms);
    return ret;
}

static int test_paly_waveform(int repeat)
{
    uint32_t timings[] = { 100, 200, 100, 200 };
    uint8_t amplitudes[] = { 0, 255, 100, 0 };
    uint8_t length = sizeof(timings) / sizeof(timings[0]);

    return vibrator_play_waveform(timings, amplitudes, repeat, length);
}

static int test_get_intensity(void)
{
    vibrator_intensity_e intensity;

    intensity = vibrator_get_intensity();

    printf("vibrator server reporting current intensity: %d\n", intensity);
    return intensity;
}

static int test_get_capabilities(void)
{
    int32_t capabilities;
    int ret;

    ret = vibrator_get_capabilities(&capabilities);
    if (ret < 0)
        return ret;

    printf("vibrator server reporting capalities: %d\n", capabilities);
    return ret;
}

static int param_parse(int argc, char* argv[],
    struct vibrator_test_s* test_data)
{
    const char* apino;
    int ch;

    while ((ch = getopt(argc, argv, "t:a:e:r:i:h")) != EOF) {
        switch (ch) {
        case 't': {
            printf("%s\n", optarg);
            test_data->time = atoi(optarg);
            printf("%d\n", test_data->time);
            if (test_data->time < 0)
                return -1;
            break;
        }
        case 'a': {
            test_data->amplitude = atoi(optarg);
            if (test_data->amplitude < 0 || test_data->amplitude > 255)
                return -1;
            break;
        }
        case 'e': {
            test_data->effectid = atoi(optarg);
            if (test_data->effectid < 0)
                return -1;
            break;
        }
        case 'r': {
            test_data->repeat = atoi(optarg);
            if (test_data->repeat < -1)
                return -1;
            break;
        }
        case 'i': {
            test_data->intensity = atoi(optarg);
            if (test_data->intensity < VIBRATION_INTENSITY_LOW
                || test_data->intensity > VIBRATION_INTENSITY_HIGH)
                return -1;
            break;
        }
        case 'h':
        default: {
            return -1;
        }
        }
    }

    if (optind < argc) {
        apino = argv[optind];
        test_data->api = atoi(apino);
        printf("cmd = %s, apino = %d\n", apino, test_data->api);
    }

    return 0;
}

static int do_vibrator_test(struct vibrator_test_s* test_data)
{
    int ret;

    switch (test_data->api) {
    case 1:
        printf("API TEST: vibrator_play_oneshot\n");
        ret = vibrator_play_oneshot(test_data->time, test_data->amplitude);
        if (ret < 0) {
            printf("play_oneshot failed: %d\n", ret);
            return ret;
        }
        break;
    case 2:
        printf("API TEST: vibrator_play_waveform\n");
        ret = test_paly_waveform(test_data->repeat);
        if (ret < 0) {
            printf("play_waveform failed: %d\n", ret);
            return ret;
        }
        break;
    case 3:
        printf("API TEST: vibrator_play_predefined\n");
        ret = test_play_predefined(test_data->effectid);
        if (ret < 0) {
            printf("play_predefined failed: %d\n", ret);
            return ret;
        }
        break;
    case 4:
        printf("API TEST: vibrator_set_amplitude\n");
        ret = vibrator_set_amplitude(test_data->amplitude);
        if (ret < 0) {
            printf("set_amplitude failed: %d\n", ret);
            return ret;
        }
        break;
    case 5:
        printf("API TEST: vibrator_start\n");
        ret = vibrator_start(test_data->time);
        if (ret < 0) {
            printf("start failed: %d\n", ret);
            return ret;
        }
        break;
    case 6:
        printf("API TEST: vibrator_cancel\n");
        ret = vibrator_cancel();
        if (ret < 0) {
            printf("cancel failed: %d\n", ret);
            return ret;
        }
        break;
    case 7:
        printf("API TEST: vibrator_get_capabilities\n");
        ret = test_get_capabilities();
        if (ret < 0) {
            printf("get_capabilities failed: %d\n", ret);
            return ret;
        }
        break;
    case 8:
        printf("API TEST: vibrator_set_intensity\n");
        ret = vibrator_set_intensity(test_data->intensity);
        if (ret < 0) {
            printf("set_intensity failed: %d\n", ret);
            return ret;
        }
        break;
    case 9:
        printf("API TEST: vibrator_get_intensity\n");
        ret = test_get_intensity();
        if (ret < 0) {
            printf("get_intensity failed: %d\n", ret);
            return ret;
        }
        break;
    default:
        printf("arg out of range\n");
        break;
    }

    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char* argv[])
{
    struct vibrator_test_s test_data;

    /*Init test data using default value*/

    test_data.intensity = VIBRATOR_TEST_DEFAULT_INTENSITY;
    test_data.amplitude = VIBRATOR_TEST_DEFAULT_AMPLITUDE;
    test_data.effectid = VIBRATOR_TEST_DEFAULT_EFFECT_ID;
    test_data.repeat = VIBRATOR_TEST_DEFAULT_REPEAT;
    test_data.time = VIBRATOR_TEST_DEFAULT_TIME;
    test_data.api = VIBRATOR_TEST_DEFAULT_API;

    /* Parse Argument */

    if (param_parse(argc, argv, &test_data) < 0) {
        goto error;
    }

    /* Do vibrator API test */

    do_vibrator_test(&test_data);

    return 0;

error:
    usage();
    return 1;
}
