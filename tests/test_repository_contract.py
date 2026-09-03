from __future__ import annotations

import re
from pathlib import Path

import pytest

from scripts.course_catalog import LESSONS, LINUX_LABS, SECTIONS

ROOT = Path(__file__).parents[1]
LESSON_ROOT = ROOT / "lessons"
LINUX_LAB_ROOT = ROOT / "linux_labs"
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
LINUX_ANNOTATION_LESSONS = {3: 3, 6: 2, 7: 2, 8: 1, 11: 3, 12: 4}


def numbered_directories(root: Path) -> list[Path]:
    if not root.exists():
        return []
    return sorted(
        path for path in root.iterdir() if path.is_dir() and re.match(r"^\d{2}_", path.name)
    )


def lesson_directories() -> list[Path]:
    return numbered_directories(LESSON_ROOT)


def linux_lab_directories() -> list[Path]:
    return numbered_directories(LINUX_LAB_ROOT)


def implementation_sources() -> list[Path]:
    return [
        path
        for path in [
            *LESSON_ROOT.glob("**/*.c"),
            *LESSON_ROOT.glob("**/*.h"),
            *LINUX_LAB_ROOT.glob("**/*.c"),
            *LINUX_LAB_ROOT.glob("**/*.h"),
            *ROOT.glob("include/**/*.h"),
            *ROOT.glob("tests/*.c"),
        ]
        if path.is_file()
    ]


