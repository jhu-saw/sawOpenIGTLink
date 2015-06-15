/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Ali Uneri
  Created on: 2009-08-10

  (C) Copyright 2009-2012 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


#include <igtl/igtlOSUtil.h>
#include <igtl/igtlTransformMessage.h>
#include <igtl/igtlServerSocket.h>

#include <sawOpenIGTLink/mtsOpenIGTLinkBridge.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <cisstParameterTypes/prmPositionCartesianGet.h>

CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsOpenIGTLinkBridge, mtsTaskPeriodic, mtsTaskPeriodicConstructorArg);



class mtsOpenIGTLinkBridgeData {
public:
    int Port;
    std::string Name;

    igtl::ServerSocket::Pointer ServerSocket;
    igtl::TransformMessage::Pointer TransformMessage;
    typedef std::list<igtl::Socket::Pointer> SocketsType;
    SocketsType Sockets;

    mtsInterfaceRequired * InterfaceRequired;
    mtsFunctionRead ReadFunction;
    prmPositionCartesianGet PositionCartesian;

    // takes in cisst frame and outputs igtl frame
    void CISSTToIGT(const prmPositionCartesianGet & cisstData,
                    igtl::Matrix4x4 & igtData);
};


void mtsOpenIGTLinkBridge::Cleanup(void)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "~mtsOpenIGTLinkBridge: closing hanging connections" << std::endl;
    // iterate on all "bridges", i.e. igtl servers
    const BridgesType::iterator endBridges = Bridges.end();
    BridgesType::iterator bridgeIter;
    for (bridgeIter = Bridges.begin();
         bridgeIter != endBridges;
         ++bridgeIter) {
        mtsOpenIGTLinkBridgeData * bridge = *bridgeIter;
        bridge->ServerSocket->CloseSocket();
    }
}


bool mtsOpenIGTLinkBridge::AddServerFromCommandRead(const int port, const std::string & igtlFrameName,
                                                    const std::string & interfaceRequiredName,
                                                    const std::string & commandName)
{
    mtsOpenIGTLinkBridgeData * bridge = new mtsOpenIGTLinkBridgeData;
    bridge->Port = port;
    bridge->Name = igtlFrameName;
    bridge->ServerSocket = igtl::ServerSocket::New();

    // find or create cisst/SAW interface required
    bridge->InterfaceRequired = GetInterfaceRequired(interfaceRequiredName);
    if (!bridge->InterfaceRequired) {
        bridge->InterfaceRequired = AddInterfaceRequired(interfaceRequiredName);
    }
    if (bridge->InterfaceRequired) {
        // add read function
        if (!bridge->InterfaceRequired->AddFunction(commandName, bridge->ReadFunction)) {
            CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadCommand: can't add function \""
                                     << commandName << "\" to interface \""
                                     << interfaceRequiredName << "\", it probably already exists"
                                     << std::endl;
            delete bridge;
            return false;
        }
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadCommand: can't create interface \""
                                 << interfaceRequiredName << "\", it probably already exists"
                                 << std::endl;
        delete bridge;
        return false;
    }

    // create igtl server
    int result = bridge->ServerSocket->CreateServer(bridge->Port);
    if (result < 0) {
        CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadCommand: can't create server socket on port "
                                 << bridge->Port << std::endl;
        delete bridge;
        return false;
    }
    bridge->TransformMessage = igtl::TransformMessage::New();
    bridge->TransformMessage->SetDeviceName(bridge->Name.c_str());

    // and finally add to list of bridges
    Bridges.push_back(bridge);
    return true;
}

