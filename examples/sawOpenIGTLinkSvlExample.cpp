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

#include "svlOpenIGTLink.h"

#if CISST_HAS_QT4
    #include <cisstStereoVision/svlQtObjectFactory.h>
    #include <cisstStereoVision/svlQtWidgetFileOpen.h>
    #include <cisstStereoVision/svlQtWidgetFramerate.h>
    #include <cisstStereoVision/svlQtWidgetVideoEncoder.h>
    #include <cisstStereoVision/svlQtWidgetVidCapSrcImageProperties.h>
    #if CISST_HAS_OPENGL
        #include <cisstStereoVision/svlFilterImageWindowQt.h>
    #endif
    // Qt dialogs are disabled by default
    #define _USE_QT_    0
#endif


using namespace std;


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


#if _USE_QT_ && CISST_HAS_OPENGL
    svlFilterImageWindowQt window;
#else // _USE_QT_ && CISST_HAS_OPENGL
    svlFilterImageWindow window;
#endif // _USE_QT_ && CISST_HAS_OPENGL
    svlFilterImageOverlay overlay;
    CViewerEventHandler window_eh;

    // setup source
    // Delete "device.dat" to reinitialize input device
    if (source.LoadSettings("device.dat") != SVL_OK) {
        cout << endl;
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

    int fullscreen = 1;

    // Set to FALSE if fullscreen is not desired 
    window.SetFullScreen(true);
/*
    if (fullscreen >= 0) {
		window.SetFullScreen(true);
		if (fullscreen == 0) {
			window.SetPosition(offsetx, 0, SVL_LEFT);
			window.SetPosition(offsetx, height / 2, SVL_RIGHT);
		}
		else if (fullscreen == 1) {
			window.SetPosition(offsetx, 0, SVL_LEFT);
			window.SetPosition(offsetx + width / 2, 0, SVL_RIGHT);
		}
		else if (fullscreen == 2) {
			window.SetPosition(offsetx, 0);
		}
	}
*/


    // Add framerate overlay
    svlOverlayFramerate fps_overlay(SVL_LEFT,
                                    true,
                                    &window,
                                    svlRect(4, 24, 47, 40),
                                    14.0,
                                    svlRGB(255, 255, 255),
                                    svlRGB(128, 0, 0));
    overlay.AddOverlay(fps_overlay);

    cerr << "Assembling stream..." << endl;

    // chain filters to pipeline
    svlFilterOutput *output;

    // Add source
    svlFilterSplitter Splitter;
    stream.SetSourceFilter(&source);
    //source.GetOutput()->Connect(OpenIGTlinkFilter.GetInput());
    //output = OpenIGTlinkFilter.GetOutput();

    Splitter.AddOutput("async_out");
    source.GetOutput()->Connect(Splitter.GetInput());



    svlFilterImageChannelSwapper rgb_swapper;
    Splitter.GetOutput()->Connect(rgb_swapper.GetInput());

    //output = Splitter.GetOutput();


    svlOpenIGTLinkBridge OpenIGTlinkFilter;
    int inputPort = atoi(port);
    std::cerr<<inputPort<<std::endl;
    OpenIGTlinkFilter.SetPortNumber(inputPort);
    OpenIGTlinkFilter.SetDeviceName("OpenIGTLink Conversion Filter");
    rgb_swapper.GetOutput()->Connect(OpenIGTlinkFilter.GetInput());
    

    output = Splitter.GetOutput("async_out");//OpenIGTlinkFilter.GetOutput();

    // Add window
    // Add shifter if fullscreen
    /*
	if (fullscreen >= 0) {
        output->Connect(shifter.GetInput());
            output = shifter.GetOutput();
	}
    */

    // Add resizer if required
    if (width > 0 && height > 0) {
        output->Connect(resizer.GetInput());
            output = resizer.GetOutput();
    }

    output->Connect(window.GetInput());
        output = window.GetOutput();

    cerr << "Starting stream... ";

    // initialize and start stream
    if (stream.Play() != SVL_OK) return 0;

    cerr << "Done" << endl;

    // wait for keyboard input in command window
    int ch;

    do {
        ch = cmnGetChar();

        switch (ch)
        {

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

    cerr << "Stream released" << endl;

    return 0;
}


//////////////////////////////////
//             main             //
//////////////////////////////////
int my_main(int argc, char** argv)
{
    bool interpolation = false;
    bool save = false;
    int  width = 1680;
    int  height = 1050;

    //////////////////////////////
    // starting viewer
    if(argc > 1){
        CameraViewer(interpolation, save, width, height, argv[1]);
    }
    else{
        std::cerr<<"Enter desired port number, eg ./sawOpenIGTLinkSvlExample 18944" << std::endl;
    }

    cerr << "Quit" << endl;
    return 1;
}

SETUP_QT_ENVIRONMENT(my_main)
