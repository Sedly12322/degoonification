#include "window_overlay.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace degoonification::windows_backend {

#ifdef _WIN32
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
#endif

WindowsOverlay::WindowsOverlay(WindowsOverlayConfig config)
    : config_(config) {}

WindowsOverlay::~WindowsOverlay() {
#ifdef _WIN32
    if (mem_bitmap_) DeleteObject(mem_bitmap_);
    if (mem_dc_) DeleteDC(mem_dc_);
    if (hwnd_) DestroyWindow(hwnd_);
#endif
}

bool WindowsOverlay::init() {
#ifdef _WIN32
    screen_width_ = GetSystemMetrics(SM_CXSCREEN);
    screen_height_ = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"DegoonOverlayClass";
    RegisterClassExW(&wc);

    DWORD ex_style = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    if (config_.clickthrough) {
        ex_style |= WS_EX_TRANSPARENT; // Click-through
    }

    hwnd_ = CreateWindowExW(
        ex_style,
        L"DegoonOverlayClass",
        L"Degoonification Overlay",
        WS_POPUP | WS_VISIBLE,
        0, 0, screen_width_, screen_height_,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr
    );

    if (!hwnd_) return false;

    // Create 32-bit DIB section for per-pixel alpha blending
    HDC screen_dc = GetDC(nullptr);
    mem_dc_ = CreateCompatibleDC(screen_dc);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = screen_width_;
    bmi.bmiHeader.biHeight = -screen_height_; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    mem_bitmap_ = CreateDIBSection(mem_dc_, &bmi, DIB_RGB_COLORS, &bitmap_pixels_, nullptr, 0);
    SelectObject(mem_dc_, mem_bitmap_);
    ReleaseDC(nullptr, screen_dc);

    is_active_ = true;
    return true;
#else
    return false;
#endif
}

void WindowsOverlay::render_boxes(const std::vector<core::BoundingBox>& boxes) {
#ifdef _WIN32
    if (!is_active_ || !bitmap_pixels_) return;

    size_t total_pixels = static_cast<size_t>(screen_width_) * screen_height_;
    auto* pixels = static_cast<uint32_t*>(bitmap_pixels_);

    // Clear to 100% transparent
    std::memset(pixels, 0, total_pixels * sizeof(uint32_t));

    if (!boxes.empty()) {
        const uint32_t color = config_.frosted_color_argb;
        for (const auto& box : boxes) {
            int x1 = std::clamp(static_cast<int>(box.left() * screen_width_), 0, screen_width_);
            int y1 = std::clamp(static_cast<int>(box.top() * screen_height_), 0, screen_height_);
            int x2 = std::clamp(static_cast<int>(box.right() * screen_width_), 0, screen_width_);
            int y2 = std::clamp(static_cast<int>(box.bottom() * screen_height_), 0, screen_height_);

            for (int y = y1; y < y2; ++y) {
                uint32_t* row = pixels + (y * screen_width_);
                for (int x = x1; x < x2; ++x) {
                    row[x] = color;
                }
            }
        }
    }

    // UpdateLayeredWindow with per-pixel alpha
    POINT pt_src{0, 0};
    SIZE size_wnd{screen_width_, screen_height_};
    POINT pt_dst{0, 0};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    HDC screen_dc = GetDC(nullptr);
    UpdateLayeredWindow(hwnd_, screen_dc, &pt_dst, &size_wnd, mem_dc_, &pt_src, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screen_dc);
#else
    (void)boxes;
#endif
}

void WindowsOverlay::message_pump() {
#ifdef _WIN32
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
#endif
}

} // namespace degoonification::windows_backend
