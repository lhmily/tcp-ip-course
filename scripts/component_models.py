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
