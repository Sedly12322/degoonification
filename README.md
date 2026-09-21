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

## 💻 Degoonification CLI & TUI Interface (`degoon`)

The native command-line suite communicates with the running daemon over an ultra-low latency UNIX Domain Socket (`/run/user/<UID>/degoon.sock`).

### 🎮 Interactive Fullscreen TUI Dashboard
Launch the live terminal monitoring dashboard:
```bash
degoon tui
# or simply:
degoon
```
**Interactive Hotkeys:**
- `[p]` Pause / Resume visual screen blur
- `[+]` / `[-]` Increase / decrease dynamic box padding (+10% to +35%)
- `[r]` Log a relapse & reset streak
- `[q]` Exit TUI dashboard (daemon continues running silently in the background)

---

### ⌨️ CLI Subcommands

| Command | Description |
| :--- | :--- |
| `degoon status` | Displays formatted status banner, active blur boxes, FPS, and streak |
| `degoon tui` | Opens the live interactive TUI dashboard |
| `degoon start` | Launches the daemon in background |
| `degoon stop` | Gracefully stops the running daemon |
| `degoon restart` | Restarts the background daemon |
| `degoon pause` | Temporarily pauses screen blur |
| `degoon resume` | Resumes screen blur |
| `degoon streak` | Shows days clean, recovery milestones, and dopamine restoration stage |
| `degoon relapse [reason]` | Records relapse trigger notes and resets current streak |
| `degoon padding <ratio>` | Sets dynamic bounding box expansion ratio (e.g. `0.20` for +20%) |

---

## 🚀 Running on Linux (Arch / Hyprland / Wayland)

### Quick Start
To build and launch the daemon in one command:
```bash
cd ~/degoonification
./run.sh
```
Or install system-wide / user-wide:
```bash
make install
```
*(Installs `degoon` and `degoonification-daemon` to `~/.local/bin/`)*

### Running System Tests
```bash
make test
```

### Optional: Enable System-Wide DNS Sinkhole
To redirect all outbound port 53 traffic to the local in-memory sinkhole (`127.0.0.1:5353`):
```bash
sudo ./platform/linux/network/redirect_dns.sh enable
```
To disable:
```bash
sudo ./platform/linux/network/redirect_dns.sh disable
```
