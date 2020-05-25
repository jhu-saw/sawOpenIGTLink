/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Youri Tan, Anton Deguet
  Created on: 2015-07-24

  (C) Copyright 2009-2015 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


#include <igtlOSUtil.h>
#include <igtlServerSocket.h>

#include <sawOpenIGTLink/mtsOpenIGTLinkBridge.h>
#include <sawOpenIGTLink/mtsCISSTToIGTL.h>
#include <sawOpenIGTLink/mtsIGTLToCISST.h>

#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>
#include <cisstMultiTask/mtsManagerLocal.h>
#include <cisstParameterTypes/prmPositionCartesianGet.h>
#include <cisstParameterTypes/prmPositionCartesianSet.h>

#define SENDING 0
#define RECEIVING 1

CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsOpenIGTLinkBridge, mtsTaskPeriodic, mtsTaskPeriodicConstructorArg);

class mtsOpenIGTLinkBridgeData
{
public:
    int Port;
    std::string Name;
    std::string ComponentName;
    std::string ProvidedInterfaceName;

    igtl::ServerSocket::Pointer ServerSocket;
    igtl::TransformMessage::Pointer TransformMessage;
    typedef std::list<igtl::Socket::Pointer> SocketsType;
    SocketsType Sockets;
    int ServerType;

    mtsInterfaceRequired * InterfaceRequired;
    mtsInterfaceProvided * InterfaceProvided;
    mtsFunctionRead ReadFunction;
    mtsFunctionWrite WriteFunction;
    prmPositionCartesianGet PositionCartesianGet;
    prmPositionCartesianSet PositionCartesianSet;

    mtsStateTable * mStateTable;

    void ProcessIncomingMessage(mtsOpenIGTLinkBridgeData * bridge,
                                igtl::Socket::Pointer socket,
                                igtl::MessageHeader::Pointer headerMsg);

};


void mtsOpenIGTLinkBridge::Cleanup(void)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "Cleanup: closing hanging connections" << std::endl;
    // iterate on all "bridges", i.e. igtl servers
    const BridgesType::iterator endBridges = Bridges.end();
    BridgesType::iterator bridgeIter;
    for (bridgeIter = Bridges.begin();
         bridgeIter != endBridges;
         ++bridgeIter) {
        mtsOpenIGTLinkBridgeData * bridge = *bridgeIter;
        bridge->ServerSocket->CloseSocket();

        // iterate on all sockets of bridge, i.e. igtl server
        typedef mtsOpenIGTLinkBridgeData::SocketsType SocketsType;
        const SocketsType::iterator endSocket = bridge->Sockets.end();
        SocketsType::iterator socketIter ;
        for (socketIter = bridge->Sockets.begin();
             socketIter != endSocket;
             ++socketIter) {
            igtl::Socket::Pointer socket = *socketIter;
            socket->CloseSocket();
            socket->Delete();
        }
        bridge->ServerSocket->Delete();
    }
}

void mtsOpenIGTLinkBridge::Configure(const std::string & jsonFile)
{
    std::ifstream jsonStream;
    jsonStream.open(jsonFile.c_str());

    Json::Value jsonConfig, jsonValue;
    Json::Reader jsonReader;
    if (!jsonReader.parse(jsonStream, jsonConfig)) {
        CMN_LOG_CLASS_INIT_ERROR << "Configure: failed to parse configuration\n"
                                 << jsonReader.getFormattedErrorMessages();
        return;
    }

    const Json::Value servers = jsonConfig["servers"];
    for (unsigned int index = 0; index < servers.size(); ++index) {
        if (!ConfigureServer(servers[index])) {
            CMN_LOG_CLASS_INIT_ERROR << "Configure: failed to configure servers[" << index << "]" << std::endl;
            return;
        }
    }
}

bool mtsOpenIGTLinkBridge::ConfigureServer(const Json::Value & jsonServer)
{
    const std::string serverName = jsonServer["name"].asString();
    const std::string serverComponent = jsonServer["component"].asString();
    const std::string serverProvided = jsonServer["provided-interface"].asString();
    const int serverPort = jsonServer["port-number"].asInt();
    CMN_LOG_CLASS_INIT_VERBOSE << "ConfigureServer: adding server \"" << serverName
                               << "\", on port number \"" << serverPort
                               << "\"" << std::endl;

    if (!AddServerFromCommandRead(serverPort, serverName, serverName,serverComponent, serverProvided)) {
        return false;
    }
    return true;
}

