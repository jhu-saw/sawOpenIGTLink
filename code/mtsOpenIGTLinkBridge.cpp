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


#include <igtl/igtlOSUtil.h>
#include <igtl/igtlTransformMessage.h>
#include <igtl/igtlServerSocket.h>

#include <sawOpenIGTLink/mtsOpenIGTLinkBridge.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <cisstParameterTypes/prmPositionCartesianGet.h>
#include <cisstParameterTypes/prmPositionCartesianSet.h>

#define SENDING 0
#define RECEIVING 1

CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsOpenIGTLinkBridge, mtsTaskPeriodic, mtsTaskPeriodicConstructorArg);



class mtsOpenIGTLinkBridgeData {
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
    mtsFunctionRead ReadFunction;
    mtsFunctionWrite WriteFunction;
    prmPositionCartesianGet PositionCartesianGet;
    prmPositionCartesianSet PositionCartesianSet;

    // takes in cisst frame and outputs igtl frame
    void CISSTToIGT(const prmPositionCartesianGet & frameCISST,
                    igtl::Matrix4x4 & frameIGTL);

    void IGTtoCISST(const igtl::Matrix4x4 & frameIGTL,
                    prmPositionCartesianSet & frameCISST);

    void ProcessIncomingMessage(mtsOpenIGTLinkBridgeData *bridge,
                                igtl::Socket::Pointer socket,
                                igtl::MessageHeader::Pointer headerMsg);

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

    const Json::Value arms = jsonConfig["arms"];
    for (unsigned int index = 0; index < arms.size(); ++index) {
        if (!ConfigureArmServers(arms[index])) {
            CMN_LOG_CLASS_INIT_ERROR << "Configure: failed to configure arms[" << index << "]" << std::endl;
            return;
        }
    }
}

