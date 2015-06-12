/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*

  Author(s):  Ali Uneri
  Created on: 2009-08-10

  (C) Copyright 2009-2012 Johns Hopkins University (JHU), All Rights Reserved.

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

  The component may be configured as a server by providing a target hostname/IP
  and a port number (delimited by ":"), or as a client by simply omitting the host.
    \code
    mtsOpenIGTLinkBridge server("trackerServer", 50.0 * cmn_ms);
    server.Configure("18944");
    mtsOpenIGTLinkBridge client("trackerClient", 50.0 * cmn_ms);
    client.Configure("localhost:18944");
    \endcode

  igtlutil library is required to compile and run the examples.

  SVN repository of the source code:
  http://svn.na-mic.org/NAMICSandBox/trunk/OpenIGTLink

  Build instructions for various platforms:
  http://www.na-mic.org/Wiki/index.php/OpenIGTLink/Library/Build

  \bug Server is set up to handle a single client (osaSocketServer has support for multiple clients).

  \todo Handle multiple connections by storing the returned socket pointer in an array.
  \todo Check for cyclic redundancy (CRC).
  \todo Handle message types besides TRANSFORM.
*/

#ifndef _mtsOpenIGTLinkBridge_h
#define _mtsOpenIGTLinkBridge_h

#include <igtl/igtlOSUtil.h>
#include <igtl/igtlTransformMessage.h>
#include <igtl/igtlServerSocket.h>

#include <cisstMultiTask/mtsTaskPeriodic.h>
#include <cisstParameterTypes/prmPositionCartesianGet.h>
#include <sawOpenIGTLink/sawOpenIGTLinkExport.h>




class CISST_EXPORT mtsOpenIGTLinkBridge: public mtsTaskPeriodic
{
    CMN_DECLARE_SERVICES(CMN_DYNAMIC_CREATION_ONEARG, CMN_LOG_ALLOW_DEFAULT);

 public:
    /*! Constructors */
    mtsOpenIGTLinkBridge(const std::string & taskName, const double period) :
        mtsTaskPeriodic(taskName, period, false, 500) {}
    mtsOpenIGTLinkBridge(const mtsTaskPeriodicConstructorArg & arg) :
        mtsTaskPeriodic(arg) {}

    /*! Destructor */
    ~mtsOpenIGTLinkBridge(void) {}

    void Configure(const std::string & hostAndPort);
    void Startup(void);
    void Run(void);
    void Cleanup(void);

 protected:

    // takes in cisst frame and outputs igtl frame
    void prmPositionCartesianToOIGTL(const prmPositionCartesianGet& frameCISST,
                                     igtl::Matrix4x4& frameIGTL);
    
    enum ConnectionTypes { SERVER, CLIENT };
    int ConnectionType;
    bool IsConnected;

    igtl::ServerSocket::Pointer serverSocket;
    igtl::ClientSocket::Pointer clientSocket;

    igtl::Socket::Pointer socket;

    std::string Host;
    int ServerPort;
    int ClientPort;
    
    mtsFunctionRead GetPositionCartesian;
    prmPositionCartesianGet PositionCartesian;

};

CMN_DECLARE_SERVICES_INSTANTIATION(mtsOpenIGTLinkBridge);

#endif  // _mtsOpenIGTLinkBridge_h
