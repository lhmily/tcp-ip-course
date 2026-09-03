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
from scripts.course_catalog import LESSONS

ROOT = Path(__file__).parents[1]
LESSON_DOCUMENTS = [ROOT / "lessons" / lesson.source / "README.md" for lesson in LESSONS]
DOCUMENTS = [ROOT / "README.md", *LESSON_DOCUMENTS]


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


def test_documents_have_unique_valid_mermaid_diagrams():
    require_lesson_documents()
    seen: set[str] = set()
    for path in DOCUMENTS:
        blocks = mermaid_blocks(path.read_text())
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
    for document in DOCUMENTS:
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
            assert resolved.exists(), (document, target)


def test_root_links_to_all_lessons_in_catalog_order():
    linked = re.findall(r"\(lessons/(\d{2}_[^/]+)/README\.md\)", (ROOT / "README.md").read_text())
    assert linked == [lesson.source for lesson in LESSONS]


def test_generated_assets_are_reproducible_and_accessible():
    for script in ("generate_branding_assets.py", "generate_documentation_assets.py"):
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


def test_built_site_metadata_sitemap_manifest_and_links(tmp_path):
    require_lesson_documents()
    output = tmp_path / "site"
    result = build_site(output)
    assert result.returncode == 0, result.stdout + result.stderr
    pages = [
        output / "index.html",
        *(output / "lessons" / lesson.slug / "index.html" for lesson in LESSONS),
    ]
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
    expected = {SITE_URL, *(f"{SITE_URL}lessons/{lesson.slug}/" for lesson in LESSONS)}
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


def test_cloudflare_analytics_is_optional_and_escaped(tmp_path):
    require_lesson_documents()
    output = tmp_path / "site"
    result = build_site(output, token="synthetic-test-token")
    assert result.returncode == 0, result.stdout + result.stderr
    pages = [
        output / "index.html",
        *(output / "lessons" / lesson.slug / "index.html" for lesson in LESSONS),
    ]
    for page in pages:
        beacons = re.findall(
            r'<script defer src="https://static\.cloudflareinsights\.com/beacon\.min\.js"\s+'
            r"data-cf-beacon='([^']+)'></script>",
            page.read_text(),
        )
        assert len(beacons) == 1
        assert json.loads(beacons[0]) == {"token": "synthetic-test-token"}
