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

find_package (OpenIGTLink REQUIRED)

if (OpenIGTLink_FOUND)
  add_subdirectory (code)
  add_subdirectory (examples)
else (OpenIGTLink_FOUND)
  message (SEND_ERROR "code in ${CMAKE_CURRENT_SOURCE_DIR} requires OpenIGTLink.")
endif (OpenIGTLink_FOUND)
