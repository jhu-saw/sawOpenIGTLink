/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Youri Tan, Anton Deguet
  Created on: 2015-07-24

  (C) Copyright 2009-2020 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <sawOpenIGTLink/mtsIGTLBridge.h>
#include <sawOpenIGTLink/mtsCISSTToIGTL.h>
#include <sawOpenIGTLink/mtsIGTLToCISST.h>

#include <igtlServerSocket.h>
#include <igtlTimeStamp.h>
#include <igtlMessageBase.h>

CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsIGTLBridge, mtsTaskPeriodic, mtsTaskPeriodicConstructorArg);

class mtsIGTLBridgeData {
public:
    igtl::ServerSocket::Pointer mServerSocket;
    typedef std::list<igtl::ClientSocket::Pointer> SocketsType;
    SocketsType mSockets;
};

void mtsIGTLBridge::Init(void)
{
    CMN_ASSERT(mData == nullptr);
    mData = new mtsIGTLBridgeData();
}

void mtsIGTLBridge::InitServer(void)
{
    mData->mServerSocket = igtl::ServerSocket::New();
    const int result = mData->mServerSocket->CreateServer(mPort);
    if (result < 0) {
        CMN_LOG_CLASS_INIT_ERROR << "InitServer: can't create server socket on port "
                                 << mPort << std::endl;
    }
}

void mtsIGTLBridge::Configure(const std::string & jsonFile)
{
    std::ifstream jsonStream;
    jsonStream.open(jsonFile.c_str());

    Json::Value jsonConfig;
    Json::Reader jsonReader;
    if (!jsonReader.parse(jsonStream, jsonConfig)) {
        CMN_LOG_CLASS_INIT_ERROR << "Configure: failed to parse configuration\n"
                                 << jsonReader.getFormattedErrorMessages();
        return;
    }

    ConfigureJSON(jsonConfig);

    InitServer();
}

void mtsIGTLBridge::ConfigureJSON(const Json::Value & jsonConfig)
{
    Json::Value jsonValue = jsonConfig["port"];
    if (!jsonValue.empty()) {
        mPort = jsonValue.asInt();
        CMN_LOG_CLASS_INIT_VERBOSE << "Configure: starting OpenIGTLink server on port \"" << mPort
                                   << "\"" << std::endl;
    } else {
        CMN_LOG_CLASS_INIT_VERBOSE << "Configure: OpenIGTLink port is not defined, using default: " << mPort << std::endl;
    }
}

void mtsIGTLBridge::Startup(void)
{
    if (mPort == 0) {
        SetPort(18944);
    }
}

void mtsIGTLBridge::Cleanup(void)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "Cleanup: closing hanging connections" << std::endl;
    mData->mServerSocket->CloseSocket();
    // iterate on all sockets
    for (auto & socket : mData->mSockets) {
        socket->CloseSocket();
        socket->Delete();
    }
    mData->mServerSocket->Delete();
}

void mtsIGTLBridge::Run(void)
{
    ProcessQueuedCommands();
    ProcessQueuedEvents();

    // first see if we have any new client
    igtl::ClientSocket::Pointer newSocket = mData->mServerSocket->WaitForConnection(1);
    if (newSocket.IsNotNull()) {
        // log some information and add to list
        std::string address;
        int port;
        newSocket->GetSocketAddressAndPort(address, port);
        CMN_LOG_CLASS_RUN_VERBOSE << "Run: found new client from "
                                  << address << ":" << port << std::endl;
        // set socket time out
        newSocket->SetTimeout(10);
        newSocket->SetSendBlocking(10);
        // add new socket to the list
        mData->mSockets.push_back(newSocket);
    }

    // update all senders
    SendAll();

    // update all receivers
    ReceiveAll();
}

void mtsIGTLBridge::SendAll(void)
{
    // get data if we have any socket
    if (mData->mSockets.empty()) {
        return;
    }

    for (auto & sender : mSenders) {
        sender->Execute();
    }
}

