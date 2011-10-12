/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Ali Uneri
  Created on: 2009-08-10

  (C) Copyright 2009-2011 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstOSAbstraction/osaSleep.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>

#include <sawOpenIGTLink/mtsOpenIGTLink.h>

#include <igtl_util.h>
#include <igtl_header.h>
#include <igtl_transform.h>


/*! \brief Hard copies the rotation and translation from a
  prmPositionCartesianGet to a float array of size 12
  \param frameCISST Frame to be converted
  \param frameIGT Result of conversion */
void mtsOpenIGTLinkFrameCISSTtoIGT(const prmPositionCartesianGet & frameCISST,
                                   igtl_float32 * frameIGT);

/*! \brief Hard copies a float array of size 12 to the rotation and
  translation of a prmPositionCartesianGet
  \param frameIGT Frame to be converted
  \param frameCISST Converted frame */
void mtsOpenIGTLinkFrameIGTtoCISST(const igtl_float32 * frameIGT,
                                   prmPositionCartesianGet & frameCISST);


// private data using igtl types
class sawOpenIGTLinkData {
public:
    igtl_header HeaderIGT;
    igtl_float32 FrameIGT[12];
};


CMN_IMPLEMENT_SERVICES(mtsOpenIGTLink);


mtsOpenIGTLink::mtsOpenIGTLink(const std::string & taskName, const double period,
                               const unsigned short port):
    mtsTaskPeriodic(taskName, period, false, 500)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "mtsOpenIGTLink: setting up as server" << std::endl;
    ConnectionType = SERVER;
    DeviceName = taskName;
    Port = port;
    IsConnected = false;
    SocketServer = new osaSocketServer();
    Socket = 0;
    Initialize();
}


mtsOpenIGTLink::mtsOpenIGTLink(const std::string & taskName, const double period,
                               const std::string & host, const unsigned short port):
    mtsTaskPeriodic(taskName, period, false, 500)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "mtsOpenIGTLink: setting up as client" << std::endl;
    ConnectionType = CLIENT;
    DeviceName = taskName;
    Host = host;
    Port = port;
    IsConnected = false;
    SocketServer = 0;
    Socket = new osaSocket();
    IGTLData = new sawOpenIGTLinkData;
    Initialize();
}


mtsOpenIGTLink::~mtsOpenIGTLink(void)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "~mtsOpenIGTLink: closing hanging connections" << std::endl;
    if (Socket) Socket->Close();
    delete Socket;
    if (SocketServer) SocketServer->Close();
    delete SocketServer;
    delete IGTLData;
}


void mtsOpenIGTLink::Initialize(void)
{
    // required interfaces
    mtsInterfaceRequired * requiredInterface;
    requiredInterface = AddInterfaceRequired("RequiresPositionCartesian");
    if (requiredInterface) {
        requiredInterface->AddFunction("GetPositionCartesian", GetPositionCartesian);
    }

    // provided interfaces
    mtsInterfaceProvided * providedInterface;
    providedInterface = AddInterfaceProvided("ProvidesPositionCartesian");
    if (providedInterface) {
        StateTable.AddData(FrameRecv, "FrameRecv");
        providedInterface->AddCommandReadState(StateTable, FrameRecv, "GetPositionCartesian");
    }
}


void mtsOpenIGTLink::Startup(void)
{
    if (ConnectionType == SERVER) {
        while (!SocketServer->AssignPort(Port)) {
            CMN_LOG_CLASS_INIT_WARNING << "Startup: will try again in 5 seconds" << std::endl;
            osaSleep(5.0 * cmn_s);
        }
        SocketServer->Listen();
    }
}


void mtsOpenIGTLink::Run(void)
{
    if (!IsConnected) {
        if (ConnectionType == SERVER) {
            do {
                Socket = SocketServer->Accept();
            } while (Socket == NULL);
            IsConnected = true;
        } else if (ConnectionType == CLIENT) {
            IsConnected = Socket->Connect(Host.c_str(), Port);
        }
    }

    if (IsConnected) {
        GetPositionCartesian(FrameSend);
        // assume the device is disconnected, if it is not able to send a message
        IsConnected = SendFrame(FrameSend);

        // if there is message
        if (ReceiveHeader(MessageType)) {
            if (MessageType.compare("TRANSFORM") == 0) {
                ReceiveFrame(FrameRecv);
            } else if (MessageType.compare("POSITION") == 0) {
                CMN_LOG_CLASS_RUN_WARNING << "Run: data type not supported, skipping" << std::endl;
                SkipMessage();
            } else if (MessageType.compare("IMAGE") == 0) {
                CMN_LOG_CLASS_RUN_WARNING << "Run: data type not supported, skipping" << std::endl;
                SkipMessage();
            } else if (MessageType.compare("STATUS") == 0) {
                CMN_LOG_CLASS_RUN_WARNING << "Run: data type not supported, skipping" << std::endl;
                SkipMessage();
            } else {
                CMN_LOG_CLASS_RUN_ERROR << "Run: receiving unknown data type, skipping" << std::endl;
                SkipMessage();
            }
        }
    }
}


