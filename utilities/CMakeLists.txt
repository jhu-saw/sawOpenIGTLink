project (sawOpenIGTLinkUtilities)

cmake_minimum_required (VERSION 2.4)
if (COMMAND cmake_policy)
  cmake_policy (SET CMP0003 NEW)
endif (COMMAND cmake_policy)

find_package (OpenIGTLink 3
              HINTS igtl /usr/lib/igtl /usr/local/lib/igtl/cmake/igtl-3.1 /usr/local/lib/igtl /usr/local/)

if (OpenIGTLink_FOUND)
  include (${OpenIGTLink_USE_FILE})

  set (CMAKE_SKIP_BUILD_RPATH FALSE)
  set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  add_executable (igtl_receive igtl_receive.cxx)
  target_link_libraries (igtl_receive ${OpenIGTLink_LIBRARIES})

  add_executable (igtl_send_sensor igtl_send_sensor.cxx)
  target_link_libraries (igtl_send_sensor ${OpenIGTLink_LIBRARIES})

  add_executable (igtl_send_string igtl_send_string.cxx)
  target_link_libraries (igtl_send_string ${OpenIGTLink_LIBRARIES})

  install (TARGETS igtl_receive igtl_send_sensor igtl_send_string
           RUNTIME DESTINATION bin
           LIBRARY DESTINATION lib
           ARCHIVE DESTINATION lib)

else ()
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires OpenIGTLink 3")
endif ()
