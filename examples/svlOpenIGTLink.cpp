// See license at http://www.cisst.org/cisst/license.txt


#include <cisstOSAbstraction/osaSleep.h>

#include "svlOpenIGTLink.h"

class svlOpenIGTLinkImageServer {
public:

    int Port;
    std::string DeviceName;
    igtl::ServerSocket::Pointer ServerSocket;
    igtl::ImageMessage::Pointer IGTLImageMessage;
    igtl::Matrix4x4 IGTLImageMatrix;
    typedef std::list<igtl::Socket::Pointer> SocketsType;
    SocketsType Sockets;

    void InitializeIGTLData();
    void InitializeIGTServerSocket(int port);
};

void svlOpenIGTLinkImageServer::InitializeIGTLData()
{
    // Assign image transformation matrix
    this->IGTLImageMatrix[0][0] = -1.0;  this->IGTLImageMatrix[1][0] = 0.0;  this->IGTLImageMatrix[2][0] = 0.0; this->IGTLImageMatrix[3][0] = 0.0;
    this->IGTLImageMatrix[0][1] = 0.0;  this->IGTLImageMatrix[1][1] = -1.0;  this->IGTLImageMatrix[2][1] = 0.0; this->IGTLImageMatrix[3][1] = 0.0;
    this->IGTLImageMatrix[0][2] = 0.0;  this->IGTLImageMatrix[1][2] = 0.0;  this->IGTLImageMatrix[2][2] = 1.0; this->IGTLImageMatrix[3][2] = 0.0;
    this->IGTLImageMatrix[0][3] = 0.0;  this->IGTLImageMatrix[1][3] = 0.0;  this->IGTLImageMatrix[2][3] = 0.0; this->IGTLImageMatrix[3][3] = 1.0;

    // Hard code image size -> NEEDS CHANGE (ideally image size automatically determined based on input stream data)
    // subOffset ??
    int imageSizePixels[3]={720,480,1}, subOffset[3]={0};

    // Initialize image message fixed data and allocate memory space
    this->IGTLImageMessage = igtl::ImageMessage::New();
    this->IGTLImageMessage->SetDimensions(imageSizePixels);
    this->IGTLImageMessage->SetDeviceName(this->DeviceName.c_str());
    this->IGTLImageMessage->SetScalarType(igtl::ImageMessage::TYPE_UINT8);
    this->IGTLImageMessage->SetOrigin(0.0, 0.0, 0.0);
    this->IGTLImageMessage->SetSpacing(1.0, 1.0, 1.0);
    this->IGTLImageMessage->SetEndian(igtl_is_little_endian() ? igtl::ImageMessage::ENDIAN_LITTLE : igtl::ImageMessage::ENDIAN_BIG);
    this->IGTLImageMessage->SetSubVolume( imageSizePixels, subOffset );
    this->IGTLImageMessage->SetNumComponents(3);
    this->IGTLImageMessage->AllocateScalars();
    this->IGTLImageMessage->SetMatrix(this->IGTLImageMatrix);
}

void svlOpenIGTLinkImageServer::InitializeIGTServerSocket(int port)
{
    this->ServerSocket = igtl::ServerSocket::New();
    int r = this->ServerSocket->CreateServer(port);
    if (r < 0)
    {
        std::cerr << "Cannot create a server socket." << std::endl;
        exit(0);
    }
}


svlOpenIGTLinkBridge::svlOpenIGTLinkBridge() :
  svlFilterBase()                                                       // Call baseclass' constructor
{                                                                         //
  AddInput("input", true);                                              // Create synchronous input connector
  AddInputType("input", svlTypeImageRGB);                               // Set sample type for input connector
                                                                        //
  AddOutput("output", true);                                            // Create synchronous ouput connector
  SetAutomaticOutputType(true);                                         // Set output sample type the same as input sample type
}

int svlOpenIGTLinkBridge::Initialize(svlSample* syncInput, svlSample* &syncOutput)
{
    IGTLImageServer = new svlOpenIGTLinkImageServer();
    IGTLImageServer->InitializeIGTLData();
    IGTLImageServer->InitializeIGTServerSocket(18944);

    syncOutput = syncInput;                                              // Pass the input sample forward to the output
    NumberOfClients = 0;
    return SVL_OK;                                                        //
}

