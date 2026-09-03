"""MkDocs hooks that derive track navigation from the course catalog."""

from __future__ import annotations

import sys
from pathlib import Path

SCRIPTS = Path(__file__).parent
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))

from course_catalog import LINUX_LABS, grouped_lessons  # noqa: E402


def on_config(config):
    """Populate navigation without duplicating catalog metadata in YAML."""
    nav: list[dict[str, object]] = [
        {"lhmily Home ↗": "https://lhmily.github.io/"},
        {"Course overview": "index.md"},
    ]
    for section, lessons in grouped_lessons():
        nav.append(
            {section: [{lesson.title: f"lessons/{lesson.slug}/index.md"} for lesson in lessons]}
        )

    docs_dir = Path(config["docs_dir"])
    linux_nav: list[dict[str, str]] = [
        {"Overview": "linux-labs/index.md"},
    ]
    linux_nav.extend(
        {lab.title: f"linux-labs/{lab.slug}/index.md"}
        for lab in LINUX_LABS
        if (docs_dir / "linux-labs" / lab.slug / "index.md").is_file()
    )
    nav.append({"Optional Linux track": linux_nav})
    config["nav"] = nav
    return config
