function(tcpip_add_linux_lab)
  cmake_parse_arguments(LAB "THREADS" "NUMBER;SLUG;CLI_SOURCE" "LINK_SOLUTIONS" ${ARGN})
  if(NOT LAB_NUMBER OR NOT LAB_SLUG)
    message(FATAL_ERROR "tcpip_add_linux_lab requires NUMBER and SLUG")
  endif()

  set(prefix "tcpip_linux_l${LAB_NUMBER}")
  set(directory "${CMAKE_CURRENT_SOURCE_DIR}/${LAB_NUMBER}_${LAB_SLUG}")

  add_library(${prefix}_exercise STATIC "${directory}/exercise.c")
  add_library(${prefix}_solution STATIC "${directory}/solution.c")
  foreach(target IN ITEMS ${prefix}_exercise ${prefix}_solution)
    target_include_directories(${target} PUBLIC "${directory}")
    target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
    target_link_libraries(${target} PRIVATE tcpip_warnings tcpip_sanitizers)
  endforeach()

  add_library(tcpip::linux::l${LAB_NUMBER}::exercise ALIAS ${prefix}_exercise)
  add_library(tcpip::linux::l${LAB_NUMBER}::solution ALIAS ${prefix}_solution)

  if(TCPIP_USE_SOLUTIONS)
    set(implementation ${prefix}_solution)
  else()
    set(implementation ${prefix}_exercise)
  endif()

  add_executable(${prefix}_test "${directory}/test.c")
  target_compile_definitions(${prefix}_test PRIVATE _GNU_SOURCE)
  target_link_libraries(${prefix}_test PRIVATE
    ${implementation} tcpip_test_harness tcpip_warnings tcpip_sanitizers
  )

  foreach(dependency IN LISTS LAB_LINK_SOLUTIONS)
    target_link_libraries(${prefix}_exercise PUBLIC "tcpip_l${dependency}_solution")
    target_link_libraries(${prefix}_solution PUBLIC "tcpip_l${dependency}_solution")
  endforeach()

  if(LAB_THREADS)
    find_package(Threads REQUIRED)
    target_link_libraries(${prefix}_exercise PRIVATE Threads::Threads)
    target_link_libraries(${prefix}_solution PRIVATE Threads::Threads)
    target_link_libraries(${prefix}_test PRIVATE Threads::Threads)
  endif()

  if(LAB_CLI_SOURCE)
    add_executable(${prefix}_cli "${directory}/${LAB_CLI_SOURCE}")
    target_link_libraries(${prefix}_cli PRIVATE
      ${prefix}_solution tcpip_warnings tcpip_sanitizers
    )
  endif()

  add_test(NAME "linux_lab_${LAB_NUMBER}_${LAB_SLUG}" COMMAND ${prefix}_test)
  set_tests_properties("linux_lab_${LAB_NUMBER}_${LAB_SLUG}" PROPERTIES
    LABELS "linux;optional;linux-lab-${LAB_NUMBER};unit"
    TIMEOUT 10
  )
endfunction()
