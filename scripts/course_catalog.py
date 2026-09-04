"""Single source of truth for the TCP/IP course page catalog."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Protocol


class CatalogPage(Protocol):
    """Shared fields for pages staged into the course site."""

    source: str
    slug: str
    title: str
    key: str
    description: str
    source_root: str
    route_root: str


@dataclass(frozen=True)
class Lesson:
    number: int
    source: str
    slug: str
    title: str
    description: str
    section: str

    @property
    def key(self) -> str:
        return self.slug

    @property
    def source_root(self) -> str:
        return "lessons"

    @property
    def route_root(self) -> str:
        return "lessons"

    @property
    def route(self) -> str:
        return f"lessons/{self.slug}/"


@dataclass(frozen=True)
class LinuxLab:
    number: int
    source: str
    slug: str
    title: str
    description: str

    @property
    def key(self) -> str:
        return self.slug

    @property
    def source_root(self) -> str:
        return "linux_labs"

    @property
    def route_root(self) -> str:
        return "linux-labs"

    @property
    def route(self) -> str:
        return f"linux-labs/{self.slug}/"


LESSONS = (
    Lesson(
        1,
        "01_bytes_addressing_checksum",
        "bytes-addressing-checksum",
        "Bytes, Addressing, and Checksums",
        "Learn byte order, bounded parsing, Internet addressing, and checksum arithmetic in portable C17.",
        "Foundations",
    ),
    Lesson(
        2,
        "02_ethernet_arp",
        "ethernet-arp",
        "Ethernet and ARP",
        "Decode Ethernet frames and ARP messages safely while understanding local-link address resolution.",
        "Foundations",
    ),
    Lesson(
        3,
        "03_ipv4_packets",
        "ipv4-packets",
        "IPv4 Packets",
        "Parse and validate IPv4 headers, lengths, fragmentation fields, and checksums with portable C17.",
        "Internet layer",
    ),
    Lesson(
        4,
        "04_icmp",
        "icmp",
        "ICMP",
        "Understand ICMP messages, echo exchanges, error quotation, and safe packet parsing in C17.",
        "Internet layer",
    ),
    Lesson(
        5,
        "05_udp",
        "udp",
        "UDP",
        "Encode UDP datagrams, verify pseudo-header checksums, and reason about datagram boundaries and loss.",
        "Transport",
    ),
    Lesson(
        6,
        "06_tcp_segments",
        "tcp-segments",
        "TCP Segments",
        "Parse TCP segments and reason about sequence space, flags, options, windows, and checksums.",
        "Transport",
    ),
    Lesson(
        7,
        "07_tcp_state_reliability",
        "tcp-state-reliability",
        "TCP State and Reliability",
        "Model TCP connection states, acknowledgments, retransmission, flow control, and orderly shutdown.",
        "Transport",
    ),
    Lesson(
        8,
        "08_stream_sockets",
        "stream-sockets",
        "Stream Sockets",
        "Build bounded POSIX stream-socket loops that handle partial reads, partial writes, and clean shutdown.",
        "Applications",
    ),
    Lesson(
        9,
        "09_dns",
        "dns",
        "DNS",
        "Encode and decode DNS messages safely, including labels, compression pointers, and resource records.",
        "Applications",
    ),
    Lesson(
        10,
        "10_http",
        "http",
        "HTTP",
        "Implement a small HTTP/1.1 message parser while handling framing, limits, and stream boundaries.",
        "Applications",
    ),
    Lesson(
        11,
        "11_routing_nat",
        "routing-nat",
        "Routing and NAT",
        "Apply longest-prefix routing and model bounded NAT mappings, expiry, and transport checksums.",
        "Networks in practice",
    ),
    Lesson(
        12,
        "12_diagnostics_integration",
        "diagnostics-integration",
        "Diagnostics and Integration",
        "Integrate protocol parsers into safe offline diagnostics and explain failures across the TCP/IP stack.",
        "Networks in practice",
    ),
)

SECTIONS = (
    "Foundations",
    "Internet layer",
    "Transport",
    "Applications",
    "Networks in practice",
)

LINUX_LABS = (
    LinuxLab(
        1,
        "01_epoll_event_loop",
        "epoll-event-loop",
        "epoll Event Loop",
        "Build a bounded Linux epoll event loop and practice readiness, framing, deadlines, and descriptor ownership.",
    ),
    LinuxLab(
        2,
        "02_tcp_info",
        "tcp-info",
        "TCP_INFO",
        "Observe a loopback TCP connection through Linux TCP_INFO while handling versioned UAPI data safely.",
    ),
    LinuxLab(
        3,
        "03_userspace_mini_stack",
        "userspace-mini-stack",
        "Userspace Mini-Stack",
        "Compose routing, TCP, IPv4, Ethernet, and diagnostics over a deterministic local Unix socket pair.",
    ),
    LinuxLab(
        4,
        "04_kernel_source_walkthrough",
        "kernel-source-walkthrough",
        "Kernel Source Walkthrough",
        "Navigate authored ingress and egress paths through pinned Linux v6.6 source without copying kernel code.",
    ),
)


def source_document(page: CatalogPage, root: Path) -> Path:
    """Return the repository README path for a catalog page."""
    return root / page.source_root / page.source / "README.md"


def route_document(page: CatalogPage, output: Path) -> Path:
    """Return the staged site index path for a catalog page."""
    return output / page.route / "index.md"


def grouped_lessons() -> tuple[tuple[str, tuple[Lesson, ...]], ...]:
    """Return catalog lessons grouped in stable navigation order."""
    return tuple(
        (section, tuple(item for item in LESSONS if item.section == section))
        for section in SECTIONS
    )


@dataclass(frozen=True)
class PageIdentity:
    key: str
    route: str
    title: str
    description: str
    track: str


COURSE_OVERVIEW = PageIdentity(
    "course-overview",
    "",
    "TCP/IP Course in C17",
    "Learn TCP/IP in 12 portable C17 core lessons and 4 optional Linux implementation labs.",
    "overview",
)
LINUX_OVERVIEW = PageIdentity(
    "linux-overview",
    "linux-labs/",
    "Optional Linux implementation track",
    "Explore Linux networking implementation boundaries in four optional, unprivileged C17 labs.",
    "linux",
)


def all_pages() -> tuple[CatalogPage, ...]:
    """Return all numbered course pages in stable navigation order."""
    return (*LESSONS, *LINUX_LABS)


def page_identities() -> tuple[PageIdentity, ...]:
    """Return all 18 canonical page identities in stable site order."""
    lesson_pages = tuple(
        PageIdentity(page.key, page.route, page.title, page.description, "core") for page in LESSONS
    )
    linux_pages = tuple(
        PageIdentity(page.key, page.route, page.title, page.description, "linux")
        for page in LINUX_LABS
    )
    return (COURSE_OVERVIEW, *lesson_pages, LINUX_OVERVIEW, *linux_pages)
