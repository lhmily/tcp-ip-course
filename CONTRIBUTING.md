# Contributing

Thanks for improving the TCP/IP Course. Keep changes focused, portable, safe, and useful to someone learning both C17 and networking fundamentals.

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

## Documentation contract

A lesson README needs all required headings listed in the issue form and repository tests, at least one unique Mermaid diagram, a C code block, a useful table, an explicit non-goal, and a `**What to notice:**` callout. Explain byte offsets and state transitions rather than relying on packed structs or compiler-specific layouts. Keep links and images local when practical.

## Safety rules

Use fixed in-memory fixtures for packet parsing. Native socket tests must use loopback and an operating-system-assigned ephemeral port. The course does not use raw sockets, packet-capture tools or libraries, privileged operations, shell execution, external network endpoints, or wildcard listening addresses. Never cast a wire buffer to a protocol struct, use packed layout directives, or depend on C bitfield layout.

## Validation

Before submitting a change, run the checks relevant to it:

```sh
cmake --preset student && cmake --build --preset student
cmake --preset reference && cmake --build --preset reference && ctest --preset reference
uv run pytest tests/test_repository_contract.py tests/test_documentation.py
uv run ruff check .
uv run ruff format --check .
uv run python scripts/generate_branding_assets.py --check
uv run python scripts/generate_documentation_assets.py --check
uv run python scripts/build_site.py
uv run mkdocs build --strict
```

Use the sanitizer preset for C changes when Clang is available. Keep commits narrow and describe the invariant or learner outcome being improved.
