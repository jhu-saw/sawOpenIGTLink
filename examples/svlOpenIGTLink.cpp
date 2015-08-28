// See license at http://www.cisst.org/cisst/license.txt


#include <cisstOSAbstraction/osaSleep.h>

// #include <igtlOSUtil.h>
#include <igtlTransformMessage.h>
#include <igtlServerSocket.h>
#include <igtlImageMessage.h>
#include <igtlMessageBase.h>
#include <igtlSocket.h>
#include <igtlImageMetaMessage.h>
#include <igtlTransformMessage.h>
#include <igtlPositionMessage.h>
#include <igtl_util.h>

#include "svlOpenIGTLink.h"

typedef std::list<igtl::Socket::Pointer> SocketsType;

class svlOpenIGTLinkImageServer {
public:

    int                                         Port;
    std::string                                 DeviceName;
    igtl::ServerSocket::Pointer                 ServerSocket;
    igtl::ImageMessage::Pointer                 IGTLImageMessage;
    igtl::Matrix4x4                             IGTLImageMatrix;
    SocketsType                                 Sockets;
    bool                                        MessageInitialized;

    int InitializeIGTLData(svlSample *inputImage);
    void InitializeIGTServerSocket(int port);
};


int svlOpenIGTLinkImageServer::InitializeIGTLData(svlSample* inputImage)
{
    const svlSampleImage* image = (svlSampleImage*)(inputImage);

    this->IGTLImageMessage = igtl::ImageMessage::New();
    this->IGTLImageMessage->SetDeviceName(this->DeviceName.c_str());
    this->IGTLImageMessage->SetOrigin(0.0, 0.0, 0.0);
    this->IGTLImageMessage->SetSpacing(1.0, 1.0, 1.0);
    this->IGTLImageMessage->SetEndian(igtl_is_little_endian() ? igtl::ImageMessage::ENDIAN_LITTLE : igtl::ImageMessage::ENDIAN_BIG);

    // Assign image transformation matrix
    this->IGTLImageMatrix[0][0] = -1.0;  this->IGTLImageMatrix[1][0] = 0.0;  this->IGTLImageMatrix[2][0] = 0.0; this->IGTLImageMatrix[3][0] = 0.0;
    this->IGTLImageMatrix[0][1] = 0.0;   this->IGTLImageMatrix[1][1] = -1.0; this->IGTLImageMatrix[2][1] = 0.0; this->IGTLImageMatrix[3][1] = 0.0;
    this->IGTLImageMatrix[0][2] = 0.0;   this->IGTLImageMatrix[1][2] = 0.0;  this->IGTLImageMatrix[2][2] = 1.0; this->IGTLImageMatrix[3][2] = 0.0;
    this->IGTLImageMatrix[0][3] = 0.0;   this->IGTLImageMatrix[1][3] = 0.0;  this->IGTLImageMatrix[2][3] = 0.0; this->IGTLImageMatrix[3][3] = 1.0;

    int subOffset[3]={0};
    int imageSizePixels[3]={    image->GetWidth(),
                                image->GetHeight(),
                                image->GetVideoChannels() };

    if(image->GetVideoChannels()==2)
    {
        if(image->GetWidth(SVL_LEFT) !=image->GetWidth(SVL_RIGHT) || image->GetHeight(SVL_LEFT) !=image->GetHeight(SVL_RIGHT) )
        {
            std::cerr<<"Stereo images are not equal size"<<std::endl;
            return SVL_FAIL;
        }
    }

    std::cerr<< "Input image width: " <<
                imageSizePixels[0] <<
                ", height: " <<
                imageSizePixels[1] <<
                ", channels: " <<
                imageSizePixels[2] <<
                std::endl;

    svlStreamType type = image->GetType();

    if(type == svlTypeImageMono8 || type == svlTypeImageMono8Stereo)
    {
        this->IGTLImageMessage->SetScalarType(igtl::ImageMessage::TYPE_INT8);
        this->IGTLImageMessage->SetNumComponents(1);
    }
    else if(type == svlTypeImageRGB || type == svlTypeImageRGBStereo)
    {
        this->IGTLImageMessage->SetScalarType(igtl::ImageMessage::TYPE_UINT8);
        this->IGTLImageMessage->SetNumComponents(3);
    }
    else if(type == svlTypeImageMono16 ||type== svlTypeImageMono16Stereo )
    {
        this->IGTLImageMessage->SetScalarType(igtl::ImageMessage::TYPE_INT16);
        this->IGTLImageMessage->SetNumComponents(1);
    }
    else if(type == svlTypeImageMono32 || type == svlTypeImageMono32Stereo)
    {
        this->IGTLImageMessage->SetScalarType(igtl::ImageMessage::TYPE_INT32);
        this->IGTLImageMessage->SetNumComponents(1);
    }
    else if(type== svlTypeImageRGBA || type==svlTypeImageRGBAStereo)
    {
        this->IGTLImageMessage->SetScalarType(igtl::ImageMessage::TYPE_UINT8);
        this->IGTLImageMessage->SetNumComponents(4);
    }
    else if(type== svlTypeImage3DMap)
    {
        this->IGTLImageMessage->SetScalarType(igtl::ImageMessage::TYPE_FLOAT32);
        this->IGTLImageMessage->SetNumComponents(3);
    }
    else
    {
        std::cerr<<"Image type not supported"<<std::endl;
        return SVL_FAIL;
    }

    this->IGTLImageMessage->SetDimensions(imageSizePixels);
    this->IGTLImageMessage->SetSubVolume(imageSizePixels, subOffset);
    this->IGTLImageMessage->SetMatrix(this->IGTLImageMatrix);
    this->IGTLImageMessage->AllocateScalars();
    this->MessageInitialized = true;

    return SVL_OK;
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
    this->Port = port;
}


