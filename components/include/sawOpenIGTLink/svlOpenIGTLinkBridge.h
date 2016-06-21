// See license at http://www.cisst.org/cisst/license.txt

#ifndef _svlOpenIGTLink_h
#define _svlOpenIGTLink_h

#include <cisstStereoVision/svlFilterBase.h>
#include <cisstStereoVision/svlDefinitions.h>

class svlOpenIGTLinkImageServer;

class svlOpenIGTLinkBridge : public svlFilterBase
{
public:
    svlOpenIGTLinkBridge();
    virtual ~svlOpenIGTLinkBridge();

    bool SetPortNumber(int port);
    int GetPortNumber(void) const;
    void SetDeviceName(const std::string & name);
    const std::string & GetDeviceName(void) const;

    svlOpenIGTLinkImageServer* IGTLImageServer;
    std::string DeviceName;
    int Port;
    int SendIGTLImageMessages(svlSample *syncInput);

protected:
    int Initialize(svlSample* syncInput, svlSample* &syncOutput);
    int Process(svlProcInfo* procInfo, svlSample* syncInput, svlSample* &syncOutput);
};

#endif // _svlOpenIGTLink_h
