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

#include <sawOpenIGTLink/mtsCISSTToIGTL.h>

bool mtsCISSTToIGTL(const prmPositionCartesianGet & cisstData,
                    igtl::Matrix4x4 & igtlData)
{
    if (!cisstData.Valid()) {
        return false;
    }
    igtlData[0][0] = cisstData.Position().Rotation().Element(0, 0);
    igtlData[1][0] = cisstData.Position().Rotation().Element(1, 0);
    igtlData[2][0] = cisstData.Position().Rotation().Element(2, 0);
    igtlData[3][0] = 0.0;

    igtlData[0][1] = cisstData.Position().Rotation().Element(0, 1);
    igtlData[1][1] = cisstData.Position().Rotation().Element(1, 1);
    igtlData[2][1] = cisstData.Position().Rotation().Element(2, 1);
    igtlData[3][1] = 0.0;

    igtlData[0][2] = cisstData.Position().Rotation().Element(0, 2);
    igtlData[1][2] = cisstData.Position().Rotation().Element(1, 2);
    igtlData[2][2] = cisstData.Position().Rotation().Element(2, 2);
    igtlData[3][2] = 0.0;

    igtlData[0][3] = cisstData.Position().Translation().Element(0);
    igtlData[1][3] = cisstData.Position().Translation().Element(1);
    igtlData[2][3] = cisstData.Position().Translation().Element(2);
    igtlData[3][3] = 1.0;

    return true;
}

bool mtsCISSTToIGTL(const prmPositionCartesianGet & cisstData,
                    igtl::TransformMessage::Pointer igtlData)
{
    // there must be a way to convert in-place
    igtl::Matrix4x4 dataMatrix;
    mtsCISSTToIGTL(cisstData, dataMatrix);
    igtlData->SetMatrix(dataMatrix);
    igtl::TimeStamp::Pointer timeStamp;
    timeStamp = igtl::TimeStamp::New();
    timeStamp->SetTime(cisstData.Timestamp());
    igtlData->SetTimeStamp(timeStamp);
    igtlData->Pack();
    return true;
}
