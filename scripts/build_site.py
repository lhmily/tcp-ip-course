"""Prepare repository Markdown for the MkDocs course site."""

from __future__ import annotations

import argparse
import json
import re
import shutil
from pathlib import Path

try:
    from scripts.course_catalog import LESSONS
except ModuleNotFoundError:  # Direct script execution adds scripts/, not the repository root.
    from course_catalog import LESSONS

ROOT = Path(__file__).parents[1]
DEFAULT_OUTPUT = ROOT / ".site-docs"
SITE_URL = "https://lhmily.github.io/tcp-ip-course/"
GITHUB_URL = "https://github.com/lhmily/tcp-ip-course"
SOURCE_TO_ROUTE = {lesson.source: lesson.route for lesson in LESSONS}


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


def rewrite_markdown(text: str, *, lesson=None) -> str:
    for source, route in SOURCE_TO_ROUTE.items():
        target = f"../{route.removeprefix('lessons/')}index.md" if lesson else f"{route}index.md"
        text = re.sub(rf"(?:\.\./|lessons/){re.escape(source)}/README\.md", target, text)
    text = text.replace("../../docs/assets/", "../../assets/")
    text = text.replace("(docs/assets/", "(assets/")
    text = text.replace('src="docs/assets/', 'src="assets/')
    for name in ("CONTRIBUTING.md", "CHANGELOG.md", "LICENSE"):
        text = text.replace(f"({name})", f"({GITHUB_URL}/blob/main/{name})")
    if lesson is not None:
        index = lesson.number - 1
        links = ["[Course overview](../../index.md)"]
        if index:
            previous = LESSONS[index - 1]
            links.append(
                f"[← Lesson {previous.number}: {previous.title}](../{previous.slug}/index.md)"
            )
        if index + 1 < len(LESSONS):
            following = LESSONS[index + 1]
            links.append(
                f"[Lesson {following.number}: {following.title} →](../{following.slug}/index.md)"
            )
        text += "\n\n---\n\n" + " · ".join(links) + "\n"
    return text


def prepare(output: Path = DEFAULT_OUTPUT) -> None:
    output = output.resolve()
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    homepage_description = "Learn TCP/IP from bytes to diagnostics in 12 tested C17 lessons using CMake, CTest, and POSIX sockets."
    homepage = rewrite_markdown((ROOT / "README.md").read_text())
    (output / "index.md").write_text(
        front_matter("TCP/IP Course in C17", homepage_description, "Course") + homepage
    )

    for lesson in LESSONS:
        source = ROOT / "lessons" / lesson.source / "README.md"
        if not source.is_file():
            raise FileNotFoundError(f"missing lesson document: {source.relative_to(ROOT)}")
        destination = output / "lessons" / lesson.slug / "index.md"
        destination.parent.mkdir(parents=True)
        destination.write_text(
            front_matter(lesson.title, lesson.description, "LearningResource")
            + rewrite_markdown(source.read_text(), lesson=lesson)
        )

    shutil.copytree(ROOT / "docs" / "assets", output / "assets")
    shutil.copytree(ROOT / "docs" / "stylesheets", output / "stylesheets")
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
    print(f"prepared {len(LESSONS)} lessons in {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