void mtsOpenIGTLinkBridge::Run(void)
{
    ProcessQueuedCommands();
    ProcessQueuedEvents();

    igtl::Socket::Pointer newSocket;

    // iterate on all "bridges", i.e. igtl servers
    const BridgesType::iterator endBridges = Bridges.end();
    BridgesType::iterator bridgeIter;
    for (bridgeIter = Bridges.begin();
         bridgeIter != endBridges;
         ++bridgeIter) {
        mtsOpenIGTLinkBridgeData * bridge = *bridgeIter;

        // first see if we have any new client
        newSocket = bridge->ServerSocket->WaitForConnection(1);
        if (newSocket.IsNotNull()) {
            // log some information and add to list
            std::string address;
            int port;
            newSocket->GetSocketAddressAndPort(address, port);
            CMN_LOG_CLASS_RUN_VERBOSE << "Found new client for bridge \""
                                      << bridge->Name << " from "
                                      << address << ":" << port << std::endl;
            bridge->Sockets.push_back(newSocket);
        }

        // get data if we have any socket and check if it's valid
        bool dataNeedsSend = false;
        if (!bridge->Sockets.empty()) {
            mtsExecutionResult result;
            result = bridge->ReadFunction(bridge->PositionCartesian);
            if (result.IsOK()) {
                dataNeedsSend = bridge->PositionCartesian.Valid();
            } else {
                dataNeedsSend = false;
            }
        }

        if (dataNeedsSend) {
            igtl::Matrix4x4 dataMatrix;
            bridge->CISSTToIGT(bridge->PositionCartesian, dataMatrix);
            bridge->TransformMessage->SetMatrix(dataMatrix);
            bridge->TransformMessage->Pack();

            typedef std::list<mtsOpenIGTLinkBridgeData::SocketsType::iterator> RemovedType;
            RemovedType toBeRemoved;

            // send to all clients of this server
            const mtsOpenIGTLinkBridgeData::SocketsType::iterator endSockets = bridge->Sockets.end();
            mtsOpenIGTLinkBridgeData::SocketsType::iterator socketIter;
            for (socketIter = bridge->Sockets.begin();
                 socketIter != endSockets;
                 ++socketIter) {
                igtl::Socket::Pointer socket = *socketIter;
                // keep track of which client we can send to
                int clientActive =
                    socket->Send(bridge->TransformMessage->GetPackPointer(),
                                 bridge->TransformMessage->GetPackSize());
                // remove the client if we can't send
                if (clientActive == 0) {
                    // log some information and remove from list
                    std::string address;
                    int port;
                    socket->GetSocketAddressAndPort(address, port);
                    CMN_LOG_CLASS_RUN_VERBOSE << "Can't send to client for bridge \""
                                              << bridge->Name << " from "
                                              << address << ":" << port
                                              << std::endl;
                    toBeRemoved.push_back(socketIter);
                }
            }
            // remove all sockets we identified as inactive
            const RemovedType::iterator removedEnd = toBeRemoved.end();
            RemovedType::iterator removedIter;
            for (removedIter = toBeRemoved.begin();
                 removedIter != removedEnd;
                 ++removedIter) {
                CMN_LOG_CLASS_RUN_VERBOSE << "Removing client socket for bridge \""
                                          << bridge->Name << "\"" << std::endl;
                bridge->Sockets.erase(*removedIter);
            }
        }
    }
}

void mtsOpenIGTLinkBridgeData::CISSTToIGT(const prmPositionCartesianGet & frameCISST,
                                          igtl::Matrix4x4 & frameIGTL)
{
    frameIGTL[0][0] = frameCISST.Position().Rotation().Element(0,0);
    frameIGTL[1][0] = frameCISST.Position().Rotation().Element(1,0);
    frameIGTL[2][0] = frameCISST.Position().Rotation().Element(2,0);
    frameIGTL[3][0] = 0;

    frameIGTL[0][1] = frameCISST.Position().Rotation().Element(0,1);
    frameIGTL[1][1] = frameCISST.Position().Rotation().Element(1,1);
    frameIGTL[2][1] = frameCISST.Position().Rotation().Element(2,1);
    frameIGTL[3][1] = 0;

    frameIGTL[0][2] = frameCISST.Position().Rotation().Element(0,2);
    frameIGTL[1][2] = frameCISST.Position().Rotation().Element(1,2);
    frameIGTL[2][2] = frameCISST.Position().Rotation().Element(2,2);
    frameIGTL[3][2] = 0;

    frameIGTL[0][3] = frameCISST.Position().Translation().Element(0);
    frameIGTL[1][3] = frameCISST.Position().Translation().Element(1);
    frameIGTL[2][3] = frameCISST.Position().Translation().Element(2);
    frameIGTL[3][3] = 1;
}
