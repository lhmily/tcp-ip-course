"""Prepare repository Markdown for the MkDocs course site."""

from __future__ import annotations

import argparse
import html
import json
import re
import shutil
from pathlib import Path
from urllib.parse import quote, urlsplit

try:
    from scripts.course_catalog import (
        LESSONS,
        LINUX_LABS,
        CatalogPage,
        route_document,
        source_document,
    )
except ModuleNotFoundError:  # Direct script execution adds scripts/, not the repository root.
    from course_catalog import (  # type: ignore[no-redef]
        LESSONS,
        LINUX_LABS,
        CatalogPage,
        route_document,
        source_document,
    )

ROOT = Path(__file__).parents[1]
DEFAULT_OUTPUT = ROOT / ".site-docs"
SITE_URL = "https://lhmily.github.io/tcp-ip-course/"
GITHUB_URL = "https://github.com/lhmily/tcp-ip-course"
CATALOG_PAGES = (*LESSONS, *LINUX_LABS)
SOURCE_TO_ROUTE = {(page.source_root, page.source): page.route for page in CATALOG_PAGES}
WALKTHROUGH_SOURCE = ROOT / "linux_labs" / "04_kernel_source_walkthrough" / "README.md"
WALKTHROUGH_DATA = ROOT / "docs" / "data" / "linux-v6.6-walkthrough.json"
WALKTHROUGH_MARKERS = (
    "<!-- L04_INTERACTIVE_EXPLORER -->",
    "<!-- L04_SOURCE_TABLE -->",
)
WALKTHROUGH_SYMBOL_FIELDS = {
    "id": int,
    "enum": str,
    "key": str,
    "name": str,
    "path": str,
    "line": int,
    "description": str,
    "role": str,
    "ownership": str,
    "layer": str,
    "phase": str,
    "memberships": list,
}
WALKTHROUGH_EDGE_FIELDS = {
    "from": str,
    "to": str,
    "route": str,
    "type": str,
    "explanation": str,
}


def front_matter(title: str, description: str, resource_type: str) -> str:
    values = {
        "title": title,
        "description": description,
        "learning_resource_type": resource_type,
    }
    return (
        "---\n"
        + "\n".join(f"{key}: {json.dumps(value)}" for key, value in values.items())
        + "\n---\n\n"
    )


def _page_link(current: CatalogPage | None, target: CatalogPage) -> str:
    if current is None:
        return f"{target.route}index.md"
    if current.route_root == target.route_root:
        return f"../{target.slug}/index.md"
    return f"../../{target.route_root}/{target.slug}/index.md"


def _rewrite_linux_overview(text: str) -> str:
    for lab in LINUX_LABS:
        text = text.replace(f"{lab.source}/README.md", f"{lab.slug}/index.md")
    return text


def _remove_missing_lab_links(text: str) -> str:
    for lab in LINUX_LABS:
        text = re.sub(
            rf"^.*\((?:linux_labs/|\.\./\.\./linux_labs/)?{re.escape(lab.source)}/README\.md\).*$\n?",
            "",
            text,
            flags=re.MULTILINE,
        )
        text = re.sub(
            rf"^.*\((?:\.\./)*linux-labs/{re.escape(lab.slug)}/index\.md\).*$\n?",
            "",
            text,
            flags=re.MULTILINE,
        )
    text = text.replace(
        "See the [Linux track overview](linux_labs/README.md) for exact commands and prerequisites.",
        "See the Linux track overview for exact commands and prerequisites.",
    )
    return text


def rewrite_markdown(
    text: str, *, page: CatalogPage | None = None, linux_overview: bool = False
) -> str:
    if linux_overview:
        overview_target = "index.md"
    elif page is None:
        overview_target = "linux-labs/index.md"
    else:
        overview_target = "../../linux-labs/index.md"
    text = text.replace("linux_labs/README.md", overview_target)
    for (source_root, source), _route in SOURCE_TO_ROUTE.items():
        target_page = next(
            item
            for item in CATALOG_PAGES
            if item.source_root == source_root and item.source == source
        )
        target = _page_link(page, target_page)
        patterns = (
            rf"(?:\.\./|{re.escape(source_root)}/){re.escape(source)}/README\.md",
            rf"\.\./{re.escape(source)}/README\.md",
        )
        for pattern in patterns:
            text = re.sub(pattern, target, text)
    text = text.replace("../../docs/assets/", "../../assets/")
    text = text.replace("(docs/assets/", "(assets/")
    text = text.replace('src="docs/assets/', 'src="assets/')
    for name in ("CONTRIBUTING.md", "CHANGELOG.md", "LICENSE"):
        text = text.replace(f"({name})", f"({GITHUB_URL}/blob/main/{name})")
    return text