bool mtsOpenIGTLinkBridge::ConfigureArmServers(const Json::Value &jsonArm)
{
    std::string armName = jsonArm["name"].asString();
    std::string armComponent = jsonArm["component"].asString();
    std::string armProvided = jsonArm["provided-interface"].asString();
    int armPort = jsonArm["port-number"].asInt();
    std::cerr << "mtsOpenIGTLink bridge: Adding arm \"" << armName
              << "\", on port number \"" << armPort
              << "\"" << std::endl;

    if(!(this->AddServerFromCommandRead(armPort, armName, armName,armComponent, armProvided)))
    {
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


bool mtsOpenIGTLinkBridge::AddServerFromCommandRead(const int port, const std::string & igtlFrameName,
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
    int result = bridge->ServerSocket->CreateServer(bridge->Port);
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

bool mtsOpenIGTLinkBridge::AddServerFromCommandWrite(const int port, const std::string & igtlFrameName,
                                                    const std::string & interfaceRequiredName,
                                                    const std::string & commandName)
{
    mtsOpenIGTLinkBridgeData * bridge = new mtsOpenIGTLinkBridgeData;
    bridge->Port = port;
    bridge->Name = igtlFrameName;
    bridge->ServerSocket = igtl::ServerSocket::New();
    bool newInterface;

    // find or create cisst/SAW interface required
    /*
    bridge->InterfaceRequired = GetInterfaceRequired(interfaceRequiredName);
    if (!bridge->InterfaceRequired) {
        bridge->InterfaceRequired = AddInterfaceRequired(interfaceRequiredName);
        newInterface = true;
    } else {
        newInterface = false;
    }
    if (bridge->InterfaceRequired) {
        // add write function
        if (!bridge->InterfaceRequired->AddFunction(commandName, bridge->ReadFunction)) {
            CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadWrite: can't add function \""
                                     << commandName << "\" to interface \""
                                     << interfaceRequiredName << "\", it probably already exists"
                                     << std::endl;
            delete bridge;
            return false;
        }
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadWrite: can't create interface \""
                                 << interfaceRequiredName << "\", it probably already exists"
                                 << std::endl;
        delete bridge;
        return false;
    }
    */
    // create igtl server
    int result = bridge->ServerSocket->CreateServer(bridge->Port);
    if (result < 0) {
        CMN_LOG_CLASS_INIT_ERROR << "AddServerFromReadWrite: can't create server socket on port "
                                 << bridge->Port << std::endl;
        if (newInterface) {
           this->RemoveInterfaceRequired(interfaceRequiredName);
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
        //std::cout<<bridge->PositionCartesianGet.Valid()<<std::endl;
        if (result.IsOK()) {
            dataNeedsSend = bridge->PositionCartesianGet.Valid();
        } else {
            dataNeedsSend = false;
        }
    }

    if (dataNeedsSend) {
        igtl::Matrix4x4 dataMatrix;
        bridge->CISSTToIGT(bridge->PositionCartesianGet, dataMatrix);
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
            socket->SetTimeout(1);
            socket->SetSendBlocking(1);
            // keep track of which client we can send to
            int receivingClientActive =
                socket->Send(bridge->TransformMessage->GetPackPointer(),
                             bridge->TransformMessage->GetPackSize());
            //std::cerr<<"Server trying to send data: " <<
            //           receivingClientActive << std::endl;
            // remove the client if we can't send
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

void mtsOpenIGTLinkBridge::ServerReceive(mtsOpenIGTLinkBridgeData * bridge){
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
            newSocket->SetSendBlocking(1);
            // add new socket to the list
            bridge->Sockets.push_back(newSocket);
        }

        if(bridge->ServerType == SENDING)
        {
            ServerSend(bridge);
        }
        else if(bridge->ServerType == RECEIVING)
        {
            ServerReceive(bridge);
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

    // meter to mm conversion
    frameIGTL[0][3] = frameCISST.Position().Translation().Element(0)*1000;
    frameIGTL[1][3] = frameCISST.Position().Translation().Element(1)*1000;
    frameIGTL[2][3] = frameCISST.Position().Translation().Element(2)*1000;
    frameIGTL[3][3] = 1;
}

void mtsOpenIGTLinkBridgeData::IGTtoCISST(const igtl::Matrix4x4 & frameIGTL,
                                          prmPositionCartesianSet & frameCISST)
{
    frameCISST.Goal().Rotation().Element(0,0) = frameIGTL[0][0];
    frameCISST.Goal().Rotation().Element(1,0) = frameIGTL[1][0];
    frameCISST.Goal().Rotation().Element(2,0) = frameIGTL[2][0];

    frameCISST.Goal().Rotation().Element(0,1) = frameIGTL[0][1];
    frameCISST.Goal().Rotation().Element(1,1) = frameIGTL[1][1];
    frameCISST.Goal().Rotation().Element(2,1) = frameIGTL[2][1];

    frameCISST.Goal().Rotation().Element(0,2) = frameIGTL[0][2];
    frameCISST.Goal().Rotation().Element(1,2) = frameIGTL[1][2];
    frameCISST.Goal().Rotation().Element(2,2) = frameIGTL[2][2];

    frameCISST.Goal().Translation().Element(0) = frameIGTL[0][3];
    frameCISST.Goal().Translation().Element(1) = frameIGTL[1][3];
    frameCISST.Goal().Translation().Element(2) = frameIGTL[2][3];
}

void mtsOpenIGTLinkBridgeData::ProcessIncomingMessage(mtsOpenIGTLinkBridgeData *bridge,
                                                      igtl::Socket::Pointer socket,
                                                      igtl::MessageHeader::Pointer headerMsg){

    if(strcmp(headerMsg->GetDeviceType(), "TRANSFORM") == 0){
        //std::cout<<"Server received transform message"<<std::endl;
        bridge->TransformMessage->SetMessageHeader(headerMsg);
        bridge->TransformMessage->AllocatePack();

        socket->Receive(bridge->TransformMessage->GetPackBodyPointer(), bridge->TransformMessage->GetPackBodySize());
        int c = bridge->TransformMessage->Unpack();

        if (c & igtl::MessageHeader::UNPACK_BODY){
            igtl::Matrix4x4 matrix;
            bridge->TransformMessage->GetMatrix(matrix);
            IGTtoCISST(matrix,bridge->PositionCartesianSet);
            igtl::PrintMatrix(matrix);
        }
    }
}
