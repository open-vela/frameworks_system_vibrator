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

#include <kvdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
#define VIBRATOR_TEST_DEFAULT_STRENGTH 2
#define VIBRATOR_TEST_WAVEFORM_MAX 7
#define VIBRATOR_TEST_COMPOSE_MAX 4
#define VIBRATOR_TEST_DEFAULT_INTERVAL 1000
#define VIBRATOR_TEST_DEFAULT_COUNT 5

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct waveform_arrays_s {
    uint32_t* timings;
    uint8_t* amplitudes;
    uint8_t length;
};

struct compose_arrays_s {
    vibrator_composite_effect_t* composite_effects;
    uint8_t length;
};

struct vibrator_test_s {
    int api;
    int time;
    int amplitude;
    int intensity;
    int effectid;
    int es;
    int repeat;
    int waveformid;
    int composeid;
    int interval;
    int count;
    struct waveform_arrays_s waveform_args[VIBRATOR_TEST_WAVEFORM_MAX];
    struct compose_arrays_s compose_args[VIBRATOR_TEST_COMPOSE_MAX];
};

enum vibrator_test_apino_e {
    VIBRATOR_TEST_OENSHOT = 1,
    VIBRATOR_TEST_WAVEFORM,
    VIBRATOR_TEST_PREDEFINED,
    VIBRATOR_TEST_PRIMITIVE,
    VIBRATOR_TEST_SETAMPLITUDE,
    VIBRATOR_TEST_START,
    VIBRATOR_TEST_CANCEL,
    VIBRATOR_TEST_GETCAPABILITIES,
    VIBRATOR_TEST_SETINTENSITY,
    VIBRATOR_TEST_GETINTENSITY,
    VIBRATOR_TEST_INTERVAL,
    VIBRATOR_TEST_CALIBRATE,
    VIBRATOR_TEST_SET_CALIBVALUE,
    VIBRATOR_TEST_COMPOSE,
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
           "\t[-i <val> ] The intensity of vibration[0,3], default: 2\n"
           "\t[-s <val> ] The effect strength, [0, 2], default: 2\n"
           "\t[-l <val> ] The waveform array id, [0, 6], default: 0\n"
           "\t[-p <val> ] The compose array id, [0, 3], default: 0\n"
           "\t[-d <val> ] The interval of vibration in milliseconds, default: 1000\n"
           "\t[-c <val> ] The count of vibration, default: 5\n");
}

static int test_play_predefined(uint8_t id, vibrator_effect_strength_e es)
{
    int32_t play_length_ms;
    int ret;
    printf("id = %d, es = %d\n", id, es);
    ret = vibrator_play_predefined(id, es, &play_length_ms);
    printf("Effect(with strength) play length: %" PRIi32 "\n", play_length_ms);
    return ret;
}

static int test_paly_waveform(int repeat, struct waveform_arrays_s waveform_args)
{
    printf("repeat = %d, length = %d\n", repeat, waveform_args.length);
    return vibrator_play_waveform(waveform_args.timings,
        waveform_args.amplitudes, repeat, waveform_args.length);
}

static int test_play_interval(int duration, int interval, int count)
{
    return vibrator_play_interval(duration, interval, count);
}

static int test_play_primitive(uint8_t id, uint16_t amplitude)
{
    int32_t play_length_ms;
    int ret;

    float amplitude_f = (float)amplitude / 255.0;

    ret = vibrator_play_primitive(id, amplitude_f, &play_length_ms);
    printf("Effect(with amplitude) play length: %" PRIi32 "\n", play_length_ms);
    return ret;
}

static int test_play_compose(int repeat, struct compose_arrays_s compose_args)
{
    int ret;

    ret = vibrator_play_compose(compose_args.composite_effects, repeat, compose_args.length);
    // sleep(6);
    printf("Play compose done: ret = %d\n", ret);
    return ret;
}

static int test_get_intensity(void)
{
    vibrator_intensity_e intensity;
    int ret;

    ret = vibrator_get_intensity(&intensity);
    if (ret >= 0)
        printf("vibrator server reporting current intensity: %d\n", intensity);

    return ret;
}

