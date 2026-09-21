#!/usr/bin/env bash
# Degoonification Launcher for Arch Linux / Hyprland
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Degoonification Launcher ==="

# 1. Check if model exists
if [ ! -f "core/models/yolov8n-nsfw.onnx" ]; then
    echo "AI model not found. Downloading YOLOv8n-NSFW model..."
    python3 core/models/download_models.py --model nano
fi

# 2. Build if binary doesn't exist
if [ ! -f "platform/linux/bin/degoonification-daemon" ]; then
    echo "Building daemon and shared engine..."
    make all
fi

# 3. Launch daemon
echo "Launching daemon on Wayland/Hyprland..."
exec ./platform/linux/bin/degoonification-daemon "$@"
