set(_tcpip_l12_directory "${CMAKE_CURRENT_SOURCE_DIR}")
get_filename_component(CMAKE_CURRENT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

set(tcpip_l12_solution_dependencies 09 10)
foreach(dependency IN ITEMS 02 03 05 06)
  if(TARGET "tcpip_l${dependency}_solution")
    list(APPEND tcpip_l12_solution_dependencies "${dependency}")
  endif()
endforeach()

tcpip_add_lesson(
  NUMBER 12
  SLUG diagnostics_integration
  LINK_SOLUTIONS ${tcpip_l12_solution_dependencies}
)

set(CMAKE_CURRENT_SOURCE_DIR "${_tcpip_l12_directory}")
unset(_tcpip_l12_directory)
unset(tcpip_l12_solution_dependencies)
