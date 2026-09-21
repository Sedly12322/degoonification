#include "wayland_overlay.hpp"
#include "protocols/wlr-layer-shell-unstable-v1-client-protocol.h"
#include <wayland-client.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace degoonification::linux_backend {

struct ShmBuffer {
    wl_buffer* wl_buf{nullptr};
    uint32_t* data{nullptr};
    size_t size{0};
    bool busy{false};
};

struct WaylandOverlay::Impl {
    wl_display* display{nullptr};
    wl_registry* registry{nullptr};
    wl_compositor* compositor{nullptr};
    wl_shm* shm{nullptr};
    zwlr_layer_shell_v1* layer_shell{nullptr};

    wl_surface* surface{nullptr};
    zwlr_layer_surface_v1* layer_surface{nullptr};

    uint32_t width{1920};
    uint32_t height{1080};
    bool configured{false};

    ShmBuffer buffers[2];
    int current_buffer{0};
    bool had_boxes{false};
    std::vector<core::BoundingBox> last_rendered_boxes;
};

static void buffer_release(void* data, wl_buffer* wl_buffer) {
    (void)wl_buffer;
    auto* buf = static_cast<ShmBuffer*>(data);
    buf->busy = false;
}

static const wl_buffer_listener buffer_listener = {
    .release = buffer_release,
};

static int create_anonymous_shm_file(size_t size) {
    int fd = memfd_create("degoonification-overlay-shm", MFD_CLOEXEC);
    if (fd < 0) return -1;
    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static bool allocate_shm_buffer(wl_shm* shm, ShmBuffer* buf, uint32_t width, uint32_t height) {
    size_t stride = width * 4;
    size_t size = stride * height;

    int fd = create_anonymous_shm_file(size);
    if (fd < 0) return false;

    void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return false;
    }

    wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size);
    buf->wl_buf = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    buf->data = static_cast<uint32_t*>(data);
    buf->size = size;
    buf->busy = false;

    wl_buffer_add_listener(buf->wl_buf, &buffer_listener, buf);
    return true;
}

static void layer_surface_configure(void* data, zwlr_layer_surface_v1* surface, uint32_t serial, uint32_t width, uint32_t height) {
    auto* impl = static_cast<WaylandOverlay::Impl*>(data);
    zwlr_layer_surface_v1_ack_configure(surface, serial);

    if (width > 0 && height > 0) {
        impl->width = width;
        impl->height = height;
    }
    impl->configured = true;
}

static void layer_surface_closed(void* data, zwlr_layer_surface_v1* surface) {
    (void)surface;
    auto* impl = static_cast<WaylandOverlay::Impl*>(data);
    impl->configured = false;
}

static const zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

static void registry_global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    (void)version;
    auto* impl = static_cast<WaylandOverlay::Impl*>(data);

    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        impl->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
    } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        impl->shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        impl->layer_shell = static_cast<zwlr_layer_shell_v1*>(wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1));
    }
}

static void registry_global_remove(void* data, wl_registry* registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

WaylandOverlay::WaylandOverlay(OverlayConfig config)
    : config_(config) {}

WaylandOverlay::~WaylandOverlay() {
    if (impl_) {
        for (int i = 0; i < 2; ++i) {
            if (impl_->buffers[i].wl_buf) {
                wl_buffer_destroy(impl_->buffers[i].wl_buf);
            }
            if (impl_->buffers[i].data) {
                munmap(impl_->buffers[i].data, impl_->buffers[i].size);
            }
        }
        if (impl_->layer_surface) zwlr_layer_surface_v1_destroy(impl_->layer_surface);
        if (impl_->surface) wl_surface_destroy(impl_->surface);
        if (impl_->layer_shell) zwlr_layer_shell_v1_destroy(impl_->layer_shell);
        if (impl_->shm) wl_shm_destroy(impl_->shm);
        if (impl_->compositor) wl_compositor_destroy(impl_->compositor);
        if (impl_->registry) wl_registry_destroy(impl_->registry);
        if (impl_->display) wl_display_disconnect(impl_->display);
    }
}

bool WaylandOverlay::init() {
    impl_ = std::make_unique<Impl>();

    impl_->display = wl_display_connect(nullptr);
    if (!impl_->display) {
        std::cerr << "[WaylandOverlay] Failed to connect to Wayland display\n";
        return false;
    }

    impl_->registry = wl_display_get_registry(impl_->display);
    wl_registry_add_listener(impl_->registry, &registry_listener, impl_.get());
    wl_display_roundtrip(impl_->display);

    if (!impl_->compositor || !impl_->shm || !impl_->layer_shell) {
        std::cerr << "[WaylandOverlay] Missing required Wayland protocols (compositor, shm, or layer-shell)\n";
        return false;
    }

    // Create surface
    impl_->surface = wl_compositor_create_surface(impl_->compositor);
    if (!impl_->surface) return false;

    // Create Layer Surface on OVERLAY layer
    impl_->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        impl_->layer_shell,
        impl_->surface,
        nullptr, // all outputs / active output
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        "degoonification-blur-overlay"
    );

    if (!impl_->layer_surface) return false;

    // Anchor to all screen edges (fullscreen)
    uint32_t anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                       ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                       ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                       ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    zwlr_layer_surface_v1_set_anchor(impl_->layer_surface, anchors);
    zwlr_layer_surface_v1_set_exclusive_zone(impl_->layer_surface, -1);
    zwlr_layer_surface_v1_set_size(impl_->layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_keyboard_interactivity(impl_->layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);

    // Enforce click-through via empty input region
    if (config_.enable_clickthrough) {
        wl_region* empty_region = wl_compositor_create_region(impl_->compositor);
        wl_surface_set_input_region(impl_->surface, empty_region);
        wl_region_destroy(empty_region);
    }

    zwlr_layer_surface_v1_add_listener(impl_->layer_surface, &layer_surface_listener, impl_.get());
    wl_surface_commit(impl_->surface);
    wl_display_roundtrip(impl_->display);

    // Allocate double buffers
    allocate_shm_buffer(impl_->shm, &impl_->buffers[0], impl_->width, impl_->height);
    allocate_shm_buffer(impl_->shm, &impl_->buffers[1], impl_->width, impl_->height);

    return true;
}

static bool boxes_differ(const std::vector<core::BoundingBox>& a, const std::vector<core::BoundingBox>& b) {
    if (a.size() != b.size()) return true;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::abs(a[i].x - b[i].x) > 0.003f ||
            std::abs(a[i].y - b[i].y) > 0.003f ||
            std::abs(a[i].width - b[i].width) > 0.003f ||
            std::abs(a[i].height - b[i].height) > 0.003f) {
            return true;
        }
    }
    return false;
}

