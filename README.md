# sawOpenIGTLink
This library contains two SAW components that make it easier to send and receive messages over OpenIGTLink (aka IGTL, see http://openigtlink.org/).  The two components are a base IGTL bridge class (`mtsIGTLBridge`) as well as a CRTK dedicated bridge (`mtsIGTLCRTKBridge`).  For more information regarding CRTK, see https://github.com/collaborative-robotics/documentation/wiki/Robot-API.  The base bridge has to be populated manually, i.e. the user has to implement their own C++ code with explicit calls to map `cisstMultiTask` commands (see https://github.com/jhu-cisst/cisst/wiki/cisstMultiTask-concepts) to IGTL messages one by one.  Meanwhile the CRTK bridge will search a given interface for all CRTK compatible commands and events and will automatically bridge them.

Note: the logic of sawOpenIGTLink is similar to `mtsROSBridge` and `mts_ros_crtk_bridge` (see https://github.com/jhu-cisst/cisst-ros),

# Links
 * License: http://github.com/jhu-cisst/cisst/blob/master/license.txt
 * JHU-LCSR software: http://jhu-lcsr.github.io/software/

# Dependencies
 * cisst libraries: https://github.com/jhu-cisst/cisst
 * OpenIGTLink, version 3.0 or higher: https://github.com/openigtlink/OpenIGTLink
 
# Building the code
For cisst/SAW and sawOpenIGTLink, see https://github.com/jhu-cisst/cisst/wiki/Compiling-cisst-and-SAW-with-CMake.  If you're using Linux, we strongly recommend to also use ROS.  You might not use the ROS middleware but the build tools (`catkin build`) greatly ease the compilation process.

For OpenIGTLink, we recommand to compile from source: https://github.com/openigtlink/OpenIGTLink/blob/master/BUILD.md

Note that Ubuntu has some binary packages for OpenIGTLink but these are quite old (version 1.1) so make sure you compile sawOpenIGTLink against the IGTL version you compiled from source.  The simplest way to make sure CMake for sawOpenIGTLink doesn't find the Ubuntu provided version is to not install said version :-) 

# Examples

## Base class C++
The best example of use of the base IGTL bridge is the CRTK bridge itself.  Look at the method `mtsIGTLCRTKBridge::BridgeInterfaceProvided` implementation in the `components/code` folder.

## CRTK bridge
This bridge can be use on any cisst/SAW components that has a CRTK compatible interface (see cisst/SAW components in https://github.com/jhu-saw/ and https://github.com/jhu-dvrk/sawIntuitiveResearchKit/wiki).

### Static application
The CRTK bridge can be used in a single executable with a bit of C++ coding.  For example, https://github.com/jhu-saw/sawSensablePhantom has an example in `igtl`.

### Dynamic loading
It can also be used without any coding using a few configurations files.  This assumes that the main executable has an option to load configuration files for the `cisstMultiTask` component manager.  The main two files needed are -1- a configuration for the component manager itself and -2- a configuration file for the CRTK bridge.   Examples can be found in the `examples/sensable`.  The CRTK bridge configuration file is used to define the IGTL port, the rate to send data over IGTL, the cisst/SAW component and interface to bridge, the IGTL device name given for the bridged interface and optionally an explicit list of CRTK commands and events to bridge.  By default, the CRTK bridge will bridge all CRTK compatible commands and events. 

# Testing

Once you have your cisst/SAW application configured as an IGTL server, you can test what the application is sending and receiving using the examples found in the OpenIGTLink repository.  In the OpenIGTLink build directory, locate all the executables and more specifically ` ReceiveClient`.  Assuming that you're using both program on the same PC and you're using the default IGTL/Slicer port, try: `ReceiveClient localhost 18944`.  You should see the different messages the device is sending.
 
 
