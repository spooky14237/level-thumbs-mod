#!/usr/bin/env python3
import base64
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
FILES = [
    ("scripts/AuthManager.cpp.b64", "src/managers/AuthManager.cpp"),
    ("scripts/ThumbnailManager.cpp.b64", "src/managers/ThumbnailManager.cpp"),
    ("scripts/PauseLayer.cpp.b64", "src/hooks/PauseLayer.cpp"),
    ("scripts/ThumbnailPopup.cpp.b64", "src/layers/ThumbnailPopup.cpp"),
]
for src, dst in FILES:
    raw = (ROOT / src).read_text()
    b64 = "".join(raw.split())
    data = base64.b64decode(b64)
    path = ROOT / dst
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    print(f"Wrote {dst} ({len(data)} bytes)")
