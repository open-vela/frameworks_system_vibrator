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
 * @brief Included Files
 ****************************************************************************/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netpacket/rpmsg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#ifdef CONFIG_VIBRATOR_UV_API
#include <uv.h>
#endif

#include "vibrator_internal.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_VIBRATOR_UV_API
/* struct vibrator_pipe_t
 * @loop: the loop of uv
 * @handle: the handle of pipe
 * @connect_req: the connect request of pipe
 * @shutdown_req: the shutdown request of pipe
 * @on_connect: the callback function when connect to vibrator server
 * @on_read: the callback function when read from vibrator server
 * @cookie: Long-term private context
 * @msg: the vibrator_msg_t of above structure
 * @on_read_pending: the flag of read pending
 */

typedef struct {
    uv_loop_t* loop;
    uv_pipe_t handle;
    uv_connect_t connect_req;
    uv_shutdown_t shutdown_req;
    vibrator_uv_callback on_connect;
    vibrator_uv_callback on_read;
    void* cookie;
    vibrator_msg_t msg;
    int on_read_pending;
} vibrator_pipe_t;
#endif

/****************************************************************************
 * @brief Private Functions
 ****************************************************************************/

/**
 * @brief Fill the vibrator message header
 *
 * @details This function fills the vibrator message header using the specified type.
 *
 * @param buffer The buffer of the vibrator_msg_tS.
 */
static void vibrator_msg_packet(vibrator_msg_t* buffer)
{
    switch (buffer->type) {
    case VIBRATION_WAVEFORM:
    case VIBRATION_INTERVAL:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(vibrator_waveform_t);
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_EFFECT:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(vibrator_effect_t);
        buffer->response_len = VIBRATOR_MSG_HEADER + sizeof(vibrator_effect_t);
        break;
    case VIBRATION_START:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(uint32_t);
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_STOP:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_SET_AMPLITUDE:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(uint8_t);
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_GET_CAPABLITY:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_HEADER + sizeof(int32_t);
        break;
    case VIBRATION_GET_INTENSITY:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_HEADER + sizeof(int32_t);
        break;
    case VIBRATION_SET_INTENSITY:
        buffer->request_len = sizeof(vibrator_intensity_e) + VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_CALIBRATE:
        buffer->request_len = VIBRATOR_MSG_HEADER;
        buffer->response_len = VIBRATOR_MSG_HEADER + VIBRATOR_CALIBVALUE_MAX;
        break;
    case VIBRATION_SET_CALIBVALUE:
        buffer->request_len = VIBRATOR_MSG_HEADER + VIBRATOR_CALIBVALUE_MAX;
        buffer->response_len = VIBRATOR_MSG_RESULT;
        break;
    case VIBRATION_CONTROL:
        buffer->request_len = VIBRATOR_MSG_HEADER + sizeof(vibrator_control_t);
        buffer->response_len = VIBRATOR_MSG_RESULT + sizeof(vibrator_control_t);
        break;
    default:
        VIBRATORERR("unknown message type %d", buffer->type);
        buffer->request_len = sizeof(vibrator_msg_t);
        buffer->response_len = sizeof(vibrator_msg_t);
        break;
    }
}

/**
 * @brief Open message queue
 *
 * @details This function opens the message queue for vibrator messages.
 *
 * @param buffer The type of the vibrator_msg_t.
 *
 * @return Returns a flag indicating whether the vibration is sent.
 */
static int vibrator_commit(vibrator_msg_t* buffer)
{
    int fd;
    int ret;

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

#ifdef CONFIG_VIBRATOR_SERVER
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    fd = socket(AF_RPMSG, SOCK_STREAM | SOCK_CLOEXEC, 0);
#endif
    if (fd < 0) {
        VIBRATORERR("socket fail, errno = %d", errno);
        return fd;
    }

    ret = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        VIBRATORERR("client: connect failure, errno = %d", errno);
        ret = -errno;
        goto errout;
    }

    vibrator_msg_packet(buffer);

    ret = send(fd, buffer, buffer->request_len, 0);
    if (ret < 0) {
        VIBRATORERR("send fail, errno = %d", errno);
        ret = -errno;
        goto errout;
    }

    ret = recv(fd, buffer, sizeof(vibrator_msg_t), 0);
    if (ret < buffer->response_len) {
        VIBRATORERR("recv fail, errno = %d", errno);
        ret = ret < 0 ? -errno : -EINVAL;
        goto errout;
    }
    VIBRATORINFO("recv len = %d, result = %" PRIi32, ret, buffer->result);
    ret = buffer->result;

