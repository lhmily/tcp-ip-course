"""Prepare repository Markdown for the MkDocs course site."""

from __future__ import annotations

import argparse
import json
import re
import shutil
from pathlib import Path

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
    content = rewrite_markdown(source.read_text(), page=page)
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
