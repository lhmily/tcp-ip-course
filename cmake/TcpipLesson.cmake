function(tcpip_add_lesson)
  cmake_parse_arguments(LESSON "THREADS" "NUMBER;SLUG" "LINK_SOLUTIONS" ${ARGN})
  if(NOT LESSON_NUMBER OR NOT LESSON_SLUG)
    message(FATAL_ERROR "tcpip_add_lesson requires NUMBER and SLUG")
  endif()

  set(prefix "tcpip_l${LESSON_NUMBER}")
  set(directory "${CMAKE_CURRENT_SOURCE_DIR}/${LESSON_NUMBER}_${LESSON_SLUG}")

  add_library(${prefix}_exercise STATIC "${directory}/exercise.c")
  add_library(${prefix}_solution STATIC "${directory}/solution.c")
  foreach(target IN ITEMS ${prefix}_exercise ${prefix}_solution)
    target_include_directories(${target} PUBLIC "${directory}")
    target_link_libraries(${target} PRIVATE tcpip_warnings tcpip_sanitizers)
  endforeach()

  add_library(tcpip::l${LESSON_NUMBER}::exercise ALIAS ${prefix}_exercise)
  add_library(tcpip::l${LESSON_NUMBER}::solution ALIAS ${prefix}_solution)

  if(TCPIP_USE_SOLUTIONS)
    set(implementation ${prefix}_solution)
  else()
    set(implementation ${prefix}_exercise)
  endif()

  add_executable(${prefix}_test "${directory}/test.c")
  target_link_libraries(${prefix}_test PRIVATE
    ${implementation} tcpip_test_harness tcpip_warnings tcpip_sanitizers
  )

  foreach(dependency IN LISTS LESSON_LINK_SOLUTIONS)
    target_link_libraries(${prefix}_test PRIVATE "tcpip_l${dependency}_solution")
  endforeach()

  if(LESSON_THREADS)
    find_package(Threads REQUIRED)
    target_link_libraries(${prefix}_exercise PRIVATE Threads::Threads)
    target_link_libraries(${prefix}_solution PRIVATE Threads::Threads)
    target_link_libraries(${prefix}_test PRIVATE Threads::Threads)
  endif()

  add_test(NAME "lesson_${LESSON_NUMBER}_${LESSON_SLUG}" COMMAND ${prefix}_test)
  set_tests_properties("lesson_${LESSON_NUMBER}_${LESSON_SLUG}" PROPERTIES
    LABELS "lesson-${LESSON_NUMBER};unit"
    TIMEOUT 5
  )
endfunction()