def native_test_sources() -> list[Path]:
    return [
        path
        for path in [*LESSON_ROOT.glob("**/test.c"), *LINUX_LAB_ROOT.glob("**/test.c")]
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


def test_linux_lab_catalog_is_exact():
    assert [lab.number for lab in LINUX_LABS] == [1, 2, 3, 4]
    assert [lab.source for lab in LINUX_LABS] == [
        "01_epoll_event_loop",
        "02_tcp_info",
        "03_userspace_mini_stack",
        "04_kernel_source_walkthrough",
    ]
    assert [lab.slug for lab in LINUX_LABS] == [
        "epoll-event-loop",
        "tcp-info",
        "userspace-mini-stack",
        "kernel-source-walkthrough",
    ]
    assert [lab.route for lab in LINUX_LABS] == [
        "linux-labs/epoll-event-loop/",
        "linux-labs/tcp-info/",
        "linux-labs/userspace-mini-stack/",
        "linux-labs/kernel-source-walkthrough/",
    ]
    assert len({lab.title for lab in LINUX_LABS}) == 4
    assert len({lab.description for lab in LINUX_LABS}) == 4
    assert all(50 <= len(lab.description) <= 160 for lab in LINUX_LABS)


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


def test_linux_lab_files_follow_contract_when_present():
    labs = linux_lab_directories()
    if not labs:
        pytest.skip("numbered Linux labs are supplied by parallel lab branches")
    assert [path.name for path in labs] == [lab.source for lab in LINUX_LABS]
    for lab in labs:
        required = {"README.md", "lab.h", "exercise.c", "solution.c", "test.c", "CMakeLists.txt"}
        assert {path.name for path in lab.iterdir()} >= required, lab
        document = (lab / "README.md").read_text()
        assert "Linux" in document
        assert "```mermaid" in document and "```c" in document
        assert re.search(r"^\|.+\|\n\|(?:\s*:?-+:?\s*\|)+", document, re.MULTILINE), lab
        assert re.search(r"\bnon-goal", document, re.IGNORECASE), lab


def test_core_annotations_use_pinned_linux_sources_and_lab_links():
    for lesson_number, lab_number in LINUX_ANNOTATION_LESSONS.items():
        lesson = LESSONS[lesson_number - 1]
        text = (LESSON_ROOT / lesson.source / "README.md").read_text()
        section = text.split("## Linux implementation connection\n", 1)
        assert len(section) == 2, lesson.source
        annotation = section[1].split("\n## ", 1)[0]
        assert f"../../linux_labs/{LINUX_LABS[lab_number - 1].source}/README.md" in annotation
        assert "https://github.com/torvalds/linux/blob/v6.6/" in annotation
        assert "UAPI" in annotation and "internal" in annotation
        assert "github.com/torvalds/linux/blob/master/" not in annotation


def test_public_header_is_shared_by_both_implementations():
    for directory in lesson_directories():
        for name in ("exercise.c", "solution.c", "test.c"):
            text = strip_c_comments((directory / name).read_text())
            assert re.search(r'#\s*include\s*[<"]lesson\.h[>"]', text), (directory, name)
    for directory in linux_lab_directories():
        for name in ("exercise.c", "solution.c", "test.c"):
            text = strip_c_comments((directory / name).read_text())
            assert re.search(r'#\s*include\s*[<"]lab\.h[>"]', text), (directory, name)


def test_python_is_tooling_only():
    assert not (ROOT / "src").exists()
    assert not (LESSON_ROOT / "__init__.py").exists()
    assert not (LINUX_LAB_ROOT / "__init__.py").exists()
    assert not list(LESSON_ROOT.glob("**/*.py"))
    assert not list(LINUX_LAB_ROOT.glob("**/*.py"))
    pyproject = (ROOT / "pyproject.toml").read_text().lower()
    assert "[project]" not in pyproject
    assert "[build-system]" not in pyproject
    assert "hatchling" not in pyproject
    repository_config = "\n".join(
        path.read_text(errors="ignore").lower()
        for path in [ROOT / "CMakeLists.txt", ROOT / "CMakePresets.json", ROOT / "pyproject.toml"]
    )
    assert "lesson_impl" not in repository_config


def test_c_sources_avoid_unsafe_wire_layout_and_privileged_execution():
    forbidden_patterns = {
        "packed attribute": r"__attribute__\s*\(\([^)]*packed",
        "packing pragma": r"#\s*pragma\s+pack\b",
        "bitfield": r"\b(?:unsigned|signed|_Bool|bool|u?int(?:8|16|32|64)_t)\s+\w+\s*:\s*\d+",
        "raw or packet socket": r"\bSOCK_RAW\b|\bAF_PACKET\b|\bPF_PACKET\b",
        "packet capture": r"\b(?:pcap(?:_t|_open|_loop|_next|_dispatch)?|scapy|tcpdump|tshark|dumpcap)\b",
        "process execution": r"\b(?:subprocess|exec(?:l|le|lp|lpe|v|ve|vp|vpe)?|system|popen|posix_spawn)\s*\(",
        "wildcard bind": r"\bINADDR_ANY\b|\bin6addr_any\b",
        "tun or tap": r"\b(?:TUNSETIFF|IFF_TUN|IFF_TAP|/dev/net/tun)\b",
        "namespace or privilege": r"\b(?:setns|unshare|setuid|seteuid|setgid|setegid|capset)\s*\(",
        "fork or clone": r"\b(?:fork|vfork|clone|clone3)\s*\(",
    }
    wire_struct_names = (
        r"(?:ethernet|arp|ipv4|ip|icmp|udp|tcp|dns|http|packet|header|segment|datagram|frame)_\w+"
    )
    wire_cast = re.compile(
        rf"\(\s*(?:const\s+)?struct\s+{wire_struct_names}\s*\*\s*\)\s*\w+", re.IGNORECASE
    )
    for path in implementation_sources():
        text = strip_c_comments(path.read_text(errors="ignore"))
        for label, pattern in forbidden_patterns.items():
            assert not re.search(pattern, text, re.IGNORECASE), f"{label}: {path}"
        assert not wire_cast.search(text), f"wire-pointer cast: {path}"


def test_native_socket_tests_stay_on_loopback_and_use_ephemeral_ports():
    address_calls = re.compile(
        r"\b(?:inet_addr|inet_aton|inet_pton|getaddrinfo)\s*\(\s*(?:AF_INET6?\s*,\s*)?"
        r'"((?:\d{1,3}\.){3}\d{1,3}|[0-9a-fA-F:]+)"',
    )
    numeric_addresses = re.compile(
        r"\b(?:bind|connect|sendto|inet_addr|inet_aton|inet_pton|getaddrinfo)\b[^;\n]*"
        r'"((?:\d{1,3}\.){3}\d{1,3}|[0-9a-fA-F]*:[0-9a-fA-F:]+)"'
    )
    fixed_listen_port = re.compile(
        r"(?:sin6?_port\s*=\s*htons|bind_loopback\s*\()[^;\n]*\b([1-9]\d*)\b"
    )
    for path in native_test_sources():
        text = strip_c_comments(path.read_text())
        for address in {*address_calls.findall(text), *numeric_addresses.findall(text)}:
            assert address in {"127.0.0.1", "::1"}, (path, address)
        assert not fixed_listen_port.search(text), path


def test_no_unpinned_kernel_links_or_vendored_source_snapshots():
    tracked_text = [
        path
        for path in [
            ROOT / "README.md",
            ROOT / "CONTRIBUTING.md",
            LINUX_LAB_ROOT / "README.md",
            *LESSON_ROOT.glob("*/README.md"),
            *LINUX_LAB_ROOT.glob("*/README.md"),
        ]
        if path.is_file()
    ]
    linux_link = re.compile(r"https://github\.com/torvalds/linux/(?:blob|tree)/([^/]+)/")
    for path in tracked_text:
        for ref in linux_link.findall(path.read_text()):
            assert ref == "v6.6", (path, ref)
    snapshots = [
        path
        for path in LINUX_LAB_ROOT.rglob("*")
        if path.is_file() and (path.suffix in {".patch", ".diff"} or "linux-source" in path.name)
    ]
    assert not snapshots


def test_no_forbidden_network_tooling_anywhere_in_sources():
    names = re.compile(
        r"\b(?:SOCK_RAW|AF_PACKET|PF_PACKET|Scapy|tcpdump|tshark|dumpcap|TUNSETIFF)\b",
        re.IGNORECASE,
    )
    for path in implementation_sources():
        assert not names.search(strip_c_comments(path.read_text(errors="ignore"))), path
