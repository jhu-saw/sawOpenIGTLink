/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Balazs Vagvolgyi
  Created on: 2008

  (C) Copyright 2006-2014 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#include <cisstCommon/cmnGetChar.h>
#include <cisstOSAbstraction/osaSleep.h>
#include <cisstMultiTask/mtsManagerLocal.h>
#include <cisstMultiTask/mtsComponentViewer.h>

#include <cisstStereoVision/svlInitializer.h>
#include <cisstStereoVision/svlFilterOutput.h>
#include <cisstStereoVision/svlStreamManager.h>

#include <cisstStereoVision/svlWindowManagerBase.h>
#include <cisstStereoVision/svlFilterImageFileWriter.h>
#include <cisstStereoVision/svlFilterVideoFileWriter.h>
#include <cisstStereoVision/svlFilterImageExposureCorrection.h>
#include <cisstStereoVision/svlFilterOutput.h>
#include <cisstStereoVision/svlFilterSourceVideoCapture.h>
#include <cisstStereoVision/svlFilterVideoExposureManager.h>
#include <cisstStereoVision/svlFilterSplitter.h>
#include <cisstStereoVision/svlFilterImageResizer.h>
#include <cisstStereoVision/svlFilterImageWindow.h>
#include <cisstStereoVision/svlFilterImageOverlay.h>
#include <cisstStereoVision/svlFilterImageChannelSwapper.h>

#include <sawOpenIGTLink/svlOpenIGTLinkBridge.h>

/////////////////////////////#///////////
//     Window event handler class     //
////////////////////////////////////////

class CViewerEventHandler : public svlWindowEventHandlerBase
{
public:
    CViewerEventHandler() :
        svlWindowEventHandlerBase()
        ,ImageWriterFilter(0)
        ,RecorderFilter(0)
        ,Gamma(0)
        ,SplitterOutput(0)
        ,Recording(false)
    {
    }

    svlFilterImageFileWriter* ImageWriterFilter;
    svlFilterVideoFileWriter* RecorderFilter;
    svlFilterImageExposureCorrection* Gamma;
    svlFilterOutput* SplitterOutput;
    bool Recording;
};
#

////////////////////
//  CameraViewer  //
////////////////////

int CameraViewer(bool interpolation, bool save, int width, int height, char* port)
{

    svlInitialize();

    // instantiating SVL stream and filters
    svlStreamManager stream(8);
    svlFilterSourceVideoCapture source(2);
    svlFilterImageResizer resizer;
    svlFilterImageWindow window;
    svlFilterImageOverlay overlay;
    CViewerEventHandler window_eh;

    // setup source
    // Delete "device.dat" to reinitialize input device
    if (source.LoadSettings("device.dat") != SVL_OK) {
        std::cout << std::endl;
        source.DialogSetup(SVL_LEFT);
        source.DialogSetup(SVL_RIGHT);
    }

    // setup resizer
    if (width > 0 && height > 0) {
        resizer.SetInterpolation(false);
        resizer.SetOutputSize(width, height, SVL_LEFT);
        resizer.SetOutputSize(width, height, SVL_RIGHT);
    }
    window.SetEventHandler(&window_eh);
    window.SetTitle("Camera Viewer");

    // Set to FALSE if fullscreen is not desired 
    window.SetFullScreen(true);

    // Add framerate overlay
    svlOverlayFramerate fps_overlay(SVL_LEFT,
                                    true,
                                    &window,
                                    svlRect(4, 24, 47, 40),
                                    14.0,
                                    svlRGB(255, 255, 255),
                                    svlRGB(128, 0, 0));
    overlay.AddOverlay(fps_overlay);

    std::cout << "Assembling stream..." << std::endl;

    // chain filters to pipeline
    svlFilterOutput *output;

    // Add source
    svlFilterSplitter Splitter;
    stream.SetSourceFilter(&source);

    Splitter.AddOutput("async_out");
    source.GetOutput()->Connect(Splitter.GetInput());

    svlFilterImageChannelSwapper rgb_swapper;
    Splitter.GetOutput()->Connect(rgb_swapper.GetInput());

    svlOpenIGTLinkBridge OpenIGTlinkFilter;
    int inputPort = atoi(port);

    std::cout << "Using OpenIGTLink port: " << inputPort << std::endl;

    OpenIGTlinkFilter.SetPortNumber(inputPort);
    OpenIGTlinkFilter.SetDeviceName("OpenIGTLink Conversion Filter");
    rgb_swapper.GetOutput()->Connect(OpenIGTlinkFilter.GetInput());
    
    output = Splitter.GetOutput("async_out");//OpenIGTlinkFilter.GetOutput();

    // Add resizer if required
    if (width > 0 && height > 0) {
        output->Connect(resizer.GetInput());
        output = resizer.GetOutput();
    }

    output->Connect(window.GetInput());
    output = window.GetOutput();

    std::cout << "Starting stream... ";

    // initialize and start stream
    if (stream.Play() != SVL_OK) {
        std::cerr << "Failed to start stream" << std::endl;
        return 0;
    }

    std::cout << "Done" << std::endl;

    // wait for keyboard input in command window
    int ch;
    do {
        ch = cmnGetChar();
        switch (ch) {
        default:
            break;
        }
    } while (ch != 'q');
    
    // stop stream
    stream.Stop();

    // save settings
    source.SaveSettings("device.dat");

    // release stream
    stream.Release();
    stream.DisconnectAll();

    std::cout << "Stream released" << std::endl;

    return 0;
}


//////////////////////////////////
//             main             //
//////////////////////////////////
int main(int argc, char** argv)
{
    bool interpolation = false;
    bool save = false;
    int  width = 1680;
    int  height = 1050;

    //////////////////////////////
    // starting viewer
    if (argc > 1) {
        CameraViewer(interpolation, save, width, height, argv[1]);
    }
    else {
        std::cerr << "Enter desired port number, eg ./sawOpenIGTLinkSvlExample 18944" << std::endl;
    }
    std::cout << "Quit" << std::endl;
    return 1;
}
