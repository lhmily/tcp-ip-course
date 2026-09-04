from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

import pytest

from scripts.build_site import prepare
from scripts.component_models import ComponentModelError, load_page_model
from scripts.component_renderers import render_component
from scripts.course_catalog import page_identities
from scripts.generate_course_components import OUTPUT, expected_manifest

ROOT = Path(__file__).parents[1]
MODELS = ROOT / "docs" / "data" / "course-pages"
START = re.compile(r"<!-- COURSE_COMPONENT:([a-z0-9-]+) START -->")
END = re.compile(r"<!-- COURSE_COMPONENT:([a-z0-9-]+) END -->")


def source_for(key: str) -> Path:
    if key == "course-overview":
        return ROOT / "README.md"
    if key == "linux-overview":
        return ROOT / "linux_labs" / "README.md"
    for page in page_identities():
        if page.key != key:
            continue
        prefix = "lessons" if page.track == "core" else "linux_labs"
        directories = sorted((ROOT / prefix).glob(f"*_{key.replace('-', '_')}"))
        assert len(directories) == 1, (key, directories)
        return directories[0] / "README.md"
    raise AssertionError(key)


def test_all_eighteen_page_identities_have_models():
    identities = page_identities()
    assert len(identities) == 18
    assert [page.key for page in identities] == [
        "course-overview",
        "bytes-addressing-checksum",
        "ethernet-arp",
        "ipv4-packets",
        "icmp",
        "udp",
        "tcp-segments",
        "tcp-state-reliability",
        "stream-sockets",
        "dns",
        "http",
        "routing-nat",
        "diagnostics-integration",
        "linux-overview",
        "epoll-event-loop",
        "tcp-info",
        "userspace-mini-stack",
        "kernel-source-walkthrough",
    ]
    assert len({page.route for page in identities}) == 18
    assert len({page.title for page in identities}) == 18
    assert {path.stem for path in MODELS.glob("*.json")} == {page.key for page in identities}


def test_models_match_catalog_and_markdown_markers():
    identities = {page.key: page for page in page_identities()}
    model_paths = sorted(MODELS.glob("*.json"))
    assert model_paths
    for path in model_paths:
        model = load_page_model(path, root=ROOT, catalog_keys=set(identities))
        assert path.stem == model.catalog_key
        source = source_for(model.catalog_key)
        text = source.read_text()
        starts = START.findall(text)
        ends = END.findall(text)
        declared = [component.id for component in model.components]
        assert starts == ends
        assert sorted(starts) == sorted(declared)
        assert len(starts) == len(set(starts))
        for component in model.components:
            start = f"<!-- COURSE_COMPONENT:{component.id} START -->"
            end = f"<!-- COURSE_COMPONENT:{component.id} END -->"
            fallback = text.split(start, 1)[1].split(end, 1)[0]
            assert fallback.strip()
            rendered = render_component(component)
            assert f'id="component-{component.id}"' in rendered
            assert component.heading in rendered


