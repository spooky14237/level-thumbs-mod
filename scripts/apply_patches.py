#!/usr/bin/env python3
import base64, pathlib
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parent.parent
chunks = defaultdict(list)
for p in sorted((ROOT / "scripts/chunks").glob("*.b64")):
    parts = p.name.split(".")
    idx = int(parts[-2])
    base = ".".join(parts[:-2])
    chunks[base].append((idx, p))

MAPPING = {
    "AuthManager.cpp": "src/managers/AuthManager.cpp",
    "ThumbnailManager.cpp": "src/managers/ThumbnailManager.cpp",
    "PauseLayer.cpp": "src/hooks/PauseLayer.cpp",
    "ThumbnailPopup.cpp": "src/layers/ThumbnailPopup.cpp",
}

for base, items in chunks.items():
    items.sort()
    b64 = "".join(p.read_text().strip() for _, p in items)
    data = base64.b64decode(b64)
    dst = ROOT / MAPPING[base]
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    print(f"Wrote {dst.relative_to(ROOT)} ({len(data)} bytes)")
