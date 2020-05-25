/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/*

  Author(s):  Ali Uneri, Anton Deguet, Youri Tan
  Created on: 2009-08-10

  (C) Copyright 2009-2020 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

/*!
  \file
  \brief SAW component for establishing a network connection via OpenIGTLink protocol.
  \ingroup sawComponents
*/

#ifndef _mtsIGTLBridge_h
#define _mtsIGTLBridge_h

#include <cisstMultiTask/mtsTaskPeriodic.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>

#include <igtlServerSocket.h>
#include <igtlTimeStamp.h>
#include <igtlMessageBase.h>

// Always include last!
#include <sawOpenIGTLink/sawOpenIGTLinkExport.h>

class mtsIGTLBridge;

class mtsIGTLSenderBase
{
public:
    inline mtsIGTLSenderBase(const std::string & name, mtsIGTLBridge * bridge):
        mName(name), mBridge(bridge) {}

    //! Function used to pull data from the cisst component
    mtsFunctionRead Function;

    virtual ~mtsIGTLSenderBase() {};

    virtual bool Execute(void) = 0;

protected:
    std::string mName;
    mtsIGTLBridge * mBridge;
};

template <typename _cisstType, typename _igtlType>
class mtsIGTLSender: public mtsIGTLSenderBase
{
public:
    inline mtsIGTLSender(const std::string & name, mtsIGTLBridge * bridge):
        mtsIGTLSenderBase(name, bridge) {
        mIGTLData = _igtlType::New();
        mIGTLData->SetDeviceName(name);
    }
    inline virtual ~mtsIGTLSender() {}
    bool Execute(void);

protected:
    _cisstType mCISSTData;
    typedef typename _igtlType::Pointer IGTLPointer;
    IGTLPointer mIGTLData;
};

class CISST_EXPORT mtsIGTLBridge: public mtsTaskPeriodic
{
    CMN_DECLARE_SERVICES(CMN_DYNAMIC_CREATION_ONEARG, CMN_LOG_ALLOW_DEFAULT);

 public:
    /*! Constructors */
    inline mtsIGTLBridge(const std::string & componentName, const double & period):
        mtsTaskPeriodic(componentName, period, false, 500) {
    }

    inline mtsIGTLBridge(const mtsTaskPeriodicConstructorArg & arg):
        mtsTaskPeriodic(arg) {
    }

    /*! Destructor */
    inline ~mtsIGTLBridge(void) {};

    /*! Must be called after Configure or SetPort. */
    void InitServer(void);

    void Configure(const std::string & jsonFile) override;
    virtual void ConfigureJSON(const Json::Value & jsonConfig);

    inline void Startup(void) override {};
    void Run(void) override;
    void Cleanup(void) override;

    template <typename _cisstType, typename _igtlType>
    bool AddSenderFromCommandRead(const std::string & interfaceRequiredName,
                                  const std::string & functionName,
                                  const std::string & igtlDeviceName);

    void SendAll(void);
    void Send(igtl::MessageBase & message);

 protected:
    // igtl networking
    int mPort = 18944; // default
    igtl::ServerSocket::Pointer mServerSocket;
    typedef std::list<igtl::ClientSocket::Pointer> SocketsType;
    SocketsType mSockets;

    // cisst interfaces
    typedef std::list<mtsIGTLSenderBase *> SendersType;
    SendersType mSenders;
};


template <typename _cisstType, typename _igtlType>
bool mtsIGTLSender<_cisstType, _igtlType>::Execute(void) {
    mtsExecutionResult result = Function(mCISSTData);
    if (result) {
        if (mtsCISSTToIGTL(mCISSTData, mIGTLData)) {
            mBridge->Send(*mIGTLData);
            return true;
        }
    } else {
        CMN_LOG_RUN_ERROR << "mtsIGTLSender::Execute: " << result
                          << " for " << mName << std::endl;
    }
    return false;
}


template <typename _cisstType, typename _igtlType>
bool mtsIGTLBridge::AddSenderFromCommandRead(const std::string & interfaceRequiredName,
                                             const std::string & functionName,
                                             const std::string & igtlDeviceName) {
    // check if the interface exists of try to create one
    mtsInterfaceRequired * interfaceRequired
        = this->GetInterfaceRequired(interfaceRequiredName);
    if (!interfaceRequired) {
        interfaceRequired = this->AddInterfaceRequired(interfaceRequiredName);
    }
    if (!interfaceRequired) {
        CMN_LOG_CLASS_INIT_ERROR << "AddSenderFromCommandRead: failed to create required interface \""
                                 << interfaceRequiredName << "\"" << std::endl;
        return false;
    }
    mtsIGTLSenderBase * sender =
        new mtsIGTLSender<_cisstType, _igtlType>(igtlDeviceName, this);
    if (!interfaceRequired->AddFunction(functionName, sender->Function)) {
        CMN_LOG_CLASS_INIT_ERROR << "AddSenderFromCommandRead: failed to create function \""
                                 << functionName << "\"" << std::endl;
        delete sender;
        return false;
    }
    mSenders.push_back(sender);
    return true;
}

CMN_DECLARE_SERVICES_INSTANTIATION(mtsIGTLBridge);

#endif  // _mtsIGTLBridge_h
