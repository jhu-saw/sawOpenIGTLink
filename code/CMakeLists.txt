#
#
# (C) Copyright 2011 Johns Hopkins University (JHU), All Rights
# Reserved.
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

find_package (OpenIGTLink REQUIRED)

if (OpenIGTLink_FOUND)

  set (REQUIRED_CISST_LIBRARIES cisstCommon
                                cisstVector
                                cisstOSAbstraction
                                cisstMultiTask
                                cisstParameterTypes)

  find_package (cisst REQUIRED ${REQUIRED_CISST_LIBRARIES})

  if (cisst_FOUND_AS_REQUIRED)

    # OpenIGTLink code
    include (${OpenIGTLink_USE_FILE})

    # load cisst configuration
    include (${CISST_USE_FILE})

    # create/configure file for find_package (sawOpenIGTLink)
    set (sawOpenIGTLink_INCLUDE_DIR "${sawOpenIGTLink_SOURCE_DIR}/include")
    set (sawOpenIGTLink_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}" "${OpenIGTLink_LIBRARY_DIRS}")
    set (sawOpenIGTLink_LIBRARIES sawOpenIGTLink ${OpenIGTLink_LIBRARIES})

    include_directories (${sawOpenIGTLink_INCLUDE_DIR})
    set (sawOpenIGTLink_HEADER_DIR "${sawOpenIGTLink_INCLUDE_DIR}/sawOpenIGTLink")
    add_library (sawOpenIGTLink ${IS_SHARED}
                 ${sawOpenIGTLink_HEADER_DIR}/sawOpenIGTLinkExport.h
                 mtsOpenIGTLink.cpp
                 ${sawOpenIGTLink_HEADER_DIR}/mtsOpenIGTLink.h
                 )
    set_property (TARGET sawOpenIGTLink PROPERTY FOLDER "sawOpenIGTLink")
    target_link_libraries (sawOpenIGTLink ${OpenIGTLink_LIBRARIES})
    cisst_target_link_libraries (sawOpenIGTLink ${REQUIRED_CISST_LIBRARIES})

    configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/sawOpenIGTLinkConfig.cmake.in"
                    "${sawOpenIGTLink_BINARY_DIR}/sawOpenIGTLinkConfig.cmake"
                     @ONLY@)

    # Install target for headers and library
    install (DIRECTORY
             ${sawOpenIGTLink_INCLUDE_DIR}/sawOpenIGTLink
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
