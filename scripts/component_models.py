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
