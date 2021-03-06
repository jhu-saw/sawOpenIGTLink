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

#include <sawOpenIGTLink/mtsIGTLCRTKBridge.h>
#include <sawOpenIGTLink/mtsCISSTToIGTL.h>
#include <sawOpenIGTLink/mtsIGTLToCISST.h>

#include <cisstMultiTask/mtsManagerLocal.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>

CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsIGTLCRTKBridge, mtsIGTLBridge, mtsTaskPeriodicConstructorArg);

void mtsIGTLCRTKBridge::ConfigureJSON(const Json::Value & jsonConfig)
{
    // to set port
    mtsIGTLBridge::ConfigureJSON(jsonConfig);

    Json::Value jsonValue;
    const Json::Value interfaces = jsonConfig["interfaces"];
    if (interfaces.empty()) {
        CMN_LOG_CLASS_INIT_WARNING << "ConfigureJSON: no \"interfaces\" defined, this will not add any IGTL CRTK bridge"
                                   <<  std::endl;
    }
    for (unsigned int index = 0; index < interfaces.size(); ++index) {
        jsonValue = interfaces[index]["component"];
        if (jsonValue.empty()) {
            CMN_LOG_CLASS_INIT_ERROR << "ConfigureJSON: all \"interfaces\" must define \"component\"" << std::endl;
            return;
        }
        // future implementation could also support interface-required
        std::string componentName = jsonValue.asString();
        jsonValue = interfaces[index]["interface-provided"];
        if (jsonValue.empty()) {
            CMN_LOG_CLASS_INIT_ERROR << "ConfigureJSON: all \"interfaces\" must define \"interface-provided\"" << std::endl;
            return;
        }
        std::string interfaceName = jsonValue.asString();

        // namespace is optional, use interface name by default
        jsonValue = interfaces[index]["namespace"];
        std::string name;
        if (!jsonValue.empty()) {
            name = jsonValue.asString();
        } else {
            name = interfaceName;
        }

        // if set, only bridge CRTK commands that matches user provided list
        const Json::Value bridgeOnly = interfaces[index]["bridge-only"];
        for (unsigned int bo = 0; bo < bridgeOnly.size(); ++bo) {
            mBridgeOnly.insert(bridgeOnly[bo].asString());
        }

        // and now add the bridge
        BridgeInterfaceProvided(componentName, interfaceName, name);
    }

    // skip connecting interfaces in case users want to add more
    // commands/functions/events to bridge before connecting
    jsonValue = jsonConfig["skip-connect"];
    if (!jsonValue.empty() && jsonValue.asBool()) {
        CMN_LOG_CLASS_INIT_WARNING << "ConfigureJSON: \"skip-connect\" set to \"true\", user is responsible for connecting components" <<  std::endl;
    } else {
        mtsComponentManager::GetInstance()->AddComponent(this);
        Connect();
    }
}

