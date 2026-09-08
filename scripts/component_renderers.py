"""Pure semantic HTML renderers for structured course components."""

from __future__ import annotations

import html
from collections.abc import Callable

try:
    from scripts.component_models import Component, ComponentModelError
except ModuleNotFoundError:
    from component_models import Component, ComponentModelError  # type: ignore[no-redef]


def _text(value: object, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ComponentModelError(f"{label} must be a non-empty string")
    return html.escape(value)


def _strings(value: object, label: str) -> list[str]:
    if not isinstance(value, list) or not value:
        raise ComponentModelError(f"{label} must be a non-empty array")
    return [_text(item, f"{label} item") for item in value]


def _link(value: object, label: str) -> tuple[str, str]:
    if not isinstance(value, dict) or "label" not in value or "href" not in value:
        raise ComponentModelError(f"{label} must contain label and href")
    href = _text(value["href"], f"{label}.href")
    if href.startswith(("javascript:", "data:")):
        raise ComponentModelError(f"{label}.href uses a forbidden scheme")
    return _text(value["label"], f"{label}.label"), href


def _section(component: Component, body: str, modifier: str) -> str:
    return (
        f'<section class="course-component course-component--{modifier}" '
        f'id="component-{html.escape(component.id, quote=True)}" '
        f'data-course-component="{html.escape(component.type, quote=True)}">'
        f"<h2>{html.escape(component.heading)}</h2>{body}</section>"
    )


def render_page_hero(component: Component) -> str:
    payload = component.payload
    allowed = {"title", "summary", "badges", "actions"}
    if set(payload) != allowed:
        raise ComponentModelError(f"{component.id}: page_hero payload keys mismatch")
    badges = "".join(f"<li>{badge}</li>" for badge in _strings(payload["badges"], "badges"))
    actions = "".join(
        f'<a href="{href}">{label}</a>'
        for label, href in (_link(item, "action") for item in payload["actions"])
    )
    body = (
        f'<div class="course-hero__copy"><h1>{_text(payload["title"], "title")}</h1>'
        f"<p>{_text(payload['summary'], 'summary')}</p>"
        f'<ul class="course-badges" aria-label="Page attributes">{badges}</ul>'
        f'<nav class="course-actions" aria-label="Page actions">{actions}</nav></div>'
    )
    return _section(component, body, "hero")


def render_prerequisites_outcomes(component: Component) -> str:
    payload = component.payload
    if set(payload) != {"prerequisites", "outcomes"}:
        raise ComponentModelError(f"{component.id}: prerequisites_outcomes payload keys mismatch")
    prerequisites = "".join(
        f"<li>{item}</li>" for item in _strings(payload["prerequisites"], "prerequisites")
    )
    outcomes = "".join(f"<li>{item}</li>" for item in _strings(payload["outcomes"], "outcomes"))
    body = (
        '<div class="course-two-column"><div><h3>Before you start</h3>'
        f"<ul>{prerequisites}</ul></div>"
        f"<div><h3>After this page</h3><ul>{outcomes}</ul></div></div>"
    )
    return _section(component, body, "outcomes")


def render_curriculum_cards(component: Component) -> str:
    items = component.payload.get("items")
    if set(component.payload) != {"items"} or not isinstance(items, list) or not items:
        raise ComponentModelError(f"{component.id}: curriculum_cards requires items")
    cards: list[str] = []
    for index, item in enumerate(items):
        label, href = _link(item, f"items[{index}]")
        if set(item) != {"label", "href", "description", "meta"}:
            raise ComponentModelError(f"{component.id}: curriculum item keys mismatch")
        meta = "".join(f"<li>{entry}</li>" for entry in _strings(item["meta"], "meta"))
        cards.append(
            f'<article class="course-card"><h3><a href="{href}">{label}</a></h3>'
            f"<p>{_text(item['description'], 'description')}</p><ul>{meta}</ul></article>"
        )
    return _section(component, f'<div class="course-card-grid">{"".join(cards)}</div>', "cards")


def render_exercise_test_contract(component: Component) -> str:
    payload = component.payload
    if set(payload) != {"exercise", "reference", "checks"}:
        raise ComponentModelError(f"{component.id}: exercise_test_contract payload keys mismatch")
    checks = "".join(f"<li>{item}</li>" for item in _strings(payload["checks"], "checks"))
    exercise = _text(payload["exercise"], "exercise")
    reference = _text(payload["reference"], "reference")
    body = (
        '<div class="course-contract"><div><h3>Student exercise</h3>'
        f"<pre><code>{exercise}</code></pre></div>"
        f"<div><h3>Reference check</h3><pre><code>{reference}</code></pre></div>"
        f"<div><h3>Completion checks</h3><ul>{checks}</ul></div></div>"
    )
    return _section(component, body, "contract")


def render_safety_boundary(component: Component) -> str:
    payload = component.payload
    if set(payload) != {"allowed", "excluded"}:
        raise ComponentModelError(f"{component.id}: safety_boundary payload keys mismatch")
    allowed = "".join(f"<li>{item}</li>" for item in _strings(payload["allowed"], "allowed"))
    excluded = "".join(f"<li>{item}</li>" for item in _strings(payload["excluded"], "excluded"))
    body = (
        '<div class="course-safety"><div><h3>Included</h3>'
        f"<ul>{allowed}</ul></div><div><h3>Outside this course</h3><ul>{excluded}</ul></div></div>"
    )
    return _section(component, body, "safety")


def _fixture_bytes(component: Component) -> bytes:
    return bytes.fromhex(component.payload["fixture"].replace(" ", "").replace("\n", ""))


def render_protocol_fields(component: Component) -> str:
    fields = component.payload["fields"]
    total = len(_fixture_bytes(component))
    bands = "".join(
        f'<span class="protocol-field" style="--field-grow:{field["length"]}" '
        f'data-field-id="{html.escape(field["id"], quote=True)}">'
        f"<strong>{html.escape(field['label'])}</strong><small>{html.escape(field['value'])}</small></span>"
        for field in fields
    )
    rows = "".join(
        f"<tr><td><code>{field['offset']}</code></td><td><code>{field['length']}</code></td>"
        f"<td>{html.escape(field['label'])}</td><td><code>{html.escape(field['value'])}</code></td>"
        f"<td>{html.escape(field['meaning'])}</td></tr>"
        for field in fields
    )
    body = (
        f'<div class="protocol-field-map" aria-label="{total}-byte protocol fixture">{bands}</div>'
        '<div class="component-table" role="region" aria-label="Protocol fields" tabindex="0">'
        "<table><thead><tr><th>Offset</th><th>Bytes</th><th>Field</th><th>Value</th><th>Meaning</th></tr>"
        f"</thead><tbody>{rows}</tbody></table></div>"
    )
    return _section(component, body, "protocol-fields")


def render_byte_inspector(component: Component) -> str:
    fixture = _fixture_bytes(component)
    annotations = component.payload["annotations"]
    labels: list[str] = []
    cells: list[str] = []
    for offset, value in enumerate(fixture):
        annotation = next(
            (
                item
                for item in annotations
                if item["offset"] <= offset < item["offset"] + item["length"]
            ),
            None,
        )
        label = annotation["label"] if annotation is not None else "unlabeled"
        labels.append(label)
        escaped_label = html.escape(label, quote=True)
        cells.append(
            f'<li data-byte-group="{escaped_label}" title="{escaped_label}">'
            f"<span>{offset:02x}</span><code>{value:02x}</code></li>"
        )
    legend = "".join(f"<li>{html.escape(label)}</li>" for label in dict.fromkeys(labels))
    body = (
        f'<ol class="byte-grid" style="--bytes-per-row:{component.payload["bytes_per_row"]}" '
        f'aria-label="{len(fixture)} fixture bytes">{"".join(cells)}</ol>'
        f'<ul class="byte-legend" aria-label="Byte groups">{legend}</ul>'
    )
    return _section(component, body, "byte-inspector")


def render_checksum_view(component: Component) -> str:
    fixture = _fixture_bytes(component)
    regions = "".join(
        f"<li><strong>{html.escape(region['role'])}</strong>"
        f"<span>bytes {region['offset']}–{region['offset'] + region['length'] - 1}</span></li>"
        for region in component.payload["regions"]
    )
    word_values = [
        int.from_bytes(fixture[index : index + 2].ljust(2, bytes(1)), "big")
        for index in range(0, len(fixture), 2)
    ]
    words = "".join(f"<li><code>{value:04x}</code></li>" for value in word_values)
    expected = component.payload["expected"]
    body = (
        '<div class="checksum-result"><span>Expected checksum</span>'
        f"<strong><code>0x{expected:04x}</code></strong></div>"
        f'<ol class="checksum-regions">{regions}</ol>'
        f'<ol class="checksum-words" aria-label="16-bit words">{words}</ol>'
    )
    return _section(component, body, "checksum")


def render_state_machine(component: Component) -> str:
    payload = component.payload
    state_labels = {item["id"]: item["label"] for item in payload["states"]}
    event_labels = {item["id"]: item["label"] for item in payload["events"]}
    states = "".join(
        f'<li data-state-id="{html.escape(item["id"], quote=True)}" '
        f'class="{"is-initial" if item["id"] == payload["initial"] else ""}">'
        f"<strong>{html.escape(item['label'])}</strong></li>"
        for item in payload["states"]
    )
    transitions = "".join(
        f"<li><span>{html.escape(state_labels[item['from']])}</span>"
        f"<strong>{html.escape(event_labels[item['event']])}</strong>"
        f"<span>{html.escape(state_labels[item['to']])}</span></li>"
        for item in payload["transitions"]
    )
    body = (
        f'<ol class="state-nodes" aria-label="States">{states}</ol>'
        f'<ol class="state-transitions" aria-label="Transitions">{transitions}</ol>'
    )
    return _section(component, body, "state-machine")


def render_transition_table(component: Component) -> str:
    columns = "".join(
        f'<th scope="col">{html.escape(value)}</th>' for value in component.payload["columns"]
    )
    rows = "".join(
        "<tr>" + "".join(f"<td>{html.escape(value)}</td>" for value in row) + "</tr>"
        for row in component.payload["rows"]
    )
    body = (
        '<div class="component-table" role="region" aria-label="Transition table" tabindex="0">'
        f"<table><thead><tr>{columns}</tr></thead><tbody>{rows}</tbody></table></div>"
    )
    return _section(component, body, "transition-table")


def render_socket_timeline(component: Component) -> str:
    lane_labels = {item["id"]: item["label"] for item in component.payload["lanes"]}
    events = "".join(
        f'<li data-lane="{html.escape(item["lane"], quote=True)}">'
        f"<span>{html.escape(lane_labels[item['lane']])}</span>"
        f"<strong>{html.escape(item['label'])}</strong><p>{html.escape(item['detail'])}</p></li>"
        for item in component.payload["events"]
    )
    return _section(component, f'<ol class="socket-timeline">{events}</ol>', "timeline")


def render_routing_table(component: Component) -> str:
    routes = "".join(
        f"<tr><td><code>{html.escape(item['network'])}/{item['prefix']}</code></td>"
        f"<td><code>{html.escape(item['next_hop'])}</code></td>"
        f"<td>{html.escape(str(item['interface']))}</td><td>{item['metric']}</td></tr>"
        for item in component.payload["routes"]
    )
    cases = "".join(
        f"<li><code>{html.escape(str(item.get('destination', '')))}</code>"
        f"<span>{html.escape(str(item.get('status', '')))}</span>"
        f"<strong>{html.escape(str(item.get('selected', 'none')))}</strong></li>"
        for item in component.payload["cases"]
    )
    body = (
        '<div class="component-table" role="region" aria-label="Routing table" tabindex="0">'
        "<table><thead><tr><th>Network</th><th>Next hop</th>"
        "<th>Interface</th><th>Metric</th></tr></thead>"
        f'<tbody>{routes}</tbody></table></div><ol class="routing-cases">{cases}</ol>'
    )
    return _section(component, body, "routing")


def render_nat_table(component: Component) -> str:
    steps = "".join(
        f"<tr><td>{index}</td><td>{html.escape(item['action'])}</td>"
        f"<td><code>{html.escape(item['input'])}</code></td>"
        f"<td><code>{html.escape(item['output'])}</code></td>"
        f"<td>{html.escape(item['status'])}</td></tr>"
        for index, item in enumerate(component.payload["steps"], 1)
    )
    public_ip = html.escape(component.payload["public_ip"])
    first_port = component.payload["first_port"]
    body = (
        f'<p class="nat-summary">Public address <code>{public_ip}</code>, '
        f"first translated port <code>{first_port}</code>.</p>"
        '<div class="component-table" role="region" aria-label="NAT lifecycle" tabindex="0">'
        "<table><thead><tr><th>Step</th><th>Action</th><th>Input</th><th>Output</th><th>Status</th></tr></thead>"
        f"<tbody>{steps}</tbody></table></div>"
    )
    return _section(component, body, "nat")


def render_diagnostics_explorer(component: Component) -> str:
    cards: list[str] = []
    for case in component.payload["cases"]:
        status = html.escape(case["status"])
        status_attribute = html.escape(case["status"], quote=True)
        layers = " → ".join(html.escape(layer) for layer in case["layers"])
        diagnostics = ", ".join(html.escape(item) for item in case["diagnostics"]) or "none"
        cards.append(
            f'<article class="diagnostic-case" data-status="{status_attribute}">'
            f"<h3>{html.escape(case['label'])}</h3>"
            f"<p>{html.escape(case['summary'])}</p>"
            f"<p><strong>Status:</strong> <code>{status}</code></p>"
            f"<p><strong>Layers:</strong> {layers}</p>"
            f"<p><strong>Diagnostics:</strong> {diagnostics}</p>"
            f"<p><strong>Checksums:</strong> {case['checksums_valid']}/"
            f"{case['checksums_checked']} valid</p></article>"
        )
    body = f'<div class="diagnostic-grid">{"".join(cards)}</div>'
    return _section(component, body, "diagnostics")


def render_linux_uapi_view(component: Component) -> str:
    fields = "".join(
        f"<tr><td><code>{html.escape(field['name'])}</code></td>"
        f"<td>{html.escape(field['presence'])}</td><td>{html.escape(field['meaning'])}</td></tr>"
        for field in component.payload["fields"]
    )
    invariants = "".join(
        f"<li>{html.escape(item)}</li>" for item in component.payload["invariants"]
    )
    body = (
        f"<p>{html.escape(component.payload['title'])}</p>"
        '<div class="component-table" role="region" aria-label="Linux UAPI fields" tabindex="0">'
        f"<table><thead><tr><th>Field</th><th>Presence</th><th>Meaning</th></tr></thead>"
        f'<tbody>{fields}</tbody></table></div><ul class="uapi-invariants">{invariants}</ul>'
    )
    return _section(component, body, "uapi")


RENDERERS: dict[str, Callable[[Component], str]] = {
    "page_hero": render_page_hero,
    "prerequisites_outcomes": render_prerequisites_outcomes,
    "curriculum_cards": render_curriculum_cards,
    "protocol_fields": render_protocol_fields,
    "byte_inspector": render_byte_inspector,
    "checksum_view": render_checksum_view,
    "state_machine": render_state_machine,
    "transition_table": render_transition_table,
    "socket_timeline": render_socket_timeline,
    "routing_table": render_routing_table,
    "nat_table": render_nat_table,
    "diagnostics_explorer": render_diagnostics_explorer,
    "linux_uapi_view": render_linux_uapi_view,
    "exercise_test_contract": render_exercise_test_contract,
    "safety_boundary": render_safety_boundary,
}


def render_component(component: Component) -> str:
    """Render one validated component to complete semantic HTML."""
    renderer = RENDERERS.get(component.type)
    if renderer is None:
        raise ComponentModelError(f"renderer not implemented for {component.type!r}")
    return renderer(component)
