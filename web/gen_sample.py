#!/usr/bin/env python3
"""Generate a small self-contained textured cube GLB (web/Cube.glb).

Uses the tracked Cube assets from models/ (Cube.gltf geometry in Cube.bin)
plus a tiny checkerboard PNG generated with only the standard library, so
the demo ships without large binary assets.
"""

import base64
import json
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "models" / "Cube" / "Cube.bin"
OUT = Path(__file__).resolve().parent / "Cube.glb"


def make_png(width=64, height=64, rgb=(230, 140, 60), rgb2=(245, 245, 245)):
    """Minimal zlib-based RGB PNG writer."""
    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)  # 8-bit RGB
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter: none
        for x in range(width):
            c = rgb if (x // 8 + y // 8) % 2 == 0 else rgb2
            raw += bytes(c)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def main():
    bin_data = BIN.read_bytes()

    png = make_png()
    total = len(bin_data) + len(png)

    j = {
        "asset": {"version": "2.0", "generator": "tinygltf web demo (gen_sample.py)"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "Cube"}],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 1, "NORMAL": 2, "TANGENT": 3, "TEXCOORD_0": 4,
                },
                "indices": 0,
                "material": 0,
            }]
        }],
        "materials": [{
            "name": "checker",
            "pbrMetallicRoughness": {
                "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
                "baseColorTexture": {"index": 0},
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0,
            },
        }],
        "textures": [{"source": 0, "sampler": 0}],
        "images": [{"bufferView": 5, "mimeType": "image/png"}],
        "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
        "buffers": [{"byteLength": total}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 72, "target": 34963},   # indices
            {"buffer": 0, "byteOffset": 72, "byteLength": 432, "target": 34962},  # positions
            {"buffer": 0, "byteOffset": 504, "byteLength": 432, "target": 34962},  # normals
            {"buffer": 0, "byteOffset": 936, "byteLength": 576, "target": 34962},  # tangents
            {"buffer": 0, "byteOffset": 1512, "byteLength": 288, "target": 34962},  # uvs
            {"buffer": 0, "byteOffset": len(bin_data), "byteLength": len(png)},    # image
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5123, "count": 36, "type": "SCALAR", "min": [0], "max": [35]},
            {"bufferView": 1, "componentType": 5126, "count": 36, "type": "VEC3", "min": [-1, -1, -1], "max": [1, 1, 1]},
            {"bufferView": 2, "componentType": 5126, "count": 36, "type": "VEC3"},
            {"bufferView": 3, "componentType": 5126, "count": 36, "type": "VEC4"},
            {"bufferView": 4, "componentType": 5126, "count": 36, "type": "VEC2"},
        ],
    }

    json_chunk = json.dumps(j, separators=(",", ":")).encode("utf-8")
    json_chunk += b" " * ((4 - len(json_chunk) % 4) % 4)
    bin_chunk = bin_data + png
    bin_chunk += b"\x00" * ((4 - len(bin_chunk) % 4) % 4)

    total_len = 12 + 8 + len(json_chunk) + 8 + len(bin_chunk)
    glb = struct.pack("<4sII", b"glTF", 2, total_len)
    glb += struct.pack("<II", len(json_chunk), 0x4E4F534A) + json_chunk
    glb += struct.pack("<II", len(bin_chunk), 0x004E4942) + bin_chunk

    OUT.write_bytes(glb)
    print(f"wrote {OUT} ({len(glb)} bytes, image {len(png)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