static int test_get_capabilities(void)
{
    int32_t capabilities;
    int ret;

    ret = vibrator_get_capabilities(&capabilities);
    if (ret < 0)
        return ret;

    printf("vibrator server reporting capalities: %" PRIi32 "\n", capabilities);
    return ret;
}

static int test_calibrate(void)
{
    uint8_t value[32] = { 0 };
    char value_fmt[32] = { 0 };
    uint8_t calib_finish = 0;
    int* calib_value = (int*)value;
    int ret;

    ret = vibrator_calibrate(value);
    if (ret >= 0) {
        calib_finish = 1;
        printf("vibrator calibrate finished calibrate value: ");
        for (int i = 0; i < sizeof(value); i++) {
            printf("%02x ", value[i]);
        }
        printf("\n");
    }
    sprintf(value_fmt, "%d,%d", calib_finish, calib_value[0]);
    printf("calib_value[0]: %d, value_fmt: %s\n", calib_value[0], value_fmt);
    property_set("calibvalue.testkey", (char*)value_fmt);

    return ret;
}

static int test_set_calibvalue(void)
{
    uint8_t value[PROP_VALUE_MAX] = { 0 };
    int ret;

    ret = property_get("calibvalue.testkey", (char*)value, "no_value");
    printf("calibvalue.testkey: %s\n", value);

    if (ret < 0 || strcmp((char*)value, "no_value") == 0) {
        printf("get vibrator calib failed, errno = %d", errno);
        return -1;
    }

    return vibrator_set_calibvalue(value);
}

static int param_parse(int argc, char* argv[],
    struct vibrator_test_s* test_data)
{
    const char* apino;
    int ch;