int svlOpenIGTLinkBridge::SendIGTLImageMessages(svlOpenIGTLinkImageServer* server, svlSample* syncInput)
{
    // check if we have new client
    if (server->IGTLImageMessage.IsNotNull())
    {
        if(!server->Sockets.empty())
        {
            double timeStamp = syncInput->GetTimestamp();
            igtl::TimeStamp::Pointer igtlImageTimestamp = igtl::TimeStamp::New();
            igtlImageTimestamp->SetTime(timeStamp);

            const svlSampleImageRGB* image = (svlSampleImageRGB*)(syncInput);   // Cast input sample to RGB image type
            memcpy(server->IGTLImageMessage->GetScalarPointer(),
                   image->GetUCharPointer(SVL_LEFT),
                   server->IGTLImageMessage->GetImageSize());

            server->IGTLImageMessage->SetMatrix(server->IGTLImageMatrix);
            server->IGTLImageMessage->SetTimeStamp(igtlImageTimestamp);
            server->IGTLImageMessage->Pack();
        }

        if (!server->Sockets.empty())
        {
            typedef std::list<svlOpenIGTLinkImageServer::SocketsType::iterator> RemovedType;
            RemovedType toBeRemoved;

            const svlOpenIGTLinkImageServer::SocketsType::iterator endSockets = server->Sockets.end();
            svlOpenIGTLinkImageServer::SocketsType::iterator socketIter;

            for (socketIter = server->Sockets.begin();
                 socketIter != endSockets;
                 ++socketIter)
            {
                igtl::Socket::Pointer socket = *socketIter;
                int socketSuccess = socket->Send(server->IGTLImageMessage->GetPackPointer(),
                                                 server->IGTLImageMessage->GetPackSize());
                if (socketSuccess==0)
                {
                    // log some information and remove from list
                    std::string address;
                    int port;
                    socket->GetSocketAddressAndPort(address, port);
                    CMN_LOG_CLASS_RUN_VERBOSE << "Can't send to client for bridge \""
                                              << server->DeviceName << " from "
                                              << address << ":" << port
                                              << std::endl;
                    toBeRemoved.push_back(socketIter);
                }
            }

            const RemovedType::iterator removedEnd = toBeRemoved.end();
            RemovedType::iterator removedIter;
            for (removedIter = toBeRemoved.begin();
                 removedIter != removedEnd;
                 ++removedIter) {
                CMN_LOG_CLASS_RUN_VERBOSE << "Removing client socket for bridge \""
                                          << server->DeviceName << "\"" << std::endl;
                server->Sockets.erase(*removedIter);
            }
        }
        else
        {
            std::cerr<<"Currently there are no clients avaiable for server [ "
                     << IGTLImageServer->DeviceName
                     <<" ] "
                     << std::endl;
            return SVL_FAIL;
        }
    }
    else
    {
        std::cerr << "Broken Message!";
    }
    return 1;
}

int svlOpenIGTLinkBridge::Process(svlProcInfo* procInfo, svlSample* syncInput, svlSample* &syncOutput)
{
    _OnSingleThread(procInfo)                                               // Execute the following block on one thread only
    {

        igtl::Socket::Pointer newSocket;

        // First check if there are new clients
        newSocket = IGTLImageServer->ServerSocket->WaitForConnection(5);
        if (newSocket.IsNotNull()) {
            // log some information and add to list
            std::string address;
            int port;
            newSocket->GetSocketAddressAndPort(address, port);
            std::cerr << "Found new client for svlOpenIGTLinkBridge \""
                                      << IGTLImageServer->DeviceName << " from "
                                      << address << ":" << port << std::endl;
            // set socket time out
            newSocket->SetReceiveTimeout(10);
            // add new socket to the list
            IGTLImageServer->Sockets.push_back(newSocket);
        }

        SendIGTLImageMessages(IGTLImageServer,syncInput);
    }
    syncOutput = syncInput;
    return SVL_OK;
}
