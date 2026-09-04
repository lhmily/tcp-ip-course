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

if(BUILD_TESTING)
  set(linux_l04_cli tcpip_linux_l04_cli)
  set(linux_l04_cli_labels "linux;optional;linux-lab-04;cli")

  add_test(NAME linux_lab_04_cli_list COMMAND ${linux_l04_cli} --list)
  set_tests_properties(linux_lab_04_cli_list PROPERTIES
    LABELS "${linux_l04_cli_labels}"
    PASS_REGULAR_EXPRESSION
      "netif_receive_skb.*https://github.com/torvalds/linux/blob/v6\\.6/net/core/dev\\.c#L5805;tcp_get_info.*TCP_INFO.*https://github.com/torvalds/linux/blob/v6\\.6/net/ipv4/tcp\\.c#L3699"
    TIMEOUT 10
  )

  add_test(NAME linux_lab_04_cli_ingress COMMAND ${linux_l04_cli} --ingress)
  set_tests_properties(linux_lab_04_cli_ingress PROPERTIES
    LABELS "${linux_l04_cli_labels}"
    PASS_REGULAR_EXPRESSION
      "^netif_receive_skb -> __netif_receive_skb_one_core -> ip_rcv -> nf_inet_pre_routing -> ip_rcv_finish -> ip_route_input_noref -> ip_route_input_slow -> dst_input -> ip_local_deliver -> nf_inet_local_in -> ip_local_deliver_finish -> tcp_v4_rcv -> __inet_lookup_skb -> __inet_lookup_established -> tcp_rcv_established -> tcp_ack"
    TIMEOUT 10
  )

  add_test(NAME linux_lab_04_cli_egress COMMAND ${linux_l04_cli} --egress)
  set_tests_properties(linux_lab_04_cli_egress PROPERTIES
    LABELS "${linux_l04_cli_labels}"
    PASS_REGULAR_EXPRESSION
      "^sock_sendmsg -> tcp_sendmsg -> tcp_sendmsg_locked -> tcp_write_xmit -> tcp_transmit_skb -> inet_queue_xmit -> ip_queue_xmit -> __ip_queue_xmit -> ip_route_output_flow -> ip_local_out -> ip_output -> ip_finish_output -> __dev_queue_xmit"
    TIMEOUT 10
  )

  add_test(NAME linux_lab_04_cli_tcp_ack COMMAND ${linux_l04_cli} tcp_ack)
  set_tests_properties(linux_lab_04_cli_tcp_ack PROPERTIES
    LABELS "${linux_l04_cli_labels}"
    PASS_REGULAR_EXPRESSION
      "^tcp_ack.*Apply an acknowledgment to TCP sender state\\..*https://github.com/torvalds/linux/blob/v6\\.6/net/ipv4/tcp_input\\.c#L3784"
    TIMEOUT 10
  )

  add_test(NAME linux_lab_04_cli_tcp_get_info COMMAND ${linux_l04_cli} tcp_get_info)
  set_tests_properties(linux_lab_04_cli_tcp_get_info PROPERTIES
    LABELS "${linux_l04_cli_labels}"
    PASS_REGULAR_EXPRESSION
      "^tcp_get_info.*TCP_INFO.*https://github.com/torvalds/linux/blob/v6\\.6/net/ipv4/tcp\\.c#L3699"
    TIMEOUT 10
  )

  add_test(
    NAME linux_lab_04_cli_unknown_option
    COMMAND sh -c "\"$1\" --unknown 2>&1; test $? -eq 2" sh $<TARGET_FILE:${linux_l04_cli}>
  )
  set_tests_properties(linux_lab_04_cli_unknown_option PROPERTIES
    LABELS "${linux_l04_cli_labels}"
    PASS_REGULAR_EXPRESSION "unknown option: --unknown;usage: .* --list \\| --ingress \\| --egress \\| SYMBOL_ID"
    TIMEOUT 10
  )

  add_test(
    NAME linux_lab_04_cli_extra_arg
    COMMAND sh -c "\"$1\" tcp_ack extra 2>&1; test $? -eq 2" sh $<TARGET_FILE:${linux_l04_cli}>
  )
  set_tests_properties(linux_lab_04_cli_extra_arg PROPERTIES
    LABELS "${linux_l04_cli_labels}"
    PASS_REGULAR_EXPRESSION "usage: .* --list \\| --ingress \\| --egress \\| SYMBOL_ID"
    TIMEOUT 10
  )
endif()
