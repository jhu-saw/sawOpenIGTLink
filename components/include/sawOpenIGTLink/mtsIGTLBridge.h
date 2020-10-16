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

#include <map>

#include <cisstMultiTask/mtsTaskPeriodic.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>

// Always include last!
#include <sawOpenIGTLink/sawOpenIGTLinkExport.h>

class mtsIGTLBridge;
class mtsIGTLBridgeData;

namespace igtl {
    class MessageBase;
    class Socket;
}

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
    }
    inline virtual ~mtsIGTLSender() {}
    bool Execute(void) override;

protected:
    _cisstType mCISSTData;
    typedef typename _igtlType::Pointer IGTLPointer;
    IGTLPointer mIGTLData;
};

template <typename _cisstType, typename _igtlType>
class mtsIGTLEventWriteSender: public mtsIGTLSenderBase
{
public:
    inline mtsIGTLEventWriteSender(const std::string & name, mtsIGTLBridge * bridge):
        mtsIGTLSenderBase(name, bridge) {
        mIGTLData = _igtlType::New();
        mIGTLData->SetDeviceName(name);
    }
    inline virtual ~mtsIGTLEventWriteSender() {}
    bool Execute(void) override {
        return true;
    }
    void EventHandler(const _cisstType & cisstData);
protected:
    typedef typename _igtlType::Pointer IGTLPointer;
    IGTLPointer mIGTLData;
};


class mtsIGTLReceiverBase
{
public:
    inline mtsIGTLReceiverBase(const std::string & name, mtsIGTLBridge * bridge):
        mName(name), mBridge(bridge) {}

    //! Function used to pull data from the cisst component
    mtsFunctionWrite Function;

    virtual ~mtsIGTLReceiverBase() {};

    virtual bool Execute(igtl::Socket * socket, igtl::MessageBase * header) = 0;

protected:
    std::string mName;
    mtsIGTLBridge * mBridge;
};

template <typename _igtlType, typename _cisstType>
class mtsIGTLReceiver: public mtsIGTLReceiverBase
{
public:
    inline mtsIGTLReceiver(const std::string & name, mtsIGTLBridge * bridge):
        mtsIGTLReceiverBase(name, bridge) {
    }
    inline virtual ~mtsIGTLReceiver() {}
    bool Execute(igtl::Socket * socket, igtl::MessageBase * header) override;

protected:
    typedef typename _igtlType::Pointer IGTLPointer;
    _cisstType mCISSTData;
};


class CISST_EXPORT mtsIGTLBridge: public mtsTaskPeriodic
{
    CMN_DECLARE_SERVICES(CMN_DYNAMIC_CREATION_ONEARG, CMN_LOG_ALLOW_DEFAULT);

 public:
    /*! Constructors */
    inline mtsIGTLBridge(const std::string & componentName, const double & period):
        mtsTaskPeriodic(componentName, period, false, 500) {
        Init();
    }

    inline mtsIGTLBridge(const mtsTaskPeriodicConstructorArg & arg):
        mtsTaskPeriodic(arg) {
        Init();
    }

    /*! Destructor */
    inline ~mtsIGTLBridge(void) {}

 protected:
    /*! Must be called in all constructors */
    void Init(void);

    /*! Must be called after Configure or SetPort. */
    void InitServer(void);

 public:
    inline void SetPort(const int port) {
        mPort = port;
        InitServer();
    }

    void Configure(const std::string & jsonFile) override;
    virtual void ConfigureJSON(const Json::Value & jsonConfig);

    void Startup(void) override;
    void Run(void) override;
    void Cleanup(void) override;

    template <typename _cisstType, typename _igtlType>
    bool AddSenderFromCommandRead(const std::string & interfaceRequiredName,
                                  const std::string & functionName,
                                  const std::string & igtlDeviceName);

    template <typename _cisstType, typename _igtlType>
    bool AddSenderFromEventWrite(const std::string & interfaceRequiredName,
                                 const std::string & eventName,
                                 const std::string & igtlDeviceName);

    template <typename _igtlType, typename _cisstType>
    bool AddReceiverToCommandWrite(const std::string & interfaceRequiredName,
                                   const std::string & commandName,
                                   const std::string & igtlDeviceName);

    void SendAll(void);

    template <typename _igtlMessagePointer>
    void Send(_igtlMessagePointer message);

    void ReceiveAll(void);

 protected:
    // igtl networking
    int mPort = 0; // default
    mtsIGTLBridgeData * mData = nullptr;

    // cisst interfaces
    typedef std::list<mtsIGTLSenderBase *> SendersType;
    SendersType mSenders;

    typedef std::map<std::string, mtsIGTLReceiverBase *> ReceiversType;
    ReceiversType mReceivers;
};


