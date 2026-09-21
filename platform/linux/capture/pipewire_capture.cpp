#include "pipewire_capture.hpp"
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/param.h>
#include <spa/buffer/buffer.h>
#include <iostream>
#include <vector>
#include <unistd.h>

namespace degoonification::linux_backend {

struct PipeWireCapture::Impl {
    pw_thread_loop* loop{nullptr};
    pw_context* context{nullptr};
    pw_core* core{nullptr};
    pw_stream* stream{nullptr};
    spa_hook stream_listener{};

    std::atomic<bool> is_running{false};
    uint32_t width{0};
    uint32_t height{0};
    FrameCallback frame_callback;
};

static void on_stream_state_changed(void* data, pw_stream_state old_state, pw_stream_state new_state, const char* error) {
    (void)old_state;
    (void)data;
    if (new_state == PW_STREAM_STATE_ERROR) {
        std::cerr << "[PipeWireCapture] Stream error: " << (error ? error : "unknown") << "\n";
    }
}

static void on_stream_param_changed(void* data, uint32_t id, const struct spa_pod* param) {
    auto* impl = static_cast<PipeWireCapture::Impl*>(data);
    if (!param || id != SPA_PARAM_Format || !impl) return;

    struct spa_video_info_raw info{};
    if (spa_format_video_raw_parse(param, &info) < 0) {
        return;
    }

    impl->width = info.size.width;
    impl->height = info.size.height;
}

static void on_stream_process(void* data) {
    auto* impl = static_cast<PipeWireCapture::Impl*>(data);
    if (!impl || !impl->stream) return;

    pw_buffer* b = pw_stream_dequeue_buffer(impl->stream);
    if (!b) return;

    struct spa_buffer* spa_buf = b->buffer;
    if (spa_buf && spa_buf->n_datas > 0) {
        struct spa_data* d = &spa_buf->datas[0];

        VideoFrame frame;
        frame.width = impl->width;
        frame.height = impl->height;
        frame.timestamp_us = b->time;

        if (d->type == SPA_DATA_DmaBuf) {
            frame.dmabuf_fd = d->fd;
            frame.data = nullptr;
            frame.data_size = d->maxsize;
            frame.stride = d->chunk ? d->chunk->stride : (frame.width * 4);
        } else if (d->type == SPA_DATA_MemPtr) {
            frame.dmabuf_fd = -1;
            frame.data = static_cast<const uint8_t*>(d->data);
            frame.data_size = d->chunk ? d->chunk->size : d->maxsize;
            frame.stride = d->chunk ? d->chunk->stride : (frame.width * 4);
        }

        if (impl->frame_callback) {
            impl->frame_callback(frame);
        }
    }

    pw_stream_queue_buffer(impl->stream, b);
}

static const pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = on_stream_state_changed,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = on_stream_param_changed,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = on_stream_process,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr
};

PipeWireCapture::PipeWireCapture(PipeWireConfig config)
    : config_(config) {
    pw_init(nullptr, nullptr);
}

PipeWireCapture::~PipeWireCapture() {
    stop();
}

bool PipeWireCapture::is_running() const noexcept {
    return impl_ && impl_->is_running.load();
}

uint32_t PipeWireCapture::current_width() const noexcept {
    return impl_ ? impl_->width : 0;
}

uint32_t PipeWireCapture::current_height() const noexcept {
    return impl_ ? impl_->height : 0;
}

bool PipeWireCapture::init() {
    impl_ = std::make_unique<Impl>();

    impl_->loop = pw_thread_loop_new("pipewire-capture-loop", nullptr);
    if (!impl_->loop) return false;

    impl_->context = pw_context_new(pw_thread_loop_get_loop(impl_->loop), nullptr, 0);
    if (!impl_->context) {
        pw_thread_loop_destroy(impl_->loop);
        impl_->loop = nullptr;
        return false;
    }

    return true;
}

bool PipeWireCapture::start(int stream_fd, FrameCallback callback) {
    if (!impl_ || !impl_->loop || impl_->is_running.load()) return false;

    impl_->frame_callback = std::move(callback);

    if (pw_thread_loop_start(impl_->loop) < 0) {
        return false;
    }

    pw_thread_loop_lock(impl_->loop);

    impl_->core = pw_context_connect_fd(impl_->context, stream_fd, nullptr, 0);
    if (!impl_->core) {
        pw_thread_loop_unlock(impl_->loop);
        return false;
    }

    pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Screen",
        nullptr
    );

    if (config_.target_node_id > 0) {
        pw_properties_setf(props, PW_KEY_TARGET_OBJECT, "%u", config_.target_node_id);
    }

    impl_->stream = pw_stream_new(impl_->core, "degoonification-screencast", props);
    if (!impl_->stream) {
        pw_thread_loop_unlock(impl_->loop);
        return false;
    }

    pw_stream_add_listener(impl_->stream, &impl_->stream_listener, &stream_events, impl_.get());

    // Build format negotiation params (BGRx / BGRA / RGBA)
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    struct spa_fraction fps_fraction = { config_.target_fps, 1 };

    const struct spa_pod* params[2];
    params[0] = static_cast<const struct spa_pod*>(
        spa_pod_builder_add_object(&b,
            SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
            SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
            SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
            SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(5,
                SPA_VIDEO_FORMAT_BGRx,
                SPA_VIDEO_FORMAT_BGRx,
                SPA_VIDEO_FORMAT_BGRA,
                SPA_VIDEO_FORMAT_RGBA,
                SPA_VIDEO_FORMAT_RGBx),
            SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&fps_fraction)
        )
    );

    int flags = PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS;
    if (pw_stream_connect(impl_->stream, PW_DIRECTION_INPUT, PW_ID_ANY,
                          static_cast<pw_stream_flags>(flags), params, 1) < 0) {
        pw_thread_loop_unlock(impl_->loop);
        return false;
    }

    pw_thread_loop_unlock(impl_->loop);
    impl_->is_running.store(true);
    return true;
}

void PipeWireCapture::stop() {
    if (!impl_ || !impl_->is_running.exchange(false)) return;

    if (impl_->loop) {
        pw_thread_loop_stop(impl_->loop);

        if (impl_->stream) {
            pw_stream_destroy(impl_->stream);
            impl_->stream = nullptr;
        }
        if (impl_->core) {
            pw_core_disconnect(impl_->core);
            impl_->core = nullptr;
        }
        if (impl_->context) {
            pw_context_destroy(impl_->context);
            impl_->context = nullptr;
        }
        pw_thread_loop_destroy(impl_->loop);
        impl_->loop = nullptr;
    }
}

} // namespace degoonification::linux_backend
