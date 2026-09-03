"""Generate deterministic overview and social assets."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).parents[1]
ASSETS = ROOT / "docs" / "assets"

GLYPHS = {
    " ": ("00000",) * 7,
    "/": ("00001", "00010", "00100", "01000", "10000", "00000", "00000"),
    "-": ("00000", "00000", "00000", "11111", "00000", "00000", "00000"),
    "+": ("00000", "00100", "00100", "11111", "00100", "00100", "00000"),
    "·": ("00000", "00000", "00000", "00100", "00000", "00000", "00000"),
    "0": ("01110", "10001", "10011", "10101", "11001", "10001", "01110"),
    "1": ("00100", "01100", "00100", "00100", "00100", "00100", "01110"),
    "2": ("01110", "10001", "00001", "00010", "00100", "01000", "11111"),
    "7": ("11111", "00001", "00010", "00100", "01000", "01000", "01000"),
    "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
    "B": ("11110", "10001", "10001", "11110", "10001", "10001", "11110"),
    "C": ("01111", "10000", "10000", "10000", "10000", "10000", "01111"),
    "D": ("11110", "10001", "10001", "10001", "10001", "10001", "11110"),
    "E": ("11111", "10000", "10000", "11110", "10000", "10000", "11111"),
    "F": ("11111", "10000", "10000", "11110", "10000", "10000", "10000"),
    "G": ("01111", "10000", "10000", "10111", "10001", "10001", "01111"),
    "H": ("10001", "10001", "10001", "11111", "10001", "10001", "10001"),
    "I": ("11111", "00100", "00100", "00100", "00100", "00100", "11111"),
    "K": ("10001", "10010", "10100", "11000", "10100", "10010", "10001"),
    "L": ("10000", "10000", "10000", "10000", "10000", "10000", "11111"),
    "M": ("10001", "11011", "10101", "10101", "10001", "10001", "10001"),
    "N": ("10001", "11001", "10101", "10011", "10001", "10001", "10001"),
    "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
    "P": ("11110", "10001", "10001", "11110", "10000", "10000", "10000"),
    "R": ("11110", "10001", "10001", "11110", "10100", "10010", "10001"),
    "S": ("01111", "10000", "10000", "01110", "00001", "00001", "11110"),
    "T": ("11111", "00100", "00100", "00100", "00100", "00100", "00100"),
    "U": ("10001", "10001", "10001", "10001", "10001", "10001", "01110"),
    "Y": ("10001", "10001", "01010", "00100", "00100", "00100", "00100"),
}


def overview_svg() -> str:
    return """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 500" width="1200" height="500" role="img">
  <title>TCP/IP Course layer progression</title>
  <desc>Twelve portable C17 core lessons progress from bytes to diagnostics, followed by four optional Linux implementation labs.</desc>
  <rect width="1200" height="500" rx="24" fill="#07111f"/>
  <text x="600" y="65" text-anchor="middle" font-family="system-ui,sans-serif" font-size="40" font-weight="700" fill="#f8fafc">TCP/IP from bytes to diagnostics</text>
  <text x="600" y="102" text-anchor="middle" font-family="system-ui,sans-serif" font-size="20" fill="#a7bdd4">12 portable core lessons · 4 optional Linux labs · C17 · CMake + CTest</text>
  <g font-family="system-ui,sans-serif">
    <rect x="45" y="145" width="200" height="170" rx="18" fill="#0c2a3f" stroke="#38bdf8" stroke-width="2"/><text x="145" y="185" text-anchor="middle" font-size="14" font-weight="700" fill="#7dd3fc">LESSONS 1–2</text><text x="145" y="228" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Foundations</text><text x="145" y="264" text-anchor="middle" font-size="15" fill="#a7bdd4">Bytes · checksum</text><text x="145" y="289" text-anchor="middle" font-size="15" fill="#a7bdd4">Ethernet · ARP</text>
    <rect x="285" y="145" width="200" height="170" rx="18" fill="#14284a" stroke="#818cf8" stroke-width="2"/><text x="385" y="185" text-anchor="middle" font-size="14" font-weight="700" fill="#a5b4fc">LESSONS 3–4</text><text x="385" y="228" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Internet</text><text x="385" y="264" text-anchor="middle" font-size="15" fill="#a7bdd4">IPv4 packets</text><text x="385" y="289" text-anchor="middle" font-size="15" fill="#a7bdd4">ICMP messages</text>
    <rect x="525" y="145" width="200" height="170" rx="18" fill="#12332f" stroke="#34d399" stroke-width="2"/><text x="625" y="185" text-anchor="middle" font-size="14" font-weight="700" fill="#6ee7b7">LESSONS 5–7</text><text x="625" y="228" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Transport</text><text x="625" y="264" text-anchor="middle" font-size="15" fill="#a7bdd4">UDP · TCP segments</text><text x="625" y="289" text-anchor="middle" font-size="15" fill="#a7bdd4">State · reliability</text>
    <rect x="765" y="145" width="200" height="170" rx="18" fill="#422b17" stroke="#f59e0b" stroke-width="2"/><text x="865" y="185" text-anchor="middle" font-size="14" font-weight="700" fill="#fbbf24">LESSONS 8–10</text><text x="865" y="228" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Applications</text><text x="865" y="264" text-anchor="middle" font-size="15" fill="#a7bdd4">Sockets · DNS</text><text x="865" y="289" text-anchor="middle" font-size="15" fill="#a7bdd4">HTTP framing</text>
    <rect x="1005" y="145" width="150" height="170" rx="18" fill="#3e2135" stroke="#f472b6" stroke-width="2"/><text x="1080" y="185" text-anchor="middle" font-size="14" font-weight="700" fill="#f9a8d4">11–12</text><text x="1080" y="225" text-anchor="middle" font-size="20" font-weight="700" fill="#f8fafc">Practice</text><text x="1080" y="261" text-anchor="middle" font-size="14" fill="#a7bdd4">Routing · NAT</text><text x="1080" y="286" text-anchor="middle" font-size="14" fill="#a7bdd4">Diagnostics</text>
  </g>
  <rect x="275" y="350" width="650" height="46" rx="23" fill="#10263b" stroke="#38bdf8"/><text x="600" y="379" text-anchor="middle" font-family="system-ui,sans-serif" font-size="17" font-weight="700" fill="#dbeafe">OPTIONAL LINUX LABS 1–4 · INTERFACES · TCP · SOCKETS · ROUTING</text>
  <text x="600" y="450" text-anchor="middle" font-family="system-ui,sans-serif" font-size="17" font-weight="600" fill="#dbeafe">Offline buffers first · loopback only · unprivileged · no copied kernel code</text>
