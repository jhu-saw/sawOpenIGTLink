#
# (C) Copyright 2011-2015 Johns Hopkins University (JHU), All Rights Reserved.
#
# --- begin cisst license - do not edit ---
#
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
#
# --- end cisst license ---

# To find FindOpenIGTLink.cmake
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

# Version 2+ is required but somehow OpenIGTLinkConfig doesn't define version
find_package (OpenIGTLink REQUIRED
              HINTS igtl /usr/lib/igtl /usr/local/lib/igtl)

if (OpenIGTLink_FOUND)

  # OpenIGTLink settings
  include (${OpenIGTLink_USE_FILE})

  set (REQUIRED_CISST_LIBRARIES cisstCommon
                                cisstVector
                                cisstOSAbstraction
                                cisstMultiTask
                                cisstStereoVision
                                cisstParameterTypes)

  find_package (cisst REQUIRED ${REQUIRED_CISST_LIBRARIES})

  if (cisst_FOUND_AS_REQUIRED)

    # load cisst configuration
    include (${CISST_USE_FILE})

    # set flag to indicate that sawOpenIGTLink has been compiled with SVL
    # ideally this should be optional
    set (sawOpenIGTLink_HAS_SVL ON)

    # catkin/ROS paths
    cisst_is_catkin_build (sawOpenIGTLink_IS_CATKIN_BUILT)
    if (sawOpenIGTLink_IS_CATKIN_BUILT)
       set (sawOpenIGTLink_CONFIG_FILE_DIR "${CATKIN_DEVEL_PREFIX}/cmake")
       set (LIBRARY_OUTPUT_PATH "${sawOpenIGTLink_BINARY_DIR}/lib")
    else ()
       set (sawOpenIGTLink_CONFIG_FILE_DIR "${sawOpenIGTLink_BINARY_DIR}")
    endif ()

    # create/configure file for find_package (sawOpenIGTLink)
    set (sawOpenIGTLink_INCLUDE_DIR "${sawOpenIGTLink_SOURCE_DIR}/include")
    set (sawOpenIGTLink_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}" "${OpenIGTLink_LIBRARY_DIRS}")
    set (sawOpenIGTLink_LIBRARIES sawOpenIGTLink ${OpenIGTLink_LIBRARIES})

    include_directories (${sawOpenIGTLink_INCLUDE_DIR})

    set (sawOpenIGTLink_HEADER_DIR "${sawOpenIGTLink_INCLUDE_DIR}/sawOpenIGTLink")
    add_library (sawOpenIGTLink ${IS_SHARED}
                 ${sawOpenIGTLink_HEADER_DIR}/sawOpenIGTLinkExport.h
                 # old implementation
                 mtsOpenIGTLink.cpp
                 ${sawOpenIGTLink_HEADER_DIR}/mtsOpenIGTLink.h
                 # new implementation (05/2015)
                 mtsOpenIGTLinkBridge.cpp
                 ${sawOpenIGTLink_HEADER_DIR}/mtsOpenIGTLinkBridge.h
                 svlOpenIGTLinkBridge.cpp
                 ${sawOpenIGTLink_HEADER_DIR}/svlOpenIGTLinkBridge.h
                 )
    set_property (TARGET sawOpenIGTLink PROPERTY FOLDER "sawOpenIGTLink")
    target_link_libraries (sawOpenIGTLink ${OpenIGTLink_LIBRARIES})
    cisst_target_link_libraries (sawOpenIGTLink ${REQUIRED_CISST_LIBRARIES})

    configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/sawOpenIGTLinkConfig.cmake.in"
                    "${sawOpenIGTLink_CONFIG_FILE_DIR}/sawOpenIGTLinkConfig.cmake"
                     @ONLY@)

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
  message (SEND_ERROR "code in ${CMAKE_CURRENT_SOURCE_DIR} requires OpenIGTLink.")
endif (OpenIGTLink_FOUND)