bool mtsOpenIGTLinkBridge::Connect(void)
{
    mtsManagerLocal * componentManager = mtsManagerLocal::GetInstance();

    const BridgesType::iterator endBridges = Bridges.end();
    BridgesType::iterator bridgeIter;
    for (bridgeIter = Bridges.begin();
         bridgeIter != endBridges;
         ++bridgeIter) {
        mtsOpenIGTLinkBridgeData * bridge = *bridgeIter;
        componentManager->Connect(this->Name, bridge->Name, bridge->ComponentName, bridge->ProvidedInterfaceName);
    }
    return true;
}


bool mtsOpenIGTLinkBridge::AddServerFromCommandRead(const int port,
                                                    const std::string & igtlFrameName,
                                                    const std::string & interfaceRequiredName,
                                                    const std::string & componentName,
                                                    const std::string & providedName,
                                                    const std::string & commandName)
{
    mtsOpenIGTLinkBridgeData * bridge = new mtsOpenIGTLinkBridgeData;
    bridge->Port = port;
    bridge->Name = igtlFrameName;
    bridge->ComponentName = componentName;
    bridge->ProvidedInterfaceName = providedName;
    bridge->ServerSocket = igtl::ServerSocket::New();
    bridge->mStateTable = new mtsStateTable(500, componentName);

    bool newInterface;

    // find or create cisst/SAW interface required
    bridge->InterfaceRequired = GetInterfaceRequired(interfaceRequiredName);
    if (!bridge->InterfaceRequired) {
        bridge->InterfaceRequired = AddInterfaceRequired(interfaceRequiredName);
        newInterface = true;
    } else {
        newInterface = false;
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
    const int result = bridge->ServerSocket->CreateServer(bridge->Port);
    if (result < 0) {
        CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadCommand: can't create server socket on port "
                                 << bridge->Port << std::endl;
        if (newInterface) {
            this->RemoveInterfaceRequired(interfaceRequiredName);
        }
        delete bridge;
        return false;
    }
    bridge->TransformMessage = igtl::TransformMessage::New();
    bridge->TransformMessage->SetDeviceName(bridge->Name.c_str());
    bridge->ServerType = SENDING;
    // and finally add to list of bridges
    Bridges.push_back(bridge);
    return true;
}

bool mtsOpenIGTLinkBridge::AddServerFromCommandWrite(const int port,
                                                     const std::string & igtlFrameName,
                                                     const std::string & interfaceProvidedName,
                                                     const std::string & commandName)
{
    mtsOpenIGTLinkBridgeData * bridge = new mtsOpenIGTLinkBridgeData;
    bridge->Port = port;
    bridge->Name = igtlFrameName;
    bridge->ServerSocket = igtl::ServerSocket::New();
    bool newInterface;

    bridge->InterfaceProvided = GetInterfaceProvided(interfaceProvidedName);
    if (!bridge->InterfaceProvided) {
        bridge->InterfaceProvided = AddInterfaceProvided(interfaceProvidedName);
        newInterface = true;
    } else {
        newInterface = false;
    }
    if (bridge->InterfaceProvided) {
        // add write function
        bridge->mStateTable->AddData(bridge->PositionCartesianSet, "measured_cp");
        if (!bridge->InterfaceProvided->AddCommandReadState(*bridge->mStateTable, bridge->PositionCartesianSet, commandName)) {
            CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadWrite: can't add function \""
                                     << commandName << "\" to interface \""
                                     << interfaceProvidedName << "\", it probably already exists"
                                     << std::endl;
            delete bridge;
            return false;
            }
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadWrite: can't create interface \""
                                 << interfaceProvidedName << "\", it probably already exists"
                                 << std::endl;
        delete bridge;
        return false;
    }
    // create igtl server
    int result = bridge->ServerSocket->CreateServer(bridge->Port);
    if (result < 0) {
        CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadWrite: can't create server socket on port "
                                 << bridge->Port << std::endl;
        if (newInterface) {
            this->RemoveInterfaceRequired(interfaceProvidedName);
        }
        delete bridge;
        return false;
    }
    bridge->TransformMessage = igtl::TransformMessage::New();
    bridge->TransformMessage->SetDeviceName(bridge->Name.c_str());
    bridge->ServerType = RECEIVING;

    // and finally add to list of bridges
    Bridges.push_back(bridge);
    return true;
}

void mtsOpenIGTLinkBridge::ServerSend(mtsOpenIGTLinkBridgeData * bridge)
{
    // get data if we have any socket and check if it's valid
    bool dataNeedsSend = false;
    if (!bridge->Sockets.empty()) {
        mtsExecutionResult result;
        result = bridge->ReadFunction(bridge->PositionCartesianGet);
        if (result.IsOK()) {
            dataNeedsSend = bridge->PositionCartesianGet.Valid();
        } else {
            dataNeedsSend = false;
        }
    }

    if (dataNeedsSend) {
        igtl::Matrix4x4 dataMatrix;
        mtsCISSTToIGTL(bridge->PositionCartesianGet, dataMatrix);
        bridge->TransformMessage->SetMatrix(dataMatrix);
        igtl::TimeStamp::Pointer ts;
        ts = igtl::TimeStamp::New();
        ts->SetTime(bridge->PositionCartesianGet.Timestamp());
        bridge->TransformMessage->SetTimeStamp(ts);
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
            socket->SetTimeout(10);
            socket->SetSendBlocking(10);
            // keep track of which client we can send to
            int receivingClientActive =
                socket->Send(bridge->TransformMessage->GetPackPointer(),
                             bridge->TransformMessage->GetPackSize());

            if (receivingClientActive == 0 /*|| receivingClientActive == -1*/) {
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
    return;
}

void mtsOpenIGTLinkBridge::ServerReceive(mtsOpenIGTLinkBridgeData * bridge)
{
    // get data if we have any socket and check if it's valid
    bool dataIsIncoming = false;
    if (!bridge->Sockets.empty()) {
        dataIsIncoming = true;
    }

    if (dataIsIncoming) {
        typedef std::list<mtsOpenIGTLinkBridgeData::SocketsType::iterator> RemovedType;
        RemovedType toBeRemoved;

        // receive from all clients of this server
        const mtsOpenIGTLinkBridgeData::SocketsType::iterator endSockets = bridge->Sockets.end();
        mtsOpenIGTLinkBridgeData::SocketsType::iterator socketIter;
        for (socketIter = bridge->Sockets.begin();
             socketIter != endSockets;
             ++socketIter) {
            igtl::Socket::Pointer socket = *socketIter;
            igtl::MessageHeader::Pointer headerMsg;
            headerMsg = igtl::MessageHeader::New();

            // keep track of which client we can send to
            headerMsg->InitPack();
            int sendingClientActive =
                socket->Receive(headerMsg->GetPackPointer(),
                                headerMsg->GetPackSize());
            // std::cerr<<"Server trying to receive data: " << sendingClientActive << std::endl;
            /*
              if (sendingClientActive != headerMsg->GetPackSize()){
              continue;
              }
            */
            headerMsg->Unpack();

            if (sendingClientActive >= 1 && sendingClientActive == headerMsg->GetPackSize()) {
                // This looks wrong
                bridge->ProcessIncomingMessage(bridge,socket,headerMsg);
                if (bridge->PositionCartesianSet.Valid()) {
                    // bridge->WriteFunction(bridge->PositionCartesianGet);
                    //std::cerr<<"Recievd valid transform"<<std::endl;
                }
            }
            // remove the client if we can't receive
            if (sendingClientActive < 1) {
                // log some information and remove from list
                std::cerr<<"Socket should be deleted" << std::endl;
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
    return;
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
            // set socket time out
            newSocket->SetTimeout(10);
            newSocket->SetSendBlocking(10);
            // add new socket to the list
            bridge->Sockets.push_back(newSocket);
        }

        if (bridge->ServerType == SENDING) {
            ServerSend(bridge);
        }
        else if (bridge->ServerType == RECEIVING) {
            ServerReceive(bridge);
        }
    }
}

void mtsOpenIGTLinkBridgeData::ProcessIncomingMessage(mtsOpenIGTLinkBridgeData *bridge,
                                                      igtl::Socket::Pointer socket,
                                                      igtl::MessageHeader::Pointer headerMsg){

    if (strcmp(headerMsg->GetDeviceType(), "TRANSFORM") == 0) {
        //std::cout<<"Server received transform message"<<std::endl;
        bridge->TransformMessage->SetMessageHeader(headerMsg);
        bridge->TransformMessage->AllocatePack();

        socket->Receive(bridge->TransformMessage->GetPackBodyPointer(), bridge->TransformMessage->GetPackBodySize());
        int c = bridge->TransformMessage->Unpack();

        if (c & igtl::MessageHeader::UNPACK_BODY) {
            igtl::Matrix4x4 matrix;
            bridge->TransformMessage->GetMatrix(matrix);
            mtsIGTLtoCISST(matrix,bridge->PositionCartesianSet);
            igtl::PrintMatrix(matrix);
        }
    }
}
