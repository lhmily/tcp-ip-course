from __future__ import annotations

import json
import os
import re
import struct
import subprocess
import sys
import zlib
from pathlib import Path
from urllib.parse import urlsplit
from xml.etree import ElementTree

import pytest

from scripts.build_site import DEFAULT_OUTPUT, SITE_URL, prepare
from scripts.course_catalog import LESSONS, LINUX_LABS

ROOT = Path(__file__).parents[1]
LESSON_DOCUMENTS = [ROOT / "lessons" / lesson.source / "README.md" for lesson in LESSONS]
LINUX_LAB_DOCUMENTS = [ROOT / "linux_labs" / lab.source / "README.md" for lab in LINUX_LABS]
DOCUMENTS = [ROOT / "README.md", *LESSON_DOCUMENTS]


def source_documents() -> list[Path]:
    documents = list(DOCUMENTS)
    if linux_lab_documents_exist():
        documents.append(ROOT / "linux_labs" / "README.md")
        documents.extend(LINUX_LAB_DOCUMENTS)
    return documents


def linux_lab_documents_exist() -> bool:
    return all(path.is_file() for path in LINUX_LAB_DOCUMENTS)


def site_pages(output: Path) -> list[Path]:
    pages = [
        output / "index.html",
        *(output / "lessons" / lesson.slug / "index.html" for lesson in LESSONS),
        output / "linux-labs" / "index.html",
    ]
    if linux_lab_documents_exist():
        pages.extend(output / "linux-labs" / lab.slug / "index.html" for lab in LINUX_LABS)
    return pages


def require_lesson_documents() -> None:
    if not all(path.is_file() for path in LESSON_DOCUMENTS):
        pytest.skip("lesson documentation is supplied by parallel lesson branches")


def mermaid_blocks(text: str) -> list[str]:
    return re.findall(r"```mermaid\s*\n(.*?)```", text, flags=re.DOTALL)


def markdown_targets(text: str) -> list[str]:
    return re.findall(r"!?\[[^]]*]\(([^)]+)\)", text)


def run_generator(name: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(ROOT / "scripts" / name), "--check"],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )


def build_site(output: Path, *, token: str | None = None) -> subprocess.CompletedProcess[str]:
    prepare(DEFAULT_OUTPUT)
    environment = os.environ.copy()
    if token is None:
        environment.pop("CLOUDFLARE_WEB_ANALYTICS_TOKEN", None)
    else:
        environment["CLOUDFLARE_WEB_ANALYTICS_TOKEN"] = token
    return subprocess.run(
        [sys.executable, "-m", "mkdocs", "build", "--strict", "--site-dir", str(output)],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        env=environment,
    )


def test_documents_have_unique_valid_visuals():
    require_lesson_documents()
    seen: set[str] = set()
    walkthrough = ROOT / "linux_labs" / "04_kernel_source_walkthrough" / "README.md"
    for path in source_documents():
        text = path.read_text()
        blocks = mermaid_blocks(text)
        if path == walkthrough:
            assert not blocks
            for stem in ("ingress", "egress", "ownership", "uapi-boundary"):
                for theme in ("light", "dark"):
                    asset = (
                        ROOT
                        / "docs"
                        / "assets"
                        / "kernel-source-walkthrough"
                        / f"{stem}-{theme}.svg"
                    )
                    assert asset.is_file()
                    svg = asset.read_text()
                    assert "<title" in svg and "<desc" in svg
            continue
        assert blocks, f"missing Mermaid diagram: {path}"
        for block in blocks:
            body = block.strip()
            assert body.startswith(("flowchart ", "graph ", "sequenceDiagram", "stateDiagram"))
            assert body.count("[") == body.count("]")
            assert body.count("(") == body.count(")")
            normalized = re.sub(r"\s+", " ", body)
            assert normalized not in seen, f"duplicate diagram: {path}"
            seen.add(normalized)


