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
                              cisstMultiTask cisstParameterTypes cisstFLTK)

# find cisst and make sure the required libraries have been compiled
find_package (cisst REQUIRED ${REQUIRED_CISST_LIBRARIES})

if (cisst_FOUND_AS_REQUIRED)

  # load cisst configuration
  include (${CISST_USE_FILE})

  # sawOpenIGTLink has been compiled within cisst, we should find it automatically
  cisst_find_saw_component (sawOpenIGTLink REQUIRED)

  if (sawOpenIGTLink_FOUND)

    # sawOpenIGTLink configuration
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

  endif (sawOpenIGTLink_FOUND)

else (cisst_FOUND_AS_REQUIRED)
  message ("Information: code in ${CMAKE_CURRENT_SOURCE_DIR} will not be compiled, it requires ${REQUIRED_CISST_LIBRARIES}")
endif (cisst_FOUND_AS_REQUIRED)
