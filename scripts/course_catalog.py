"""Single source of truth for the TCP/IP course lesson catalog."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Lesson:
    number: int
    source: str
    slug: str
    title: str
    description: str
    section: str

    @property
    def route(self) -> str:
        return f"lessons/{self.slug}/"


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


def grouped_lessons() -> tuple[tuple[str, tuple[Lesson, ...]], ...]:
    """Return catalog lessons grouped in stable navigation order."""
    return tuple(
        (section, tuple(item for item in LESSONS if item.section == section))
        for section in SECTIONS
    )