def test_relative_links_and_images_resolve_locally():
    require_lesson_documents()
    for document in source_documents():
        for raw_target in markdown_targets(document.read_text()):
            target = raw_target.split("#", 1)[0]
            if not target:
                continue
            if target.startswith(("http://", "https://")):
                if raw_target.lower().endswith((".png", ".jpg", ".jpeg", ".gif", ".svg")):
                    raise AssertionError(f"external image in {document}: {raw_target}")
                continue
            resolved = (document.parent / target).resolve()
            assert resolved.is_relative_to(ROOT.resolve()), (document, target)
            if (
                target.startswith(("linux_labs/", "../../linux_labs/"))
                and not linux_lab_documents_exist()
            ):
                continue
            assert resolved.exists(), (document, target)


def test_overviews_link_tracks_in_catalog_order():
    root_text = (ROOT / "README.md").read_text()
    linked = re.findall(r"\(lessons/(\d{2}_[^/]+)/README\.md\)", root_text)
    assert linked == [lesson.source for lesson in LESSONS]
    assert "(linux_labs/README.md)" in root_text
    assert not re.findall(r"\(linux_labs/(\d{2}_[^/]+)/README\.md\)", root_text)

    linux_text = (ROOT / "linux_labs" / "README.md").read_text()
    positions = [linux_text.index(f"({lab.source}/README.md)") for lab in LINUX_LABS]
    assert positions == sorted(positions)


def test_generated_assets_are_reproducible_and_accessible():
    for script in (
        "generate_branding_assets.py",
        "generate_documentation_assets.py",
        "generate_kernel_walkthrough.py",
    ):
        result = run_generator(script)
        assert result.returncode == 0, result.stdout + result.stderr

    overview = (ROOT / "docs" / "assets" / "tcp-ip-overview.svg").read_text()
    assert "<title>" in overview and "<desc>" in overview
    social = (ROOT / "docs" / "assets" / "tcp-ip-social.png").read_bytes()
    assert social.startswith(b"\x89PNG\r\n\x1a\n")
    assert struct.unpack(">II", social[16:24]) == (1200, 630)

    branding = ROOT / "docs" / "assets" / "branding"
    assert "<title>" in (branding / "lemon.svg").read_text()
    expected_png_sizes = {
        "favicon.png": 32,
        "apple-touch-icon.png": 180,
        "icon-192.png": 192,
        "icon-512.png": 512,
    }
    for name, expected_size in expected_png_sizes.items():
        data = (branding / name).read_bytes()
        assert data.startswith(b"\x89PNG\r\n\x1a\n")
        assert struct.unpack(">II", data[16:24]) == (expected_size, expected_size)
        assert zlib.crc32(data) != 0
    assert struct.unpack("<HHH", (branding / "favicon.ico").read_bytes()[:6]) == (0, 1, 3)


def test_mkdocs_navigation_is_not_duplicated_in_yaml():
    config = (ROOT / "mkdocs.yml").read_text()
    assert "hooks:\n  - scripts/mkdocs_hooks.py" in config
    assert not re.search(r"^nav:", config, re.MULTILINE)
    assert all(f"lessons/{lesson.slug}/index.md" not in config for lesson in LESSONS)
    assert all(f"linux-labs/{lab.slug}/index.md" not in config for lab in LINUX_LABS)