svlOpenIGTLinkBridge::svlOpenIGTLinkBridge() :
    svlFilterBase()                                                       // Call baseclass' constructor
{
    this->IGTLImageServer=0;
    AddInput("input", true);                                              // Create synchronous input connector
    AddInputType("input", svlTypeImageRGBStereo);                         // Set sample type for input connector
    AddInputType("input", svlTypeImageRGB);                               // Set sample type for input connector
    AddInputType("input", svlTypeImageRGBA);                         // Set sample type for input connector
    AddInputType("input", svlTypeImageRGBAStereo);                               // Set sample type for input connector
    AddInputType("input", svlTypeImageMono8);                         // Set sample type for input connector
    AddInputType("input", svlTypeImageMono8Stereo);                               // Set sample type for input connector
    AddInputType("input", svlTypeImageMono16);                         // Set sample type for input connector
    AddInputType("input", svlTypeImageMono16Stereo);                               // Set sample type for input connector
    AddInputType("input", svlTypeImageMono32);                         // Set sample type for input connector
    AddInputType("input", svlTypeImageMono32Stereo);                               // Set sample type for input connector
    AddInputType("input", svlTypeImage3DMap);                               // Set sample type for input connector

    AddOutput("output", true);                                            // Create synchronous ouput connector
    SetAutomaticOutputType(true);                                         // Set output sample type the same as input sample type

    this->Port = -1;
    this->DeviceName= "svlOpenIGTLinkBridge";
}

svlOpenIGTLinkBridge::~svlOpenIGTLinkBridge()
{
    if(IGTLImageServer)
    {
        delete IGTLImageServer;
    }
}

int svlOpenIGTLinkBridge::SetDeviceName(std::string name)
{
    this->DeviceName = name;
    return 1;
}

std::string svlOpenIGTLinkBridge::GetDeviceName()
{
    return this->IGTLImageServer->DeviceName;
}

int svlOpenIGTLinkBridge::SetPortNumber(int port)
{
    if(this->Port == -1)
    {
        this->Port = port;
        return 1;
    }
    return 0;
}


int svlOpenIGTLinkBridge::Initialize(svlSample* syncInput, svlSample* &syncOutput)
{
    this->IGTLImageServer = new svlOpenIGTLinkImageServer();
    this->IGTLImageServer->MessageInitialized = false;
    this->IGTLImageServer->DeviceName = this->DeviceName;
    int success = this->IGTLImageServer->InitializeIGTLData(syncInput);
    this->IGTLImageServer->Port = -1;

    syncOutput = syncInput;                                              // Pass the input sample forward to the output
    return success;                                                        //
}

