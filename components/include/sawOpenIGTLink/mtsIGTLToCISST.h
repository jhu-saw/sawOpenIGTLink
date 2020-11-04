/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/*

  Author(s):  Anton Deguet
  Created on: 2020-05-25

  (C) Copyright 2020 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#ifndef _mtsITGLToCISST_h
#define _mtsITGLToCISST_h

#include <igtlTransformMessage.h>
#include <igtlStringMessage.h>
#include <igtlSensorMessage.h>
#include <igtlPointMessage.h>
#include <cisstMultiTask/mtsParameterTypes.h>
#include <cisstParameterTypes/prmPositionJointSet.h>
#include <cisstParameterTypes/prmPositionCartesianSet.h>
#include <cisstParameterTypes/prmForceCartesianSet.h>
#include <cisstParameterTypes/prmStateJoint.h>

bool mtsIGTLToCISST(const igtl::StringMessage::Pointer igtlData,
                    std::string & cisstData);

bool mtsIGTLToCISST(const igtl::SensorMessage::Pointer & igtlData,
                    prmPositionJointSet & cisstData);

bool mtsIGTLToCISST(const igtl::Matrix4x4 & igtlData,
                    prmPositionCartesianSet & cisstData);

bool mtsIGTLToCISST(const igtl::TransformMessage::Pointer igtlData,
                    prmPositionCartesianSet & cisstData);

bool mtsIGTLToCISST(const igtl::SensorMessage::Pointer igtlData,
                    prmForceCartesianSet & cisstData);

bool mtsIGTLToCISST(const igtl::SensorMessage::Pointer igtlData,
                    prmStateJoint & cisstData);

bool mtsIGTLToCISST(const igtl::PointMessage::Pointer igtlData,
                    vct3 & cisstData);

#endif  // _mtsITGLToCISST_h