    while ((ch = getopt(argc, argv, "t:a:e:r:i:s:l:p:d:c:h")) != EOF) {
        switch (ch) {
        case 't': {
            printf("%s\n", optarg);
            test_data->time = atoi(optarg);
            printf("%d\n", test_data->time);
            if (test_data->time < 0)
                printf("NOTE: Invalid time, use positive integer\n");
            break;
        }
        case 'a': {
            test_data->amplitude = atoi(optarg);
            if (test_data->amplitude < 0 || test_data->amplitude > 255)
                printf("NOTE: amplitude should be in range [0,255]\n");
            break;
        }
        case 'e': {
            test_data->effectid = atoi(optarg);
            if (test_data->effectid < 0)
                printf("NOTE: effect id should be non-negative\n");
            break;
        }
        case 'r': {
            test_data->repeat = atoi(optarg);
            printf("test_data->repeat = %d\n", test_data->repeat);
            if (test_data->repeat < -1)
                printf("NOTE: Invalid repeat, use -1 to disable \
                        repeat or index of timings array\n");
            break;
        }
        case 'i': {
            test_data->intensity = atoi(optarg);
            if (test_data->intensity < VIBRATION_INTENSITY_LOW
                || test_data->intensity > VIBRATION_INTENSITY_HIGH)
                printf("NOTE: Invalid intensity, use 0, 1, 2, 3\n");
            break;
        }
        case 's': {
            test_data->es = atoi(optarg);
            if (test_data->es < VIBRATION_LIGHT
                || test_data->es > VIBRATION_STRONG)
                printf("NOTE: Invalid effect strength, use 0, 1, 2\n");
            break;
        }
        case 'l': {
            test_data->waveformid = atoi(optarg);
            printf("test_data->waveformid = %d\n", test_data->waveformid);
            if (test_data->waveformid < 0
                || test_data->waveformid >= VIBRATOR_TEST_WAVEFORM_MAX) {
                printf("NOTE: Invalid waveform id, use 0 to 6\n");
                return -1;
            }
            break;
        }
        case 'p': {
            test_data->composeid = atoi(optarg);
            printf("test_data->composeid = %d\n", test_data->composeid);
            if (test_data->composeid < 0
                || test_data->composeid >= VIBRATOR_TEST_COMPOSE_MAX) {
                printf("NOTE: Invalid compose id, use 0 to 3\n");
                return -1;
            }
            break;
        }
        case 'd': {
            test_data->interval = atoi(optarg);
            printf("test_data->interval = %d\n", test_data->interval);
            if (test_data->interval < 0) {
                printf("NOTE: Invalid interval, use non-negative value\n");
            }
            break;
        }
        case 'c': {
            test_data->count = atoi(optarg);
            printf("test_data->count = %d\n", test_data->count);
            if (test_data->count < 0) {
                printf("NOTE: Invalid count, use non-negative value\n");
            }
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
    case VIBRATOR_TEST_OENSHOT:
        printf("API TEST: vibrator_play_oneshot\n");
        ret = vibrator_play_oneshot(test_data->time, test_data->amplitude);
        if (ret < 0) {
            printf("play_oneshot failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_WAVEFORM:
        printf("API TEST: vibrator_play_waveform, id = %d\n", test_data->waveformid);
        ret = test_paly_waveform(test_data->repeat, test_data->waveform_args[test_data->waveformid]);
        if (ret < 0) {
            printf("play_waveform failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_PREDEFINED:
        printf("API TEST: vibrator_play_predefined\n");
        ret = test_play_predefined(test_data->effectid, test_data->es);
        if (ret < 0) {
            printf("play_predefined failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_PRIMITIVE:
        printf("API TEST: test_play_primitive\n");
        ret = test_play_primitive(test_data->effectid, test_data->amplitude);
        if (ret < 0) {
            printf("play_predefined failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_COMPOSE:
        printf("API TEST: vibrator_play_compose\n");
        ret = test_play_compose(test_data->repeat, test_data->compose_args[test_data->composeid]);
        if (ret < 0) {
            printf("play_compose failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_SETAMPLITUDE:
        printf("API TEST: vibrator_set_amplitude\n");
        ret = vibrator_set_amplitude(test_data->amplitude);
        if (ret < 0) {
            printf("set_amplitude failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_START:
        printf("API TEST: vibrator_start\n");
        ret = vibrator_start(test_data->time);
        if (ret < 0) {
            printf("start failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_CANCEL:
        printf("API TEST: vibrator_cancel\n");
        ret = vibrator_cancel();
        if (ret < 0) {
            printf("cancel failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_GETCAPABILITIES:
        printf("API TEST: vibrator_get_capabilities\n");
        ret = test_get_capabilities();
        if (ret < 0) {
            printf("get_capabilities failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_SETINTENSITY:
        printf("API TEST: vibrator_set_intensity\n");
        ret = vibrator_set_intensity(test_data->intensity);
        if (ret < 0) {
            printf("set_intensity failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_GETINTENSITY:
        printf("API TEST: vibrator_get_intensity\n");
        ret = test_get_intensity();
        if (ret < 0) {
            printf("get_intensity failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_INTERVAL:
        printf("API TEST: vibrator_play_interval\n");
        ret = test_play_interval(test_data->time, test_data->interval, test_data->count);
        if (ret < 0) {
            printf("play_interval failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_CALIBRATE:
        printf("API TEST: vibrator_calibrate\n");
        ret = test_calibrate();
        if (ret < 0) {
            printf("calibrate failed: %d\n", ret);
            return ret;
        }
        break;
    case VIBRATOR_TEST_SET_CALIBVALUE:
        printf("API TEST: vibrator_set_calibvalue\n");
        ret = test_set_calibvalue();
        if (ret < 0) {
            printf("set_calibvalue failed: %d\n", ret);
            return ret;
        }
        break;
    default:
        printf("arg out of range\n");
        break;
    }

    printf("PASSED\n");
    return 0;
}

static void waveform_args_init(struct vibrator_test_s* test_data)
{
    static uint32_t timings0[] = { 100, 100, 100, 100 };
    static uint8_t amplitudes0[] = { 51, 0, 51, 0 };
    static uint32_t timings1[] = { 200, 100, 0, 100, 200, 100, 1, 100, 200, 100, 200 };
    static uint8_t amplitudes1[] = { 255, 0, 153, 0, 102, 0, 255, 0, 153, 0, 102 };
    static uint32_t timings2[] = { 100, 100, 100, 100 };
    static uint8_t amplitudes2[] = { 255, 0, 102, 0 };
    static uint32_t timings3[] = { 200, 100, 200, 100 };
    static uint8_t amplitudes3[] = { 255, 0, 51, 0 };
    static uint32_t timings4[] = { 4294967295, 0 };
    static uint8_t amplitudes4[] = { 255, 0 };
    static uint32_t timings5[] = { 1000 };
    static uint8_t amplitudes5[] = { 1 };
    static uint32_t timings6[] = { 2000, 0, 0 };
    static uint8_t amplitudes6[] = { 100, 0, 0 };

    /* vaild waveform arrays */

    test_data->waveform_args[0] = (struct waveform_arrays_s) { timings0, amplitudes0, 4 };
    test_data->waveform_args[1] = (struct waveform_arrays_s) { timings1, amplitudes1, 11 };

    /* invalid waveform arrays */

    test_data->waveform_args[2] = (struct waveform_arrays_s) { timings2, amplitudes2, 10 };
    test_data->waveform_args[3] = (struct waveform_arrays_s) { timings3, amplitudes3, 0 };

    /* boundary waveform arrays */

    test_data->waveform_args[4] = (struct waveform_arrays_s) { timings4, amplitudes4, 2 };

    /* low amplitude waveform arrays */

    test_data->waveform_args[5] = (struct waveform_arrays_s) { timings5, amplitudes5, 1 };

    /* for -r invalid index test*/

    test_data->waveform_args[6] = (struct waveform_arrays_s) { timings6, amplitudes6, 3 };
}

static void compose_args_init(struct vibrator_test_s* test_data)
{
    static vibrator_composite_effect_t compose_effect1[] = {
        {
            .delay_ms = 0,
            .primitive = 20,
            .scale = 1,
        },
        {
            .delay_ms = 0,
            .primitive = 20,
            .scale = 1,
        }
    };

    static vibrator_composite_effect_t compose_effect2[] = {
        {
            .delay_ms = 0,
            .primitive = 21,
            .scale = 1,
        },
        {
            .delay_ms = 0,
            .primitive = 21,
            .scale = 1,
        }
    };

    static vibrator_composite_effect_t compose_effect3[] = {
        {
            .delay_ms = 0,
            .primitive = 22,
            .scale = 1,
        }
    };

    static vibrator_composite_effect_t compose_effect4[] = {
        {
            .delay_ms = 0,
            .primitive = 22,
            .scale = 0.2,
        },
        {
            .delay_ms = 0,
            .primitive = 22,
            .scale = 0.4,
        },
        {
            .delay_ms = 0,
            .primitive = 22,
            .scale = 0.6,
        },
        {
            .delay_ms = 0,
            .primitive = 22,
            .scale = 0.8,
        },
        {
            .delay_ms = 0,
            .primitive = 22,
            .scale = 1,
        },
    };

    test_data->compose_args[0] = (struct compose_arrays_s) { compose_effect1, 2 };
    test_data->compose_args[1] = (struct compose_arrays_s) { compose_effect2, 2 };
    test_data->compose_args[2] = (struct compose_arrays_s) { compose_effect3, 1 };
    test_data->compose_args[3] = (struct compose_arrays_s) { compose_effect4, 5 };
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
    test_data.es = VIBRATOR_TEST_DEFAULT_STRENGTH;
    test_data.time = VIBRATOR_TEST_DEFAULT_TIME;
    test_data.api = VIBRATOR_TEST_DEFAULT_API;
    test_data.interval = VIBRATOR_TEST_DEFAULT_TIME;
    test_data.count = VIBRATOR_TEST_DEFAULT_COUNT;

    /*Init waveform test arrays*/
    waveform_args_init(&test_data);
    test_data.waveformid = 0;

    /* Init compose test arrays */
    compose_args_init(&test_data);
    test_data.composeid = 0;

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
