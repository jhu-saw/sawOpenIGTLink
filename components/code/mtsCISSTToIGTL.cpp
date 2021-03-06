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

bool mtsCISSTToIGTL(const std::string & cisstData,
                    igtl::StringMessage::Pointer igtlData)
{
    igtlData->SetString(cisstData);
    return true;
}

bool mtsCISSTToIGTL(const mtsMessage & cisstData,
                    igtl::StringMessage::Pointer igtlData)
{
    igtlData->SetString(cisstData.Message);
    igtl::TimeStamp::Pointer timeStamp;
    timeStamp = igtl::TimeStamp::New();
    timeStamp->SetTime(cisstData.Timestamp);
    igtlData->SetTimeStamp(timeStamp);
    return true;
}

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
    if (mtsCISSTToIGTL(cisstData, dataMatrix)) {
        igtlData->SetMatrix(dataMatrix);
        igtl::TimeStamp::Pointer timeStamp;
        timeStamp = igtl::TimeStamp::New();
        timeStamp->SetTime(cisstData.Timestamp());
        igtlData->SetTimeStamp(timeStamp);
        return true;
    }
    return false;
}

bool mtsCISSTToIGTL(const prmVelocityCartesianGet & cisstData,
                    igtl::SensorMessage::Pointer igtlData)
{
    if (!cisstData.Valid()) {
        return false;
    }
    igtlData->SetLength(6);
    igtlData->SetValue(0, cisstData.VelocityLinear().Element(0));
    igtlData->SetValue(1, cisstData.VelocityLinear().Element(1));
    igtlData->SetValue(2, cisstData.VelocityLinear().Element(2));
    igtlData->SetValue(3, cisstData.VelocityAngular().Element(0));
    igtlData->SetValue(4, cisstData.VelocityAngular().Element(1));
    igtlData->SetValue(5, cisstData.VelocityAngular().Element(2));
    igtl::TimeStamp::Pointer timeStamp;
    timeStamp = igtl::TimeStamp::New();
    timeStamp->SetTime(cisstData.Timestamp());
    igtlData->SetTimeStamp(timeStamp);
    return true;
}

bool mtsCISSTToIGTL(const prmForceCartesianGet & cisstData,
                    igtl::SensorMessage::Pointer igtlData)
{
    if (!cisstData.Valid()) {
        return false;
    }
    igtlData->SetLength(6);
    igtlData->SetValue(const_cast<double *>(cisstData.Force().Pointer()));
    igtl::TimeStamp::Pointer timeStamp;
    timeStamp = igtl::TimeStamp::New();
    timeStamp->SetTime(cisstData.Timestamp());
    igtlData->SetTimeStamp(timeStamp);
    return true;
}

bool mtsCISSTToIGTL(const prmStateJoint & cisstData,
                    igtl::SensorMessage::Pointer igtlData)
{
    if (!cisstData.Valid()) {
        return false;
    }
    igtlData->SetLength(cisstData.Position().size());
    igtlData->SetValue(const_cast<double *>(cisstData.Position().Pointer()));
    igtl::TimeStamp::Pointer timeStamp;
    timeStamp = igtl::TimeStamp::New();
    timeStamp->SetTime(cisstData.Timestamp());
    igtlData->SetTimeStamp(timeStamp);
    return true;
}

bool mtsCISSTToIGTL(const prmStateJoint & cisstData,
                    igtl::NDArrayMessage::Pointer igtlData)
{
    if (!cisstData.Valid()) {
        return false;
    }
    // determine number of joints for igtl
    igtlUint16 nbJoints = cisstData.Name().size();
    // create 6 rows for pos_flag/pos/vel_flag/vel/effort_flag/effort
    vctDoubleMat matrix(6, nbJoints);
    if (nbJoints != 0) {
        // position
        if (cisstData.Position().size() == 0) {
            matrix.Row(0).SetAll(0.0);
            matrix.Row(1).SetAll(0.0);
        } else {
            matrix.Row(0).SetAll(1.0);
            matrix.Row(1).Assign(cisstData.Position());
        }
        // velocity
        if (cisstData.Velocity().size() == 0) {
            matrix.Row(2).SetAll(0.0);
            matrix.Row(3).SetAll(0.0);
        } else {
            matrix.Row(2).SetAll(1.0);
            matrix.Row(3).Assign(cisstData.Velocity());
        }
        // effort
        if (cisstData.Effort().size() == 0) {
            matrix.Row(4).SetAll(0.0);
            matrix.Row(5).SetAll(0.0);
        } else {
            matrix.Row(4).SetAll(1.0);
            matrix.Row(5).Assign(cisstData.Effort());
        }
    }
    // copy all matrix to array
    std::vector<igtlUint16> size(2);
    size[0] = 6;
    size[1] = nbJoints;
    igtl::Array<igtl_float64> * array = new igtl::Array<igtl_float64>;
    array->SetSize(size);
    array->SetArray((void*) matrix.Pointer()); // this perform a deep copy
    // assign array to message, message will delete array when done
    igtlData->SetArray(igtl::NDArrayMessage::TYPE_FLOAT64, array);
    igtl::TimeStamp::Pointer timeStamp;
    timeStamp = igtl::TimeStamp::New();
    timeStamp->SetTime(cisstData.Timestamp());
    igtlData->SetTimeStamp(timeStamp);
    return true;
}

bool mtsCISSTToIGTL(const prmEventButton & cisstData,
                    igtl::SensorMessage::Pointer igtlData)
{
    if (!cisstData.Valid()) {
        return false;
    }
    igtlData->SetLength(1);
    if (cisstData.Type() == prmEventButton::PRESSED) {
        igtlData->SetValue(0, 1.0);
    } else if (cisstData.Type() == prmEventButton::RELEASED) {
        igtlData->SetValue(0, 0.0);
    } else if (cisstData.Type() == prmEventButton::CLICKED) {
        igtlData->SetValue(0, 2.0);
    }
    igtl::TimeStamp::Pointer timeStamp;
    timeStamp = igtl::TimeStamp::New();
    timeStamp->SetTime(cisstData.Timestamp());
    igtlData->SetTimeStamp(timeStamp);
    return true;
}

bool mtsCISSTToIGTL(const vct3 &cisstData, igtl::PointMessage::Pointer igtlData)
{
    igtl::PointElement::Pointer p;
    p = igtl::PointElement::New();
    p->SetPosition(cisstData.X(),cisstData.Y(),cisstData.Z());
    igtlData->AddPointElement(p);
    return true;
}
