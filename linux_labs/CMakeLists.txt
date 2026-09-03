# Optional Linux implementation labs are registered here.

# Event-driven I/O and transport diagnostics.
include(01_epoll_event_loop/CMakeLists.txt)
include(02_tcp_info/CMakeLists.txt)

# Integration and source-navigation capstones.
tcpip_add_linux_lab(
  NUMBER 03
  SLUG userspace_mini_stack
  LINK_SOLUTIONS 03 06 07 11 12
)
tcpip_add_linux_lab(
  NUMBER 04
  SLUG kernel_source_walkthrough
  CLI_SOURCE index.c
)