errout:
    close(fd);
    return ret;
}

#ifdef CONFIG_VIBRATOR_UV_API
/**
 * @brief callback functions for uv operations
 */

static void vibrator_uv_close_cb(uv_handle_t* handle)
{
    vibrator_pipe_t* pipe = uv_handle_get_data(handle);
    free(pipe);
}

static void vibrator_uv_close(void* handle)
{
    uv_read_stop((uv_stream_t*)handle);
    if (!uv_is_closing((uv_handle_t*)handle)) {
        uv_close((uv_handle_t*)handle, vibrator_uv_close_cb);
    }
}

static void vibrator_uv_shutdown_cb(uv_shutdown_t* req, int status)
{
    vibrator_pipe_t* pipe = uv_handle_get_data((uv_handle_t*)req->handle);
    vibrator_uv_close(&pipe->handle);
}

static void vibrator_uv_write_cb(uv_write_t* req, int status)
{
    vibrator_pipe_t* pipe = uv_handle_get_data((uv_handle_t*)req->handle);
    if (pipe->on_read == NULL) {
        pipe->on_read_pending = 0;
    }

    free(req);
}

static void vibrator_uv_alloc_cb(uv_handle_t* handle,
    size_t suggested_size, uv_buf_t* buf)
{
    vibrator_pipe_t* pipe = uv_handle_get_data(handle);
    buf->base = (char*)&pipe->msg;
    buf->len = pipe->msg.response_len;
}

static void vibrator_uv_read_cb(uv_stream_t* stream, ssize_t nread,
    const uv_buf_t* buf)
{
    vibrator_pipe_t* pipe = uv_handle_get_data((uv_handle_t*)stream);

    if (nread == 0)
        return;

    if (nread < 0) {
        VIBRATORERR("client: read failure, errno = %d", errno);
        vibrator_uv_close(&pipe->handle);
        return;
    }

    if (pipe->on_read) {
        vibrator_msg_t* msg = (vibrator_msg_t*)buf->base;
        void* arg = NULL;
        int ret = msg->result;
        switch (msg->type) {
        case VIBRATION_EFFECT:
            arg = &msg->effect.play_length;
            break;
        default:
            break;
        }
        pipe->on_read(&pipe->handle, pipe->cookie, arg, ret);
    }

    pipe->on_read_pending = 0;
    VIBRATORINFO("client: read success, nread = %zd\n", nread);
}

static void vibrator_uv_connect_cb(uv_connect_t* req, int status)
{
    vibrator_pipe_t* pipe = uv_handle_get_data((uv_handle_t*)req->handle);

    if (pipe->on_connect) {
        pipe->on_connect(&pipe->handle, pipe->cookie, NULL, status);
    }

    if (status < 0) {
        VIBRATORERR("client: connect failure, uv_errno_name(status) = %s", uv_err_name(status));
        vibrator_uv_close(&pipe->handle);
        return;
    }

    VIBRATORINFO("client: connect success");
}
#endif

/****************************************************************************
 * @brief Public Functions
 *
 * @details This file contains nine interfaces vibrator_play_waveform,
 *   vibrator_play_oneshot, vibrator_play_predefined, vibrator_get_intensity,
 *   vibrator_set_intensity, vibrator_cancel, vibrator_start,
 *   vibrator_set_amplitude, and vibrator_get_capabilities
 *   and the detailed information of each interface has been described
 *
 ****************************************************************************/

