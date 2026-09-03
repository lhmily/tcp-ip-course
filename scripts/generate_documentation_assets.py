"""Generate deterministic overview and social SVG assets."""

from __future__ import annotations

import argparse
from pathlib import Path

ROOT = Path(__file__).parents[1]
ASSETS = ROOT / "docs" / "assets"


def overview_svg(*, social: bool = False) -> str:
    width, height = (1200, 630) if social else (1200, 500)
    subtitle_y = 118 if social else 102
    box_y = 190 if social else 145
    footer_y = 535 if social else 430
    return f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" role="img">
  <title>TCP/IP Course layer progression</title>
  <desc>A twelve-lesson C17 course progresses from bytes and local links through Internet and transport protocols to applications and diagnostics.</desc>
  <rect width="{width}" height="{height}" rx="24" fill="#07111f"/>
  <text x="600" y="65" text-anchor="middle" font-family="system-ui,sans-serif" font-size="40" font-weight="700" fill="#f8fafc">TCP/IP from bytes to diagnostics</text>
  <text x="600" y="{subtitle_y}" text-anchor="middle" font-family="system-ui,sans-serif" font-size="20" fill="#a7bdd4">12 tested lessons · portable C17 · CMake + CTest · POSIX sockets</text>
  <g font-family="system-ui,sans-serif">
    <rect x="45" y="{box_y}" width="200" height="170" rx="18" fill="#0c2a3f" stroke="#38bdf8" stroke-width="2"/>
    <text x="145" y="{box_y + 40}" text-anchor="middle" font-size="14" font-weight="700" fill="#7dd3fc">LESSONS 1–2</text><text x="145" y="{box_y + 83}" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Foundations</text><text x="145" y="{box_y + 119}" text-anchor="middle" font-size="15" fill="#a7bdd4">Bytes · checksum</text><text x="145" y="{box_y + 144}" text-anchor="middle" font-size="15" fill="#a7bdd4">Ethernet · ARP</text>
    <path d="M255 {box_y + 85} H275" stroke="#64748b" stroke-width="4"/><path d="M268 {box_y + 76} L279 {box_y + 85} L268 {box_y + 94}" fill="none" stroke="#64748b" stroke-width="4"/>
    <rect x="285" y="{box_y}" width="200" height="170" rx="18" fill="#14284a" stroke="#818cf8" stroke-width="2"/>
    <text x="385" y="{box_y + 40}" text-anchor="middle" font-size="14" font-weight="700" fill="#a5b4fc">LESSONS 3–4</text><text x="385" y="{box_y + 83}" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Internet</text><text x="385" y="{box_y + 119}" text-anchor="middle" font-size="15" fill="#a7bdd4">IPv4 packets</text><text x="385" y="{box_y + 144}" text-anchor="middle" font-size="15" fill="#a7bdd4">ICMP messages</text>
    <path d="M495 {box_y + 85} H515" stroke="#64748b" stroke-width="4"/><path d="M508 {box_y + 76} L519 {box_y + 85} L508 {box_y + 94}" fill="none" stroke="#64748b" stroke-width="4"/>
    <rect x="525" y="{box_y}" width="200" height="170" rx="18" fill="#12332f" stroke="#34d399" stroke-width="2"/>
    <text x="625" y="{box_y + 40}" text-anchor="middle" font-size="14" font-weight="700" fill="#6ee7b7">LESSONS 5–7</text><text x="625" y="{box_y + 83}" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Transport</text><text x="625" y="{box_y + 119}" text-anchor="middle" font-size="15" fill="#a7bdd4">UDP · TCP segments</text><text x="625" y="{box_y + 144}" text-anchor="middle" font-size="15" fill="#a7bdd4">State · reliability</text>
    <path d="M735 {box_y + 85} H755" stroke="#64748b" stroke-width="4"/><path d="M748 {box_y + 76} L759 {box_y + 85} L748 {box_y + 94}" fill="none" stroke="#64748b" stroke-width="4"/>
    <rect x="765" y="{box_y}" width="200" height="170" rx="18" fill="#422b17" stroke="#f59e0b" stroke-width="2"/>
    <text x="865" y="{box_y + 40}" text-anchor="middle" font-size="14" font-weight="700" fill="#fbbf24">LESSONS 8–10</text><text x="865" y="{box_y + 83}" text-anchor="middle" font-size="23" font-weight="700" fill="#f8fafc">Applications</text><text x="865" y="{box_y + 119}" text-anchor="middle" font-size="15" fill="#a7bdd4">Sockets · DNS</text><text x="865" y="{box_y + 144}" text-anchor="middle" font-size="15" fill="#a7bdd4">HTTP framing</text>
    <path d="M975 {box_y + 85} H995" stroke="#64748b" stroke-width="4"/><path d="M988 {box_y + 76} L999 {box_y + 85} L988 {box_y + 94}" fill="none" stroke="#64748b" stroke-width="4"/>
    <rect x="1005" y="{box_y}" width="150" height="170" rx="18" fill="#3e2135" stroke="#f472b6" stroke-width="2"/>
    <text x="1080" y="{box_y + 40}" text-anchor="middle" font-size="14" font-weight="700" fill="#f9a8d4">11–12</text><text x="1080" y="{box_y + 80}" text-anchor="middle" font-size="20" font-weight="700" fill="#f8fafc">Practice</text><text x="1080" y="{box_y + 116}" text-anchor="middle" font-size="14" fill="#a7bdd4">Routing · NAT</text><text x="1080" y="{box_y + 141}" text-anchor="middle" font-size="14" fill="#a7bdd4">Diagnostics</text>
  </g>
  <text x="600" y="{footer_y}" text-anchor="middle" font-family="system-ui,sans-serif" font-size="17" font-weight="600" fill="#dbeafe">Offline byte buffers first · loopback only for native socket exercises · no privileged capture</text>
</svg>
"""


def expected_assets() -> dict[Path, str]:
    return {
        ASSETS / "tcp-ip-overview.svg": overview_svg(),
        ASSETS / "tcp-ip-social.svg": overview_svg(social=True),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    failures = []
    for path, expected in expected_assets().items():
        if args.check:
            if not path.exists() or path.read_text() != expected:
                failures.append(path.relative_to(ROOT))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(expected)
            print(f"wrote {path.relative_to(ROOT)}")
    if failures:
        print("out-of-date assets:", ", ".join(map(str, failures)))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
