#
#
#   Author(s):  Ali Uneri
#   Created on: 2009-08-13
#
#   (C) Copyright 2007-2011 Johns Hopkins University (JHU), All Rights Reserved.
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
                              cisstMultiTask cisstParameterTypes cisstFLTK cisstStereoVision cisstStereoVisionQt)

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


  # sawOpenIGTLink has been compiled within cisst, we should find it automatically
  cisst_find_saw_component (sawOpenIGTLink REQUIRED)

  if (sawOpenIGTLink_FOUND)

    include_directories (${sawOpenIGTLink_INCLUDE_DIR})
    link_directories (${sawOpenIGTLink_LIBRARY_DIR})

      if (CISST_SVL_HAS_X11 OR WIN32)

        if (CISST_HAS_QT4)
          set (USED_CISST_LIBRARIES cisstCommon cisstVector cisstOSAbstraction cisstMultiTask cisstStereoVision cisstStereoVisionQt)
        else (CISST_HAS_QT4)
          set (USED_CISST_LIBRARIES cisstCommon cisstVector cisstOSAbstraction cisstMultiTask cisstStereoVision)
        endif (CISST_HAS_QT4)

        fltk_wrap_ui (sawOpenIGTLinkExampleFLTK
                      trackerUI.fl
                      )

        add_executable (sawOpenIGTLinkExampleFLTK
                        main.cpp
                        trackerSimulator.cpp
                        trackerSimulator.h
                        ${sawOpenIGTLinkExampleFLTK_FLTK_UI_SRCS}
                        )

        add_executable (sawOpenIGTLinkSvlExample
                        sawOpenIGTLinkSvlExample.cpp
                        svlOpenIGTLink.cpp
                        svlOpenIGTLink.h
                        )

        set_property (TARGET sawOpenIGTLinkExampleFLTK PROPERTY FOLDER "sawOpenIGTLink")

        target_link_libraries (sawOpenIGTLinkExampleFLTK ${sawOpenIGTLink_LIBRARIES})

        cisst_target_link_libraries (sawOpenIGTLinkExampleFLTK ${REQUIRED_CISST_LIBRARIES})

        set_property (TARGET sawOpenIGTLinkSvlExample PROPERTY FOLDER "sawOpenIGTLink")

        target_link_libraries (sawOpenIGTLinkSvlExample ${sawOpenIGTLink_LIBRARIES})

        cisst_target_link_libraries (sawOpenIGTLinkSvlExample ${REQUIRED_CISST_LIBRARIES})


      else (CISST_SVL_HAS_X11 OR WIN32)
        message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires CISST_SVL_HAS_X11 or WIN32")
      endif (CISST_SVL_HAS_X11 OR WIN32)
    # sawOpenIGTLink configuration
  endif (sawOpenIGTLink_FOUND)

else (cisst_FOUND_AS_REQUIRED)
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires ${REQUIRED_CISST_LIBRARIES}")
endif (cisst_FOUND_AS_REQUIRED)
