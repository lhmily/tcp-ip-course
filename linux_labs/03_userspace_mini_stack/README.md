<!-- COURSE_COMPONENT:mini-stack-hero START -->
# Linux Lab 03: A deterministic userspace mini-stack

This optional Linux lab ties several earlier lessons into one deliberately small experiment. You will send TCP payload chunks through an IPv4 frame, choose a route, inject bounded faults, reassemble bytes, and inspect the final Ethernet frame. The implementation uses a real Linux `AF_UNIX` `SOCK_DGRAM` socket pair only as the simulated wire. Everything that normally makes a network experiment flaky—wall-clock time, scheduler timing, sleeping, background threads, heap allocation, and global mutable state—is excluded. The result is a repeatable model that still crosses useful operating-system and protocol boundaries.

[Review the contract](#contract-at-a-glance) · [Run the Linux labs](https://github.com/lhmily/tcp-ip-course/blob/main/linux_labs/README.md#prerequisites-and-workflow)
<!-- COURSE_COMPONENT:mini-stack-hero END -->

The single public operation is `tcpip_linux_l03_run(scenario, result)`. The scenario object and its borrowed caller-owned input and output spans must not overlap the result storage. It also supplies a route table, addresses and ports, initial sequence numbers, an initial retransmission timeout, an attempt bound, and a finite list of fault actions. A result records the selected route, virtual elapsed time, attempt and retransmission counts, conceptual TCP state, reassembled length, and diagnostics for the last frame that actually crossed the socket pair. No returned pointer outlives the call, and the implementation performs no dynamic allocation.

<!-- COURSE_COMPONENT:mini-stack-prerequisites START -->
## Data flow and virtual time

```mermaid
flowchart LR
    A[Caller payload span] --> B[Chunk and build TCP]
    B --> C[Wrap in IPv4 and Ethernet]
    C --> D{Fault script}
    D -->|DROP| T[Advance to virtual RTO]
    D -->|DELAY| Q[Fixed event queue]
    D -->|REORDER| Q
    D -->|DELIVER| Q
    Q --> S[Blocking AF_UNIX datagram pair]
    S --> P[Parse and verify TCP/IPv4]
    P --> R[Sequence reassembly]
    R --> O[Caller output span]
    P --> X[Final frame diagnostics]
    T --> B
```

Every transmission attempt has a virtual deadline. `DELIVER` schedules a frame immediately. `DROP` schedules nothing. `DELAY` uses the action's explicit millisecond offset. `REORDER` postpones a chunk by half the current RTO, allowing later chunks to arrive first. The fixed-capacity queue orders events by virtual due time and then insertion order. The attempt deadline is exclusive: an event due exactly at the deadline is not delivered during that attempt. When no useful event remains before the deadline, the clock jumps directly to that deadline and the RTO doubles for the next attempt. There is no `sleep`, polling loop, or dependence on when Linux schedules the process. A delayed original and a retransmission can both arrive, which provides a deterministic duplicate test: the reassembler must accept matching bytes without counting them twice.

The route table follows Lesson 11 semantics: longest prefix wins, then lower metric, then earlier table position. A Lesson 11 no-match result maps to `TCPIP_LINUX_L03_ROUTE_NOT_FOUND`, malformed routes map to `TCPIP_LINUX_L03_MALFORMED`, and invalid route arguments map to `TCPIP_LINUX_L03_INVALID_ARGUMENT`. The solution calls that lesson's reference target rather than maintaining a second routing implementation. It likewise reuses Lesson 03 for IPv4 parsing, Lesson 06 for TCP construction and checksum validation, Lesson 07 for state transitions and reassembly, and Lesson 12 for complete-frame diagnostics. Diagnostics are observational: Lesson 12 application heuristics may report malformed or truncated payloads, especially on HTTP ports with arbitrary bytes, but only IPv4/TCP parsing, checksums, and reassembly govern transfer success. The includes are explicit relative paths because several course units intentionally use the same filename, `lesson.h`; relying on include-directory order would make the lab ambiguous.
<!-- COURSE_COMPONENT:mini-stack-prerequisites END -->

<!-- COURSE_COMPONENT:mini-stack-fault-timeline START -->
## Deterministic fault and virtual-time scenarios

| Lane | Virtual time | Event | Outcome |
|---|---:|---|---|
| Clean | 0 ms | One 11-byte chunk is delivered immediately | `attempts=1`, `retransmits=0`, `TCPIP_LINUX_L03_OK`, final state `TCPIP_L07_TCP_STATE_ESTABLISHED` |
| Drop/retry | 0 ms | Attempt 1 consumes `DROP`; no event is queued | The clock advances to the exclusive 100 ms deadline |
| Drop/retry | 100 ms | Attempt 2 retransmits with `DELIVER` | `attempts=2`, `retransmits=1`, `TCPIP_LINUX_L03_OK`, final state established |
| Deadline | 100 ms | An original delayed by 100 ms is due exactly at attempt 1's deadline | The exclusive deadline defers it to attempt 2; the transfer ends with `attempts=2`, `retransmits=1`, elapsed 100 ms, final state established |
| Reorder | 0 ms | For 400 bytes, chunk 1 arrives before chunk 0 | The later sequence range waits in the reassembler |
| Reorder | 50 ms | Chunk 0 arrives after half the 100 ms RTO | The gap closes and all 400 bytes reassemble with `attempts=1`, `retransmits=0`, final state established |
| Duplicate | 100 ms | The immediate attempt-2 retransmission arrives before the original delayed to 200 ms | The retransmission supplies the 23 payload bytes |
| Duplicate | 200 ms | The delayed matching original arrives | It accepts zero new bytes; `attempts=2`, `retransmits=1`, final state established |
| Exhaustion | 100 ms | First dropped attempt reaches its deadline | RTO doubles before attempt 2 |
| Exhaustion | 300 ms | Second dropped attempt reaches its deadline | RTO doubles before attempt 3 |
| Exhaustion | 700 ms | Third dropped attempt reaches its deadline | `attempts=3`, `retransmits=2`, `TCPIP_LINUX_L03_RETRY_EXHAUSTED`; established state is preserved |
<!-- COURSE_COMPONENT:mini-stack-fault-timeline END -->

<!-- COURSE_COMPONENT:mini-stack-route-table START -->
## Mini-stack route selection

| Route | Network | Next hop | Interface | Metric |
|---:|---|---|---:|---:|
| 0 | `0.0.0.0/0` | `0.0.0.0` | 1 | 100 |
| 1 | `198.51.100.0/24` | `0.0.0.0` | 7 | 10 |

| Case | Status | Selection |
|---|---|---|
| `198.51.100.7` with both routes | `TCPIP_LINUX_L03_OK` | Route 1, the `/24` longest-prefix match |
| `198.51.100.7` with only the default route | `TCPIP_LINUX_L03_OK` | Route 0 |
| `198.51.100.7` with only `203.0.113.0/24` | `TCPIP_LINUX_L03_ROUTE_NOT_FOUND` | None; Lesson 11 `TCPIP_L11_TRUNCATED` maps to the lab status |
| `198.51.100.7` with prefix length 33 | `TCPIP_LINUX_L03_MALFORMED` | None; the Lesson 11 route is malformed |
<!-- COURSE_COMPONENT:mini-stack-route-table END -->

<!-- COURSE_COMPONENT:mini-stack-diagnostics START -->
## Transfer status versus final-frame diagnostics

| Case | Transfer status | Parsed layers | Diagnostic | Checksums | Final-frame observation |
|---|---|---|---|---:|---|
| Clean 11-byte payload to port 443 | `TCPIP_LINUX_L03_OK` | Ethernet → IPv4 → TCP | `TCPIP_L12_DIAG_UNSUPPORTED` (`0x00000002`) | 2/2 valid | The 65-byte frame is internally valid; no supported application parser applies |
| Arbitrary 20 bytes to HTTP port 80 | `TCPIP_LINUX_L03_OK` | Ethernet → IPv4 → TCP → HTTP | `TCPIP_L12_DIAG_MALFORMED` (`0x00000004`) | 2/2 valid | Lesson 12 reports malformed HTTP, but the lab transfer and reassembly remain successful |
| Reordered 400-byte transfer | `TCPIP_LINUX_L03_OK` | Ethernet → IPv4 → TCP | `TCPIP_L12_DIAG_UNSUPPORTED` (`0x00000002`) | 2/2 valid | Chunk 0 is the final delivered frame at 50 ms after chunk 1 arrived first |
| Delayed duplicate, 23-byte payload | `TCPIP_LINUX_L03_OK` | Ethernet → IPv4 → TCP | `TCPIP_L12_DIAG_UNSUPPORTED` (`0x00000002`) | 2/2 valid | The original arrives at 200 ms after the retransmission and accepts no new bytes |
<!-- COURSE_COMPONENT:mini-stack-diagnostics END -->

## Contract at a glance

| Input or result | Rule | Why it matters |
|---|---|---|
| `payload` / `payload_length` | Borrowed, nonempty, at most 1024 bytes, and not overlapping `result` | Keeps frame and queue storage bounded and prevents result writes from corrupting inputs |
| `output` / `output_capacity` | Caller-owned, large enough for all payload bytes, and not overlapping `result` | Avoids allocation, partial success, and output/result alias corruption |
| `routes` | Canonical Lesson 11 routes that do not overlap `result` | Makes route choice inspectable without result writes corrupting route selection |
| `initial_rto_ms` | Nonzero virtual duration | Guarantees progress without real time |
| `max_attempts` | Nonzero finite bound | Prevents an infinite retry exercise |
| `faults` | At most 32 authored actions with valid `tcpip_linux_l03_fault_action` values and no overlap with `result` | Makes failure behavior deterministic and keeps the fault script immutable during the call |
| `final_frame_diagnostic` | Best-effort formatted Lesson 12 report for the last delivered frame | Records diagnostics without governing transfer success; a formatting-capacity failure leaves an empty diagnostic string |

A minimal caller can use a default route and no faults:

```c
uint8_t output[5];
const uint8_t payload[] = "hello";
tcpip_l11_route route = {{0, 0, 0, 0}, 0, {0, 0, 0, 0}, 1, 100};
tcpip_linux_l03_scenario scenario = {
    .payload = payload, .payload_length = 5,
    .output = output, .output_capacity = sizeof(output),
    .routes = &route, .route_count = 1,
    .source_ipv4 = {192, 0, 2, 10},
    .destination_ipv4 = {198, 51, 100, 7},
    .source_port = 40000, .destination_port = 443,
    .sender_initial_seq = 1000, .receiver_initial_seq = 9000,
    .initial_rto_ms = 100, .max_attempts = 3
};
tcpip_linux_l03_result result;
tcpip_linux_l03_status status = tcpip_linux_l03_run(&scenario, &result);
```

<!-- COURSE_COMPONENT:mini-stack-contract START -->
## Exercise workflow

Start with `exercise.c`. It already clears the result, establishes failure sentinels, and validates the main scalar and span contracts. The `TCPIP_LINUX_L03_TODO` return is intentional. Implement the scenario without changing `lab.h`, then compare behavior with the solution build. Useful milestones are route selection; transition from closed through active open to established; fixed-size chunk construction; fault-to-event translation; nonblocking datagram transfer; parser and checksum verification; reassembly; timeout backoff; and final diagnostics. Preserve transactional behavior where practical: validation failures must not write payload output, and the result should remain predictably initialized.

The test program covers a clean delivery, a first-attempt drop followed by retransmission, out-of-order chunks, a delayed duplicate, retry exhaustion, longest-prefix route selection, insufficient output capacity, deterministic repeated runs, and the mandatory final diagnostic. Configure on Linux with `-DTCPIP_BUILD_LINUX_LABS=ON` and choose `-DTCPIP_USE_SOLUTIONS=ON` for the reference implementation. Sanitizer and warnings-as-errors configurations are strongly recommended.
<!-- COURSE_COMPONENT:mini-stack-contract END -->

<!-- COURSE_COMPONENT:mini-stack-safety START -->
## Safety, provenance, and non-goals

This is a userspace teaching model, not a network stack suitable for production or hostile input. It does not open an Internet or packet socket, change interfaces, require root, send traffic outside the local process, or mutate kernel state. The socket pair carries complete synthetic Ethernet frames, but Unix datagrams do not model MTUs, NIC queues, congestion, stream semantics, or real packet loss. TCP handshaking and acknowledgments are conceptual; the model focuses on bounded payload reliability and reassembly rather than complete RFC conformance. There is no congestion control, receive-window evolution, fragmentation, IPv6, NAT, TLS, application protocol, persistence, or concurrent endpoint.

The course code is authored for this repository. Its use of Linux APIs is based on their documented userspace interface and does not copy Linux kernel implementation text. Linux itself is licensed under GPL-2.0-only; that license and Linux authorship apply to the kernel, not automatically to independently authored examples that merely call system interfaces. If you inspect or redistribute Linux source while extending this exercise, retain its GPL notices and follow the GPL's requirements. Do not paste kernel code into this lab. The explicit source boundaries keep provenance review straightforward.

Because virtual time is not wall time, `virtual_elapsed_ms` must never be interpreted as a latency benchmark. Likewise, successful diagnostics prove internal frame consistency under this model, not interoperability with a real host. Treat the lab as a bridge between protocol mechanics and Linux I/O primitives: a controlled environment in which every retry, queue slot, sequence offset, and final byte can be explained.
<!-- COURSE_COMPONENT:mini-stack-safety END -->
