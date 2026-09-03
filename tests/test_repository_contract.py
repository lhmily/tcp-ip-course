from __future__ import annotations

import re
from pathlib import Path

import pytest

from scripts.course_catalog import LESSONS, SECTIONS

ROOT = Path(__file__).parents[1]
LESSON_ROOT = ROOT / "lessons"
REQUIRED_FILES = {"README.md", "lesson.h", "exercise.c", "solution.c", "test.c", "CMakeLists.txt"}
REQUIRED_HEADINGS = {
    "## Learning objectives",
    "## Prerequisites",
    "## Mental model",
    "## Wire format or API",
    "## Algorithm and state transitions",
    "## Worked C example",
    "## Exercise",
    "## Test contract and invariants",
    "## Common mistakes",
    "## Safety and network boundaries",
    "## Further experiments",
    "## Summary",
}


def lesson_directories() -> list[Path]:
    if not LESSON_ROOT.exists():
        return []
    return sorted(
        path for path in LESSON_ROOT.iterdir() if path.is_dir() and re.match(r"^\d{2}_", path.name)
    )


def implementation_sources() -> list[Path]:
    return [
        path
        for path in [
            *LESSON_ROOT.glob("**/*.c"),
            *LESSON_ROOT.glob("**/*.h"),
            *ROOT.glob("include/**/*.h"),
            *ROOT.glob("tests/*.c"),
        ]
        if path.is_file()
    ]


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//.*", "", text)


def test_catalog_is_exact_and_grouped():
    expected_sources = [
        "01_bytes_addressing_checksum",
        "02_ethernet_arp",
        "03_ipv4_packets",
        "04_icmp",
        "05_udp",
        "06_tcp_segments",
        "07_tcp_state_reliability",
        "08_stream_sockets",
        "09_dns",
        "10_http",
        "11_routing_nat",
        "12_diagnostics_integration",
    ]
    expected_slugs = [
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
    ]
    assert [lesson.number for lesson in LESSONS] == list(range(1, 13))
    assert [lesson.source for lesson in LESSONS] == expected_sources
    assert [lesson.slug for lesson in LESSONS] == expected_slugs
    assert SECTIONS == (
        "Foundations",
        "Internet layer",
        "Transport",
        "Applications",
        "Networks in practice",
    )
    assert [lesson.section for lesson in LESSONS] == [
        "Foundations",
        "Foundations",
        "Internet layer",
        "Internet layer",
        "Transport",
        "Transport",
        "Transport",
        "Applications",
        "Applications",
        "Applications",
        "Networks in practice",
        "Networks in practice",
    ]
    assert len({lesson.title for lesson in LESSONS}) == 12
    assert len({lesson.description for lesson in LESSONS}) == 12
    assert all(50 <= len(lesson.description) <= 160 for lesson in LESSONS)


def test_all_twelve_lessons_follow_six_file_contract():
    lessons = lesson_directories()
    if not lessons:
        pytest.skip("numbered lessons are supplied by parallel lesson branches")
    assert [path.name for path in lessons] == [lesson.source for lesson in LESSONS]
    for lesson in lessons:
        assert {path.name for path in lesson.iterdir()} >= REQUIRED_FILES, lesson
        document = (lesson / "README.md").read_text()
        assert set(document.splitlines()) >= REQUIRED_HEADINGS, lesson
        assert len(document.split()) >= 300, lesson
        assert "```c" in document, lesson
        assert "**What to notice:**" in document, lesson
        assert re.search(r"^\|.+\|\n\|(?:\s*:?-+:?\s*\|)+", document, re.MULTILINE), lesson
        assert re.search(r"\bnon-goal\b", document, re.IGNORECASE), lesson


def test_public_header_is_shared_by_both_implementations():
    for lesson in lesson_directories():
        for name in ("exercise.c", "solution.c", "test.c"):
            text = strip_c_comments((lesson / name).read_text())
            assert re.search(r'#\s*include\s*[<"]lesson\.h[>"]', text), (lesson, name)


def test_python_is_tooling_only():
    assert not (ROOT / "src").exists()
    assert not (LESSON_ROOT / "__init__.py").exists()
    assert not list(LESSON_ROOT.glob("**/*.py"))
    pyproject = (ROOT / "pyproject.toml").read_text().lower()
    assert "[project]" not in pyproject
    assert "[build-system]" not in pyproject
    assert "hatchling" not in pyproject
    repository_config = "\n".join(
        path.read_text(errors="ignore").lower()
        for path in [ROOT / "CMakeLists.txt", ROOT / "CMakePresets.json", ROOT / "pyproject.toml"]
    )
    assert "lesson_impl" not in repository_config


def test_c_sources_avoid_unsafe_wire_layout_and_process_execution():
    forbidden_patterns = {
        "packed attribute": r"__attribute__\s*\(\([^)]*packed",
        "packing pragma": r"#\s*pragma\s+pack\b",
        "bitfield": r"\b(?:unsigned|signed|_Bool|bool|u?int(?:8|16|32|64)_t)\s+\w+\s*:\s*\d+",
        "raw socket": r"\bSOCK_RAW\b|\bAF_PACKET\b",
        "packet capture": r"\b(?:pcap(?:_t|_open|_loop|_next|_dispatch)?|scapy|tcpdump|tshark|dumpcap)\b",
        "process execution": r"\b(?:subprocess|exec(?:l|le|lp|lpe|v|ve|vp|vpe)?|system|popen)\s*\(",
        "wildcard bind": r"\bINADDR_ANY\b",
    }
    wire_struct_names = (
        r"(?:ethernet|arp|ipv4|ip|icmp|udp|tcp|dns|http|packet|header|segment|datagram|frame)_\w+"
    )
    wire_cast = re.compile(
        rf"\(\s*(?:const\s+)?struct\s+{wire_struct_names}\s*\*\s*\)\s*\w+", re.IGNORECASE
    )
    for path in [*LESSON_ROOT.glob("**/*.c"), *LESSON_ROOT.glob("**/*.h")]:
        text = strip_c_comments(path.read_text())
        for label, pattern in forbidden_patterns.items():
            assert not re.search(pattern, text, re.IGNORECASE), f"{label}: {path}"
        assert not wire_cast.search(text), f"wire-pointer cast: {path}"


def test_native_socket_tests_stay_on_loopback_and_use_ephemeral_ports():
    address_calls = re.compile(
        r"\b(?:inet_addr|inet_aton|inet_pton|getaddrinfo)\s*\(\s*(?:AF_INET6?\s*,\s*)?"
        r'"((?:\d{1,3}\.){3}\d{1,3}|[0-9a-fA-F:]+)"',
    )
    fixed_listen_port = re.compile(
        r"(?:sin6?_port\s*=\s*htons|bind_loopback\s*\()[^;\n]*\b([1-9]\d*)\b"
    )
    for path in LESSON_ROOT.glob("**/test.c"):
        text = strip_c_comments(path.read_text())
        for address in address_calls.findall(text):
            assert address in {"127.0.0.1", "::1"}, (path, address)
        assert not fixed_listen_port.search(text), path


def test_no_forbidden_network_tooling_anywhere_in_sources():
    names = re.compile(r"\b(?:SOCK_RAW|AF_PACKET|Scapy|tcpdump|tshark|dumpcap)\b", re.IGNORECASE)
    for path in implementation_sources():
        assert not names.search(strip_c_comments(path.read_text(errors="ignore"))), path
