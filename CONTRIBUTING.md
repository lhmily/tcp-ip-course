# Contributing

Thanks for improving the TCP/IP Course: 12 portable core lessons plus 4 optional Linux implementation labs. Keep changes focused, portable where the track requires it, safe, and useful to someone learning both C17 and networking fundamentals.

## Development setup

Native lesson work requires CMake 3.25 or newer, CTest, and a C17 compiler on Linux or macOS. Documentation tooling requires uv and Python 3.14.

```sh
cmake --preset reference
cmake --build --preset reference
ctest --preset reference
uv sync --locked --dev
```

## Six-file lesson contract

Every numbered lesson directory must contain exactly these required learning files (supporting fixture files may be added only when necessary):

- `README.md` explains the protocol, safety boundary, exercise, and test invariants.
- `lesson.h` declares the small public C interface shared by starter, solution, and tests.
- `exercise.c` is the learner implementation and must always compile without warnings.
- `solution.c` is a complete, defensive reference implementation.
- `test.c` is a deterministic native test using offline buffers or loopback with an ephemeral port.
- `CMakeLists.txt` registers the lesson through the repository CMake helper.

The starter and solution must implement the same declarations. Do not hide alternate implementations behind generated code or environment variables.

## Optional Linux lab contract

The four numbered `linux_labs/` directories use the same six-file shape but may call Linux-only, unprivileged APIs. Develop them on Linux with `linux-student`, `linux-reference`, and `linux-sanitize`; the existing core presets and Linux/macOS support remain independent. Linux references must point to the pinned `torvalds/linux` `v6.6` tree and identify UAPI contracts separately from internal implementation files.

Linux is GPL-2.0-only. Write original course explanations and code: do not copy kernel code, comments, tables, or source snapshots into this MIT-licensed repository. A kernel version bump requires reviewing every lab and core-lesson annotation.

## Documentation contract

A lesson README needs all required headings listed in the issue form and repository tests, at least one unique Mermaid diagram, a C code block, a useful table, an explicit non-goal, and a `**What to notice:**` callout. Explain byte offsets and state transitions rather than relying on packed structs or compiler-specific layouts. Keep links and images local when practical.

## Safety rules

Use fixed in-memory fixtures for packet parsing. Native socket tests must use loopback and an operating-system-assigned ephemeral port. Both tracks reject raw or packet sockets, packet-capture tools or libraries, privileged or namespace operations, process or shell execution, external numeric endpoints, wildcard listening addresses, TUN/TAP devices, and `fork`/`clone`. Never cast a wire buffer to a protocol struct, use packed layout directives, or depend on C bitfield layout.

## Validation

Before submitting a change, run the checks relevant to it:

```sh
cmake --preset student && cmake --build --preset student
cmake --preset reference && cmake --build --preset reference && ctest --preset reference
# Linux track changes only:
cmake --preset linux-student && cmake --build --preset linux-student
cmake --preset linux-reference && cmake --build --preset linux-reference && ctest --preset linux-reference
cmake --preset linux-sanitize && cmake --build --preset linux-sanitize && ctest --preset linux-sanitize
uv run pytest tests/test_repository_contract.py tests/test_documentation.py
uv run ruff check .
uv run ruff format --check .
uv run python scripts/generate_branding_assets.py --check
uv run python scripts/generate_documentation_assets.py --check
uv run python scripts/build_site.py
uv run mkdocs build --strict
```

Use the sanitizer preset for C changes when Clang is available. Keep commits narrow and describe the invariant or learner outcome being improved.
