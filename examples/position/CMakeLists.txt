#
#   Author(s):  Ali Uneri
#   Created on: 2009-08-13
#
#   (C) Copyright 2009-2017 Johns Hopkins University (JHU), All Rights Reserved.
#
# --- begin cisst license - do not edit ---
#
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
#
# --- end cisst license ---

# create a list of libraries needed for this project
set (REQUIRED_CISST_LIBRARIES cisstCommon cisstVector cisstOSAbstraction
                              cisstMultiTask cisstParameterTypes cisstFLTK)

# find cisst and make sure the required libraries have been compiled
find_package (cisst REQUIRED ${REQUIRED_CISST_LIBRARIES})

if (cisst_FOUND_AS_REQUIRED)

  # load cisst configuration
  include (${CISST_USE_FILE})

  # catkin/ROS paths
  cisst_is_catkin_build (sawOpenIGTLinkExamples_IS_CATKIN_BUILT)
  if (sawOpenIGTLinkExamples_IS_CATKIN_BUILT)
    set (EXECUTABLE_OUTPUT_PATH "${CATKIN_DEVEL_PREFIX}/bin")
  endif ()

  find_package (OpenIGTLink)

  if (OpenIGTLink_FOUND)
     # OpenIGTLink code
    include (${OpenIGTLink_USE_FILE})
  endif (OpenIGTLink_FOUND)

  # sawOpenIGTLink has been compiled within cisst, we should find it automatically
  find_package (sawOpenIGTLink)

  if (sawOpenIGTLink_FOUND)
    include_directories (${sawOpenIGTLink_INCLUDE_DIR})
    link_directories (${sawOpenIGTLink_LIBRARY_DIR})

    fltk_wrap_ui (sawOpenIGTLinkExampleFLTK
                  trackerUI.fl
                  )

    add_executable (sawOpenIGTLinkExampleFLTK
                    main.cpp
                    trackerSimulator.cpp
                    trackerSimulator.h
                    ${sawOpenIGTLinkExampleFLTK_FLTK_UI_SRCS}
                    )

    set_property (TARGET sawOpenIGTLinkExampleFLTK PROPERTY FOLDER "sawOpenIGTLink")
    target_link_libraries (sawOpenIGTLinkExampleFLTK ${sawOpenIGTLink_LIBRARIES})
    cisst_target_link_libraries (sawOpenIGTLinkExampleFLTK ${REQUIRED_CISST_LIBRARIES})

  else (sawOpenIGTLink_FOUND)
    message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires sawOpenIGTLink")
  endif (sawOpenIGTLink_FOUND)

else (cisst_FOUND_AS_REQUIRED)
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires ${REQUIRED_CISST_LIBRARIES}")
endif (cisst_FOUND_AS_REQUIRED)