bool mtsOpenIGTLink::SendFrame(const prmPositionCartesianGet & frameCISST)
{
    mtsOpenIGTLinkFrameCISSTtoIGT(frameCISST, IGTLData->FrameIGT);
    igtl_transform_convert_byte_order(IGTLData->FrameIGT);  // convert endian if necessary

    igtl_uint64 crc = crc64(0, 0, 0LL);  // initial CRC

    IGTLData->HeaderIGT.version   = IGTL_HEADER_VERSION;
    IGTLData->HeaderIGT.timestamp = 0;
    IGTLData->HeaderIGT.body_size = IGTL_TRANSFORM_SIZE;
    IGTLData->HeaderIGT.crc       = crc64(reinterpret_cast<unsigned char *>(IGTLData->FrameIGT),
                                          IGTL_TRANSFORM_SIZE, crc);

    strncpy(IGTLData->HeaderIGT.name, "TRANSFORM", 12);
    strncpy(IGTLData->HeaderIGT.device_name, DeviceName.c_str(), 20);

    igtl_header_convert_byte_order(&(IGTLData->HeaderIGT));  // convert endian if necessary

    Socket->Send(reinterpret_cast<const char *>(&(IGTLData->HeaderIGT)),
                 IGTL_HEADER_SIZE);
    int bytesSent = Socket->Send(reinterpret_cast<const char *>(IGTLData->FrameIGT),
                                 IGTL_TRANSFORM_SIZE);
    if (bytesSent == -1) {
        return false;
    }
    CMN_LOG_CLASS_RUN_DEBUG << "SendFrame: sent " << frameCISST << std::endl;
    return true;
}


bool mtsOpenIGTLink::ReceiveHeader(std::string & messageType)
{
    int bytesRead = Socket->Receive(reinterpret_cast<char *>(&(IGTLData->HeaderIGT)),
                                    IGTL_HEADER_SIZE);
    if (bytesRead == 0) {
        CMN_LOG_CLASS_RUN_DEBUG << "ReceiveHeader: no message to receive" << std::endl;
        return false;
    } else if (bytesRead != IGTL_HEADER_SIZE) {
        CMN_LOG_CLASS_RUN_ERROR << "ReceiveHeader: size mismatch" << std::endl;
        return false;
    }

    igtl_header_convert_byte_order(&(IGTLData->HeaderIGT));  // convert endian if necessary
    messageType.assign(IGTLData->HeaderIGT.name);
    CMN_LOG_CLASS_RUN_DEBUG << "ReceiveHeader: receiving a " << messageType << std::endl;
    return true;
}


bool mtsOpenIGTLink::ReceiveFrame(prmPositionCartesianGet & frameCISST)
{
    int bytesRead = Socket->Receive(reinterpret_cast<char *>(IGTLData->FrameIGT),
                                    IGTL_TRANSFORM_SIZE);
    if (bytesRead != IGTL_TRANSFORM_SIZE) {
        CMN_LOG_CLASS_RUN_ERROR << "ReceiveFrame: size mismatch" << std::endl;
        return false;
    }

    igtl_transform_convert_byte_order(IGTLData->FrameIGT);  // convert endian if necessary
    mtsOpenIGTLinkFrameIGTtoCISST(IGTLData->FrameIGT, frameCISST);
    CMN_LOG_CLASS_RUN_DEBUG << "ReceiveFrame: received " << frameCISST << std::endl;
    return true;
}


void mtsOpenIGTLink::SkipMessage(void)
{
    unsigned long long dummySize = IGTLData->HeaderIGT.body_size;
    const int blockSize = 512;
    char dummy[blockSize];
    int bytesRead;

    do {
        bytesRead = Socket->Receive(dummy, blockSize);
        dummySize -= bytesRead;
    } while (dummySize > 0);
}


void mtsOpenIGTLinkFrameCISSTtoIGT(const prmPositionCartesianGet & frameCISST,
                                   igtl_float32 * frameIGT)
{
    for (size_t i = 0; i < 3; i++) {
        frameIGT[i]   = static_cast<float>(frameCISST.Position().Rotation().Element(i,0));
        frameIGT[i+3] = static_cast<float>(frameCISST.Position().Rotation().Element(i,1));
        frameIGT[i+6] = static_cast<float>(frameCISST.Position().Rotation().Element(i,2));
        frameIGT[i+9] = static_cast<float>(frameCISST.Position().Translation().Element(i));
    }
}


void mtsOpenIGTLinkFrameIGTtoCISST(const igtl_float32 * frameIGT,
                                   prmPositionCartesianGet & frameCISST)
{
    for (size_t i = 0; i < 3; i++) {
        frameCISST.Position().Rotation().Element(i,0) = frameIGT[i];
        frameCISST.Position().Rotation().Element(i,1) = frameIGT[i+3];
        frameCISST.Position().Rotation().Element(i,2) = frameIGT[i+6];
        frameCISST.Position().Translation().Element(i) = frameIGT[i+9];
    }
}