int svlOpenIGTLinkBridge::SendIGTLImageMessages(svlSample* syncInput)
{
    // check if we have new client
    if (this->IGTLImageServer->IGTLImageMessage.IsNotNull())
    {
        if(!this->IGTLImageServer->Sockets.empty())
        {
            double timeStamp = syncInput->GetTimestamp();
            igtl::TimeStamp::Pointer igtlImageTimestamp = igtl::TimeStamp::New();
            igtlImageTimestamp->SetTime(timeStamp);

            const svlSampleImage * image = dynamic_cast<const svlSampleImage *>(syncInput);
            CMN_ASSERT(image);

            unsigned char* destination = (unsigned char*)(this->IGTLImageServer->IGTLImageMessage->GetScalarPointer());

            int imageSizeAtChannel = this->IGTLImageServer->IGTLImageMessage->GetImageSize() / image->GetVideoChannels();

            for(unsigned int channel =0; channel < image->GetVideoChannels(); channel++  )
            {
                memcpy(destination + imageSizeAtChannel * channel,
                       image->GetUCharPointer(channel),
                       imageSizeAtChannel);
            }

            this->IGTLImageServer->IGTLImageMessage->SetMatrix(this->IGTLImageServer->IGTLImageMatrix);
            this->IGTLImageServer->IGTLImageMessage->SetTimeStamp(igtlImageTimestamp);
            this->IGTLImageServer->IGTLImageMessage->Pack();

            typedef std::list<SocketsType::iterator> RemovedType;
            RemovedType toBeRemoved;

            const SocketsType::iterator endSockets = this->IGTLImageServer->Sockets.end();
            SocketsType::iterator socketIter;

            for (socketIter = this->IGTLImageServer->Sockets.begin();
                 socketIter != endSockets;
                 ++socketIter)
            {
                igtl::Socket::Pointer socket = *socketIter;
				socket->SetTimeout(1000);
                int socketSuccess = socket->Send(this->IGTLImageServer->IGTLImageMessage->GetPackPointer(),
                                                 this->IGTLImageServer->IGTLImageMessage->GetPackSize());
                if (socketSuccess==0 /*|| socketSuccess==-1*/)
                {
                    // log some information and remove from list
                    std::string address;
                    int port;
                    socket->GetSocketAddressAndPort(address, port);
                    CMN_LOG_CLASS_RUN_VERBOSE << "Can't send to client for bridge \""
                                              << this->IGTLImageServer->DeviceName << " from "
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
                std::cerr << "Removing client socket for bridge \""
                          << this->IGTLImageServer->DeviceName << "\"" << std::endl;
                this->IGTLImageServer->Sockets.erase(*removedIter);
            }
        }
        else
        {
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
        if(this->IGTLImageServer->DeviceName!=this->DeviceName)
        {
            this->IGTLImageServer->DeviceName = this->DeviceName;
        }

        if(this->IGTLImageServer->Port!=-1)
        {
            igtl::Socket::Pointer newSocket;

            // First check if there are new clients
            newSocket = this->IGTLImageServer->ServerSocket->WaitForConnection(5);
            if (newSocket.IsNotNull()) {
                // log some information and add to list
                std::string address;
                int port;
                newSocket->GetSocketAddressAndPort(address, port);
                std::cerr << "Found new client for svlOpenIGTLinkBridge \""
                          << this->IGTLImageServer->DeviceName << " from "
                          << address << ":" << port << std::endl;
                // set socket time out
                //newSocket->SetReceiveTimeout(10);
				newSocket->SetTimeout(1000);
                // add new socket to the list
                this->IGTLImageServer->Sockets.push_back(newSocket);
            }

            this->SendIGTLImageMessages(syncInput);
        }
        else{
            if(this->Port!=-1 && this->IGTLImageServer->Port==-1 )
            {
                std::cerr<<"Asign port number, namely: "
                        << this->Port
                        <<std::endl;
                this->IGTLImageServer->InitializeIGTServerSocket(Port);
            }
        }
    }
    syncOutput = syncInput;
    return SVL_OK;
}
