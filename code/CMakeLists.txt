#
# $Id$
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

  add_library (sawOpenIGTLink ${IS_SHARED}
               ../sawOpenIGTLinkExport.h
               mtsOpenIGTLink.cpp
               ../mtsOpenIGTLink.h
               )

  target_link_libraries (sawOpenIGTLink ${OpenIGTLink_LIBRARIES})

  set (sawOpenIGTLink_INCLUDE_DIR "${sawComponents_SOURCE_DIR}")
  set (sawOpenIGTLink_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}" "${OpenIGTLink_LIBRARY_DIRS}")
  set (sawOpenIGTLink_LIBRARIES sawOpenIGTLink ${OpenIGTLink_LIBRARIES})

  configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/sawOpenIGTLinkConfig.cmake.in"
                  "${sawComponents_BINARY_DIR}/sawOpenIGTLinkConfig.cmake"
                   @ONLY@)

  cisst_target_link_libraries (sawOpenIGTLink ${REQUIRED_CISST_LIBRARIES})

else (cisst_FOUND_AS_REQUIRED)
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires ${REQUIRED_CISST_LIBRARIES}")
endif (cisst_FOUND_AS_REQUIRED)