/**
 * @brief Play a waveform vibration.
 *
 * @param timings The pattern of alternating on-off timings, starting with off.
 *                Timing values of 0 will cause the timing/amplitude pair to be ignored.
 * @param amplitudes The amplitude values of the timing/amplitude pairs.
 *                   Amplitude values must be between 0 and 255, or equal to DEFAULT_AMPLITUDE.
 *                   An amplitude value of 0 implies the motor is off.
 * @param repeat The index into the timings array at which to repeat, or -1 if
 *               you don't want to repeat.
 * @param length The length of timings and amplitudes pairs.
 * @return Returns the flag that the vibrator is playing waveform.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_waveform(uint32_t timings[], uint8_t amplitudes[],
    int8_t repeat, uint8_t length)
{
    vibrator_waveform_t wave;
    vibrator_msg_t buffer;

    if (repeat < -1 || repeat >= length)
        return -EINVAL;

    wave.length = length;
    wave.repeat = repeat;
    memcpy(wave.timings, timings, sizeof(uint32_t) * length);
    memcpy(wave.amplitudes, amplitudes, sizeof(uint8_t) * length);

    buffer.type = VIBRATION_WAVEFORM;
    buffer.wave = wave;

    return vibrator_commit(&buffer);
}

/**
 * @brief Play composed primitive effect.
 *
 * @param composite_effects The composition of primitive effects.
 * @param repeat The index into the primitive array at which to repeat, or -1 if
 *               you don't want to repeat.
 * @param length The length of composite effects array.
 * @return Returns the flag that the vibrator is playing the predefined effect.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_compose(vibrator_composite_effect_t* composite_effects,
    int8_t repeat, uint8_t length)
{
    vibrator_msg_t buffer;

    if (repeat < -1 || repeat >= length)
        return -EINVAL;

    buffer.type = VIBRATION_COMPOSITION;
    buffer.composition.length = length;
    buffer.composition.repeat = repeat;
    buffer.composition.index = 0;
    memcpy(buffer.composition.composite_effect, composite_effects,
        sizeof(vibrator_composite_effect_t) * length);

    return vibrator_commit(&buffer);
}

/**
 * @brief Play an interval vibration with specified duration and interval.
 *
 * @param duration The duration of vibration.
 * @param interval The time interval between two vibrations.
 * @param count The number of vibrations.
 * @return Returns the flag that the vibrator is playing interval.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_interval(int32_t duration, int32_t interval,
    int16_t count)
{
    vibrator_msg_t buffer;

    if (duration <= 0 || interval < 0 || count < 0)
        return -EINVAL;

    buffer.type = VIBRATION_INTERVAL;
    buffer.wave.timings[0] = duration;
    buffer.wave.timings[1] = interval;
    buffer.wave.count = count;

    return vibrator_commit(&buffer);
}

/**
 * @brief Play a one-shot vibration.
 *
 * @param timing The number of milliseconds to vibrate. Must be positive.
 * @param amplitude The amplitude of vibration; must be a value between 1 and 255, or DEFAULT_AMPLITUDE.
 * @return Returns the flag that the vibrator is playing one shot.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_oneshot(uint32_t timing, uint8_t amplitude)
{
    return vibrator_play_waveform(&timing, &amplitude, -1, 1);
}

/**
 * @brief Play a predefined vibration effect.
 *
 * @param effect_id The ID of the effect to perform.
 * @param es The vibration intensity.
 * @param play_length Returned effect play duration.
 * @return Returns the flag that the vibrator is playing the predefined effect.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_predefined(uint8_t effect_id, vibrator_effect_strength_e es,
    int32_t* play_length)
{
    vibrator_msg_t buffer;
    int ret;

    if (es < VIBRATION_LIGHT || es > VIBRATION_DEFAULTES)
        return -EINVAL;

    buffer.type = VIBRATION_EFFECT;
    buffer.effect.effect_id = effect_id;
    buffer.effect.es = es;

    ret = vibrator_commit(&buffer);
    if (ret >= 0) {
        if (play_length != NULL)
            *play_length = buffer.effect.play_length;
    }

    return ret;
}

/**
 * @brief Play a predefined vibration effect with the specified amplitude.
 *
 * @param effect_id The ID of the effect to perform.
 * @param amplitude Vibration amplitude (0.0~1.0).
 * @param play_length Returned effect play duration.
 * @return Returns the flag that the vibrator is playing the predefined effect.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_play_primitive(uint8_t effect_id, float amplitude,
    int32_t* play_length)
{
    vibrator_msg_t buffer;
    int ret;

    if (amplitude < 0.0 || amplitude > 1.0)
        return -EINVAL;

    buffer.type = VIBRATION_PRIMITIVE;
    buffer.effect.effect_id = effect_id;
    buffer.effect.amplitude = amplitude;

    ret = vibrator_commit(&buffer);
    if (ret >= 0) {
        if (play_length != NULL)
            *play_length = buffer.effect.play_length;
    }

    return ret;
}

/**
 * @brief Get vibration primitive effect duration.
 *
 * @param effect_id The ID of the effect.
 * @param duration BUffer that stores the effect duration.
 * @return Returns the flag indicating success in getting vibrator duration.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_primitive_duration(uint8_t effect_id, int32_t* duration)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_GET_DURATION;
    buffer.effect.effect_id = effect_id;

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        *duration = buffer.effect.play_length;

    return ret;
}

/**
 * @brief Get vibration intensity.
 *
 * @param intensity Buffer that stores intensity.
 * @return Returns the flag indicating success in getting vibrator intensity.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_intensity(vibrator_intensity_e* intensity)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_GET_INTENSITY;

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        *intensity = buffer.intensity;

    return ret;
}

/**
 * @brief Set vibration intensity.
 *
 * @param intensity The vibration intensity.
 * @return Returns the flag indicating whether setting the intensity was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_intensity(vibrator_intensity_e intensity)
{
    vibrator_msg_t buffer;

    if (intensity < VIBRATION_INTENSITY_LOW || intensity > VIBRATION_INTENSITY_HIGH)
        return -EINVAL;

    buffer.type = VIBRATION_SET_INTENSITY;
    buffer.intensity = intensity;

    return vibrator_commit(&buffer);
}

/**
 * @brief Get vibration is disabled or not.
 *
 * @param disabled Buffer that stores disabled status.
 * @return Returns the flag indicating success in getting vibrator
 *         disabled status. Greater than or equal to 0 means success;
 *         otherwise, it means failure.
 */
