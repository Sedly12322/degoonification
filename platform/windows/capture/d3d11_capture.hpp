#pragma once

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/Windows.Graphics.Capture.h>
#endif

#include <functional>
#include <cstdint>

namespace degoonification::windows_backend {

struct CapturedFrameD3D11 {
    uint32_t width{0};
    uint32_t height{0};
#ifdef _WIN32
    ID3D11Texture2D* texture{nullptr};
#endif
    uint64_t timestamp_us{0};
};

using WindowsFrameCallback = std::function<void(const CapturedFrameD3D11&)>;

/**
 * Windows.Graphics.Capture Direct3D 11 zero-copy screen capturer.
 */
class D3D11Capture {
public:
    D3D11Capture() = default;
    ~D3D11Capture();

    bool init();
    bool start(WindowsFrameCallback callback);
    void stop();

    [[nodiscard]] bool is_capturing() const noexcept { return is_capturing_; }

private:
    bool is_capturing_{false};
#ifdef _WIN32
    ID3D11Device* d3d_device_{nullptr};
    ID3D11DeviceContext* d3d_context_{nullptr};
#endif
    WindowsFrameCallback callback_;
};

} // namespace degoonification::windows_backend
