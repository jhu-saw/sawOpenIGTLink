/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Anton Deguet
  Created on: 2020-03-24

  (C) Copyright 2020 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#ifndef _mtsIGTLCRTKBridge_h
#define _mtsIGTLCRTKBridge_h

// cisst include
#include <sawOpenIGTLink/mtsIGTLBridge.h>
#include <cisstMultiTask/mtsDelayedConnections.h>


class mtsIGTLCRTKBridge: public mtsIGTLBridge
{
    CMN_DECLARE_SERVICES(CMN_DYNAMIC_CREATION_ONEARG, CMN_LOG_ALLOW_DEFAULT);

public:
    inline mtsIGTLCRTKBridge(const std::string & componentName, const double & period):
        mtsIGTLBridge(componentName, period) {}

    inline mtsIGTLCRTKBridge(const mtsTaskPeriodicConstructorArg & arg):
        mtsIGTLBridge(arg) {}

    void ConfigureJSON(const Json::Value & jsonConfig) override;

    void BridgeInterfaceProvided(const std::string & componentName,
                                 const std::string & interfaceName,
                                 const std::string & nameSpace);

    /*! Connect all components created and used so far. */
    inline virtual void Connect(void) {
        mConnections.Connect();
    }

protected:
    //! ros node
    std::string mNamespace;
    mtsDelayedConnections mConnections;

    void GetCRTKCommand(const std::string & fullCommand,
                        std::string & crtkCommand);
};

CMN_DECLARE_SERVICES_INSTANTIATION(mtsIGTLCRTKBridge);

#endif // _mtsIGTLCRTKBridge_h