def test_built_site_metadata_sitemap_manifest_and_links(tmp_path):
    require_lesson_documents()
    output = tmp_path / "site"
    result = build_site(output)
    assert result.returncode == 0, result.stdout + result.stderr
    pages = site_pages(output)
    titles: set[str] = set()
    for page in pages:
        text = page.read_text()
        title = re.search(r"<title>(.*?)</title>", text, re.DOTALL)
        descriptions = re.findall(r'<meta name="description" content="([^"]+)">', text)
        canonicals = re.findall(r'<link rel="canonical" href="([^"]+)">', text)
        structured = re.search(
            r'<script type="application/ld\+json">\s*(.*?)\s*</script>', text, re.DOTALL
        )
        assert title and title.group(1) not in titles
        titles.add(title.group(1))
        assert len(descriptions) == 1 and 50 <= len(descriptions[0]) <= 160
        assert len(canonicals) == 1 and canonicals[0].startswith(SITE_URL)
        assert 'content="index, follow"' in text
        assert 'href="https://lhmily.github.io/"' in text
        assert "assets/branding/lemon.svg" in text
        assert "site.webmanifest" in text
        assert "og:title" in text and "twitter:card" in text
        assert "assets/tcp-ip-social.png" in text
        assert 'property="og:image:type" content="image/png"' in text
        assert 'property="og:image:width" content="1200"' in text
        assert 'property="og:image:height" content="630"' in text
        assert structured
        data = json.loads(structured.group(1))
        assert data["@type"] in {"Course", "LearningResource"}
        assert data["programmingLanguage"] == "C17"
        assert "static.cloudflareinsights.com/beacon.min.js" not in text

    manifest = json.loads((output / "site.webmanifest").read_text())
    assert manifest["start_url"] == "/tcp-ip-course/"
    assert manifest["scope"] == "/tcp-ip-course/"
    assert len(manifest["icons"]) == 2
    assert (
        output / "robots.txt"
    ).read_text() == f"User-agent: *\nAllow: /\nSitemap: {SITE_URL}sitemap.xml\n"

    namespace = {"sitemap": "http://www.sitemaps.org/schemas/sitemap/0.9"}
    sitemap = ElementTree.parse(output / "sitemap.xml")
    locations = {node.text for node in sitemap.findall("sitemap:url/sitemap:loc", namespace)}
    expected = {
        SITE_URL,
        *(f"{SITE_URL}lessons/{lesson.slug}/" for lesson in LESSONS),
        f"{SITE_URL}linux-labs/",
    }
    if linux_lab_documents_exist():
        expected.update(f"{SITE_URL}{lab.route}" for lab in LINUX_LABS)
    assert expected <= locations

    broken: list[tuple[Path, str]] = []
    for page in pages:
        for raw_target in re.findall(r'(?:href|src)="([^"]+)"', page.read_text()):
            parsed = urlsplit(raw_target)
            if parsed.scheme or raw_target.startswith(("#", "mailto:", "javascript:", "data:")):
                continue
            path = parsed.path.removeprefix("/tcp-ip-course/")
            resolved = (
                output / path if parsed.path.startswith("/tcp-ip-course/") else page.parent / path
            )
            if path.endswith("/") or not resolved.suffix:
                resolved /= "index.html"
            if not resolved.exists():
                broken.append((page.relative_to(output), raw_target))
    assert not broken


def test_linux_track_pages_routes_navigation_and_links_when_present(tmp_path):
    if not linux_lab_documents_exist():
        pytest.skip("numbered Linux lab documentation is supplied by parallel lab branches")
    output = tmp_path / "site"
    result = build_site(output)
    assert result.returncode == 0, result.stdout + result.stderr
    overview = output / "linux-labs" / "index.html"
    assert overview.is_file()
    overview_text = overview.read_text()
    assert "Optional Linux track" in overview_text
    for index, lab in enumerate(LINUX_LABS):
        page = output / "linux-labs" / lab.slug / "index.html"
        text = page.read_text()
        assert lab.title in text
        assert f'<link rel="canonical" href="{SITE_URL}{lab.route}">' in text
        assert "static.cloudflareinsights.com/beacon.min.js" not in text
        if index:
            assert f"../{LINUX_LABS[index - 1].slug}/" in text
        if index + 1 < len(LINUX_LABS):
            assert f"../{LINUX_LABS[index + 1].slug}/" in text


