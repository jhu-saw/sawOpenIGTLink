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
    mtsOpenIGTLink server("trackerServer", 50.0 * cmn_ms);
    server.Configure("18944");
    mtsOpenIGTLink client("trackerClient", 50.0 * cmn_ms);
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

#ifndef _mtsOpenIGTLink_h
#define _mtsOpenIGTLink_h

#include <cisstOSAbstraction/osaSocket.h>
#include <cisstOSAbstraction/osaSocketServer.h>
#include <cisstMultiTask/mtsTaskPeriodic.h>
#include <cisstParameterTypes/prmPositionCartesianGet.h>
#include <sawOpenIGTLink/sawOpenIGTLinkExport.h>  // always include last


class sawOpenIGTLinkData;  // class containing igtl type data


class CISST_EXPORT mtsOpenIGTLink: public mtsTaskPeriodic
{
    CMN_DECLARE_SERVICES(CMN_DYNAMIC_CREATION_ONEARG, CMN_LOG_ALLOW_DEFAULT);

 public:
    /*! Constructors */
    mtsOpenIGTLink(const std::string & taskName, const double period) :
        mtsTaskPeriodic(taskName, period, false, 500) {}
    mtsOpenIGTLink(const mtsTaskPeriodicConstructorArg & arg) :
        mtsTaskPeriodic(arg) {}

    /*! Destructor */
    ~mtsOpenIGTLink(void) {}

    void Configure(const std::string & hostAndPort);
    void Startup(void);
    void Run(void);
    void Cleanup(void);

 protected:
    /*! Common initializing operations for both server and client */
    void Initialize(void);

    /*! Sends the frame through the existing socket interface
      \param frameCISST Frame to be sent
      \return true on success */
    bool SendFrame(const prmPositionCartesianGet & frameCISST);

    /*! Receives the header of the incoming message
      \param messageType Type of the message (i.e. TRANSORM)
      \return false if there's no message or recv() fails */
    bool ReceiveHeader(std::string & messageType);

    /*! Receives an incoming frame
      \param frameCISST Frame to be received
      \return true on success */
    bool ReceiveFrame(prmPositionCartesianGet & frameCISST);

    /*! Skips the received message */
    void SkipMessage(void);

    enum ConnectionTypes { SERVER, CLIENT };
    int ConnectionType;

    std::string Host;
    unsigned int Port;
    bool IsConnected;

    osaSocketServer * SocketServer;
    osaSocket * Socket;
    std::vector<osaSocket *> Sockets;

    std::string MessageType;
    sawOpenIGTLinkData * IGTLData;
    prmPositionCartesianGet FrameSend;
    prmPositionCartesianGet FrameRecv;
};

CMN_DECLARE_SERVICES_INSTANTIATION(mtsOpenIGTLink);

#endif  // _mtsOpenIGTLink_h