int vibrator_is_disabled(uint8_t* disabled)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_IS_DISABLED;

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        *disabled = buffer.disable;

    return ret;
}

/**
 * @brief Set vibration disable or not.
 *
 * @param disable The vibration disable flag.
 * @return Returns the flag indicating success in setting vibrator
 *         disabled status. Greater than or equal to 0 means success;
 *         otherwise, it means failure.
 */
int vibrator_set_disable(uint8_t disable)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_SET_DISABLE;
    buffer.disable = !!disable;

    return vibrator_commit(&buffer);
}

/**
 * @brief Cancel the vibration.
 *
 * @return Returns the flag that the vibration is stopped.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_cancel(void)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_STOP;

    return vibrator_commit(&buffer);
}

/**
 * @brief Start the vibrator with vibrate time.
 *
 * @param timeoutms Number of milliseconds to vibrate.
 * @return Returns the flag that the vibration has started.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_start(int32_t timeoutms)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_START;
    buffer.timeoutms = timeoutms;

    return vibrator_commit(&buffer);
}

/**
 * @brief Set vibration amplitude.
 *
 * @param amplitude The amplitude of vibration; must be a value between 1 and 255, or DEFAULT_AMPLITUDE.
 * @return Returns the flag indicating whether setting the vibrator amplitude was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_amplitude(uint8_t amplitude)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_SET_AMPLITUDE;
    buffer.amplitude = amplitude;

    return vibrator_commit(&buffer);
}

/**
 * @brief Get vibration capabilities.
 *
 * @param capabilities Buffer that stores capabilities.
 * @return Returns the flag indicating success in getting vibrator capability.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_get_capabilities(int32_t* capabilities)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_GET_CAPABLITY;
    buffer.capabilities = 0;

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        *capabilities = buffer.capabilities;

    return ret;
}

/**
 * @brief Calibrate vibrator when it is not calibrated, Generally at the time of leaving the factory.
 *
 * @param data Buffer that stores the calibration result data.
 * @return Returns the flag indicating whether the vibrator calibration was successful.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_calibrate(uint8_t* data)
{
    vibrator_msg_t buffer;
    int ret;

    buffer.type = VIBRATION_CALIBRATE;
    memset(buffer.calibvalue, 0, VIBRATOR_CALIBVALUE_MAX);

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        memcpy(data, buffer.calibvalue, VIBRATOR_CALIBVALUE_MAX);

    return ret;
}

/**
 * @brief Get vibration calibration data.
 *
 * @param data Buffer that stores calibration data.
 * @return Returns the flag indicating success in setting vibrator calibration data.
 *         Greater than or equal to 0 means success; otherwise, it means failure.
 */
int vibrator_set_calibvalue(uint8_t* data)
{
    vibrator_msg_t buffer;

    buffer.type = VIBRATION_SET_CALIBVALUE;
    memcpy(buffer.calibvalue, data, VIBRATOR_CALIBVALUE_MAX);

    return vibrator_commit(&buffer);
}

/**
 * @brief Send custom control command to the vibrator device driver.
 *
 * @param cmd Custom ioctl command code.
 * @param arg The control data send or received.
 * @param len The length of the control data.
 * @return Returns 0 on success, or a negative error code on failure.
 */
int vibrator_control(uint32_t cmd, uint8_t* arg, uint32_t len)
{
    vibrator_msg_t buffer;
    int ret;

    if (len > VIBRATOR_CONTROL_DATA_MAX)
        return -EINVAL;

    buffer.type = VIBRATION_CONTROL;
    buffer.control.cmd = cmd;
    memcpy(buffer.control.data, arg, len);

    ret = vibrator_commit(&buffer);
    if (ret >= 0)
        memcpy(arg, buffer.control.data, len);

    return ret;
}