void mtsIGTLBridge::ReceiveAll(void)
{
    typedef std::list<igtl::ClientSocket::Pointer> RemovedType;
    RemovedType toBeRemoved;

    // send to all clients of this server
    for (auto & socket : mData->mSockets) {
        igtl::MessageHeader::Pointer headerMsg;
        headerMsg = igtl::MessageHeader::New();

        // keep track of which client we can send to
        headerMsg->InitPack();
        int sendingClientActive =
            socket->Receive(headerMsg->GetPackPointer(),
                            headerMsg->GetPackSize());
        headerMsg->Unpack();

        // process message if valid
        if (sendingClientActive >= 1 && sendingClientActive == headerMsg->GetPackSize()) {
            const auto deviceName = headerMsg->GetDeviceName();
            auto receiver = mReceivers.find(deviceName);
            if (receiver != mReceivers.end()) {
                receiver->second->Execute(socket, headerMsg);
            } else {
                socket->Skip(headerMsg->GetBodySizeToRead(), 0);
                CMN_LOG_CLASS_RUN_WARNING << "ReceiveAll: not receiver known for device \""
                                          << deviceName << "\"" << std::endl;
            }
        }

        if (sendingClientActive == 0) {
            // log some information and remove from list
            std::string address;
            int port;
            socket->GetSocketAddressAndPort(address, port);
            CMN_LOG_CLASS_RUN_VERBOSE << "ReceiveAll: can't receive from client at "
                                      << address << ":" << port
                                      << std::endl;
            toBeRemoved.push_back(socket);
        }
    }

    // remove all sockets we identified as inactive
    for (auto & socket : toBeRemoved) {
        mData->mSockets.remove(socket);
    }
}

// templated implementation for Send
template <typename _igtlMessagePointer>
void mtsIGTLBridge::Send(_igtlMessagePointer message)
{
    typedef std::list<igtl::ClientSocket::Pointer> RemovedType;
    RemovedType toBeRemoved;

    // send to all clients of this server
    for (auto & socket : mData->mSockets) {
        socket->SetTimeout(10);
        socket->SetSendBlocking(10);
        // keep track of which client we can send to
        int receivingClientActive =
            socket->Send(message->GetPackPointer(),
                         message->GetPackSize());

        if (receivingClientActive == 0) {
            // log some information and remove from list
            std::string address;
            int port;
            socket->GetSocketAddressAndPort(address, port);
            CMN_LOG_CLASS_RUN_VERBOSE << "Send: can't send to client at "
                                      << address << ":" << port
                                      << std::endl;
            toBeRemoved.push_back(socket);
        }
    }

    // remove all sockets we identified as inactive
    for (auto & socket : toBeRemoved) {
        mData->mSockets.remove(socket);
    }
}

// force instantiation
template
void mtsIGTLBridge::Send<igtl::TransformMessage::Pointer>(igtl::TransformMessage::Pointer);
template
void mtsIGTLBridge::Send<igtl::StringMessage::Pointer>(igtl::StringMessage::Pointer);
template
void mtsIGTLBridge::Send<igtl::SensorMessage::Pointer>(igtl::SensorMessage::Pointer);
template
void mtsIGTLBridge::Send<igtl::NDArrayMessage::Pointer>(igtl::NDArrayMessage::Pointer);
template
void mtsIGTLBridge::Send<igtl::PointMessage::Pointer>(igtl::PointMessage::Pointer);


// templated implementation for mtsIGTLReceiver::Execute
template <typename _igtlType, typename _cisstType>
bool mtsIGTLReceiver<_igtlType, _cisstType>::Execute(igtl::Socket * socket,
                                                     igtl::MessageBase * header)
{
    IGTLPointer message;
    message = _igtlType::New();
    message->SetMessageHeader(header);
    message->AllocatePack();
    socket->Receive(message->GetPackBodyPointer(), message->GetPackBodySize());
    int c = message->Unpack(1);
    if (c & igtl::MessageHeader::UNPACK_BODY) {
        // convert igtl message to cisst type
        if (mtsIGTLToCISST(message, mCISSTData)) {
            mtsExecutionResult result = Function(mCISSTData);
            if (result) {
                return true;
            } else {
                CMN_LOG_RUN_WARNING << "mtsIGTLReceiver: failed to execute for device \""
                                    << header->GetDeviceName() << "\", error:"
                                    << result << std::endl;
            }
        } else {
            CMN_LOG_RUN_WARNING << "mtsIGTLReceiver: failed to convert data for device \""
                                << header->GetDeviceName() << "\"" << std::endl;
        }
    }
    return false;
}

// force implementation
template
bool mtsIGTLReceiver<igtl::StringMessage, std::string>::Execute(igtl::Socket *, igtl::MessageBase *);
template
bool mtsIGTLReceiver<igtl::SensorMessage, prmForceCartesianSet>::Execute(igtl::Socket *, igtl::MessageBase *);
template
bool mtsIGTLReceiver<igtl::SensorMessage, prmStateJoint>::Execute(igtl::Socket *, igtl::MessageBase *);
template
bool mtsIGTLReceiver<igtl::TransformMessage, prmPositionCartesianSet>::Execute(igtl::Socket *, igtl::MessageBase *);
template
bool mtsIGTLReceiver<igtl::PointMessage, vct3>::Execute(igtl::Socket *, igtl::MessageBase *);
