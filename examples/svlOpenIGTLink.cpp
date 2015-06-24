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
    IGTLImageServer->InitializeIGTServerSocket(18955);

    IGTOutputLeft = igtl::ImageMessage::New();
    IGTOutputRight = igtl::ImageMessage::New();
    IGTServerSocket = igtl::ServerSocket::New();
    socket = igtl::Socket::New();

    int r = IGTServerSocket->CreateServer(18944);

    if (r < 0)
    {
        std::cerr << "Cannot create a server socket." << std::endl;
        exit(0);
    }

    int imageSizePixels[3]={720,480,1}, subOffset[3]={0};
    IGTLMatrix[0][0] = -1.0;  IGTLMatrix[1][0] = 0.0;  IGTLMatrix[2][0] = 0.0; IGTLMatrix[3][0] = 0.0;
    IGTLMatrix[0][1] = 0.0;  IGTLMatrix[1][1] = -1.0;  IGTLMatrix[2][1] = 0.0; IGTLMatrix[3][1] = 0.0;
    IGTLMatrix[0][2] = 0.0;  IGTLMatrix[1][2] = 0.0;  IGTLMatrix[2][2] = 1.0; IGTLMatrix[3][2] = 0.0;
    IGTLMatrix[0][3] = 0.0;  IGTLMatrix[1][3] = 0.0;  IGTLMatrix[2][3] = 0.0; IGTLMatrix[3][3] = 1.0;

    IGTOutputLeft->SetDimensions(imageSizePixels);
    IGTOutputLeft->SetDeviceName("OpenIGTLinkSvlFilter");
    IGTOutputLeft->SetScalarType(igtl::ImageMessage::TYPE_UINT8);
    IGTOutputLeft->SetOrigin(0.0, 0.0, 0.0);
    IGTOutputLeft->SetSpacing(1.0, 1.0, 1.0);
    IGTOutputLeft->SetEndian(igtl_is_little_endian() ? igtl::ImageMessage::ENDIAN_LITTLE : igtl::ImageMessage::ENDIAN_BIG);
    IGTOutputLeft->SetSubVolume( imageSizePixels, subOffset );
    IGTOutputLeft->SetNumComponents(3);
    IGTOutputLeft->AllocateScalars();
    IGTOutputLeft->SetMatrix(IGTLMatrix);

    syncOutput = syncInput;                                              // Pass the input sample forward to the output
    NumberOfClients = 0;
    return SVL_OK;                                                        //
}

int svlOpenIGTLinkBridge::Process(svlProcInfo* procInfo, svlSample* syncInput, svlSample* &syncOutput)
{
    _OnSingleThread(procInfo)                                               // Execute the following block on one thread only
    {
        // check if we have new client
        if (NumberOfClients == 0) {
            socket = IGTServerSocket->WaitForConnection(5); // wait for up to 5 ms, should not slow down any camera rate
        }
        if (socket.IsNotNull()) {
            NumberOfClients++;
            std::cerr << "+";
        }

        if (IGTOutputLeft.IsNotNull()) {

            if (NumberOfClients > 0) {

                // make sure image was created properly
                if (IGTOutputLeft.IsNull()) {
                    std::cerr<<"Failed to convert image to ImageMessage, ImageMessage is null" << std::endl;
                    return SVL_FAIL;
                }

                // build image data
                double timeStamp = syncInput->GetTimestamp();
                igtl::TimeStamp::Pointer igtlImageTimestamp = igtl::TimeStamp::New();
                igtlImageTimestamp->SetTime(timeStamp);

                const svlSampleImageRGB* image = (svlSampleImageRGB*)(syncInput);   // Cast input sample to RGB image type
                std::cerr << "svl: " << image->GetDataSize() << " igtl:" << IGTOutputLeft->GetImageSize() << std::endl;
                memcpy(IGTOutputLeft->GetScalarPointer(), image->GetUCharPointer(SVL_LEFT), IGTOutputLeft->GetImageSize());

                IGTOutputLeft->SetMatrix(IGTLMatrix);
                IGTOutputLeft->SetTimeStamp(igtlImageTimestamp);
                IGTOutputLeft->Pack();

                std::cerr << "sending\n";
                int socketSuccess = socket->Send(IGTOutputLeft->GetPackPointer(), IGTOutputLeft->GetPackSize());
                if (!socketSuccess) {
                    std::cerr << "no socket\n";
                    NumberOfClients--;
                }
                std::cerr << "sent\n";
                // osaSleep(1.0 * cmn_s);
            }
            else
            {
                std::cerr <<"0";
            }
        }
        else
        {
            std::cerr << "!";
        }
    }
    syncOutput = syncInput;
    return SVL_OK;
}