#ifdef CONFIG_VIBRATOR_UV_API
/**
 * @brief Build a long-term connection with the vibrator server async.
 *
 * @param on_connect The callback function to be called when the connection
 *                   is established.
 * @param cookie     Long-term context, for on_connect.
 * @return Returns handle to the pipe on success, or NULL on failure.
 */
void* vibrator_uv_connect(vibrator_uv_callback on_connect, void* cookie)
{
    vibrator_pipe_t* pipe;
    int ret;

    pipe = zalloc(sizeof(vibrator_pipe_t));
    if (!pipe) {
        VIBRATORERR("zalloc fail, err: %d", -ENOMEM);
        return NULL;
    }

    pipe->loop = uv_default_loop();
    ret = uv_pipe_init(pipe->loop, &pipe->handle, 0);
    if (ret < 0) {
        VIBRATORERR("uv_pipe_init fail, uv_errno_name(ret) = %s", uv_err_name(ret));
        goto errout1;
    }

    pipe->on_connect = on_connect;
    pipe->cookie = cookie;
    uv_handle_set_data((uv_handle_t*)&pipe->handle, pipe);
#ifdef CONFIG_VIBRATOR_SERVER
    uv_pipe_connect(&pipe->connect_req, &pipe->handle,
        PROP_SERVER_PATH, vibrator_uv_connect_cb);
#else
    uv_pipe_rpmsg_connect(&pipe->connect_req, &pipe->handle,
        PROP_SERVER_PATH, CONFIG_VIBRATOR_SERVER_CPUNAME,
        vibrator_uv_connect_cb);
#endif
    ret = uv_read_start((uv_stream_t*)&pipe->handle,
        vibrator_uv_alloc_cb, vibrator_uv_read_cb);
    if (ret < 0) {
        VIBRATORERR("uv_read_start fail, uv_errno_name(ret) = %s", uv_err_name(ret));
        goto errout2;
    }

    return &pipe->handle;

errout2:
    uv_close((uv_handle_t*)&pipe->handle, NULL);
errout1:
    free(pipe);
    return NULL;
}

/**
 * @brief Disconnect the connection.
 *
 * @param handle The handle to the pipe to be disconnected.
 */

void vibrator_uv_disconnect(void* handle)
{
    vibrator_pipe_t* pipe = uv_handle_get_data((uv_handle_t*)handle);

    int ret = uv_shutdown(&pipe->shutdown_req, (uv_stream_t*)handle, vibrator_uv_shutdown_cb);
    if (ret < 0) {
        VIBRATORERR("uv_shutdown fail, uv_errno_name(ret) = %s", uv_err_name(ret));
        vibrator_uv_close(handle);
    }
}

/**
 * @brief Asynchronously sends a request to play a predefined vibration effect to
 *        the vibrator server over an established long connection.
 *
 * @param handle The handle to use for the connection.
 * @param effect_id The ID of the predefined vibration effect to be played.
 * @param es The strength of the vibration effect.
 * @param cb The callback function to be called when the request is sent.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */

int vibrator_uv_play_predefined(void* handle, uint8_t effect_id,
    vibrator_effect_strength_e es, vibrator_uv_callback cb)
{
    vibrator_pipe_t* pipe = uv_handle_get_data((uv_handle_t*)handle);
    uv_write_t* write_req;
    int ret = 0;

    DEBUGASSERT(pipe->loop == uv_default_loop());
    if (pipe->on_read_pending && cb != NULL) {
        VIBRATORERR("err: %d", -EBUSY);
        return -EBUSY;
    }

    pipe->on_read_pending = 1;
    pipe->msg.type = VIBRATION_EFFECT;
    pipe->msg.effect.effect_id = effect_id;
    pipe->msg.effect.es = es;
    vibrator_msg_packet(&pipe->msg);
    if (cb == NULL) {
        pipe->msg.response_len = 0;
    }

    pipe->on_read = cb;
    uv_buf_t send_buf = uv_buf_init((char*)&pipe->msg, pipe->msg.request_len);

    write_req = malloc(sizeof(uv_write_t));
    if (write_req == NULL) {
        VIBRATORERR("malloc fail, err: %d", -ENOMEM);
        return -ENOMEM;
    }

    ret = uv_write(write_req, (uv_stream_t*)&pipe->handle, &send_buf, 1, vibrator_uv_write_cb);

    if (ret < 0) {
        free(write_req);
        VIBRATORERR("uv_write fail, uv_errno_name(ret) = %s", uv_err_name(ret));
    }

    return ret;
}
#endif
