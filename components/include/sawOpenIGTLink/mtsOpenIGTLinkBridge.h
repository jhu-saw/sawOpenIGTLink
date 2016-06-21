/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/*

  Author(s):  Ali Uneri, Anton Deguet, Youri Tan
  Created on: 2009-08-10

  (C) Copyright 2009-2015 Johns Hopkins University (JHU), All Rights Reserved.

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

#ifndef _mtsOpenIGTLinkBridge_h
#define _mtsOpenIGTLinkBridge_h

#include <cisstMultiTask/mtsTaskPeriodic.h>
#include <sawOpenIGTLink/sawOpenIGTLinkExport.h>

class mtsOpenIGTLinkBridgeData;

class CISST_EXPORT mtsOpenIGTLinkBridge: public mtsTaskPeriodic
{
    CMN_DECLARE_SERVICES(CMN_DYNAMIC_CREATION_ONEARG, CMN_LOG_ALLOW_DEFAULT);

 public:
    /*! Constructors */
    inline mtsOpenIGTLinkBridge(const std::string & componentName, const double period) :
        mtsTaskPeriodic(componentName, period, false, 500) {}

    inline mtsOpenIGTLinkBridge(const mtsTaskPeriodicConstructorArg & arg) :
        mtsTaskPeriodic(arg) {}

    /*! Destructor */
    inline ~mtsOpenIGTLinkBridge(void) {}

    void Configure(const std::string & jsonFile);
    inline void Startup(void) {};
    void Run(void);
    void Cleanup(void);

    bool ConfigureServer(const Json::Value & jsonServer);

    bool Connect(void);

    bool AddServerFromCommandRead(const int port, const std::string & igtlFrameName,
                                  const std::string & interfaceRequiredName,
                                  const std::string & componentName,
                                  const std::string & providedName,
                                  const std::string & commandName = "GetPositionCartesian");

    bool AddServerFromCommandWrite(const int port, const std::string & igtlFrameName,
                                   const std::string & interfaceProvidedName,
                                   const std::string & commandName = "SetPositionCartesian");

    void ServerSend(mtsOpenIGTLinkBridgeData * bridge);
    void ServerReceive(mtsOpenIGTLinkBridgeData * bridge);

 protected:
    enum ConnectionTypes { SERVER, CLIENT };

    typedef std::list<mtsOpenIGTLinkBridgeData *> BridgesType;
    BridgesType Bridges;
};

CMN_DECLARE_SERVICES_INSTANTIATION(mtsOpenIGTLinkBridge);

#endif  // _mtsOpenIGTLinkBridge_h
