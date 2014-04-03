/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*

  Author(s):  Ali Uneri
  Created on: 2009-08-11

  (C) Copyright 2007-2012 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstCommon/cmnLogger.h>
#include <cisstOSAbstraction/osaSleep.h>
#include <sawOpenIGTLink/mtsOpenIGTLink.h>

#include "trackerSimulator.h"

#define IS_SERVER 0  // make this 0 to run as a client


int main()
{
    // log configuration
    cmnLogger::SetMask(CMN_LOG_ALLOW_ALL);
    cmnLogger::SetMaskFunction(CMN_LOG_ALLOW_ALL);
    cmnLogger::SetMaskDefaultLog(CMN_LOG_ALLOW_ALL);
    cmnLogger::SetMaskClassMatching("mtsOpenIGTLink", CMN_LOG_ALLOW_ALL);
    cmnLogger::AddChannel(std::cerr, CMN_LOG_ALLOW_ERRORS_AND_WARNINGS);

    mtsComponentManager * componentManager = mtsComponentManager::GetInstance();

    // create components
    mtsOpenIGTLink * mtsOpenIGTLinkObj = new mtsOpenIGTLink("MyOpenIGTLink", 50.0 * cmn_ms);
#if IS_SERVER
    std::cout << "Running as OpenIGTLink server on port 18944" << std::endl;
    mtsOpenIGTLinkObj->Configure("18944");
#else
    std::cout << "Running as OpenIGTLink client on port localhost:18944" << std::endl;
    mtsOpenIGTLinkObj->Configure("localhost:18944");
#endif
    trackerSimulator * trackerSimulatorObj = new trackerSimulator("trackerSimulator", 50.0 * cmn_ms);

    // add components to component manager
    componentManager->AddComponent(mtsOpenIGTLinkObj);
    componentManager->AddComponent(trackerSimulatorObj);
    trackerSimulatorObj->Configure();

    // connect components
    componentManager->Connect(trackerSimulatorObj->GetName(), "RequiresPositionCartesian",
                              mtsOpenIGTLinkObj->GetName(), "ProvidesPositionCartesian");

    // create and start all components
    componentManager->CreateAll();
    componentManager->WaitForStateAll(mtsComponentState::READY);
    componentManager->StartAll();
    componentManager->WaitForStateAll(mtsComponentState::ACTIVE);

    // loop until GUI exits
    while (!trackerSimulatorObj->GetExitFlag()) {
        osaSleep(10.0 * cmn_ms);
    }

    // kill all components and perform cleanup
    componentManager->KillAll();
    componentManager->WaitForStateAll(mtsComponentState::FINISHED, 2.0 * cmn_s);
    componentManager->Cleanup();

    return 0;
}
