/*=========================================================================

  Program:   OpenIGTLink -- Example for Data Receiving Client Program
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <iostream>
#include <iomanip>
#include <math.h>
#include <cstdlib>
#include <cstring>
#include <set>

#include "igtlOSUtil.h"
#include "igtlMessageHeader.h"
#include "igtlTransformMessage.h"
#include "igtlPositionMessage.h"
#include "igtlImageMessage.h"
#include "igtlClientSocket.h"
#include "igtlStatusMessage.h"
#include "igtlSensorMessage.h"
#include "igtlNDArrayMessage.h"
#include "igtlPointMessage.h"
#include "igtlTrajectoryMessage.h"
#include "igtlStringMessage.h"
#include "igtlTrackingDataMessage.h"
#include "igtlQuaternionTrackingDataMessage.h"
#include "igtlCapabilityMessage.h"

int ReceiveTransform(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceivePosition(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveImage(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveStatus(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveSensor(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveNDArray(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceivePoint(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveTrajectory(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveString(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveTrackingData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveQuaternionTrackingData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
int ReceiveCapability(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader * header);

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments

  if (!((argc == 3) || (argc == 4))) {
    // If not correct, print usage
    std::cerr << "    <hostname> : IP or host name" << std::endl
	      << "    <port>     : Port # (18944 in Slicer default)" << std::endl
	      << "    <device>   : Device Name (optional)" << std::endl;
    exit(0);
  }

  char*  hostname = argv[1];
  int    port     = atoi(argv[2]);
  bool filterDevice = false;
  std::string device;
  if (argc == 4) {
    filterDevice = true;
    device = argv[3];
    std::cout << "Showing only messages with device name: " << device << std::endl;
  }

  std::set<std::string> devicesSkipped;
  
  //------------------------------------------------------------
  // Establish Connection
  igtl::ClientSocket::Pointer socket;
  socket = igtl::ClientSocket::New();
  int r = socket->ConnectToServer(hostname, port);

  if (r != 0)
    {
      std::cerr << "Cannot connect to the server." << std::endl;
      exit(0);
    }

  //------------------------------------------------------------
  // Create a message buffer to receive header
  igtl::MessageHeader::Pointer headerMsg;
  headerMsg = igtl::MessageHeader::New();

  //------------------------------------------------------------
  // Allocate a time stamp
  igtl::TimeStamp::Pointer ts;
  ts = igtl::TimeStamp::New();

  //------------------------------------------------------------
  // loop
  for (int i = 0; i < 100; i++) {

    // Initialize receive buffer
    headerMsg->InitPack();

    // Receive generic header from the socket
    int r = socket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
    if (r == 0)
      {
        socket->CloseSocket();
        exit(0);
      }
    if (r != headerMsg->GetPackSize())
      {
        continue;
      }

    // Deserialize the header
    headerMsg->Unpack();

    // Filter if need
    std::string messageDevice = headerMsg->GetDeviceName();
    if (filterDevice && (messageDevice != device)) {
      devicesSkipped.insert(messageDevice);
      socket->Skip(headerMsg->GetBodySizeToRead(), 0);
      continue;
    }

    // Get time stamp
    igtlUint32 sec;
    igtlUint32 nanosec;

    headerMsg->GetTimeStamp(ts);
    ts->GetTimeStamp(&sec, &nanosec);

    std::cerr << "Device name: " << messageDevice << std::endl;
    std::cout << "Time stamp: "
	      << sec << "." << std::setw(9) << std::setfill('0')
	      << nanosec << std::endl;

    // Check data type and receive data body
    if (strcmp(headerMsg->GetDeviceType(), "TRANSFORM") == 0)
      {
        ReceiveTransform(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "POSITION") == 0)
      {
	ReceivePosition(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "IMAGE") == 0)
      {
        ReceiveImage(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "STATUS") == 0)
      {
        ReceiveStatus(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "SENSOR") == 0)
      {
	ReceiveSensor(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "NDARRAY") == 0)
      {
	ReceiveNDArray(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "POINT") == 0)
      {
        ReceivePoint(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "TRAJ") == 0)
      {
        ReceiveTrajectory(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "STRING") == 0)
      {
        ReceiveString(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "TDATA") == 0)
      {
        ReceiveTrackingData(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "QTDATA") == 0)
      {
        ReceiveQuaternionTrackingData(socket, headerMsg);
      }
    else if (strcmp(headerMsg->GetDeviceType(), "CAPABILITY") == 0)
      {
        ReceiveCapability(socket, headerMsg);;
      }
    else
      {
        std::cout << "Receiving: " << headerMsg->GetDeviceType() << std::endl;
        socket->Skip(headerMsg->GetBodySizeToRead(), 0);
      }
  }


  //------------------------------------------------------------
  // Close connection
  socket->CloseSocket();

  std::cerr << std::endl << "Received and skipped messages from devices: " << std::endl;
  for (auto & dev : devicesSkipped) {
    std::cerr << dev << std::endl;
  } 
}


int ReceiveTransform(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
  std::cout << "Receiving TRANSFORM data type." << std::endl;

  // Create a message buffer to receive transform datag
  igtl::TransformMessage::Pointer transMsg;
  transMsg = igtl::TransformMessage::New();
  transMsg->SetMessageHeader(header);
  transMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(transMsg->GetPackBodyPointer(), transMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = transMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      // Retrive the transform data
      igtl::Matrix4x4 matrix;
      transMsg->GetMatrix(matrix);
      igtl::PrintMatrix(matrix);
      return 1;
    }

  return 0;
}

int ReceiveSensor(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
  std::cout << "Receiving SENSOR data type." << std::endl;

  // Create a message buffer to receive transform datag
  igtl::SensorMessage::Pointer sensMsg;
  sensMsg = igtl::SensorMessage::New();
  sensMsg->SetMessageHeader(header);
  sensMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(sensMsg->GetPackBodyPointer(), sensMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = sensMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      for (size_t i = 0; i < sensMsg->GetLength(); ++i) {
	std::cout << "v[" << i << "]: " << sensMsg->GetValue(i) << " ";
      }
      std::cout << std::endl;
      return 1;
    }

  return 0;
}

int ReceiveNDArray(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
  std::cout << "Receiving NDARRAY data type." << std::endl;

  // Create a message buffer to receive transform datag
  igtl::NDArrayMessage::Pointer arrayMsg;
  arrayMsg = igtl::NDArrayMessage::New();
  arrayMsg->SetMessageHeader(header);
  arrayMsg->AllocatePack();
  
  // Receive array data from the socket
  socket->Receive(arrayMsg->GetPackBodyPointer(), arrayMsg->GetPackBodySize());
  
  // Deserialize the array data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = arrayMsg->Unpack(1);
  
  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      // get dimension
      std::vector<igtlUint16> sizes = arrayMsg->GetArray()->GetSize();
      std::cout << "Dimension: " << sizes.size() << " size:";
      for (auto & s : sizes) {
	std::cout << " " << s;
      }
      std::cout << std::endl;
      // get type and display as long as dimension is less than 2
      if (arrayMsg->GetType() == igtl::NDArrayMessage::TYPE_FLOAT64) {
	igtlFloat64 * rawData = reinterpret_cast<igtlFloat64 *>(arrayMsg->GetArray()->GetRawArray());
	if (sizes.size() == 1) {
	  for (size_t i = 0; i < sizes.at(0); ++i) {
	    std::cout << *rawData << " ";
	    rawData++;
	  }
	  std::cout << std::endl;
	} else if (sizes.size() == 2) {
	  for (size_t i = 0; i < sizes.at(0); ++i) {
	    for (size_t j = 0; j < sizes.at(1); ++j) {
	      std::cout << *rawData << " ";
	      rawData++;
	    }
	    std::cout << std::endl;
	  }
	}
      }
      return 1;
    }
  
  return 0;
}

int ReceivePosition(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
  std::cout << "Receiving POSITION data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::PositionMessage::Pointer positionMsg;
  positionMsg = igtl::PositionMessage::New();
  positionMsg->SetMessageHeader(header);
  positionMsg->AllocatePack();

  // Receive position position data from the socket
  socket->Receive(positionMsg->GetPackBodyPointer(), positionMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = positionMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      // Retrive the transform data
      float position[3];
      float quaternion[4];

      positionMsg->GetPosition(position);
      positionMsg->GetQuaternion(quaternion);

      std::cout << "position   = (" << position[0] << ", " << position[1] << ", " << position[2] << ")" << std::endl;
      std::cout << "quaternion = (" << quaternion[0] << ", " << quaternion[1] << ", "
		<< quaternion[2] << ", " << quaternion[3] << ")" << std::endl << std::endl;

      return 1;
    }

  return 0;
}

int ReceiveImage(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
  std::cout << "Receiving IMAGE data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::ImageMessage::Pointer imgMsg;
  imgMsg = igtl::ImageMessage::New();
  imgMsg->SetMessageHeader(header);
  imgMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(imgMsg->GetPackBodyPointer(), imgMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = imgMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      // Retrive the image data
      int   size[3];          // image dimension
      float spacing[3];       // spacing (mm/pixel)
      int   svsize[3];        // sub-volume size
      int   svoffset[3];      // sub-volume offset
      int   scalarType;       // scalar type
      int   endian;           // endian

      scalarType = imgMsg->GetScalarType();
      endian = imgMsg->GetEndian();
      imgMsg->GetDimensions(size);
      imgMsg->GetSpacing(spacing);
      imgMsg->GetSubVolume(svsize, svoffset);


      std::cout << "Device Name           : " << imgMsg->GetDeviceName() << std::endl;
      std::cout << "Scalar Type           : " << scalarType << std::endl;
      std::cout << "Endian                : " << endian << std::endl;
      std::cout << "Dimensions            : ("
		<< size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
      std::cout << "Spacing               : ("
		<< spacing[0] << ", " << spacing[1] << ", " << spacing[2] << ")" << std::endl;
      std::cout << "Sub-Volume dimensions : ("
		<< svsize[0] << ", " << svsize[1] << ", " << svsize[2] << ")" << std::endl;
      std::cout << "Sub-Volume offset     : ("
		<< svoffset[0] << ", " << svoffset[1] << ", " << svoffset[2] << ")" << std::endl << std::endl;
      return 1;
    }

  return 0;

}


int ReceiveStatus(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{

  std::cout << "Receiving STATUS data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::StatusMessage::Pointer statusMsg;
  statusMsg = igtl::StatusMessage::New();
  statusMsg->SetMessageHeader(header);
  statusMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(statusMsg->GetPackBodyPointer(), statusMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = statusMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      std::cout << "========== STATUS ==========" << std::endl;
      std::cout << " Code      : " << statusMsg->GetCode() << std::endl;
      std::cout << " SubCode   : " << statusMsg->GetSubCode() << std::endl;
      std::cout << " Error Name: " << statusMsg->GetErrorName() << std::endl;
      std::cout << " Status    : " << statusMsg->GetStatusString() << std::endl;
      std::cout << "============================" << std::endl << std::endl;
    }

  return 0;

}

int ReceivePoint(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{

  std::cout << "Receiving POINT data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::PointMessage::Pointer pointMsg;
  pointMsg = igtl::PointMessage::New();
  pointMsg->SetMessageHeader(header);
  pointMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(pointMsg->GetPackBodyPointer(), pointMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = pointMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      int nElements = pointMsg->GetNumberOfPointElement();
      for (int i = 0; i < nElements; i ++)
	{
	  igtl::PointElement::Pointer pointElement;
	  pointMsg->GetPointElement(i, pointElement);

	  igtlUint8 rgba[4];
	  pointElement->GetRGBA(rgba);

	  igtlFloat32 pos[3];
	  pointElement->GetPosition(pos);

	  std::cout << "========== Element #" << i << " ==========" << std::endl;
	  std::cout << " Name      : " << pointElement->GetName() << std::endl;
	  std::cout << " GroupName : " << pointElement->GetGroupName() << std::endl;
	  std::cout << " RGBA      : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
	  std::cout << " Position  : ( " << std::fixed << pos[0] << ", " << pos[1] << ", " << pos[2] << " )" << std::endl;
	  std::cout << " Radius    : " << std::fixed << pointElement->GetRadius() << std::endl;
	  std::cout << " Owner     : " << pointElement->GetOwner() << std::endl;
	  std::cout << "================================" << std::endl << std::endl;
	}
    }

  return 1;
}

int ReceiveTrajectory(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{

  std::cout << "Receiving TRAJECTORY data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::TrajectoryMessage::Pointer trajectoryMsg;
  trajectoryMsg = igtl::TrajectoryMessage::New();
  trajectoryMsg->SetMessageHeader(header);
  trajectoryMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(trajectoryMsg->GetPackBodyPointer(), trajectoryMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = trajectoryMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      int nElements = trajectoryMsg->GetNumberOfTrajectoryElement();
      for (int i = 0; i < nElements; i ++)
	{
	  igtl::TrajectoryElement::Pointer trajectoryElement;
	  trajectoryMsg->GetTrajectoryElement(i, trajectoryElement);

	  igtlUint8 rgba[4];
	  trajectoryElement->GetRGBA(rgba);

	  igtlFloat32 entry[3];
	  igtlFloat32 target[3];
	  trajectoryElement->GetEntryPosition(entry);
	  trajectoryElement->GetTargetPosition(target);

	  std::cout << "========== Element #" << i << " ==========" << std::endl;
	  std::cout << " Name      : " << trajectoryElement->GetName() << std::endl;
	  std::cout << " GroupName : " << trajectoryElement->GetGroupName() << std::endl;
	  std::cout << " RGBA      : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
	  std::cout << " Entry Pt  : ( " << std::fixed << entry[0] << ", " << entry[1] << ", " << entry[2] << " )" << std::endl;
	  std::cout << " Target Pt : ( " << std::fixed << target[0] << ", " << target[1] << ", " << target[2] << " )" << std::endl;
	  std::cout << " Radius    : " << std::fixed << trajectoryElement->GetRadius() << std::endl;
	  std::cout << " Owner     : " << trajectoryElement->GetOwner() << std::endl;
	  std::cout << "================================" << std::endl << std::endl;
	}
    }

  return 1;
}

int ReceiveString(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{

  std::cout << "Receiving STRING data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::StringMessage::Pointer stringMsg;
  stringMsg = igtl::StringMessage::New();
  stringMsg->SetMessageHeader(header);
  stringMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(stringMsg->GetPackBodyPointer(), stringMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = stringMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      std::cout << "Encoding: " << stringMsg->GetEncoding() << "; "
		<< "String: " << stringMsg->GetString() << std::endl << std::endl;
    }

  return 1;
}

int ReceiveTrackingData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
  std::cout << "Receiving TDATA data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::TrackingDataMessage::Pointer trackingData;
  trackingData = igtl::TrackingDataMessage::New();
  trackingData->SetMessageHeader(header);
  trackingData->AllocatePack();

  // Receive body from the socket
  socket->Receive(trackingData->GetPackBodyPointer(), trackingData->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = trackingData->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      int nElements = trackingData->GetNumberOfTrackingDataElements();
      for (int i = 0; i < nElements; i ++)
	{
	  igtl::TrackingDataElement::Pointer trackingElement;
	  trackingData->GetTrackingDataElement(i, trackingElement);

	  igtl::Matrix4x4 matrix;
	  trackingElement->GetMatrix(matrix);


	  std::cout << "========== Element #" << i << " ==========" << std::endl;
	  std::cout << " Name       : " << trackingElement->GetName() << std::endl;
	  std::cout << " Type       : " << (int) trackingElement->GetType() << std::endl;
	  std::cout << " Matrix : " << std::endl;
	  igtl::PrintMatrix(matrix);
	  std::cout << "================================" << std::endl;
	}
      return 1;
    }
  return 0;
}

int ReceiveQuaternionTrackingData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
  std::cout << "Receiving QTDATA data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::QuaternionTrackingDataMessage::Pointer quaternionTrackingData;
  quaternionTrackingData = igtl::QuaternionTrackingDataMessage::New();
  quaternionTrackingData->SetMessageHeader(header);
  quaternionTrackingData->AllocatePack();

  // Receive body from the socket
  socket->Receive(quaternionTrackingData->GetPackBodyPointer(), quaternionTrackingData->GetPackBodySize());

  // Deserialize position and quaternion (orientation) data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = quaternionTrackingData->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      int nElements = quaternionTrackingData->GetNumberOfQuaternionTrackingDataElements();
      for (int i = 0; i < nElements; i ++)
	{
	  igtl::QuaternionTrackingDataElement::Pointer quaternionTrackingElement;
	  quaternionTrackingData->GetQuaternionTrackingDataElement(i, quaternionTrackingElement);

	  float position[3];
	  float quaternion[4];
	  quaternionTrackingElement->GetPosition(position);
	  quaternionTrackingElement->GetQuaternion(quaternion);

	  std::cout << "========== Element #" << i << " ==========" << std::endl;
	  std::cout << " Name       : " << quaternionTrackingElement->GetName() << std::endl;
	  std::cout << " Type       : " << (int) quaternionTrackingElement->GetType() << std::endl;
	  std::cout << " Position   : "; igtl::PrintVector3(position);
	  std::cout << " Quaternion : "; igtl::PrintVector4(quaternion);
	  std::cout << "================================" << std::endl;
	}
      return 1;
    }
  return 0;
}

int ReceiveCapability(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader * header)
{

  std::cout << "Receiving CAPABILITY data type." << std::endl;

  // Create a message buffer to receive transform data
  igtl::CapabilityMessage::Pointer capabilMsg;
  capabilMsg = igtl::CapabilityMessage::New();
  capabilMsg->SetMessageHeader(header);
  capabilMsg->AllocatePack();

  // Receive transform data from the socket
  socket->Receive(capabilMsg->GetPackBodyPointer(), capabilMsg->GetPackBodySize());

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = capabilMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      int nTypes = capabilMsg->GetNumberOfTypes();
      for (int i = 0; i < nTypes; i ++)
	{
	  std::cout << "Typename #" << i << ": " << capabilMsg->GetType(i) << std::endl;
	}
    }

  return 1;

}
