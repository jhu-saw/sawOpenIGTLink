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

CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsIGTLBridge, mtsTaskPeriodic, mtsTaskPeriodicConstructorArg);

void mtsIGTLBridge::InitServer(void)
{
    mServerSocket = igtl::ServerSocket::New();
    const int result = mServerSocket->CreateServer(mPort);
    if (result < 0) {
        CMN_LOG_CLASS_INIT_ERROR << "mtsIGTLBridge: can't create server socket on port "
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

void mtsIGTLBridge::Cleanup(void)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "Cleanup: closing hanging connections" << std::endl;
    mServerSocket->CloseSocket();
    // iterate on all sockets
    for (auto & socket : mSockets) {
        socket->CloseSocket();
        socket->Delete();
    }
    mServerSocket->Delete();
}

void mtsIGTLBridge::Run(void)
{
    ProcessQueuedCommands();
    ProcessQueuedEvents();

    // first see if we have any new client
    igtl::ClientSocket::Pointer newSocket = mServerSocket->WaitForConnection(1);
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
        mSockets.push_back(newSocket);
    }

    // update all senders
    SendAll();
}

void mtsIGTLBridge::SendAll(void)
{
    // get data if we have any socket
    if (mSockets.empty()) {
        return;
    }

    for (auto & sender : mSenders) {
        sender->Execute();
    }
}

void mtsIGTLBridge::Send(igtl::MessageBase & message)
{
    typedef std::list<igtl::ClientSocket::Pointer> RemovedType;
    RemovedType toBeRemoved;

    // send to all clients of this server
    for (auto & socket : mSockets) {
        socket->SetTimeout(10);
        socket->SetSendBlocking(10);
        // keep track of which client we can send to
        int receivingClientActive =
            socket->Send(message.GetPackPointer(),
                         message.GetPackSize());

        if (receivingClientActive == 0) {
            // log some information and remove from list
            std::string address;
            int port;
            socket->GetSocketAddressAndPort(address, port);
            CMN_LOG_CLASS_RUN_VERBOSE << "Can't send to client at "
                                      << address << ":" << port
                                      << std::endl;
            toBeRemoved.push_back(socket);
        }
    }

    // remove all sockets we identified as inactive
    for (auto & socket : toBeRemoved) {
        mSockets.remove(socket);
    }
}