template <typename _cisstType, typename _igtlType>
bool mtsIGTLSender<_cisstType, _igtlType>::Execute(void)
{
    mtsExecutionResult result = Function(mCISSTData);
    if (result) {
        mIGTLData = _igtlType::New();
        mIGTLData->SetDeviceName(mName);
        if (mtsCISSTToIGTL(mCISSTData, mIGTLData)) {
            mIGTLData->Pack();
            mBridge->Send(mIGTLData);
            return true;
        }
    } else {
        CMN_LOG_RUN_ERROR << "mtsIGTLSender::Execute: " << result
                          << " for " << mName << std::endl;
    }
    return false;
}

template <typename _cisstType, typename _igtlType>
void mtsIGTLEventWriteSender<_cisstType, _igtlType>::EventHandler(const _cisstType & cisstData)
{
    mIGTLData = _igtlType::New();
    mIGTLData->SetDeviceName(mName);
    if (mtsCISSTToIGTL(cisstData, mIGTLData)) {
        mIGTLData->Pack();
        mBridge->Send(mIGTLData);
    }
}

template <typename _cisstType, typename _igtlType>
bool mtsIGTLBridge::AddSenderFromCommandRead(const std::string & interfaceRequiredName,
                                             const std::string & functionName,
                                             const std::string & igtlDeviceName)
{
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
    mtsIGTLSenderBase * newSender =
        new mtsIGTLSender<_cisstType, _igtlType>(igtlDeviceName, this);
    if (!interfaceRequired->AddFunction(functionName, newSender->Function)) {
        CMN_LOG_CLASS_INIT_ERROR << "AddSenderFromCommandRead: failed to add function \""
                                 << functionName << "\" to interface required \""
                                 << interfaceRequiredName << "\"" << std::endl;
        delete newSender;
        return false;
    }
    mSenders.push_back(newSender);
    return true;
}


template <typename _cisstType, typename _igtlType>
bool mtsIGTLBridge::AddSenderFromEventWrite(const std::string & interfaceRequiredName,
                                            const std::string & eventName,
                                            const std::string & igtlDeviceName)
{
    // check if the interface exists of try to create one
    mtsInterfaceRequired * interfaceRequired
        = this->GetInterfaceRequired(interfaceRequiredName);
    if (!interfaceRequired) {
        interfaceRequired = this->AddInterfaceRequired(interfaceRequiredName);
    }
    if (!interfaceRequired) {
        CMN_LOG_CLASS_INIT_ERROR << "AddSenderFromEventWrite: failed to create required interface \""
                                 << interfaceRequiredName << "\"" << std::endl;
        return false;
    }
    mtsIGTLEventWriteSender<_cisstType, _igtlType> * newSender
        = new mtsIGTLEventWriteSender<_cisstType, _igtlType>(igtlDeviceName, this);
    if (!interfaceRequired->AddEventHandlerWrite(&mtsIGTLEventWriteSender<_cisstType, _igtlType>::EventHandler,
                                                 newSender, eventName)) {
        CMN_LOG_CLASS_INIT_ERROR << "AddSenderFromEventWrite: failed to add event \""
                                 << eventName << "\" to required interface \""
                                 << interfaceRequiredName << "\"" << std::endl;
        delete newSender;
        return false;
    }
    mSenders.push_back(newSender);
    return true;
}

template <typename _igtlType, typename _cisstType>
bool mtsIGTLBridge::AddReceiverToCommandWrite(const std::string & interfaceRequiredName,
                                              const std::string & commandName,
                                              const std::string & igtlDeviceName)
{
    // check if the interface exists of try to create one
    mtsInterfaceRequired * interfaceRequired = this->GetInterfaceRequired(interfaceRequiredName);
    if (!interfaceRequired) {
        interfaceRequired = this->AddInterfaceRequired(interfaceRequiredName);
    }
    if (!interfaceRequired) {
        CMN_LOG_CLASS_INIT_ERROR << "AddReceiverToCommandWrite: failed to create required interface \""
                                 << interfaceRequiredName << "\"" << std::endl;
        return false;
    }
    mtsIGTLReceiver<_igtlType, _cisstType> * newReceiver
        = new mtsIGTLReceiver<_igtlType, _cisstType>(igtlDeviceName, this);
    if (!interfaceRequired->AddFunction(commandName, newReceiver->Function)) {
        CMN_LOG_CLASS_INIT_ERROR << "AddReceiverToCommandWrite: failed to add function \""
                                 << commandName << "\" to interface required \""
                                 << interfaceRequiredName << "\"" << std::endl;
        delete newReceiver;
        return false;
    }
    mReceivers.insert({igtlDeviceName, newReceiver});
    return true;
}

CMN_DECLARE_SERVICES_INSTANTIATION(mtsIGTLBridge);

#endif  // _mtsIGTLBridge_h
