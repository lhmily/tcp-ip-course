"""Strict structured-content models for course page components."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

ALLOWED_COMPONENT_TYPES = {
    "page_hero",
    "prerequisites_outcomes",
    "curriculum_cards",
    "protocol_fields",
    "byte_inspector",
    "checksum_view",
    "state_machine",
    "transition_table",
    "socket_timeline",
    "routing_table",
    "nat_table",
    "diagnostics_explorer",
    "exercise_test_contract",
    "safety_boundary",
    "linux_uapi_view",
}


class ComponentModelError(ValueError):
    """Raised when a structured page model violates its contract."""


@dataclass(frozen=True)
class Component:
    id: str
    type: str
    heading: str
    payload: dict[str, Any]
    provenance: tuple[dict[str, str], ...]


@dataclass(frozen=True)
class PageModel:
    schema_version: int
    catalog_key: str
    components: tuple[Component, ...]


def _exact_keys(value: dict[str, Any], allowed: set[str], label: str) -> None:
    unknown = set(value) - allowed
    missing = allowed - set(value)
    if unknown or missing:
        raise ComponentModelError(
            f"{label} keys mismatch: missing={sorted(missing)}, unknown={sorted(unknown)}"
        )


def _nonempty_string(value: object, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ComponentModelError(f"{label} must be a non-empty string")
    return value


def _validate_provenance(value: object, root: Path, label: str) -> tuple[dict[str, str], ...]:
    if value is None:
        return ()
    if not isinstance(value, list):
        raise ComponentModelError(f"{label} must be an array")
    records: list[dict[str, str]] = []
    for index, record in enumerate(value):
        if not isinstance(record, dict):
            raise ComponentModelError(f"{label}[{index}] must be an object")
        _exact_keys(record, {"path", "symbol"}, f"{label}[{index}]")
        path = _nonempty_string(record["path"], f"{label}[{index}].path")
        source = Path(path)
        if source.is_absolute() or ".." in source.parts:
            raise ComponentModelError(f"{label}[{index}].path must stay inside the repository")
        if not (root / source).is_file():
            raise ComponentModelError(f"{label}[{index}].path does not exist: {path}")
        records.append(
            {
                "path": path,
                "symbol": _nonempty_string(record["symbol"], f"{label}[{index}].symbol"),
            }
        )
    return tuple(records)


def _integer(value: object, label: str, *, minimum: int = 0, maximum: int | None = None) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < minimum:
        raise ComponentModelError(f"{label} must be an integer >= {minimum}")
    if maximum is not None and value > maximum:
        raise ComponentModelError(f"{label} must be <= {maximum}")
    return value


def _hex_bytes(value: object, label: str) -> bytes:
    if not isinstance(value, str):
        raise ComponentModelError(f"{label} must be a hex string")
    compact = value.replace(" ", "").replace("\n", "")
    if not compact or len(compact) % 2:
        raise ComponentModelError(f"{label} must contain complete bytes")
    try:
        return bytes.fromhex(compact)
    except ValueError as error:
        raise ComponentModelError(f"{label} contains invalid hex") from error


def _validate_protocol_payload(component_type: str, payload: dict[str, Any], label: str) -> None:
    if component_type == "protocol_fields":
        _exact_keys(payload, {"fixture", "fields"}, f"{label}.payload")
        fixture = _hex_bytes(payload["fixture"], f"{label}.payload.fixture")
        if not isinstance(payload["fields"], list) or not payload["fields"]:
            raise ComponentModelError(f"{label}.payload.fields must be a non-empty array")
        ids: set[str] = set()
        for index, field in enumerate(payload["fields"]):
            field_label = f"{label}.payload.fields[{index}]"
            if not isinstance(field, dict):
                raise ComponentModelError(f"{field_label} must be an object")
            _exact_keys(field, {"id", "label", "offset", "length", "value", "meaning"}, field_label)
            field_id = _nonempty_string(field["id"], f"{field_label}.id")
            if field_id in ids:
                raise ComponentModelError(f"{label}: duplicate field id {field_id!r}")
            offset = _integer(field["offset"], f"{field_label}.offset")
            length = _integer(field["length"], f"{field_label}.length", minimum=1)
            if offset + length > len(fixture):
                raise ComponentModelError(f"{field_label} exceeds fixture length")
            _nonempty_string(field["label"], f"{field_label}.label")
            _nonempty_string(field["value"], f"{field_label}.value")
            _nonempty_string(field["meaning"], f"{field_label}.meaning")
            ids.add(field_id)
    elif component_type == "byte_inspector":
        _exact_keys(payload, {"fixture", "bytes_per_row", "annotations"}, f"{label}.payload")
        fixture = _hex_bytes(payload["fixture"], f"{label}.payload.fixture")
        _integer(payload["bytes_per_row"], f"{label}.payload.bytes_per_row", minimum=4, maximum=32)
        annotations = payload["annotations"]
        if not isinstance(annotations, list) or not annotations:
            raise ComponentModelError(f"{label}.payload.annotations must be a non-empty array")
        for index, annotation in enumerate(annotations):
            item_label = f"{label}.payload.annotations[{index}]"
            if not isinstance(annotation, dict):
                raise ComponentModelError(f"{item_label} must be an object")
            _exact_keys(annotation, {"offset", "length", "label"}, item_label)
            offset = _integer(annotation["offset"], f"{item_label}.offset")
            length = _integer(annotation["length"], f"{item_label}.length", minimum=1)
            if offset + length > len(fixture):
                raise ComponentModelError(f"{item_label} exceeds fixture length")
            _nonempty_string(annotation["label"], f"{item_label}.label")
    elif component_type == "checksum_view":
        _exact_keys(payload, {"fixture", "regions", "expected"}, f"{label}.payload")
        fixture = _hex_bytes(payload["fixture"], f"{label}.payload.fixture")
        _integer(payload["expected"], f"{label}.payload.expected", maximum=65535)
        regions = payload["regions"]
        if not isinstance(regions, list) or not regions:
            raise ComponentModelError(f"{label}.payload.regions must be a non-empty array")
        for index, region in enumerate(regions):
            item_label = f"{label}.payload.regions[{index}]"
            if not isinstance(region, dict):
                raise ComponentModelError(f"{item_label} must be an object")
            _exact_keys(region, {"offset", "length", "role"}, item_label)
            offset = _integer(region["offset"], f"{item_label}.offset")
            length = _integer(region["length"], f"{item_label}.length")
            if offset + length > len(fixture):
                raise ComponentModelError(f"{item_label} exceeds fixture length")
            _nonempty_string(region["role"], f"{item_label}.role")


def _labeled_items(value: object, label: str) -> set[str]:
    if not isinstance(value, list) or not value:
        raise ComponentModelError(f"{label} must be a non-empty array")
    ids: set[str] = set()
    for index, item in enumerate(value):
        item_label = f"{label}[{index}]"
        if not isinstance(item, dict):
            raise ComponentModelError(f"{item_label} must be an object")
        _exact_keys(item, {"id", "label"}, item_label)
        item_id = _nonempty_string(item["id"], f"{item_label}.id")
        if item_id in ids:
            raise ComponentModelError(f"{label}: duplicate id {item_id!r}")
        _nonempty_string(item["label"], f"{item_label}.label")
        ids.add(item_id)
    return ids


def _validate_decision_payload(component_type: str, payload: dict[str, Any], label: str) -> None:
    if component_type == "state_machine":
        _exact_keys(payload, {"initial", "states", "events", "transitions"}, f"{label}.payload")
        states = _labeled_items(payload["states"], f"{label}.payload.states")
        events = _labeled_items(payload["events"], f"{label}.payload.events")
        if payload["initial"] not in states:
            raise ComponentModelError(f"{label}.payload.initial must name a state")
        transitions = payload["transitions"]
        if not isinstance(transitions, list) or not transitions:
            raise ComponentModelError(f"{label}.payload.transitions must be a non-empty array")
        for index, transition in enumerate(transitions):
            item_label = f"{label}.payload.transitions[{index}]"
            if not isinstance(transition, dict):
                raise ComponentModelError(f"{item_label} must be an object")
            _exact_keys(transition, {"from", "event", "to"}, item_label)
            if transition["from"] not in states or transition["to"] not in states:
                raise ComponentModelError(f"{item_label} references an unknown state")
            if transition["event"] not in events:
                raise ComponentModelError(f"{item_label} references an unknown event")
    elif component_type == "transition_table":
        _exact_keys(payload, {"columns", "rows"}, f"{label}.payload")
        columns = payload["columns"]
        rows = payload["rows"]
        if (
            not isinstance(columns, list)
            or not columns
            or not all(isinstance(item, str) for item in columns)
        ):
            raise ComponentModelError(f"{label}.payload.columns must be string array")
        if not isinstance(rows, list) or not rows:
            raise ComponentModelError(f"{label}.payload.rows must be non-empty array")
        for index, row in enumerate(rows):
            if not isinstance(row, list) or len(row) != len(columns):
                raise ComponentModelError(f"{label}.payload.rows[{index}] width mismatch")
            for value in row:
                _nonempty_string(value, f"{label}.payload.rows[{index}] value")
    elif component_type == "socket_timeline":
        _exact_keys(payload, {"lanes", "events"}, f"{label}.payload")
        lanes = _labeled_items(payload["lanes"], f"{label}.payload.lanes")
        events = payload["events"]
        if not isinstance(events, list) or not events:
            raise ComponentModelError(f"{label}.payload.events must be non-empty array")
        for index, event in enumerate(events):
            item_label = f"{label}.payload.events[{index}]"
            if not isinstance(event, dict):
                raise ComponentModelError(f"{item_label} must be an object")
            _exact_keys(event, {"lane", "label", "detail"}, item_label)
            if event["lane"] not in lanes:
                raise ComponentModelError(f"{item_label}.lane references unknown lane")
            _nonempty_string(event["label"], f"{item_label}.label")
            _nonempty_string(event["detail"], f"{item_label}.detail")
    elif component_type == "routing_table":
        _exact_keys(payload, {"routes", "cases"}, f"{label}.payload")
        if not isinstance(payload["routes"], list) or not payload["routes"]:
            raise ComponentModelError(f"{label}.payload.routes must be non-empty array")
        for index, route in enumerate(payload["routes"]):
            item_label = f"{label}.payload.routes[{index}]"
            if not isinstance(route, dict):
                raise ComponentModelError(f"{item_label} must be object")
            _exact_keys(route, {"network", "prefix", "next_hop", "interface", "metric"}, item_label)
            _integer(route["prefix"], f"{item_label}.prefix", maximum=32)
            _integer(route["metric"], f"{item_label}.metric")
            _nonempty_string(route["network"], f"{item_label}.network")
            _nonempty_string(route["next_hop"], f"{item_label}.next_hop")
            if not isinstance(route["interface"], (str, int)) or isinstance(
                route["interface"], bool
            ):
                raise ComponentModelError(f"{item_label}.interface must be a string or integer")
        if not isinstance(payload["cases"], list) or not payload["cases"]:
            raise ComponentModelError(f"{label}.payload.cases must be non-empty array")
    elif component_type == "nat_table":
        _exact_keys(payload, {"public_ip", "first_port", "steps"}, f"{label}.payload")
        _nonempty_string(payload["public_ip"], f"{label}.payload.public_ip")
        _integer(payload["first_port"], f"{label}.payload.first_port", minimum=1, maximum=65535)
        steps = payload["steps"]
        if not isinstance(steps, list) or not steps:
            raise ComponentModelError(f"{label}.payload.steps must be non-empty array")
        for index, step in enumerate(steps):
            item_label = f"{label}.payload.steps[{index}]"
            if not isinstance(step, dict):
                raise ComponentModelError(f"{item_label} must be object")
            _exact_keys(step, {"action", "input", "output", "status"}, item_label)
            _nonempty_string(step["action"], f"{item_label}.action")
            _nonempty_string(step["input"], f"{item_label}.input")
            _nonempty_string(step["output"], f"{item_label}.output")
            _nonempty_string(step["status"], f"{item_label}.status")


def load_page_model(path: Path, *, root: Path, catalog_keys: set[str]) -> PageModel:
    """Load and validate one versioned page-component model."""
    try:
        raw = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise ComponentModelError(f"cannot load {path}: {error}") from error
    if not isinstance(raw, dict):
        raise ComponentModelError(f"{path} must contain an object")
    _exact_keys(raw, {"schema_version", "catalog_key", "components"}, str(path))
    if raw["schema_version"] != 1:
        raise ComponentModelError(f"{path}: unsupported schema_version")
    catalog_key = _nonempty_string(raw["catalog_key"], f"{path}.catalog_key")
    if catalog_key not in catalog_keys:
        raise ComponentModelError(f"{path}: unknown catalog_key {catalog_key!r}")
    if not isinstance(raw["components"], list) or not raw["components"]:
        raise ComponentModelError(f"{path}: components must be a non-empty array")

    components: list[Component] = []
    seen: set[str] = set()
    for index, item in enumerate(raw["components"]):
        label = f"{path}.components[{index}]"
        if not isinstance(item, dict):
            raise ComponentModelError(f"{label} must be an object")
        allowed = {"id", "type", "heading", "payload"}
        if "provenance" in item:
            allowed.add("provenance")
        _exact_keys(item, allowed, label)
        component_id = _nonempty_string(item["id"], f"{label}.id")
        if component_id in seen:
            raise ComponentModelError(f"{path}: duplicate component id {component_id!r}")
        if not component_id.replace("-", "").isalnum() or component_id.lower() != component_id:
            raise ComponentModelError(f"{label}.id must be lower-case letters, digits, and hyphens")
        component_type = _nonempty_string(item["type"], f"{label}.type")
        if component_type not in ALLOWED_COMPONENT_TYPES:
            raise ComponentModelError(f"{label}: unsupported component type {component_type!r}")
        if not isinstance(item["payload"], dict):
            raise ComponentModelError(f"{label}.payload must be an object")
        if component_type in {"protocol_fields", "byte_inspector", "checksum_view"}:
            _validate_protocol_payload(component_type, item["payload"], label)
        if component_type in {
            "state_machine",
            "transition_table",
            "socket_timeline",
            "routing_table",
            "nat_table",
        }:
            _validate_decision_payload(component_type, item["payload"], label)
        components.append(
            Component(
                id=component_id,
                type=component_type,
                heading=_nonempty_string(item["heading"], f"{label}.heading"),
                payload=item["payload"],
                provenance=_validate_provenance(
                    item.get("provenance"), root, f"{label}.provenance"
                ),
            )
        )
        seen.add(component_id)
    return PageModel(1, catalog_key, tuple(components))
