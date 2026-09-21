# 🛡️ Degoonification

**A hyper-performant, cross-platform, privacy-first real-time content moderation & self-control system suite.**

Designed to eliminate exposure to adult and explicit content in real-time through on-device visual interception, zero-latency hardware-accelerated blur, kernel-level DNS sinkholing, and hardened anti-tamper safeguards.

---

## 🔒 Privacy & Architecture Principles

- **100% On-Device Inference:** All frame evaluation and object detection happens locally on your GPU/NPU.
- **Air-Gapped Visual Buffers:** Zero screen captures, video buffers, or telemetry ever touch the network or leave the machine.
- **Zero-Copy Pipeline:** Direct VRAM frame sharing from screen compositor to AI inference and overlay rendering.
- **Sub-millisecond DNS Sinkholing:** In-memory Bloom filters and Radix trees intercepting adult domains directly at port 53 / kernel level.

---

## 📁 Monorepo Layout

```text
degoonification/
├── apps/
│   └── mobile_desktop_client/      # Flutter (Dart) UI: Configuration, statistics & streak tracker
├── core/
│   ├── engine_shared/              # Shared C++20 engine: Bounding-box temporal smoothing, Radix Tree, Bloom filter
│   ├── models/                     # YOLOv8n-NSFW ONNX (FP16/INT8) and TFLite models
│   └── blocklists/                 # Curated adult domain blocklists & bloom filter generators
└── platform/
    ├── windows/                    # Windows.Graphics.Capture, DirectML, Direct2D click-through overlay, WFP/WinDivert
    ├── android/                    # MediaProjection, AHardwareBuffer, TFLite NNAPI, TYPE_APPLICATION_OVERLAY, VpnService
    └── linux/                      # PipeWire DMA-BUF (Wayland) / XShm (X11), ONNX Vulkan/TensorRT, wlr-layer-shell, eBPF
```

---

## 🚦 Roadmap & Implementation Phases

| Phase | Windows | Android | Arch Linux | Target Metric |
| :--- | :--- | :--- | :--- | :--- |
| **Phase 1** | D3D11 Capture | MediaProjection HardwareBuffer | PipeWire DMA-BUF Stream | Capture overhead < 5 ms |
| **Phase 2** | DirectML YOLOv8 | TFLite NNAPI YOLOv8 | ONNX Runtime (Vulkan/TensorRT) | Desktop latency < 30 ms |
| **Phase 3** | Direct2D Layered Window | RenderEffect Overlay | Wayland `wlr-layer-shell` / XFixes | Zero flicker, click-through |
| **Phase 4** | WFP DNS Sinkhole | VpnService DNS Filter | eBPF / XDP Packet Filter | DNS response < 1 ms |
| **Phase 5** | NT SYSTEM Watchdog | DeviceAdmin + Accessibility | systemd hardened daemon + `chattr` | Resistant to task killing |

---

## 🛠️ Building Core Engine

```bash
cd core/engine_shared
make test
```