def test_all_built_pages_render_structured_components(tmp_path):
    prepare()
    output = tmp_path / "site"
    result = subprocess.run(
        [sys.executable, "-m", "mkdocs", "build", "--strict", "--site-dir", str(output)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    manifest = json.loads(OUTPUT.read_text())
    expected_counts = {page["route"]: len(page["component_ids"]) for page in manifest["pages"]}
    identities = {page.key: page for page in page_identities()}
    for key, identity in identities.items():
        page = output / identity.route / "index.html"
        text = page.read_text()
        assert "COURSE_COMPONENT:" not in text
        assert text.count("data-course-component=") == expected_counts[identity.route]
        assert text.count("<h1") == 1, key
        ids = set(re.findall(r'\bid="([^"]+)"', text))
        assert all(fragment in ids for fragment in re.findall(r'href="#([^"]+)"', text)), key
        assert 'data-md-component="search"' in text
        assert "fonts.googleapis.com" not in text
        assert "fonts.gstatic.com" not in text
        if key == "kernel-source-walkthrough":
            assert "md-content--kernel-walkthrough" in text
            assert text.count("kernel-walkthrough.js") == 1
        else:
            assert "md-content--course-page" in text
            assert text.count("course-components.js") == 1

    search = json.loads((output / "search" / "search_index.json").read_text())
    indexed_routes = {
        entry.get("location", "").split("#", 1)[0]
        for entry in search["docs"]
        if entry.get("location")
    }
    assert all(identity.route in indexed_routes for identity in identities.values())


def component_payload(page_key: str, component_type: str) -> dict[str, object]:
    model = json.loads((MODELS / f"{page_key}.json").read_text())
    matches = [
        component["payload"]
        for component in model["components"]
        if component["type"] == component_type
    ]
    assert len(matches) == 1, (page_key, component_type)
    return matches[0]


def test_protocol_models_match_vetted_native_vectors():
    expected = {
        "bytes-addressing-checksum": ("aa1234bb", 0xFBFD),
        "ethernet-arp": (
            "ffffffffffff02005e1000000806000108000604000102005e100000c0000201000000000000c0000263",
            None,
        ),
        "ipv4-packets": (
            "452e00181234400040113c3cc0000201c6336402deadbeef",
            0x3C3C,
        ),
        "icmp": ("0800bbfd123400076162636465", 0xBBFD),
        "udp": ("30390035000db9676162636465", 0xB967),
        "tcp-segments": (
            "c000005012345678000000007002faf072fa0000020405b401010402",
            0x72FA,
        ),
        "dns": (
            "beef81800001000100000000076578616d706c6503636f6d0000010001"
            "c00c000100010000003c0004c0000201",
            None,
        ),
    }
    for page_key, (fixture, checksum) in expected.items():
        fields = component_payload(page_key, "protocol_fields")
        assert fields["fixture"].replace(" ", "") == fixture
        model = json.loads((MODELS / f"{page_key}.json").read_text())
        if any(component["type"] == "byte_inspector" for component in model["components"]):
            inspector = component_payload(page_key, "byte_inspector")
            assert inspector["fixture"].replace(" ", "") == fixture
        if checksum is not None:
            checksum_payload = component_payload(page_key, "checksum_view")
            assert checksum_payload["expected"] == checksum


def test_state_timeline_and_routing_models_match_native_contracts():
    state = component_payload("tcp-state-reliability", "state_machine")
    transitions = component_payload("tcp-state-reliability", "transition_table")
    assert len(state["states"]) == 10
    assert len(state["events"]) == 9
    assert len(state["transitions"]) == 15
    assert len(transitions["rows"]) == 15
    assert {row["from"] for row in state["transitions"]} >= {"closed", "established"}
    assert {(row["from"], row["event"], row["to"]) for row in state["transitions"]} >= {
        ("listen", "app-close", "closed"),
        ("time-wait", "timeout", "closed"),
    }

    stream = component_payload("stream-sockets", "socket_timeline")
    http = component_payload("http", "socket_timeline")
    assert len(stream["events"]) >= 9
    assert any("stream" in event["detail"] for event in stream["events"])
    assert any("Content-Length" in event["detail"] for event in http["events"])
    assert any("TRUNCATED" in event["detail"] for event in http["events"])

    routes = component_payload("routing-nat", "routing_table")
    assert len(routes["routes"]) == 6
    assert routes["routes"][0] == {
        "network": "0.0.0.0",
        "prefix": 0,
        "next_hop": "192.0.2.1",
        "interface": 1,
        "metric": 100,
    }
    selected_by_destination = {case["destination"]: case["selected"] for case in routes["cases"]}
    assert selected_by_destination["10.23.42.99"].startswith("route 5")
    assert selected_by_destination["10.23.7.9"].startswith("route 3")
    model = json.loads((MODELS / "routing-nat.json").read_text())
    nat_components = [item for item in model["components"] if item["type"] == "nat_table"]
    assert len(nat_components) == 3
    assert {item["payload"]["first_port"] for item in nat_components} == {40000, 45000, 50000}


def test_component_manifest_is_current():
    assert OUTPUT.is_file()
    assert OUTPUT.read_bytes() == expected_manifest()
    manifest = json.loads(OUTPUT.read_text())
    assert manifest["schema_version"] == 1
    assert {page["catalog_key"] for page in manifest["pages"]} == {
        path.stem for path in MODELS.glob("*.json")
    }


def test_renderers_escape_untrusted_text(tmp_path):
    path = tmp_path / "model.json"
    path.write_text(
        json.dumps(
            {
                "schema_version": 1,
                "catalog_key": "course-overview",
                "components": [
                    {
                        "id": "escape-check",
                        "type": "page_hero",
                        "heading": "<script>alert(1)</script>",
                        "payload": {
                            "title": "<unsafe>",
                            "summary": "A & B",
                            "badges": ["<badge>"],
                            "actions": [{"label": "Open", "href": "#safe"}],
                        },
                    }
                ],
            }
        )
    )
    model = load_page_model(path, root=ROOT, catalog_keys={"course-overview"})
    rendered = render_component(model.components[0])
    assert "<script>" not in rendered
    assert "&lt;script&gt;" in rendered
    assert "&lt;unsafe&gt;" in rendered
    assert "A &amp; B" in rendered


@pytest.mark.parametrize("bad_id", ["Bad", "has space", "under_score", "../escape"])
def test_model_rejects_unsafe_component_ids(tmp_path, bad_id):
    path = tmp_path / "bad.json"
    path.write_text(
        json.dumps(
            {
                "schema_version": 1,
                "catalog_key": "course-overview",
                "components": [
                    {
                        "id": bad_id,
                        "type": "safety_boundary",
                        "heading": "Safety",
                        "payload": {"allowed": ["local"], "excluded": ["external"]},
                    }
                ],
            }
        )
    )
    with pytest.raises(ComponentModelError):
        load_page_model(path, root=ROOT, catalog_keys={"course-overview"})
