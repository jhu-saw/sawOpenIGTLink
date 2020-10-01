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

#include <sawOpenIGTLink/mtsIGTLToCISST.h>

bool mtsIGTLToCISST(const igtl::StringMessage::Pointer igtlData,
                    std::string & cisstData)
{
    cisstData = igtlData->GetString();
    return true;
}

bool mtsIGTLToCISST(const igtl::Matrix4x4 & igtlData,
                   prmPositionCartesianSet & cisstData)
{
    cisstData.Goal().Rotation().Element(0, 0) = igtlData[0][0];
    cisstData.Goal().Rotation().Element(1, 0) = igtlData[1][0];
    cisstData.Goal().Rotation().Element(2, 0) = igtlData[2][0];

    cisstData.Goal().Rotation().Element(0, 1) = igtlData[0][1];
    cisstData.Goal().Rotation().Element(1, 1) = igtlData[1][1];
    cisstData.Goal().Rotation().Element(2, 1) = igtlData[2][1];

    cisstData.Goal().Rotation().Element(0, 2) = igtlData[0][2];
    cisstData.Goal().Rotation().Element(1, 2) = igtlData[1][2];
    cisstData.Goal().Rotation().Element(2, 2) = igtlData[2][2];

    cisstData.Goal().Rotation().NormalizedSelf();

    cisstData.Goal().Translation().Element(0) = igtlData[0][3];
    cisstData.Goal().Translation().Element(1) = igtlData[1][3];
    cisstData.Goal().Translation().Element(2) = igtlData[2][3];

    cisstData.SetValid(true);
    return true;
}

bool mtsIGTLToCISST(const igtl::TransformMessage::Pointer igtlData,
                    prmPositionCartesianSet & cisstData)
{
    igtl::Matrix4x4 m;
    igtlData->GetMatrix(m);
    if (mtsIGTLToCISST(m,cisstData)){
        return true;
    }

    return false;
}

bool mtsIGTLToCISST(const igtl::SensorMessage::Pointer igtlData,
                    prmForceCartesianSet & cisstData)
{
    const unsigned int length = igtlData->GetLength();
    if (length != 6) {
        return false;
    }
    for (unsigned int index = 0; index < length; ++index) {
        cisstData.Force().Element(index) = igtlData->GetValue(index);
    }
    return true;
}

bool mtsIGTLToCISST(const igtl::SensorMessage::Pointer igtlData,
                    prmStateJoint & cisstData)
{
    std::cerr << CMN_LOG_DETAILS << " needs to be implemented" << std::endl;
    return false;
}

bool mtsIGTLToCISST(const igtl::PointMessage::Pointer igtlData, vct3 &cisstData)
{
    igtl::PointElement::Pointer p;
    p = igtl::PointElement::New();
    igtlData->GetPointElement(0,p);
    float x,y,z;
    p->GetPosition(x,y,z);
    cisstData[0] = x;
    cisstData[1] = y;
    cisstData[2] = z;

    return true;
}