def test_kernel_walkthrough_visual_explorer_and_source_table(tmp_path):
    output = tmp_path / "site"
    result = build_site(output)
    assert result.returncode == 0, result.stdout + result.stderr

    model = json.loads((ROOT / "docs" / "data" / "linux-v6.6-walkthrough.json").read_text())
    page = output / "linux-labs" / "kernel-source-walkthrough" / "index.html"
    text = page.read_text()
    staged_walkthrough = DEFAULT_OUTPUT / "linux-labs" / "kernel-source-walkthrough" / "index.md"
    staged_lesson = DEFAULT_OUTPUT / "lessons" / LESSONS[0].slug / "index.md"

    assert 'template: "kernel-walkthrough.html"' in staged_walkthrough.read_text()
    assert 'template: "kernel-walkthrough.html"' not in staged_lesson.read_text()
    assert text.count('id="__drawer"') == 1
    assert text.count('data-md-component="header"') == 1
    assert 'data-md-component="search"' in text
    assert text.count('data-md-type="navigation"') == 1
    assert "md-sidebar--secondary" not in text
    assert "md-content--kernel-walkthrough" in text
    assert "kernel-walkthrough-page" in text
    assert text.count('<footer class="md-footer">') == 1
    assert f'<link rel="canonical" href="{SITE_URL}linux-labs/kernel-source-walkthrough/">' in text
    assert '<meta property="og:title" content="Kernel Source Walkthrough">' in text
    assert '<meta name="twitter:title" content="Kernel Source Walkthrough">' in text

    ordinary = (output / "lessons" / LESSONS[0].slug / "index.html").read_text()
    assert "md-content--kernel-walkthrough" not in ordinary
    assert "kernel-walkthrough-page" not in ordinary
    assert "md-content--course-page" in ordinary
    assert "course-page" in ordinary
    assert "md-sidebar--secondary" not in ordinary
    assert 'data-md-component="search"' in ordinary

    assert "L04_INTERACTIVE_EXPLORER" not in text
    assert "L04_SOURCE_TABLE" not in text
    assert "```mermaid" not in text
    assert text.count('src="../../javascripts/kernel-walkthrough.js"') == 1
    assert "https://cdn" not in text and "unpkg.com" not in text
    assert "data-kernel-walkthrough" in text
    assert 'class="kw-route' in text
    assert '<section class="kw-route" data-route-list="ingress" hidden' not in text
    assert '<section class="kw-route" data-route-list="egress" hidden' not in text
    table_match = re.search(
        r'<div class="kernel-source-table".*?<tbody>(.*?)</tbody></table></div>',
        text,
        re.DOTALL,
    )
    assert table_match
    assert table_match.group(1).count("<tr>") == len(model["symbols"])
    assert text.count("https://github.com/torvalds/linux/blob/v6.6/") >= len(model["symbols"])

    payload_match = re.search(
        r'<script type="application/json" data-walkthrough-data>(.*?)</script>',
        text,
        re.DOTALL,
    )
    assert payload_match
    payload = json.loads(payload_match.group(1))
    assert payload["routes"] == model["routes"]
    assert payload["edges"] == model["edges"]
    assert payload["uapi_boundary"] == model["uapi_boundary"]
    assert len(payload["symbols"]) == 30

    assert len(re.findall(r'data-route-name="ingress"', text)) == len(model["routes"]["ingress"])
    assert len(re.findall(r'data-route-name="egress"', text)) == len(model["routes"]["egress"])
    for stem in ("ingress", "egress", "ownership", "uapi-boundary"):
        for theme in ("light", "dark"):
            assert f"assets/kernel-source-walkthrough/{stem}-{theme}.svg" in text

    for other_page in site_pages(output):
        if other_page == page:
            continue
        assert "kernel-walkthrough.js" not in other_page.read_text()

    search_index = json.loads((output / "search" / "search_index.json").read_text())
    walkthrough_entries = [
        entry
        for entry in search_index["docs"]
        if "linux-labs/kernel-source-walkthrough/" in entry.get("location", "")
    ]
    indexed_text = " ".join(
        f"{entry.get('title', '')} {entry.get('text', '')}" for entry in walkthrough_entries
    )
    assert walkthrough_entries
    assert "tcp_ack" in indexed_text
    assert "ip_rcv" in indexed_text
    assert "Ownership" in indexed_text


def test_cloudflare_analytics_is_optional_and_escaped(tmp_path):
    require_lesson_documents()
    output = tmp_path / "site"
    result = build_site(output, token="synthetic-test-token")
    assert result.returncode == 0, result.stdout + result.stderr
    pages = site_pages(output)
    for page in pages:
        beacons = re.findall(
            r'<script defer src="https://static\.cloudflareinsights\.com/beacon\.min\.js"\s+'
            r"data-cf-beacon='([^']+)'></script>",
            page.read_text(),
        )
        assert len(beacons) == 1
        assert json.loads(beacons[0]) == {"token": "synthetic-test-token"}
