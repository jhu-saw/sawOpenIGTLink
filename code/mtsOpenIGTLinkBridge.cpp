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

#include <cisstOSAbstraction/osaSleep.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <sawOpenIGTLink/mtsOpenIGTLinkBridge.h>

CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsOpenIGTLinkBridge, mtsTaskPeriodic, mtsTaskPeriodicConstructorArg);

void mtsOpenIGTLinkBridge::Cleanup(void)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "~mtsOpenIGTLinkBridge: closing hanging connections" << std::endl;
    if (serverSocket) serverSocket->CloseSocket();
}


void mtsOpenIGTLinkBridge::Configure(const std::string & hostAndPort)
{
    // // parse port and [if any] host information
    std::string host;
    unsigned int port;
    size_t colonPosition = hostAndPort.find(':');
    if (colonPosition == std::string::npos) {
        host = "";
        port = atoi(hostAndPort.c_str());
    } else {
        host = hostAndPort.substr(0, colonPosition);
        port = atoi(hostAndPort.substr(colonPosition + 1).c_str());
    }

    // if a target host is not provided, configure as server, otherwise client
    if (host.empty()) {
        //CMN_LOG_CLASS_INIT_VERBOSE << "mtsOpenIGTLinkBridge: setting up as server" << std::endl;
        std::cout << "mtsOpenIGTLinkBridge: setting up as server in port: " << port << std::endl; 
        ConnectionType = SERVER;
        serverSocket = igtl::ServerSocket::New();
        ServerPort = port;
    } else {
        CMN_LOG_CLASS_INIT_VERBOSE << "mtsOpenIGTLinkBridge: setting up as client" << std::endl;
        // ConnectionType = CLIENT;
        // Host = host;
        // Port = port;
        // SocketServer = 0;
        // Socket = new osaSocket();
        // IGTLData = new sawOpenIGTLinkData;
    }
    IsConnected = false;

    /* The required interface that:
       1. grabs data from a provided interface (sawNDITracker for example)
       2. publishes data in form of openigtlink */
    mtsInterfaceRequired * requiresPositionCartesion = AddInterfaceRequired("requiresPositionCartesian");
    if (requiresPositionCartesion){
        // gets tool cartesian pose from mtsNDITracker
        requiresPositionCartesion->AddFunction("GetPositionCartesian", this->GetPositionCartesian);
    }
}


void mtsOpenIGTLinkBridge::Startup(void)
{
    NbSockets = 0;
    if (ConnectionType == SERVER) {
        int r = serverSocket->CreateServer(ServerPort);
        if (r<0){
            CMN_LOG_CLASS_INIT_VERBOSE << "Cannot create a server socket" << std::endl;
            exit(0);
        }
    }
}


void mtsOpenIGTLinkBridge::Run(void)
{
    ProcessQueuedCommands();

    std::cout<<"Application is running"<<std::endl;

    //if (!IsConnected) {
        if (ConnectionType == SERVER)
        {
            // first  to make sure socket is not null
            std::cerr << "waiting for connection" << std::endl;
            socket[NbSockets] = serverSocket->WaitForConnection(1);
            if (socket[NbSockets].IsNotNull()) {
                NbSockets++;
                std::string address;
                int port;
                socket[NbSockets - 1]->GetSocketAddressAndPort(address, port);
                std::cerr << "Detected new client " << address << ":" << port << std::endl;
            }
            if (NbSockets > 0) {
                std::cout<<"Get position"<<std::endl;
                GetPositionCartesian(PositionCartesian);
                igtl::TransformMessage::Pointer transMsg;
                transMsg = igtl::TransformMessage::New();
                transMsg->SetDeviceName("OpenIGTLink_USProbe");

                igtl::Matrix4x4 dataMatrix;
                prmPositionCartesianToOIGTL(PositionCartesian, dataMatrix);
                transMsg->SetMatrix(dataMatrix);
                transMsg->Pack();
                std::cerr << "sending to " << NbSockets << " clients ";
                for (unsigned int s = 0; s < NbSockets; ++s) {
                    std::cerr << " " << s;
                    if (socket[s]->GetConnected() == 0) {
                        std::cerr << "YEAH, detected disconnected client!" << std::endl;
                    } else {
                        std::cerr << "["
                                  << socket[s]->Send(transMsg->GetPackPointer(), transMsg->GetPackSize())
                                  << "]";
                    }
                }
                std::cerr << std::endl;
            }
        }
        //  else if (ConnectionType == CLIENT)
        // {
        //     IsConnected = Socket->Connect(Host.c_str(), Port);
        // }
        /*
        if((ConnectionType==CLIENT && IsConnected)||ConnectionType == SERVER && socket.IsNotNull())
        {
            IsConnected = true;
            std::cout<<"Server / Client is running"<<std::endl;
        }
        */
    //}

    /*
    std::cout<<"Get position"<<std::endl;
    GetPositionCartesian(PositionCartesian);

    std::cerr<<socket.IsNotNull()<<std::endl;

    if (IsConnected && ConnectionType==SERVER)
    {
        if(!socket.IsNotNull())
        {
            std::cout<<"Check if server is connected"<<std::endl;
            IsConnected = false;
            serverSocket->CloseSocket();
            //std::cout<<"Connection Lost"<<std::endl;
        }
    }

       
    if (IsConnected && socket.IsNotNull()) {
        std::cerr << ".";
        igtl::TransformMessage::Pointer transMsg;
        transMsg = igtl::TransformMessage::New();
        transMsg->SetDeviceName("OpenIGTLink_USProbe");

        igtl::Matrix4x4 dataMatrix;
        prmPositionCartesianToOIGTL(PositionCartesian, dataMatrix);
        transMsg->SetMatrix(dataMatrix);
        transMsg->Pack();
        socket->Send(transMsg->GetPackPointer(), transMsg->GetPackSize());
        
        igtl::PrintMatrix(dataMatrix);
    }
    */
}


void mtsOpenIGTLinkBridge::prmPositionCartesianToOIGTL(const prmPositionCartesianGet& frameCISST,
                                                 igtl::Matrix4x4& frameIGTL){

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
