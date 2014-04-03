/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*

  Author(s):  Ali Uneri
  Created on: 2009-08-11

  (C) Copyright 2007-2011 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include "trackerSimulator.h"
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>

CMN_IMPLEMENT_SERVICES(trackerSimulator);


trackerSimulator::trackerSimulator(const std::string & taskName, double period):
    mtsTaskPeriodic(taskName, period, false, 500),
    ExitFlag(false)
{
    mtsInterfaceRequired * requiresPositionCartesian = AddInterfaceRequired("RequiresPositionCartesian");
    if (requiresPositionCartesian) {
        requiresPositionCartesian->AddFunction("SetPositionCartesian", SetPositionCartesian);
        requiresPositionCartesian->AddFunction("GetPositionCartesian", GetPositionCartesian);
    }
}


void trackerSimulator::Configure(const std::string & CMN_UNUSED(filename))
{
    Fl::lock();
    Fl::awake();
    UI.DisplayWindow->show();
    Fl::unlock();
}


void trackerSimulator::Run(void)
{
    ProcessQueuedCommands();

    Fl::lock();
    Fl::awake();

    if (Fl::thread_message() != 0) {
        CMN_LOG_CLASS_RUN_ERROR << "Run: GUI error " << Fl::thread_message() << std::endl;
        Fl::unlock();
        return;
    }

    // get frame to be sent from the UI
    for (unsigned int i = 0; i < 3; i++) {
        FrameSend.Position().Rotation().Element(i,0) = UI.FrameSend[i]->value();
        FrameSend.Position().Rotation().Element(i,1) = UI.FrameSend[i+3]->value();
        FrameSend.Position().Rotation().Element(i,2) = UI.FrameSend[i+6]->value();
        FrameSend.Position().Translation().Element(i) = UI.FrameSend[i+9]->value();
    }
    SetPositionCartesian(FrameSend);

    // set received frame on the UI
    GetPositionCartesian(FrameRecv);
    for (unsigned int i = 0; i < 3; i++) {
        UI.FrameRecv[i]->value(FrameRecv.Position().Rotation().Element(i,0));
        UI.FrameRecv[i+3]->value(FrameRecv.Position().Rotation().Element(i,1));
        UI.FrameRecv[i+6]->value(FrameRecv.Position().Rotation().Element(i,2));
        UI.FrameRecv[i+9]->value(FrameRecv.Position().Translation().Element(i));
    }

    if (UI.QuitClicked) {
        ExitFlag = true;
    }
    Fl::unlock();
}


bool trackerSimulator::GetExitFlag(void)
{
    Fl::lock();
    Fl::awake();
    Fl::check();
    return ExitFlag;
    Fl::unlock();
}