void WaylandOverlay::render_boxes(const std::vector<core::BoundingBox>& boxes, const VideoFrame* source_frame) {
    if (!impl_ || !impl_->surface || !impl_->configured) return;

    if (boxes.empty() && impl_->last_rendered_boxes.empty()) {
        return; // Already cleared, avoid redundant composite cycles
    }

    if (!boxes_differ(boxes, impl_->last_rendered_boxes) && !source_frame) {
        return; // Boxes unchanged, retain current screen buffer to completely eliminate flicker!
    }

    impl_->current_buffer = 1 - impl_->current_buffer;
    ShmBuffer& buf = impl_->buffers[impl_->current_buffer];

    // Clear buffer to 100% transparent (0x00000000)
    std::memset(buf.data, 0, buf.size);

    if (!boxes.empty()) {
        const uint32_t w = impl_->width;
        const uint32_t h = impl_->height;
        const int block_size = std::max(8, config_.mosaic_block_size);

        for (const auto& box : boxes) {
            uint32_t bx1 = std::min(static_cast<uint32_t>(box.left() * w), w);
            uint32_t by1 = std::min(static_cast<uint32_t>(box.top() * h), h);
            uint32_t bx2 = std::min(static_cast<uint32_t>(box.right() * w), w);
            uint32_t by2 = std::min(static_cast<uint32_t>(box.bottom() * h), h);

            if (config_.enable_frosted_mosaic && source_frame && source_frame->data &&
                source_frame->width > 0 && source_frame->height > 0) {
                const bool is_rgb24 = (source_frame->format == PixelFormat::RGB);
                const int bpp = is_rgb24 ? 3 : 4;
                const uint32_t src_w = source_frame->width;
                const uint32_t src_h = source_frame->height;
                const uint32_t src_stride = source_frame->stride > 0 ? source_frame->stride : (src_w * bpp);

                for (uint32_t y = by1; y < by2; y += block_size) {
                    uint32_t cur_bh = std::min(static_cast<uint32_t>(block_size), by2 - y);
                    for (uint32_t x = bx1; x < bx2; x += block_size) {
                        uint32_t cur_bw = std::min(static_cast<uint32_t>(block_size), bx2 - x);

                        // Sample center of block from source frame
                        uint32_t sample_x = std::min(x + cur_bw / 2, src_w - 1);
                        uint32_t sample_y = std::min(y + cur_bh / 2, src_h - 1);
                        const uint8_t* px = source_frame->data + (sample_y * src_stride) + (sample_x * bpp);

                        uint32_t r = is_rgb24 ? px[0] : px[2];
                        uint32_t g = px[1];
                        uint32_t b = is_rgb24 ? px[2] : px[0];

                        // Blend 75% natural scene color with 25% frosted dark tint
                        uint32_t fr = (r * 3 + 15) / 4;
                        uint32_t fg = (g * 3 + 23) / 4;
                        uint32_t fb = (b * 3 + 42) / 4;
                        uint32_t pixel_color = 0xFF000000 | (fr << 16) | (fg << 8) | fb;

                        for (uint32_t by = 0; by < cur_bh; ++by) {
                            uint32_t* row = buf.data + ((y + by) * w);
                            for (uint32_t bx = 0; bx < cur_bw; ++bx) {
                                row[x + bx] = pixel_color;
                            }
                        }
                    }
                }
            } else {
                const uint32_t color = config_.frosted_color_argb;
                for (uint32_t y = by1; y < by2; ++y) {
                    uint32_t* row = buf.data + (y * w);
                    for (uint32_t x = bx1; x < bx2; ++x) {
                        row[x] = color;
                    }
                }
            }
        }
    }

    impl_->last_rendered_boxes = boxes;

    wl_surface_attach(impl_->surface, buf.wl_buf, 0, 0);
    wl_surface_damage_buffer(impl_->surface, 0, 0, impl_->width, impl_->height);
    wl_surface_commit(impl_->surface);
    wl_display_flush(impl_->display);
}

void WaylandOverlay::dispatch_events() {
    if (impl_ && impl_->display) {
        wl_display_dispatch_pending(impl_->display);
    }
}

bool WaylandOverlay::is_ready() const noexcept {
    return impl_ && impl_->configured;
}

uint32_t WaylandOverlay::screen_width() const noexcept {
    return impl_ ? impl_->width : 0;
}

uint32_t WaylandOverlay::screen_height() const noexcept {
    return impl_ ? impl_->height : 0;
}

} // namespace degoonification::linux_backend
