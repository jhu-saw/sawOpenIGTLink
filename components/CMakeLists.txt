#
# (C) Copyright 2011-2017 Johns Hopkins University (JHU), All Rights Reserved.
#
# --- begin cisst license - do not edit ---
#
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
#
# --- end cisst license ---

project (sawOpenIGTLink)

cmake_minimum_required (VERSION 2.8)

# Version 2+ is required but somehow OpenIGTLinkConfig doesn't define version
find_package (OpenIGTLink
              HINTS igtl /usr/lib/igtl /usr/local/lib/igtl)

if (OpenIGTLink_FOUND)

  # OpenIGTLink settings
  include (${OpenIGTLink_USE_FILE})

  set (REQUIRED_CISST_LIBRARIES cisstCommon
                                cisstVector
                                cisstOSAbstraction
                                cisstMultiTask
                                cisstParameterTypes
                                cisstStereoVision)

  find_package (cisst REQUIRED ${REQUIRED_CISST_LIBRARIES})

  if (cisst_FOUND_AS_REQUIRED)

    # load cisst configuration
    include (${CISST_USE_FILE})

    # catkin/ROS paths
    cisst_is_catkin_build (sawOpenIGTLink_IS_CATKIN_BUILT)
    if (sawOpenIGTLink_IS_CATKIN_BUILT)
       set (sawOpenIGTLink_CONFIG_FILE_DIR
            "${CATKIN_DEVEL_PREFIX}/share/sawOpenIGTLink/cmake")
       set (LIBRARY_OUTPUT_PATH "${sawOpenIGTLink_BINARY_DIR}/lib")
    else ()
       set (sawOpenIGTLink_CONFIG_FILE_DIR "${sawOpenIGTLink_BINARY_DIR}")
    endif ()

    # create/configure file for find_package (sawOpenIGTLink)
    set (sawOpenIGTLink_INCLUDE_DIR "${sawOpenIGTLink_SOURCE_DIR}/include")
    set (sawOpenIGTLink_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}" "${OpenIGTLink_LIBRARY_DIRS}")
    set (sawOpenIGTLink_LIBRARIES sawOpenIGTLink ${OpenIGTLink_LIBRARIES})

    # Set the version number
    set (sawOpenIGTLink_VERSION_MAJOR "1")
    set (sawOpenIGTLink_VERSION_MINOR "1")
    set (sawOpenIGTLink_VERSION_PATCH "1")
    set (sawOpenIGTLink_VERSION "${sawOpenIGTLink_VERSION_MAJOR}.${sawOpenIGTLink_VERSION_MINOR}.${sawOpenIGTLink_VERSION_PATCH}")
    # Generate sawOpenIGTLinkRevision.h
    configure_file ("${sawOpenIGTLink_SOURCE_DIR}/code/sawOpenIGTLinkRevision.h.in"
                    "${sawOpenIGTLink_BINARY_DIR}/include/sawOpenIGTLink/sawOpenIGTLinkRevision.h")

    include_directories (${sawOpenIGTLink_INCLUDE_DIR})

    set (sawOpenIGTLink_HEADER_DIR "${sawOpenIGTLink_INCLUDE_DIR}/sawOpenIGTLink")
    add_library (sawOpenIGTLink ${IS_SHARED}
                 ${sawOpenIGTLink_HEADER_DIR}/sawOpenIGTLinkExport.h
                 # old implementation
                 code/mtsOpenIGTLink.cpp
                 ${sawOpenIGTLink_HEADER_DIR}/mtsOpenIGTLink.h
                 # new implementation (05/2015)
                 code/mtsOpenIGTLinkBridge.cpp
                 ${sawOpenIGTLink_HEADER_DIR}/mtsOpenIGTLinkBridge.h
                 code/svlOpenIGTLinkBridge.cpp
                 ${sawOpenIGTLink_HEADER_DIR}/svlOpenIGTLinkBridge.h
                 )
    set_property (TARGET sawOpenIGTLink PROPERTY FOLDER "sawOpenIGTLink")
    target_link_libraries (sawOpenIGTLink ${OpenIGTLink_LIBRARIES})
    cisst_target_link_libraries (sawOpenIGTLink ${REQUIRED_CISST_LIBRARIES})

    set (sawOpenIGTLink_CMAKE_CONFIG_FILE
         "${sawOpenIGTLink_CONFIG_FILE_DIR}/sawOpenIGTLinkConfig.cmake")

    configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/sawOpenIGTLinkConfig.cmake.in"
                    "${sawOpenIGTLink_CMAKE_CONFIG_FILE}"
                     @ONLY)

    cisst_add_config_version ("${sawOpenIGTLink_CONFIG_FILE_DIR}/sawOpenIGTLinkConfigVersion.cmake"
                              VERSION ${sawOpenIGTLink_VERSION}
                              DESTINATION "share/sawOpenIGTLink"
                              COMPONENT sawOpenIGTLink)

    # Install target for headers and library
    install (DIRECTORY
             ${sawOpenIGTLink_INCLUDE_DIR}/sawOpenIGTLink
             ${sawOpenIGTLink_BINARY_DIR}/include/sawOpenIGTLink
             DESTINATION include
             PATTERN .svn EXCLUDE)

    install (TARGETS sawOpenIGTLink
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION lib
             ARCHIVE DESTINATION lib)

    install (FILES "${sawOpenIGTLink_BINARY_DIR}/sawOpenIGTLinkConfig.cmake"
             DESTINATION "${CISST_CMAKE_INSTALL_SUFFIX}/saw")

  else (cisst_FOUND_AS_REQUIRED)
    message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires ${REQUIRED_CISST_LIBRARIES}")
  endif (cisst_FOUND_AS_REQUIRED)

else (OpenIGTLink_FOUND)
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} requires OpenIGTLink.  Please install OpenIGTLink.  You might also have to set the OpenIGTLink directory manually using CMake, e.g. ccmake ${CMAKE_CURRENT_BINARY_DIR}")
endif (OpenIGTLink_FOUND)