void mtsIGTLCRTKBridge::BridgeInterfaceProvided(const std::string & componentName,
                                                const std::string & interfaceName,
                                                const std::string & nameSpace)
{
    // first make sure we can find the component to bridge
    mtsManagerLocal * componentManager = mtsComponentManager::GetInstance();
    mtsComponent * component = componentManager->GetComponent(componentName);
    if (!component) {
        CMN_LOG_CLASS_INIT_ERROR << "bridge_interface_provided: unable to find component \""
                                 << componentName << "\"" << std::endl;
        return;
    }
    // then try to find the interface
    mtsInterfaceProvided * interfaceProvided = component->GetInterfaceProvided(interfaceName);
    if (!interfaceProvided) {
        CMN_LOG_CLASS_INIT_ERROR << "bridgeinterfaceProvided: unable to find provided interface \""
                                 << interfaceName << "\" on component \""
                                 << componentName << "\"" << std::endl;
        return;
    }

    // required interface for bridges shared across components being
    // bridged (e.g. subscribers and events)
    const std::string requiredInterfaceName = componentName + "::" + interfaceName;
    std::string crtkCommand;
    bool connectionNeeded = false;

    // write commands
    for (auto & command : interfaceProvided->GetNamesOfCommandsWrite()) {
        if (ShouldBeBridged(command)) {
            // get the CRTK command so we know which template type to use
            GetCRTKCommand(command, crtkCommand);

        /* examples from ROS CRTK, to be ported to IGTL
          if (_crtk_command == "servo_jf") {
          m_subscribers_bridge->AddSubscriberToCommandWrite<prmForceTorqueJointSet, sensor_msgs::JointState>
        */
            if ((crtkCommand == "servo_jp")
                || (crtkCommand == "servo_jr")
                || (crtkCommand == "move_jp")
                || (crtkCommand == "move_jr")) {
                connectionNeeded = true;
                AddReceiverToCommandWrite<igtl::SensorMessage, prmPositionJointSet>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            } else if ((crtkCommand == "servo_cp")
                       || (crtkCommand == "move_cp")) {
                connectionNeeded = true;
                AddReceiverToCommandWrite<igtl::TransformMessage, prmPositionCartesianSet>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            } else if (crtkCommand == "servo_cf") {
                connectionNeeded = true;
                AddReceiverToCommandWrite<igtl::SensorMessage, prmForceCartesianSet>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            } else if (crtkCommand == "state_command") {
                connectionNeeded = true;
                AddReceiverToCommandWrite<igtl::StringMessage, std::string>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            }
        }
    }

    // read commands
    for (auto & command : interfaceProvided->GetNamesOfCommandsRead()) {
        if (ShouldBeBridged(command)) {
            // get the CRTK command so we know which template type to use
            GetCRTKCommand(command, crtkCommand);

            if ((crtkCommand == "measured_js")
                || (crtkCommand == "setpoint_js")) {
                connectionNeeded = true;
                AddSenderFromCommandRead<prmStateJoint, igtl::NDArrayMessage>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            } else if ((crtkCommand == "measured_cp")
                       || (crtkCommand == "setpoint_cp")) {
                connectionNeeded = true;
                AddSenderFromCommandRead<prmPositionCartesianGet, igtl::TransformMessage>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            } else if (crtkCommand == "measured_cv") {
                connectionNeeded = true;
                AddSenderFromCommandRead<prmVelocityCartesianGet, igtl::SensorMessage>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            } else if (crtkCommand == "measured_cf") {
                connectionNeeded = true;
                AddSenderFromCommandRead<prmForceCartesianGet, igtl::SensorMessage>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            } /* else if (_crtk_command == "jacobian") {
                 _pub_bridge_used = true;
                 _pub_bridge->AddPublisherFromCommandRead<vctDoubleMat, std_msgs::Float64MultiArray>
                 (interfaceName, *_command, _ros_topic);
                 } else if (_crtk_command == "operating_state") {
                 m_subscribers_bridge->AddServiceFromCommandRead<prmOperatingState, crtk_msgs::trigger_operating_state>
                 (_requiredinterfaceName, *_command, _ros_topic);
                 }
              */
        }
    }

    // write events
    for (auto & command : interfaceProvided->GetNamesOfEventsWrite()) {
        if (ShouldBeBridged(command)) {
            // get the CRTK command so we know which template type to use
            GetCRTKCommand(command, crtkCommand);
            /*
              if (_crtk_command == "operating_state") {
              m_events_bridge->AddPublisherFromEventWrite<prmOperatingState, crtk_msgs::operating_state>
              (_requiredinterfaceName, *_event, _ros_topic);
              } else */
            if ((crtkCommand == "error")
                || (crtkCommand == "warning")
                || (crtkCommand == "status")) {
                connectionNeeded = true;
                AddSenderFromEventWrite<mtsMessage, igtl::StringMessage>
                    (requiredInterfaceName, command, nameSpace + '/' + command);
            }
        }
    }

    if (connectionNeeded) {
        mConnections.Add(this->GetName(), requiredInterfaceName,
                         componentName, interfaceName);
    }

    // buttons are a pain, they tend to have one interface per button
    // with a single write event called "Button".  By convention, the
    // button interface is using the name of the device it is attached
    // too as prefix (e.g. ForceDimension Falcon00 has button
    // Falcon00-Left, SensablePhantom left has button leftButton1).
    // So we look at all interfaces on the component that match the
    // interface_name and have an event write called "Button".
    const size_t prefixSize = interfaceName.size();
    for (auto & buttonInterface : component->GetNamesOfInterfacesProvided()) {
        // can only be a prefix if shorter
        if (buttonInterface.size() > prefixSize) {
            if (std::equal(interfaceName.begin(),
                           interfaceName.end(),
                           buttonInterface.begin())) {
                // this interface qualifies based on the name, does it
                // contain an event write "Button"?
                auto interfaceCandidate = component->GetInterfaceProvided(buttonInterface);
                auto events = interfaceCandidate->GetNamesOfEventsWrite();
                if (std::find(events.begin(), events.end(), "Button") != events.end()) {
                    // remove heading - or _
                    size_t offset = 0;
                    const char firstChar = buttonInterface.at(prefixSize);
                    if ((firstChar == '-') || (firstChar == '_')) {
                        offset = 1;
                    }
                    std::string buttonName = buttonInterface.substr(prefixSize + offset);
                    // put all to lower case to be more ROS alike
                    std::transform(buttonName.begin(), buttonName.end(), buttonName.begin(), tolower);
                    // add and connect interface to event bridge
                    AddSenderFromEventWrite<prmEventButton, igtl::SensorMessage>
                        (componentName + "::" + buttonInterface, "Button", nameSpace + "/" + buttonName);
                    mConnections.Add(this->GetName(), componentName + "::" + buttonInterface,
                                     componentName, buttonInterface);
                }
            }
        }
    }
}

void mtsIGTLCRTKBridge::GetCRTKCommand(const std::string & fullCommand,
                                       std::string & crtkCommand)
{
    size_t pos = fullCommand.find_last_of("/");
    if (pos == std::string::npos) {
        crtkCommand = fullCommand;
    } else {
        crtkCommand = fullCommand.substr(pos + 1);
    }
}

bool mtsIGTLCRTKBridge::ShouldBeBridged(const std::string & command)
{
    if (mBridgeOnly.empty()) {
        return true;
    }
    if (mBridgeOnly.find(command) != mBridgeOnly.end()) {
        return true;
    }
    return false;
}
