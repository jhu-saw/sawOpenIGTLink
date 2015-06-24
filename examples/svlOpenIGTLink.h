// See license at http://www.cisst.org/cisst/license.txt

#ifndef _svlOpenIGTLink_h
#define _svlOpenIGTLink_h

#include <cisstStereoVision/svlFilterBase.h>
#include <cisstStereoVision/svlDefinitions.h>

#include <igtl/igtlOSUtil.h>
#include <igtl/igtlTransformMessage.h>
#include <igtl/igtlServerSocket.h>
#include <igtl/igtlImageMessage.h>
#include <igtl/igtlMessageBase.h>
#include <igtl/igtlSocket.h>
#include <igtl/igtlImageMetaMessage.h>
#include <igtl/igtlTransformMessage.h>
#include <igtl/igtlPositionMessage.h>
#include <igtl/igtl_util.h>

class svlOpenIGTLinkBridge : public svlFilterBase
{
public:
    svlOpenIGTLinkBridge();
    igtl::ImageMessage::Pointer IGTOutputLeft;
    igtl::ImageMessage::Pointer IGTOutputRight;
    igtl::ServerSocket::Pointer IGTServerSocket;
    igtl::Socket::Pointer socket;
    int NumberOfClients;
    igtl::Matrix4x4 IGTLMatrix;

protected:
    int Initialize(svlSample* syncInput, svlSample* &syncOutput);
    int Process(svlProcInfo* procInfo, svlSample* syncInput, svlSample* &syncOutput);
};

#endif // _svlOpenIGTLink_h
