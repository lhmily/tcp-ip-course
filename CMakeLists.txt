cmake_minimum_required(VERSION 3.25)
project(tcp_ip_course VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TCPIP_USE_SOLUTIONS "Link lesson tests against reference solutions" OFF)
option(TCPIP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(TCPIP_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

include(CTest)
include(cmake/TcpipLesson.cmake)

add_library(tcpip_warnings INTERFACE)
if(MSVC)
  target_compile_options(tcpip_warnings INTERFACE /W4)
  if(TCPIP_WARNINGS_AS_ERRORS)
    target_compile_options(tcpip_warnings INTERFACE /WX)
  endif()
else()
  target_compile_options(tcpip_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes
  )
  if(TCPIP_WARNINGS_AS_ERRORS)
    target_compile_options(tcpip_warnings INTERFACE -Werror)
  endif()
endif()

add_library(tcpip_sanitizers INTERFACE)
if(TCPIP_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(tcpip_sanitizers INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer
  )
  target_link_options(tcpip_sanitizers INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer
  )
endif()

add_library(tcpip_test_harness STATIC tests/test_harness.c)
target_include_directories(tcpip_test_harness PUBLIC include)
target_link_libraries(tcpip_test_harness PRIVATE tcpip_warnings tcpip_sanitizers)

add_subdirectory(lessons)

# Native cross-layer verification is intentionally separate from lesson selection.
if(BUILD_TESTING AND TARGET tcpip_l01_solution AND TARGET tcpip_l02_solution
   AND TARGET tcpip_l03_solution AND TARGET tcpip_l05_solution
   AND TARGET tcpip_l09_solution AND TARGET tcpip_l11_solution
   AND TARGET tcpip_l12_solution)
  add_executable(tcpip_integration_test tests/test_integration.c)
  target_link_libraries(tcpip_integration_test PRIVATE
    tcpip_l01_solution tcpip_l02_solution tcpip_l03_solution
    tcpip_l05_solution tcpip_l09_solution tcpip_l11_solution
    tcpip_l12_solution
    tcpip_test_harness tcpip_warnings tcpip_sanitizers
  )
  add_test(NAME native_cross_layer_integration COMMAND tcpip_integration_test)
  set_tests_properties(native_cross_layer_integration PROPERTIES
    LABELS "integration;native"
    TIMEOUT 5
  )
endif()
