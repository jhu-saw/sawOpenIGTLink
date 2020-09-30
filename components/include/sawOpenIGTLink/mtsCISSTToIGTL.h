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

#ifndef _mtsCISSTToIGTL_h
#define _mtsCISSTToIGTL_h

#include <igtlTransformMessage.h>
#include <igtlStringMessage.h>
#include <igtlSensorMessage.h>
#include <igtlNDArrayMessage.h>
#include <igtlPointMessage.h>
#include <cisstMultiTask/mtsParameterTypes.h>
#include <cisstParameterTypes/prmPositionCartesianGet.h>
#include <cisstParameterTypes/prmVelocityCartesianGet.h>
#include <cisstParameterTypes/prmForceCartesianGet.h>
#include <cisstParameterTypes/prmStateJoint.h>
#include <cisstParameterTypes/prmEventButton.h>

bool mtsCISSTToIGTL(const std::string & cisstData,
                    igtl::StringMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const mtsMessage & cisstData,
                    igtl::StringMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const prmPositionCartesianGet & cisstData,
                    igtl::Matrix4x4 & igtlData);

bool mtsCISSTToIGTL(const prmPositionCartesianGet & cisstData,
                    igtl::TransformMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const prmVelocityCartesianGet & cisstData,
                    igtl::SensorMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const prmForceCartesianGet & cisstData,
                    igtl::SensorMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const prmStateJoint & cisstData,
                    igtl::SensorMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const prmStateJoint & cisstData,
                    igtl::NDArrayMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const prmEventButton & cisstData,
                    igtl::SensorMessage::Pointer igtlData);

bool mtsCISSTToIGTL(const vct3 & cisstData,
                    igtl::PointMessage::Pointer igtlData);

#endif  // _mtsCISSTToIGTL_h
