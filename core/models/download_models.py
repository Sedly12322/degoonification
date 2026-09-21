#!/usr/bin/env python3
"""
Downloads open-weights YOLO NSFW detection models:
- 320n.onnx (Nano model: Ultra-low latency < 15ms)
- 640m.onnx (Medium model: High accuracy 640x640)
From official notAI-tech / NudeNet releases.
"""

import sys
import os
import urllib.request
import argparse
from pathlib import Path

MODELS = {
    "nano": {
        "asset_id": 176831997,
        "url": "https://api.github.com/repos/notAI-tech/NudeNet/releases/assets/176831997",
        "filename": "yolov8n-nsfw.onnx",
        "description": "YOLOv8-Nano NSFW Detector (Ultra-fast, recommended for real-time)"
    },
    "medium": {
        "asset_id": 176832019,
        "url": "https://api.github.com/repos/notAI-tech/NudeNet/releases/assets/176832019",
        "filename": "yolov8m-nsfw.onnx",
        "description": "YOLOv8-Medium NSFW Detector (Higher precision, 640x640)"
    }
}

def download_file(url: str, dest_path: Path):
    print(f"Downloading from: {url}")
    print(f"Destination: {dest_path}")

    token = os.environ.get("GITHUB_TOKEN")
    headers = {
        "User-Agent": "Mozilla/5.0",
        "Accept": "application/octet-stream"
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = urllib.request.Request(url, headers=headers)
    tmp_path = dest_path.with_suffix(".tmp")

    with urllib.request.urlopen(req) as resp, open(tmp_path, "wb") as out_file:
        total_size = int(resp.headers.get("Content-Length", 0))
        downloaded = 0
        block_size = 64 * 1024

        while True:
            chunk = resp.read(block_size)
            if not chunk:
                break
            out_file.write(chunk)
            downloaded += len(chunk)
            if total_size > 0:
                percent = int(downloaded * 100 / total_size)
                mb_down = downloaded / (1024 * 1024)
                mb_total = total_size / (1024 * 1024)
                sys.stdout.write(f"\rProgress: {percent}% ({mb_down:.1f} / {mb_total:.1f} MB)")
                sys.stdout.flush()

    tmp_path.rename(dest_path)
    print("\n✓ Download completed successfully.")

def main():
    parser = argparse.ArgumentParser(description="Download Degoonification AI models")
    parser.add_argument("--model", choices=["nano", "medium", "all"], default="nano",
                        help="Which model variant to download (default: nano)")
    args = parser.parse_args()

    models_dir = Path(__file__).parent.resolve()

    selected = ["nano", "medium"] if args.model == "all" else [args.model]
    for key in selected:
        info = MODELS[key]
        dest = models_dir / info["filename"]
        print(f"\n=== Downloading {info['description']} ===")
        if dest.exists():
            print(f"File {dest.name} already exists. Skipping.")
            continue
        try:
            download_file(info["url"], dest)
        except Exception as e:
            print(f"Error downloading {key}: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()
