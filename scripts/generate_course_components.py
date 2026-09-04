"""Validate and deterministically summarize structured course page components."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

try:
    from scripts.component_models import load_page_model
    from scripts.course_catalog import page_identities
except ModuleNotFoundError:
    from component_models import load_page_model  # type: ignore[no-redef]
    from course_catalog import page_identities  # type: ignore[no-redef]

ROOT = Path(__file__).parents[1]
MODELS = ROOT / "docs" / "data" / "course-pages"
OUTPUT = ROOT / "docs" / "assets" / "course-components" / "manifest.json"


def expected_manifest() -> bytes:
    identities = {page.key: page for page in page_identities()}
    pages = []
    for path in sorted(MODELS.glob("*.json")):
        model = load_page_model(path, root=ROOT, catalog_keys=set(identities))
        identity = identities[model.catalog_key]
        pages.append(
            {
                "catalog_key": model.catalog_key,
                "route": identity.route,
                "component_ids": [component.id for component in model.components],
                "component_types": [component.type for component in model.components],
            }
        )
    if not pages:
        raise SystemExit("no structured course page models found")
    return (json.dumps({"schema_version": 1, "pages": pages}, indent=2) + "\n").encode()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = expected_manifest()
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_bytes() != expected:
            print(f"out-of-date component manifest: {OUTPUT.relative_to(ROOT)}")
            return 1
        print("checked structured course component manifest")
        return 0
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(expected)
    print(f"wrote {OUTPUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