def _walkthrough_error(message: str) -> ValueError:
    return ValueError(f"invalid walkthrough data: {message}")


def _required_mapping(value: object, label: str) -> dict[str, object]:
    if not isinstance(value, dict):
        raise _walkthrough_error(f"{label} must be an object")
    return value


def _required_string(value: object, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise _walkthrough_error(f"{label} must be a non-empty string")
    return value


def _validate_source_path(source_path: str, label: str) -> None:
    path = Path(source_path)
    if path.is_absolute() or ".." in path.parts or source_path.startswith(("./", "/")):
        raise _walkthrough_error(f"{label} is unsafe: {source_path!r}")


def _validate_walkthrough_data(raw: object) -> dict[str, object]:
    data = _required_mapping(raw, "root")
    if data.get("schema_version") != 1:
        raise _walkthrough_error("schema_version must equal 1")
    linux = _required_mapping(data.get("linux"), "linux")
    repo = _required_string(linux.get("repository"), "linux.repository")
    ref = _required_string(linux.get("ref"), "linux.ref")
    parsed_repo = urlsplit(repo)
    if parsed_repo.scheme != "https" or not parsed_repo.netloc or parsed_repo.query or parsed_repo.fragment:
        raise _walkthrough_error("linux.repository must be a query-free HTTPS URL")
    if repo.rstrip("/") != "https://github.com/torvalds/linux":
        raise _walkthrough_error("linux.repository must be the Torvalds Linux mirror")
    if ref != "v6.6":
        raise _walkthrough_error("linux.ref must be the pinned v6.6 tag")

    phases_raw = data.get("phases")
    if not isinstance(phases_raw, list) or not phases_raw:
        raise _walkthrough_error("phases must be a non-empty array")
    phases: list[dict[str, object]] = []
    phase_keys: set[str] = set()
    for index, raw_phase in enumerate(phases_raw):
        phase = _required_mapping(raw_phase, f"phases[{index}]")
        for field in ("key", "label", "description", "pattern"):
            _required_string(phase.get(field), f"phases[{index}].{field}")
        key = str(phase["key"])
        if key in phase_keys:
            raise _walkthrough_error(f"duplicate phase key {key!r}")
        phase_keys.add(key)
        phases.append(dict(phase))

    symbols_raw = data.get("symbols")
    if not isinstance(symbols_raw, list) or not symbols_raw:
        raise _walkthrough_error("symbols must be a non-empty array")
    symbols: list[dict[str, object]] = []
    keys: set[str] = set()
    ids: set[int] = set()
    enums: set[str] = set()
    for index, raw_symbol in enumerate(symbols_raw):
        symbol = _required_mapping(raw_symbol, f"symbols[{index}]")
        for field, expected in WALKTHROUGH_SYMBOL_FIELDS.items():
            value = symbol.get(field)
            if expected is int:
                if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                    raise _walkthrough_error(f"symbols[{index}].{field} must be a non-negative integer")
            elif expected is str:
                _required_string(value, f"symbols[{index}].{field}")
            elif not isinstance(value, list) or not value or not all(
                isinstance(item, str) and item for item in value
            ):
                raise _walkthrough_error(
                    f"symbols[{index}].memberships must be a non-empty string array"
                )
        key = str(symbol["key"])
        symbol_id = int(symbol["id"])
        if key in keys:
            raise _walkthrough_error(f"duplicate symbol key {key!r}")
        if symbol_id in ids:
            raise _walkthrough_error(f"duplicate symbol id {symbol_id!r}")
        if str(symbol["enum"]) in enums:
            raise _walkthrough_error(f"duplicate symbol enum {symbol['enum']!r}")
        _validate_source_path(str(symbol["path"]), f"symbols[{index}].path")
        if str(symbol["phase"]) not in phase_keys:
            raise _walkthrough_error(f"unknown phase for symbol {key!r}")
        memberships = symbol["memberships"]
        if not set(memberships) <= {"ingress", "egress", "observation"}:
            raise _walkthrough_error(f"unknown membership for symbol {key!r}")
        keys.add(key)
        ids.add(symbol_id)
        enums.add(str(symbol["enum"]))
        symbols.append(dict(symbol))
    if len(symbols) != 30:
        raise _walkthrough_error(f"expected 30 symbols, found {len(symbols)}")
    by_id = {int(symbol["id"]): symbol for symbol in symbols}

    routes_raw = _required_mapping(data.get("routes"), "routes")
    routes: dict[str, list[int]] = {}
    expected_route_lengths = {"ingress": 16, "egress": 13}
    for route_name, expected_length in expected_route_lengths.items():
        route = routes_raw.get(route_name)
        if not isinstance(route, list) or not route or not all(
            isinstance(symbol_id, int) and not isinstance(symbol_id, bool) for symbol_id in route
        ):
            raise _walkthrough_error(f"routes.{route_name} must be a non-empty ID array")
        if len(route) != expected_length:
            raise _walkthrough_error(
                f"routes.{route_name} must contain {expected_length} IDs, found {len(route)}"
            )
        if len(route) != len(set(route)):
            raise _walkthrough_error(f"routes.{route_name} contains duplicate IDs")
        unknown = set(route) - ids
        if unknown:
            raise _walkthrough_error(
                f"routes.{route_name} refers to unknown IDs: {sorted(unknown)}"
            )
        for symbol_id in route:
            if route_name not in by_id[symbol_id]["memberships"]:
                raise _walkthrough_error(
                    f"symbol ID {symbol_id} is missing {route_name!r} membership"
                )
        routes[route_name] = list(route)

    edges_raw = data.get("edges")
    if not isinstance(edges_raw, list) or len(edges_raw) != 28:
        count = len(edges_raw) if isinstance(edges_raw, list) else 0
        raise _walkthrough_error(f"edges must contain the 28 kernel graph edges, found {count}")
    edges: list[dict[str, object]] = []
    edge_pairs: set[tuple[str, int, int]] = set()
    for index, raw_edge in enumerate(edges_raw):
        edge = _required_mapping(raw_edge, f"edges[{index}]")
        for field in ("route", "type", "explanation"):
            _required_string(edge.get(field), f"edges[{index}].{field}")
        from_id = edge.get("from")
        to_id = edge.get("to")
        if not isinstance(from_id, int) or isinstance(from_id, bool):
            raise _walkthrough_error(f"edges[{index}].from must be a symbol ID")
        if not isinstance(to_id, int) or isinstance(to_id, bool):
            raise _walkthrough_error(f"edges[{index}].to must be a symbol ID")
        route_name = str(edge["route"])
        if route_name not in routes:
            raise _walkthrough_error(f"edges[{index}].route must be ingress or egress")
        if from_id not in ids or to_id not in ids:
            raise _walkthrough_error(f"edges[{index}] refers to an unknown symbol ID")
        pair = (route_name, from_id, to_id)
        if pair in edge_pairs:
            raise _walkthrough_error(f"duplicate edge {pair!r}")
        edge_pairs.add(pair)
        edges.append(dict(edge))
    for route_name, route in routes.items():
        missing = [
            (left, right)
            for left, right in zip(route, route[1:], strict=False)
            if (route_name, left, right) not in edge_pairs
        ]
        if missing:
            raise _walkthrough_error(f"routes.{route_name} is missing adjacent edges: {missing}")

    boundary = _required_mapping(data.get("uapi_boundary"), "uapi_boundary")
    nodes_raw = boundary.get("nodes")
    boundary_edges_raw = boundary.get("edges")
    if not isinstance(nodes_raw, list) or not nodes_raw:
        raise _walkthrough_error("uapi_boundary.nodes must be a non-empty array")
    boundary_nodes: list[dict[str, object]] = []
    boundary_keys: set[str] = set()
    boundary_sides = {"userspace", "uapi", "kernel"}
    for index, raw_node in enumerate(nodes_raw):
        node = _required_mapping(raw_node, f"uapi_boundary.nodes[{index}]")
        key = _required_string(node.get("key"), f"uapi_boundary.nodes[{index}].key")
        _required_string(node.get("label"), f"uapi_boundary.nodes[{index}].label")
        side = _required_string(node.get("side"), f"uapi_boundary.nodes[{index}].side")
        if side not in boundary_sides:
            raise _walkthrough_error(f"uapi_boundary.nodes[{index}].side is unknown")
        if key in boundary_keys:
            raise _walkthrough_error(f"duplicate UAPI node key {key!r}")
        if "symbol_id" in node:
            symbol_id = node["symbol_id"]
            if not isinstance(symbol_id, int) or isinstance(symbol_id, bool) or symbol_id not in ids:
                raise _walkthrough_error(f"uapi_boundary.nodes[{index}].symbol_id is unknown")
        else:
            _required_string(node.get("path"), f"uapi_boundary.nodes[{index}].path")
            _validate_source_path(str(node["path"]), f"uapi_boundary.nodes[{index}].path")
            line = node.get("line")
            if not isinstance(line, int) or isinstance(line, bool) or line < 1:
                raise _walkthrough_error(f"uapi_boundary.nodes[{index}].line must be positive")
        boundary_keys.add(key)
        boundary_nodes.append(dict(node))
    if not isinstance(boundary_edges_raw, list) or not boundary_edges_raw:
        raise _walkthrough_error("uapi_boundary.edges must be a non-empty array")
    boundary_edges: list[dict[str, object]] = []
    for index, raw_edge in enumerate(boundary_edges_raw):
        edge = _required_mapping(raw_edge, f"uapi_boundary.edges[{index}]")
        for field in WALKTHROUGH_EDGE_FIELDS:
            _required_string(edge.get(field), f"uapi_boundary.edges[{index}].{field}")
        if edge["from"] not in boundary_keys or edge["to"] not in boundary_keys:
            raise _walkthrough_error(f"uapi_boundary.edges[{index}] refers to an unknown node")
        if edge["route"] != "observation":
            raise _walkthrough_error(f"uapi_boundary.edges[{index}].route must be observation")
        boundary_edges.append(dict(edge))

    return {
        **data,
        "linux": {**linux, "repository": repo.rstrip("/"), "ref": ref},
        "phases": phases,
        "symbols": symbols,
        "routes": routes,
        "edges": edges,
        "uapi_boundary": {"nodes": boundary_nodes, "edges": boundary_edges},
    }


def _source_url(repo: str, ref: str, symbol: dict[str, object]) -> str:
    source_path = "/".join(quote(part, safe="._-") for part in str(symbol["path"]).split("/"))
    return f"{repo}/blob/{quote(ref, safe='._-')}/{source_path}#L{symbol['line']}"


def _walkthrough_phase_order(data: dict[str, object]) -> list[dict[str, str]]:
    phases = data["phases"]
    assert isinstance(phases, list)
    return [
        {
            "key": str(phase["key"]),
            "label": str(phase["label"]),
        }
        for phase in phases
        if isinstance(phase, dict)
    ]


def _walkthrough_route_html(data: dict[str, object], route_name: str) -> str:
    symbols = data["symbols"]
    routes = data["routes"]
    edges = data["edges"]
    linux = data["linux"]
    assert isinstance(symbols, list) and isinstance(routes, dict)
    assert isinstance(edges, list) and isinstance(linux, dict)
    route = routes[route_name]
    assert isinstance(route, list)
    repo = str(linux["repository"])
    ref = str(linux["ref"])
    by_id = {int(symbol["id"]): symbol for symbol in symbols if isinstance(symbol, dict)}
    edge_types = {
        (int(edge["from"]), int(edge["to"])): str(edge["type"])
        for edge in edges
        if isinstance(edge, dict) and edge["route"] == route_name
    }
    items: list[str] = []
    for position, symbol_id in enumerate(route, 1):
        symbol = by_id[symbol_id]
        outgoing_type = (
            "end"
            if position == len(route)
            else edge_types[(symbol_id, int(route[position]))]
        )
        name = html.escape(str(symbol["name"]))
        phase = html.escape(str(symbol["phase"]))
        layer = html.escape(str(symbol["layer"]))
        url = html.escape(_source_url(repo, ref, symbol), quote=True)
        escaped_id = html.escape(str(symbol_id), quote=True)
        safe_edge_type = re.sub(r"[^a-z0-9_-]+", "-", outgoing_type.lower()).strip("-")
        items.append(
            f'<li class="kw-route-step kw-route-edge-{safe_edge_type}" '
            f'data-edge-type="{html.escape(outgoing_type, quote=True)}">'
            f'<button type="button" class="kw-node" data-symbol-id="{escaped_id}" '
            f'data-route-name="{route_name}" aria-pressed="false">'
            f'<span class="kw-node-index">{position}</span>'
            f'<span class="kw-node-copy"><code>{name}</code>'
            f'<small>{layer} · {phase}</small></span></button>'
            f'<a class="kw-no-js-source" href="{url}">source</a>'
            "</li>"
        )
    return (
        f'<section class="kw-route" data-route-list="{route_name}" '
        f'aria-labelledby="kw-{route_name}-title">'
        f'<h3 id="kw-{route_name}-title">{route_name.title()} route</h3>'
        f'<ol class="kw-route-list">{"".join(items)}</ol></section>'
    )


def _walkthrough_explorer_html(data: dict[str, object]) -> str:
    enriched = dict(data)
    symbols = enriched["symbols"]
    linux = enriched["linux"]
    assert isinstance(symbols, list) and isinstance(linux, dict)
    enriched["symbols"] = [
        {
            **symbol,
            "source_url": _source_url(
                str(linux["repository"]), str(linux["ref"]), symbol
            ),
        }
        for symbol in symbols
        if isinstance(symbol, dict)
    ]
    payload = (
        json.dumps(enriched, ensure_ascii=False, separators=(",", ":"))
        .replace("<", "\\u003c")
        .replace(">", "\\u003e")
        .replace("&", "\\u0026")
    )
    phase_buttons = "".join(
        f'<button type="button" data-phase="{html.escape(phase["key"], quote=True)}" '
        f'aria-pressed="false">{html.escape(phase["label"])}</button>'
        for phase in _walkthrough_phase_order(data)
        if phase["key"] != "observation"
    )
    return (
        '<div class="kernel-walkthrough" data-kernel-walkthrough>'
        '<noscript><p class="kw-noscript">Interactive controls need JavaScript; '
        'both routes and their pinned source links are listed below.</p></noscript>'
        '<div class="kw-controls" aria-label="Walkthrough controls">'
        '<div class="kw-control-group" aria-label="Route">'
        '<span>Route</span>'
        '<button type="button" data-route="ingress" aria-pressed="true">Ingress</button>'
        '<button type="button" data-route="egress" aria-pressed="false">Egress</button>'
        "</div>"
        '<div class="kw-control-group kw-phase-controls" aria-label="Phase">'
        '<span>Phase</span>'
        '<button type="button" data-phase="all" aria-pressed="true">All phases</button>'
        f"{phase_buttons}</div></div>"
        '<p class="kw-status" data-walkthrough-status aria-live="polite"></p>'
        '<div class="kw-explorer-layout"><div class="kw-routes">'
        f'{_walkthrough_route_html(data, "ingress")}'
        f'{_walkthrough_route_html(data, "egress")}'
        "</div>"
        '<aside class="kw-detail" data-walkthrough-detail aria-label="Selected source symbol">'
        '<p class="kw-detail-meta"><span data-detail-layer></span> · '
        '<span data-detail-phase></span></p>'
        '<h3><code data-detail-name></code></h3>'
        '<p class="kw-detail-role" data-detail-role></p>'
        '<p data-detail-description></p>'
        '<dl><div><dt>Ownership</dt><dd data-detail-ownership></dd></div>'
        '<div><dt>Edge type</dt><dd data-detail-edge-type></dd></div>'
        '<div><dt>Why this edge</dt><dd data-detail-edge></dd></div></dl>'
        '<p><a data-detail-source href="#">Open pinned Linux source</a></p>'
        "</aside></div>"
        '<div class="kw-step-controls" aria-label="Route navigation">'
        '<button type="button" data-step="previous">Previous</button>'
        '<button type="button" data-step="next">Next</button>'
        "</div>"
        f'<script type="application/json" data-walkthrough-data>{payload}</script>'
        '<script defer src="../../javascripts/kernel-walkthrough.js"></script>'
        "</div>"
    )


def _walkthrough_source_table(data: dict[str, object]) -> str:
    symbols = data["symbols"]
    linux = data["linux"]
    assert isinstance(symbols, list) and isinstance(linux, dict)
    rows: list[str] = []
    for symbol in symbols:
        assert isinstance(symbol, dict)
        url = html.escape(
            _source_url(str(linux["repository"]), str(linux["ref"]), symbol), quote=True
        )
        name = html.escape(str(symbol["name"]))
        role = html.escape(str(symbol["role"]))
        ownership = html.escape(str(symbol["ownership"]))
        phase = html.escape(str(symbol["phase"]).replace("_", " ").title())
        source = html.escape(f"{symbol['path']}:{symbol['line']}")
        rows.append(
            f"<tr><td><code>{name}</code></td><td>{phase}</td><td>{role}</td>"
            f"<td>{ownership}</td><td><a href=\"{url}\"><code>{source}</code></a></td></tr>"
        )
    return (
        '<div class="kernel-source-table" role="region" aria-label="All 30 pinned Linux source records" '
        'tabindex="0"><table><thead><tr><th scope="col">Symbol</th>'
        '<th scope="col">Phase</th><th scope="col">Role</th>'
        '<th scope="col">Ownership</th><th scope="col">Pinned source</th>'
        f"</tr></thead><tbody>{''.join(rows)}</tbody></table></div>"
    )


def _expand_walkthrough(text: str) -> str:
    for marker in WALKTHROUGH_MARKERS:
        count = text.count(marker)
        if count != 1:
            raise ValueError(f"walkthrough marker must appear exactly once: {marker} (found {count})")
    if text.count("kernel-walkthrough.js"):
        raise ValueError("walkthrough source must not add its own page-local script")
    if not WALKTHROUGH_DATA.is_file():
        raise FileNotFoundError(
            f"missing walkthrough data: {WALKTHROUGH_DATA.relative_to(ROOT)}"
        )
    try:
        raw = json.loads(WALKTHROUGH_DATA.read_text())
    except json.JSONDecodeError as error:
        raise _walkthrough_error(f"malformed JSON at line {error.lineno}, column {error.colno}") from error
    data = _validate_walkthrough_data(raw)
    return text.replace(WALKTHROUGH_MARKERS[0], _walkthrough_explorer_html(data)).replace(
        WALKTHROUGH_MARKERS[1], _walkthrough_source_table(data)
    )


def _append_track_navigation(
    text: str,
    *,
    page: CatalogPage,
    pages: tuple[CatalogPage, ...],
    overview: str,
    label: str,
) -> str:
    index = pages.index(page)
    links = [
        f"[{overview}](../../index.md)"
        if page.route_root == "lessons"
        else f"[{overview}](../index.md)"
    ]
    if index:
        previous = pages[index - 1]
        links.append(
            f"[← {label} {previous.number}: {previous.title}](../{previous.slug}/index.md)"
        )
    if index + 1 < len(pages):
        following = pages[index + 1]
        links.append(
            f"[{label} {following.number}: {following.title} →](../{following.slug}/index.md)"
        )
    return text + "\n\n---\n\n" + " · ".join(links) + "\n"


def _stage_page(
    output: Path,
    page: CatalogPage,
    *,
    pages: tuple[CatalogPage, ...],
    overview: str,
    label: str,
) -> None:
    source = source_document(page, ROOT)
    if not source.is_file():
        raise FileNotFoundError(f"missing {label.lower()} document: {source.relative_to(ROOT)}")
    destination = route_document(page, output)
    destination.parent.mkdir(parents=True, exist_ok=True)
    content = source.read_text()
    if source == WALKTHROUGH_SOURCE:
        content = _expand_walkthrough(content)
    content = rewrite_markdown(content, page=page)
    if page.route_root == "lessons":
        content = re.sub(r"(?:\.\./){4}(linux-labs/)", r"../../\1", content)
    content = _append_track_navigation(
        content, page=page, pages=pages, overview=overview, label=label
    )
    destination.write_text(front_matter(page.title, page.description, "LearningResource") + content)


def prepare(output: Path = DEFAULT_OUTPUT) -> None:
    output = output.resolve()
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    homepage_description = (
        "Learn TCP/IP in 12 portable C17 core lessons and 4 optional Linux implementation labs."
    )
    lab_documents_present = all(source_document(lab, ROOT).is_file() for lab in LINUX_LABS)
    homepage = (ROOT / "README.md").read_text()
    if not lab_documents_present:
        homepage = _remove_missing_lab_links(homepage)
    homepage = rewrite_markdown(homepage)
    (output / "index.md").write_text(
        front_matter("TCP/IP Course in C17", homepage_description, "Course") + homepage
    )

    lesson_pages: tuple[CatalogPage, ...] = LESSONS
    for lesson in LESSONS:
        _stage_page(
            output,
            lesson,
            pages=lesson_pages,
            overview="Course overview",
            label="Lesson",
        )

    linux_overview = ROOT / "linux_labs" / "README.md"
    if not linux_overview.is_file():
        raise FileNotFoundError("missing Linux track overview: linux_labs/README.md")
    linux_description = "Explore Linux networking implementation boundaries in four optional, unprivileged C17 labs."
    overview_destination = output / "linux-labs" / "index.md"
    overview_destination.parent.mkdir(parents=True, exist_ok=True)

    linux_pages: tuple[CatalogPage, ...] = LINUX_LABS
    missing_linux_labs = [
        source_document(lab, ROOT).relative_to(ROOT)
        for lab in LINUX_LABS
        if not source_document(lab, ROOT).is_file()
    ]
    overview_text = linux_overview.read_text()
    if not missing_linux_labs:
        overview_text = _rewrite_linux_overview(overview_text)
    if missing_linux_labs:
        missing = ", ".join(map(str, missing_linux_labs))
        print(f"skipping Linux lab pages; parallel files are absent: {missing}")
        overview_text = _remove_missing_lab_links(overview_text)
        overview_destination.write_text(
            front_matter(
                "Optional Linux implementation track", linux_description, "LearningResource"
            )
            + overview_text
        )
        for lesson in LESSONS:
            staged = route_document(lesson, output)
            staged.write_text(_remove_missing_lab_links(staged.read_text()))
        homepage_staged = output / "index.md"
        homepage_staged.write_text(_remove_missing_lab_links(homepage_staged.read_text()))
    else:
        overview_destination.write_text(
            front_matter(
                "Optional Linux implementation track", linux_description, "LearningResource"
            )
            + rewrite_markdown(overview_text, linux_overview=True)
        )
        for lab in LINUX_LABS:
            _stage_page(
                output,
                lab,
                pages=linux_pages,
                overview="Linux track overview",
                label="Linux lab",
            )

    shutil.copytree(ROOT / "docs" / "assets", output / "assets")
    shutil.copytree(ROOT / "docs" / "stylesheets", output / "stylesheets")
    javascript_source = ROOT / "docs" / "javascripts"
    if javascript_source.is_dir():
        shutil.copytree(javascript_source, output / "javascripts")
    manifest = {
        "name": "TCP/IP Course in C17",
        "short_name": "TCP/IP C17",
        "description": homepage_description,
        "start_url": "/tcp-ip-course/",
        "scope": "/tcp-ip-course/",
        "display": "standalone",
        "background_color": "#07111f",
        "theme_color": "#07111f",
        "icons": [
            {"src": "assets/branding/icon-192.png", "sizes": "192x192", "type": "image/png"},
            {"src": "assets/branding/icon-512.png", "sizes": "512x512", "type": "image/png"},
        ],
    }
    (output / "site.webmanifest").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "robots.txt").write_text(f"User-agent: *\nAllow: /\nSitemap: {SITE_URL}sitemap.xml\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    prepare(args.output)
    print(
        f"prepared {len(LESSONS)} core lessons and the optional Linux track "
        f"in {args.output.resolve()}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
