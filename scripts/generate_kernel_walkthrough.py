"""Generate deterministic Linux v6.6 walkthrough C data and accessible SVGs."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any
from xml.sax.saxutils import escape, quoteattr

ROOT = Path(__file__).parents[1]
DATA_PATH = ROOT / "docs" / "data" / "linux-v6.6-walkthrough.json"
ASSET_DIR = ROOT / "docs" / "assets" / "kernel-source-walkthrough"
INCLUDE_PATH = ROOT / "linux_labs" / "04_kernel_source_walkthrough" / "walkthrough_data.inc"

EXPECTED_ENUMS = (
    "TCPIP_LINUX_L04_NETIF_RECEIVE_SKB",
    "TCPIP_LINUX_L04_NETIF_RECEIVE_SKB_ONE_CORE",
    "TCPIP_LINUX_L04_IP_RCV",
    "TCPIP_LINUX_L04_NF_INET_PRE_ROUTING",
    "TCPIP_LINUX_L04_IP_RCV_FINISH",
    "TCPIP_LINUX_L04_IP_ROUTE_INPUT_NOREF",
    "TCPIP_LINUX_L04_IP_ROUTE_INPUT_SLOW",
    "TCPIP_LINUX_L04_DST_INPUT",
    "TCPIP_LINUX_L04_IP_LOCAL_DELIVER",
    "TCPIP_LINUX_L04_NF_INET_LOCAL_IN",
    "TCPIP_LINUX_L04_IP_LOCAL_DELIVER_FINISH",
    "TCPIP_LINUX_L04_TCP_V4_RCV",
    "TCPIP_LINUX_L04_INET_LOOKUP_SKB",
    "TCPIP_LINUX_L04_INET_LOOKUP_ESTABLISHED",
    "TCPIP_LINUX_L04_TCP_RCV_ESTABLISHED",
    "TCPIP_LINUX_L04_TCP_ACK",
    "TCPIP_LINUX_L04_SOCK_SENDMSG",
    "TCPIP_LINUX_L04_TCP_SENDMSG",
    "TCPIP_LINUX_L04_TCP_SENDMSG_LOCKED",
    "TCPIP_LINUX_L04_TCP_WRITE_XMIT",
    "TCPIP_LINUX_L04_TCP_TRANSMIT_SKB",
    "TCPIP_LINUX_L04_INET_QUEUE_XMIT",
    "TCPIP_LINUX_L04_IP_ROUTE_OUTPUT_FLOW",
    "TCPIP_LINUX_L04_IP_QUEUE_XMIT",
    "TCPIP_LINUX_L04___IP_QUEUE_XMIT",
    "TCPIP_LINUX_L04_IP_LOCAL_OUT",
    "TCPIP_LINUX_L04_IP_OUTPUT",
    "TCPIP_LINUX_L04_IP_FINISH_OUTPUT",
    "TCPIP_LINUX_L04___DEV_QUEUE_XMIT",
    "TCPIP_LINUX_L04_TCP_GET_INFO",
)
EXPECTED_INGRESS = tuple(range(16))
EXPECTED_EGRESS = (16, 17, 18, 19, 20, 21, 23, 24, 22, 25, 26, 27, 28)
EDGE_TYPES = (
    "direct-call",
    "hook-continuation",
    "callback-dispatch",
    "conditional-slow-path",
    "sequencing",
    "observation",
)
PHASES = ("packet-io", "network", "transport", "observation")
PHASE_COLORS = {
    "light": ("#2a78d6", "#eb6834", "#1baf7a", "#eda100"),
    "dark": ("#3987e5", "#d95926", "#199e70", "#c98500"),
}
THEMES = {
    "light": {
        "surface": "#fcfcfb",
        "plane": "#f9f9f7",
        "text": "#0b0b0b",
        "secondary": "#52514e",
        "muted": "#706f6a",
        "grid": "#e1e0d9",
        "node": "#ffffff",
        "edge": "#52514e",
    },
    "dark": {
        "surface": "#1a1a19",
        "plane": "#0d0d0d",
        "text": "#ffffff",
        "secondary": "#c3c2b7",
        "muted": "#a4a39d",
        "grid": "#383835",
        "node": "#242422",
        "edge": "#c3c2b7",
    },
}
EDGE_DASH = {
    "direct-call": "",
    "hook-continuation": "10 5",
    "callback-dispatch": "2 5",
    "conditional-slow-path": "14 5 3 5",
    "sequencing": "5 3",
    "observation": "12 4 2 4 2 4",
}


def load_data() -> dict[str, Any]:
    try:
        data = json.loads(DATA_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read {DATA_PATH.relative_to(ROOT)}: {error}") from error
    validate_data(data)
    return data


def require_keys(value: dict[str, Any], keys: set[str], context: str) -> None:
    if set(value) != keys:
        missing = sorted(keys - set(value))
        extra = sorted(set(value) - keys)
        raise ValueError(f"{context} keys differ; missing={missing}, extra={extra}")


def validate_data(data: dict[str, Any]) -> None:
    require_keys(
        data,
        {
            "schema_version",
            "linux",
            "phases",
            "symbols",
            "routes",
            "edges",
            "uapi_boundary",
        },
        "root",
    )
    if data["schema_version"] != 1:
        raise ValueError("schema_version must be 1")
    if data["linux"] != {"repository": "https://github.com/torvalds/linux", "ref": "v6.6"}:
        raise ValueError("linux repository and ref must identify torvalds/linux v6.6")

    phases = data["phases"]
    if not isinstance(phases, list) or tuple(item.get("key") for item in phases) != PHASES:
        raise ValueError("phases must use the fixed four-phase order")
    for index, phase in enumerate(phases):
        require_keys(phase, {"key", "label", "description", "pattern"}, f"phase {index}")
        if not all(isinstance(phase[key], str) and phase[key] for key in phase):
            raise ValueError(f"phase {index} values must be non-empty strings")

    symbols = data["symbols"]
    if not isinstance(symbols, list) or len(symbols) != 30:
        raise ValueError("symbols must contain exactly 30 records")
    symbol_keys = {
        "id",
        "enum",
        "key",
        "name",
        "path",
        "line",
        "description",
        "role",
        "ownership",
        "layer",
        "phase",
        "memberships",
    }
    seen_keys: set[str] = set()
    for index, symbol in enumerate(symbols):
        require_keys(symbol, symbol_keys, f"symbol {index}")
        if symbol["id"] != index or symbol["enum"] != EXPECTED_ENUMS[index]:
            raise ValueError(f"symbol {index} must match the Lab 4 enum order")
        if symbol["key"] in seen_keys:
            raise ValueError(f"duplicate symbol key {symbol['key']!r}")
        seen_keys.add(symbol["key"])
        for key in ("enum", "key", "name", "path", "description", "role", "ownership", "layer"):
            if not isinstance(symbol[key], str) or not symbol[key]:
                raise ValueError(f"symbol {index}.{key} must be a non-empty string")
        if not isinstance(symbol["line"], int) or symbol["line"] <= 0:
            raise ValueError(f"symbol {index}.line must be a positive integer")
        if symbol["phase"] not in PHASES:
            raise ValueError(f"symbol {index} has an unknown phase")
        memberships = symbol["memberships"]
        if not isinstance(memberships, list) or not memberships:
            raise ValueError(f"symbol {index}.memberships must be a non-empty array")
        if any(item not in {"ingress", "egress", "observation"} for item in memberships):
            raise ValueError(f"symbol {index} has an unknown membership")

    routes = data["routes"]
    require_keys(routes, {"ingress", "egress"}, "routes")
    if tuple(routes["ingress"]) != EXPECTED_INGRESS:
        raise ValueError("ingress must match the authoritative Lab 4 route")
    if tuple(routes["egress"]) != EXPECTED_EGRESS:
        raise ValueError("egress must match the authoritative Lab 4 route")
    for route_name, route in routes.items():
        for symbol_id in route:
            if route_name not in symbols[symbol_id]["memberships"]:
                raise ValueError(f"symbol {symbol_id} lacks {route_name} membership")

    edges = data["edges"]
    if not isinstance(edges, list) or not edges:
        raise ValueError("edges must be a non-empty array")
    expected_pairs = {
        (0, 1),
        (1, 2),
        (2, 3),
        (3, 4),
        (4, 5),
        (5, 6),
        (6, 7),
        (7, 8),
        (8, 9),
        (9, 10),
        (10, 11),
        (11, 12),
        (12, 13),
        (13, 14),
        (14, 15),
        (16, 17),
        (17, 18),
        (18, 19),
        (19, 20),
        (20, 21),
        (21, 23),
        (23, 24),
        (24, 22),
        (22, 25),
        (24, 25),
        (25, 26),
        (26, 27),
        (27, 28),
    }
    actual_pairs: set[tuple[int, int]] = set()
    edge_keys = {"from", "to", "route", "type", "explanation"}
    for index, edge in enumerate(edges):
        require_keys(edge, edge_keys, f"edge {index}")
        pair = (edge["from"], edge["to"])
        if pair in actual_pairs:
            raise ValueError(f"duplicate edge {pair}")
        actual_pairs.add(pair)
        if edge["route"] not in routes or edge["type"] not in EDGE_TYPES:
            raise ValueError(f"edge {index} has an unknown route or type")
        if not isinstance(edge["explanation"], str) or not edge["explanation"]:
            raise ValueError(f"edge {index} explanation must be non-empty")
    if actual_pairs != expected_pairs:
        raise ValueError("edges must match the authoritative Lab 4 edge set")
    for route_name, route in routes.items():
        for start, end in zip(route, route[1:], strict=False):
            if (start, end) not in actual_pairs:
                raise ValueError(f"{route_name} route lacks edge {start}->{end}")

    boundary = data["uapi_boundary"]
    require_keys(boundary, {"nodes", "edges"}, "uapi_boundary")
    expected_boundary_keys = ("getsockopt", "TCP_INFO", "struct_tcp_info", "tcp_get_info")
    if tuple(node.get("key") for node in boundary["nodes"]) != expected_boundary_keys:
        raise ValueError(
            "UAPI boundary must include getsockopt/TCP_INFO/struct tcp_info/tcp_get_info"
        )
    for index, node in enumerate(boundary["nodes"]):
        expected = (
            {"key", "label", "side", "symbol_id"}
            if "symbol_id" in node
            else {
                "key",
                "label",
                "side",
                "path",
                "line",
            }
        )
        require_keys(node, expected, f"uapi node {index}")
    boundary_keys = set(expected_boundary_keys)
    for index, edge in enumerate(boundary["edges"]):
        require_keys(edge, edge_keys, f"uapi edge {index}")
        if edge["from"] not in boundary_keys or edge["to"] not in boundary_keys:
            raise ValueError(f"uapi edge {index} refers to an unknown node")
        if edge["route"] != "observation" or edge["type"] not in EDGE_TYPES:
            raise ValueError(f"uapi edge {index} has an invalid route or type")


def c_string(value: str) -> str:
    result = ['"']
    for character in value:
        codepoint = ord(character)
        if character == "\\":
            result.append("\\\\")
        elif character == '"':
            result.append('\\"')
        elif character == "\n":
            result.append("\\n")
        elif character == "\r":
            result.append("\\r")
        elif character == "\t":
            result.append("\\t")
        elif 32 <= codepoint <= 126:
            result.append(character)
        else:
            result.extend(f"\\{byte:03o}" for byte in character.encode("utf-8"))
    result.append('"')
    return "".join(result)


def generated_include(data: dict[str, Any]) -> str:
    symbols = data["symbols"]
    ingress = data["routes"]["ingress"]
    egress = data["routes"]["egress"]
    edges = data["edges"]
    lines = [
        "/* Generated by scripts/generate_kernel_walkthrough.py; do not edit. */",
        "#define TCPIP_LINUX_L04_GENERATED_SYMBOL_COUNT 30U",
        f"#define TCPIP_LINUX_L04_GENERATED_INGRESS_COUNT {len(ingress)}U",
        f"#define TCPIP_LINUX_L04_GENERATED_EGRESS_COUNT {len(egress)}U",
        f"#define TCPIP_LINUX_L04_GENERATED_EDGE_COUNT {len(edges)}U",
        "",
        "static const tcpip_linux_l04_symbol tcpip_linux_l04_symbols[] = {",
    ]
    for symbol in symbols:
        lines.extend(
            (
                "    {",
                f"        {symbol['enum']},",
                f"        {c_string(symbol['key'])},",
                f"        {c_string(symbol['name'])},",
                f"        {c_string(symbol['path'])},",
                f"        {symbol['line']}U,",
                f"        {c_string(symbol['description'])},",
                "    },",
            )
        )
    lines.extend(("};", "", "static const tcpip_linux_l04_symbol_id tcpip_linux_l04_ingress[] = {"))
    lines.extend(f"    {symbols[symbol_id]['enum']}," for symbol_id in ingress)
    lines.extend(("};", "", "static const tcpip_linux_l04_symbol_id tcpip_linux_l04_egress[] = {"))
    lines.extend(f"    {symbols[symbol_id]['enum']}," for symbol_id in egress)
    lines.extend(("};", "", "static const tcpip_linux_l04_edge tcpip_linux_l04_edges[] = {"))
    for edge in edges:
        lines.append(f"    {{{symbols[edge['from']]['enum']}, {symbols[edge['to']]['enum']}}},")
    lines.extend(
        (
            "};",
            "",
            "_Static_assert(TCPIP_LINUX_L04_GENERATED_SYMBOL_COUNT ==",
            "                   TCPIP_LINUX_L04_SYMBOL_COUNT,",
            '               "generated symbol count must match lab.h");',
            "_Static_assert(TCPIP_LINUX_L04_GENERATED_INGRESS_COUNT ==",
            "                   TCPIP_LINUX_L04_INGRESS_COUNT,",
            '               "generated ingress count must match lab.h");',
            "_Static_assert(TCPIP_LINUX_L04_GENERATED_EGRESS_COUNT ==",
            "                   TCPIP_LINUX_L04_EGRESS_COUNT,",
            '               "generated egress count must match lab.h");',
            "",
        )
    )
    return "\n".join(lines)


def xml_text(value: object) -> str:
    return escape(str(value), {'"': "&quot;", "'": "&apos;"})


def phase_index(phase: str) -> int:
    return PHASES.index(phase)


def svg_header(
    *, title: str, description: str, diagram_id: str, theme_name: str, height: int
) -> list[str]:
    theme = THEMES[theme_name]
    colors = PHASE_COLORS[theme_name]
    lines = [
        (
            f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1440 {height}" '
            f'width="100%" role="img" aria-labelledby="{diagram_id}-title {diagram_id}-desc">'
        ),
        f"  <title id={quoteattr(diagram_id + '-title')}>{xml_text(title)}</title>",
        f"  <desc id={quoteattr(diagram_id + '-desc')}>{xml_text(description)}</desc>",
        "  <defs>",
        (
            f'    <marker id="{diagram_id}-arrow" markerWidth="8" markerHeight="8" '
            'refX="7" refY="4" orient="auto" markerUnits="strokeWidth">'
        ),
        f'      <path d="M0,0 L8,4 L0,8 Z" fill="{theme["edge"]}"/>',
        "    </marker>",
    ]
    pattern_paths = (
        '<path d="M0 4 H8" stroke="{color}" stroke-width="1" opacity=".34"/>',
        '<path d="M-2 8 L8 -2 M2 10 L10 2" stroke="{color}" stroke-width="1" opacity=".34"/>',
        (
            '<path d="M-2 8 L8 -2 M2 10 L10 2 M-2 0 L8 10 M2 -2 L10 6" '
            'stroke="{color}" stroke-width="1" opacity=".3"/>'
        ),
        '<circle cx="2" cy="2" r="1" fill="{color}" opacity=".42"/>',
    )
    sizes = ("8 8", "8 8", "8 8", "6 6")
    for phase, color, path, size in zip(PHASES, colors, pattern_paths, sizes, strict=True):
        lines.extend(
            (
                f'    <pattern id="{diagram_id}-{phase}" width="{size.split()[0]}" '
                f'height="{size.split()[1]}" patternUnits="userSpaceOnUse">',
                f"      {path.format(color=color)}",
                "    </pattern>",
            )
        )
    lines.extend(
        (
            "  </defs>",
            f'  <rect width="1440" height="{height}" fill="{theme["surface"]}"/>',
            (
                f'  <g font-family="system-ui,-apple-system,Segoe UI,sans-serif" '
                f'fill="{theme["text"]}">'
            ),
            f'    <text x="56" y="58" font-size="30" font-weight="700">{xml_text(title)}</text>',
            (
                f'    <text x="56" y="88" font-size="15" fill="{theme["secondary"]}">'
                "Linux v6.6 · authored reading route · not a universal runtime stack trace</text>"
            ),
        )
    )
    return lines


def phase_legend(data: dict[str, Any], diagram_id: str, theme_name: str) -> list[str]:
    theme = THEMES[theme_name]
    colors = PHASE_COLORS[theme_name]
    lines = ['    <g aria-label="Four labeled phases">']
    width = 317
    for index, (phase, color) in enumerate(zip(data["phases"], colors, strict=True)):
        x = 56 + index * 334
        lines.extend(
            (
                f'      <rect x="{x}" y="116" width="{width}" height="54" rx="8" '
                f'fill="{theme["node"]}" stroke="{theme["grid"]}"/>',
                f'      <rect x="{x}" y="116" width="12" height="54" rx="6" fill="{color}"/>',
                f'      <rect x="{x + 12}" y="116" width="36" height="54" '
                f'fill="url(#{diagram_id}-{phase["key"]})"/>',
                f'      <text x="{x + 58}" y="139" font-size="14" font-weight="700">'
                f"{xml_text(phase['label'])}</text>",
                f'      <text x="{x + 58}" y="158" font-size="11" fill="{theme["secondary"]}">'
                f"{xml_text(phase['pattern'])} pattern</text>",
            )
        )
    lines.append("    </g>")
    return lines


def edge_legend(theme_name: str, y: int) -> list[str]:
    theme = THEMES[theme_name]
    lines = [f'    <g aria-label="Edge type legend" transform="translate(56 {y})">']
    for index, edge_type in enumerate(EDGE_TYPES):
        column = index % 3
        row = index // 3
        x = column * 440
        item_y = row * 34
        dash = EDGE_DASH[edge_type]
        dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
        lines.extend(
            (
                f'      <line x1="{x}" y1="{item_y}" x2="{x + 64}" y2="{item_y}" '
                f'stroke="{theme["edge"]}" stroke-width="2"{dash_attr}/>',
                f'      <text x="{x + 76}" y="{item_y + 5}" font-size="13">'
                f"{xml_text(edge_type)}</text>",
            )
        )
    lines.append("    </g>")
    return lines


def route_svg(data: dict[str, Any], route_name: str, theme_name: str) -> str:
    route = data["routes"][route_name]
    symbols = data["symbols"]
    edge_by_pair = {(edge["from"], edge["to"]): edge for edge in data["edges"]}
    title = f"Linux v6.6 TCP {route_name} source walkthrough"
    description = (
        f"A {len(route)}-node {route_name} reading route grouped by four labeled phases. "
        "Every node gives a source symbol and ownership summary; line patterns identify edge types."
    )
    diagram_id = f"{route_name}-{theme_name}"
    first_y = 220
    row_height = 42
    height = first_y + len(route) * row_height + 184
    lines = svg_header(
        title=title,
        description=description,
        diagram_id=diagram_id,
        theme_name=theme_name,
        height=height,
    )
    lines.extend(phase_legend(data, diagram_id, theme_name))
    theme = THEMES[theme_name]
    colors = PHASE_COLORS[theme_name]
    lines.append(f'    <g aria-label="{route_name} symbols and edges">')
    for index, symbol_id in enumerate(route):
        symbol = symbols[symbol_id]
        y = first_y + index * row_height
        color = colors[phase_index(symbol["phase"])]
        lines.extend(
            (
                f'      <rect x="72" y="{y}" width="1296" height="34" rx="7" '
                f'fill="{theme["node"]}" stroke="{theme["grid"]}"/>',
                f'      <rect x="72" y="{y}" width="10" height="34" rx="5" fill="{color}"/>',
                f'      <rect x="82" y="{y}" width="30" height="34" '
                f'fill="url(#{diagram_id}-{symbol["phase"]})"/>',
                f'      <text x="124" y="{y + 22}" font-size="14" font-weight="700">'
                f"{xml_text(symbol['name'])}</text>",
                f'      <text x="480" y="{y + 22}" font-size="12" fill="{theme["secondary"]}">'
                f"{xml_text(symbol['path'])}:{symbol['line']}</text>",
                f'      <text x="790" y="{y + 22}" font-size="12" fill="{theme["secondary"]}">'
                f"{xml_text(symbol['ownership'])}</text>",
            )
        )
        if index + 1 < len(route):
            next_id = route[index + 1]
            edge = edge_by_pair[(symbol_id, next_id)]
            next_y = first_y + (index + 1) * row_height
            dash = EDGE_DASH[edge["type"]]
            dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
            lines.extend(
                (
                    f'      <line x1="56" y1="{y + 30}" x2="56" y2="{next_y + 4}" '
                    f'stroke="{theme["edge"]}" stroke-width="2"{dash_attr} '
                    f'marker-end="url(#{diagram_id}-arrow)"/>',
                    f'      <text x="92" y="{y + 41}" font-size="9" fill="{theme["muted"]}">'
                    f"{xml_text(edge['type'])}</text>",
                )
            )
    lines.append("    </g>")
    lines.extend(edge_legend(theme_name, first_y + len(route) * row_height + 52))
    lines.extend(
        (
            f'    <text x="56" y="{height - 28}" font-size="11" fill="{theme["muted"]}">'
            "Patterns and direct labels make phase and edge identity independent of color.</text>",
            "  </g>",
            "</svg>",
            "",
        )
    )
    return "\n".join(lines)


def ownership_svg(data: dict[str, Any], theme_name: str) -> str:
    symbols = data["symbols"]
    title = "Linux v6.6 packet and socket ownership guide"
    description = (
        "Thirty source symbols grouped into ingress, egress, and observation columns. "
        "Each row directly labels the symbol, role, phase, and ownership transition."
    )
    diagram_id = f"ownership-{theme_name}"
    columns = (
        ("Ingress ownership", list(range(16)), 56),
        ("Egress ownership", list(range(16, 29)), 512),
        ("Observation", [29], 968),
    )
    height = 1180
    lines = svg_header(
        title=title,
        description=description,
        diagram_id=diagram_id,
        theme_name=theme_name,
        height=height,
    )
    lines.extend(phase_legend(data, diagram_id, theme_name))
    theme = THEMES[theme_name]
    colors = PHASE_COLORS[theme_name]
    for heading, ids, x in columns:
        lines.extend(
            (
                f"    <g aria-label={quoteattr(heading)}>",
                f'      <text x="{x}" y="212" font-size="18" font-weight="700">'
                f"{xml_text(heading)}</text>",
            )
        )
        for row, symbol_id in enumerate(ids):
            symbol = symbols[symbol_id]
            y = 232 + row * 56
            color = colors[phase_index(symbol["phase"])]
            lines.extend(
                (
                    f'      <rect x="{x}" y="{y}" width="416" height="48" rx="7" '
                    f'fill="{theme["node"]}" stroke="{theme["grid"]}"/>',
                    f'      <rect x="{x}" y="{y}" width="9" height="48" rx="4" fill="{color}"/>',
                    f'      <rect x="{x + 9}" y="{y}" width="24" height="48" '
                    f'fill="url(#{diagram_id}-{symbol["phase"]})"/>',
                    f'      <text x="{x + 42}" y="{y + 18}" font-size="12" font-weight="700">'
                    f"{xml_text(symbol['name'])}</text>",
                    f'      <text x="{x + 42}" y="{y + 35}" font-size="10" '
                    f'fill="{theme["secondary"]}">'
                    f"{xml_text(symbol['ownership'])}</text>",
                    f'      <text x="{x + 404}" y="{y + 18}" text-anchor="end" font-size="9" '
                    f'fill="{theme["muted"]}">{xml_text(symbol["role"])}</text>',
                )
            )
        lines.append("    </g>")
    lines.extend(edge_legend(theme_name, 1100))
    lines.extend(
        (
            "  </g>",
            "</svg>",
            "",
        )
    )
    return "\n".join(lines)


def uapi_svg(data: dict[str, Any], theme_name: str) -> str:
    boundary = data["uapi_boundary"]
    title = "Linux v6.6 TCP_INFO userspace boundary"
    description = (
        "A four-step observation route from userspace getsockopt and TCP_INFO through "
        "tcp_get_info to the public struct tcp_info, with source anchors and typed edges."
    )
    diagram_id = f"uapi-boundary-{theme_name}"
    height = 690
    lines = svg_header(
        title=title,
        description=description,
        diagram_id=diagram_id,
        theme_name=theme_name,
        height=height,
    )
    lines.extend(phase_legend(data, diagram_id, theme_name))
    theme = THEMES[theme_name]
    color = PHASE_COLORS[theme_name][3]
    positions = {
        "getsockopt": (70, 264),
        "TCP_INFO": (400, 264),
        "tcp_get_info": (730, 264),
        "struct_tcp_info": (1060, 264),
    }
    node_by_key = {node["key"]: node for node in boundary["nodes"]}
    lines.extend(
        (
            f'    <rect x="56" y="205" width="626" height="216" rx="12" '
            f'fill="{theme["plane"]}" stroke="{theme["grid"]}"/>',
            f'    <rect x="702" y="205" width="682" height="216" rx="12" '
            f'fill="{theme["plane"]}" stroke="{theme["grid"]}"/>',
            '    <text x="76" y="238" font-size="15" font-weight="700">'
            "Userspace and UAPI contract</text>",
            '    <text x="722" y="238" font-size="15" font-weight="700">'
            "Kernel implementation</text>",
            '    <g aria-label="TCP INFO observation nodes and edges">',
        )
    )
    ordered_keys = ("getsockopt", "TCP_INFO", "tcp_get_info", "struct_tcp_info")
    for key in ordered_keys:
        node = node_by_key[key]
        x, y = positions[key]
        if "symbol_id" in node:
            symbol = data["symbols"][node["symbol_id"]]
            source = f"{symbol['path']}:{symbol['line']}"
        else:
            source = f"{node['path']}:{node['line']}"
        lines.extend(
            (
                f'      <rect x="{x}" y="{y}" width="276" height="92" rx="10" '
                f'fill="{theme["node"]}" stroke="{theme["grid"]}"/>',
                f'      <rect x="{x}" y="{y}" width="12" height="92" rx="6" fill="{color}"/>',
                f'      <rect x="{x + 12}" y="{y}" width="30" height="92" '
                f'fill="url(#{diagram_id}-observation)"/>',
                f'      <text x="{x + 54}" y="{y + 34}" font-size="17" font-weight="700">'
                f"{xml_text(node['label'])}</text>",
                f'      <text x="{x + 54}" y="{y + 58}" font-size="11" fill="{theme["secondary"]}">'
                f"{xml_text(source)}</text>",
                f'      <text x="{x + 54}" y="{y + 77}" font-size="10" fill="{theme["muted"]}">'
                f"{xml_text(node['side'])}</text>",
            )
        )
    forward_pairs = (
        ("getsockopt", "TCP_INFO"),
        ("TCP_INFO", "tcp_get_info"),
        ("tcp_get_info", "struct_tcp_info"),
    )
    edge_by_pair = {(edge["from"], edge["to"]): edge for edge in boundary["edges"]}
    for start, end in forward_pairs:
        edge = edge_by_pair[(start, end)]
        start_x, start_y = positions[start]
        end_x, _ = positions[end]
        dash = EDGE_DASH[edge["type"]]
        dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
        lines.extend(
            (
                f'      <line x1="{start_x + 276}" y1="{start_y + 46}" x2="{end_x - 10}" '
                f'y2="{start_y + 46}" stroke="{theme["edge"]}" stroke-width="2"{dash_attr} '
                f'marker-end="url(#{diagram_id}-arrow)"/>',
                f'      <text x="{(start_x + 276 + end_x - 10) // 2}" y="{start_y + 34}" '
                f'text-anchor="middle" font-size="9" fill="{theme["muted"]}">'
                f"{xml_text(edge['type'])}</text>",
            )
        )
    return_edge = edge_by_pair[("struct_tcp_info", "getsockopt")]
    dash = EDGE_DASH[return_edge["type"]]
    lines.extend(
        (
            f'      <path d="M1198 366 V394 H208 V366" fill="none" stroke="{theme["edge"]}" '
            f'stroke-width="2" stroke-dasharray="{dash}" marker-end="url(#{diagram_id}-arrow)"/>',
            f'      <text x="703" y="411" text-anchor="middle" font-size="10" '
            f'fill="{theme["muted"]}">observation · bounded copy to caller option length</text>',
            "    </g>",
        )
    )
    lines.extend(edge_legend(theme_name, 494))
    lines.extend(
        (
            f'    <text x="56" y="640" font-size="12" fill="{theme["secondary"]}">'
            "Stable boundary: getsockopt(IPPROTO_TCP, TCP_INFO, …) exposes only the UAPI "
            "structure.</text>",
            "  </g>",
            "</svg>",
            "",
        )
    )
    return "\n".join(lines)


def expected_outputs(data: dict[str, Any]) -> dict[Path, bytes]:
    outputs: dict[Path, bytes] = {INCLUDE_PATH: generated_include(data).encode("utf-8")}
    for theme_name in ("light", "dark"):
        outputs[ASSET_DIR / f"ingress-{theme_name}.svg"] = route_svg(
            data, "ingress", theme_name
        ).encode("utf-8")
        outputs[ASSET_DIR / f"egress-{theme_name}.svg"] = route_svg(
            data, "egress", theme_name
        ).encode("utf-8")
        outputs[ASSET_DIR / f"ownership-{theme_name}.svg"] = ownership_svg(data, theme_name).encode(
            "utf-8"
        )
        outputs[ASSET_DIR / f"uapi-boundary-{theme_name}.svg"] = uapi_svg(data, theme_name).encode(
            "utf-8"
        )
    return outputs


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify that generated outputs are present and byte-for-byte current",
    )
    args = parser.parse_args()
    try:
        data = load_data()
        outputs = expected_outputs(data)
    except ValueError as error:
        print(f"walkthrough metadata error: {error}", file=sys.stderr)
        return 2

    failures: list[Path] = []
    for path, expected in outputs.items():
        if args.check:
            if not path.exists() or path.read_bytes() != expected:
                failures.append(path.relative_to(ROOT))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(expected)
            print(f"wrote {path.relative_to(ROOT)}")
    if failures:
        print("out-of-date generated files:", ", ".join(map(str, failures)), file=sys.stderr)
        return 1
    if args.check:
        print(f"checked {len(outputs)} generated files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