</svg>
"""


def draw_rect(
    pixels: bytearray, width: int, x0: int, y0: int, x1: int, y1: int, color: tuple[int, int, int]
) -> None:
    for y in range(max(0, y0), min(630, y1)):
        for x in range(max(0, x0), min(width, x1)):
            offset = (y * width + x) * 3
            pixels[offset : offset + 3] = bytes(color)


def draw_text(
    pixels: bytearray,
    width: int,
    x: int,
    y: int,
    text: str,
    scale: int,
    color: tuple[int, int, int],
) -> None:
    cursor = x
    for character in text.upper():
        glyph = GLYPHS.get(character, GLYPHS[" "])
        for row, bits in enumerate(glyph):
            for column, bit in enumerate(bits):
                if bit == "1":
                    draw_rect(
                        pixels,
                        width,
                        cursor + column * scale,
                        y + row * scale,
                        cursor + (column + 1) * scale,
                        y + (row + 1) * scale,
                        color,
                    )
        cursor += 6 * scale


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def social_png() -> bytes:
    width, height = 1200, 630
    pixels = bytearray((7, 17, 31) * (width * height))
    colors = ((12, 42, 63), (20, 40, 74), (18, 51, 47), (66, 43, 23), (62, 33, 53))
    accents = ((56, 189, 248), (129, 140, 248), (52, 211, 153), (245, 158, 11), (244, 114, 182))
    draw_text(pixels, width, 68, 60, "TCP/IP COURSE", 12, (248, 250, 252))
    draw_text(pixels, width, 72, 170, "C17 FROM BYTES TO DIAGNOSTICS", 5, (167, 189, 212))
    for index, (fill, accent) in enumerate(zip(colors, accents, strict=True)):
        left = 68 + index * 215
        draw_rect(pixels, width, left, 285, left + 180, 455, fill)
        draw_rect(pixels, width, left, 285, left + 180, 291, accent)
    labels = ("BYTES", "IP", "TCP", "HTTP", "TOOLS")
    for index, label in enumerate(labels):
        draw_text(pixels, width, 91 + index * 215, 350, label, 4, (248, 250, 252))
    draw_text(
        pixels,
        width,
        70,
        535,
        "12 CORE + 4 LINUX LABS · CMAKE + CTEST · LOOPBACK ONLY",
        4,
        (219, 234, 254),
    )
    raw = b"".join(b"\x00" + pixels[y * width * 3 : (y + 1) * width * 3] for y in range(height))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(raw, 9))
        + png_chunk(b"IEND", b"")
    )


def expected_assets() -> dict[Path, bytes]:
    return {
        ASSETS / "tcp-ip-overview.svg": overview_svg().encode(),
        ASSETS / "tcp-ip-social.png": social_png(),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    failures = []
    for path, expected in expected_assets().items():
        if args.check:
            if not path.exists() or path.read_bytes() != expected:
                failures.append(path.relative_to(ROOT))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(expected)
            print(f"wrote {path.relative_to(ROOT)}")
    if failures:
        print("out-of-date assets:", ", ".join(map(str, failures)))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
