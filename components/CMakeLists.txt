#
# (C) Copyright 2011-2024 Johns Hopkins University (JHU), All Rights Reserved.
#
# --- begin cisst license - do not edit ---
#
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
#
# --- end cisst license ---

cmake_minimum_required (VERSION 3.10)
project (sawOpenIGTLink VERSION 1.3.0)

set (REQUIRED_CISST_LIBRARIES cisstCommon
                              cisstVector
                              cisstOSAbstraction
                              cisstMultiTask
                              cisstParameterTypes)

find_package (cisst 1.2.1 COMPONENTS ${REQUIRED_CISST_LIBRARIES})

if (cisst_FOUND_AS_REQUIRED)

  # load cisst configuration
  include (${CISST_USE_FILE})

  # Version 3+ is required
  find_package (OpenIGTLink 3
                HINTS igtl /usr/lib/igtl /usr/local/lib/igtl/cmake/igtl-3.1 /usr/local/lib/igtl /usr/local/)

  if (OpenIGTLink_FOUND)
    # OpenIGTLink settings
    include (${OpenIGTLink_USE_FILE})

    # catkin/ROS paths
    cisst_set_output_path ()

    # create/configure file for find_package (sawOpenIGTLink)
    set (sawOpenIGTLink_INCLUDE_DIR
      "${sawOpenIGTLink_SOURCE_DIR}/include"
      "${sawOpenIGTLink_BINARY_DIR}/include")
    set (sawOpenIGTLink_HEADER_DIR "${sawOpenIGTLink_SOURCE_DIR}/include/sawOpenIGTLink")
    set (sawOpenIGTLink_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}" "${OpenIGTLink_LIBRARY_DIRS}")
    set (sawOpenIGTLink_LIBRARIES sawOpenIGTLink ${OpenIGTLink_LIBRARIES})

    # Allow c++ code to find local header files
    include_directories (${sawOpenIGTLink_INCLUDE_DIR} ${sawOpenIGTLink_BINARY_DIR})

    cisst_add_config_files (sawOpenIGTLink)

    set (sawOpenIGTLink_SRC
         ${sawOpenIGTLink_HEADER_DIR}/sawOpenIGTLinkExport.h
         code/mtsCISSTToIGTL.cpp
         ${sawOpenIGTLink_HEADER_DIR}/mtsCISSTToIGTL.h
         code/mtsIGTLToCISST.cpp
         ${sawOpenIGTLink_HEADER_DIR}/mtsIGTLToCISST.h
         code/mtsIGTLBridge.cpp
         ${sawOpenIGTLink_HEADER_DIR}/mtsIGTLBridge.h
         code/mtsIGTLCRTKBridge.cpp
         ${sawOpenIGTLink_HEADER_DIR}/mtsIGTLCRTKBridge.h)

    add_library (sawOpenIGTLink ${IS_SHARED}
                 ${sawOpenIGTLink_SRC})

    set_target_properties (sawOpenIGTLink PROPERTIES
      VERSION ${sawOpenIGTLink_VERSION}
      FOLDER "sawOpenIGTLink")
    target_link_libraries (sawOpenIGTLink ${OpenIGTLink_LIBRARIES})
    cisst_target_link_libraries (sawOpenIGTLink ${REQUIRED_CISST_LIBRARIES})

    # Install target for headers and library
    install (DIRECTORY
             ${sawOpenIGTLink_SOURCE_DIR}/include/sawOpenIGTLink
             ${sawOpenIGTLink_BINARY_DIR}/include/sawOpenIGTLink
             DESTINATION include
             COMPONENT sawOpenIGTLink-dev)

    install (TARGETS sawOpenIGTLink COMPONENT sawOpenIGTLink
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION lib
             ARCHIVE DESTINATION lib)

  else (OpenIGTLink_FOUND)
    message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} requires OpenIGTLink.  Please install OpenIGTLink.  You might also have to set the OpenIGTLink directory manually using CMake, e.g. ccmake ${CMAKE_CURRENT_BINARY_DIR}")
  endif (OpenIGTLink_FOUND)

else (cisst_FOUND_AS_REQUIRED)
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires ${REQUIRED_CISST_LIBRARIES}")
endif (cisst_FOUND_AS_REQUIRED)
