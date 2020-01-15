This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control.

## Explore unlimited possibilites ...

This article contains some ideas for additional features that could be added to a PLCnext runtime application. Some of these can be implemented using patterns demonstrated in this series of articles. Others will be the subject of future articles in the PLCnext Community.

### 1. Use Profinet I/O

It is possible to communicate with Profinet I/O devices in a similar way to the Axioline I/O used in this application.

### 2. Configure and start the Axioline bus from the application

The "Axioline Master" RSC service can be used to configure any arrangement of Axioline I/O modules directly from a runtime application, without the use of PLCnext Engineer. This is demonstrated in the [BusConductor](https://github.com/PLCnext/BusConductor) application, and the technique used by that app could easily be applied here.

### 3. Control the PLC state

This series has shown how to react to changes in the PLC state using a callback from the PLCnext ANSI-C interface.

The ANSI-C interface also provides functions to start, stop and reset the PLC if necessary.

For non-real-time applications, the "PLC Manager" service provides similar control functions through an RSC service.

### 4. Send and receive notifications from other processes

The "Notification Manager" RSC service can be used to send notifications to subscribers, and to receive notifications from other processes.

For example, the runtime application could use the Notification Manager to receive notifications about changes of PLC state (running, stopped, etc).

### 5. Read and Write GDS variables

The "Data Access" RSC service can be used to read and write GDS variables that are defined by other PLCnext components. This provides a simple, reliable and consistent interprocess data transfer mechanism.

### 6. Connect to MQTT brokers

The "MQTT Client" RSC service is not included on the PLCnext Control by default, but it can be installed if required. The MQTT Client service can be used to publish and subscribe to topics on local or remote MQTT brokers.

### 7. Use other languages to develop PLCnext runtime applications

This examples in this series were written entirely in C++, however it is possible to write similar applications in most modern languages that have a compiler suitable for your selected PLCnext Control (e.g. ARMv7 hard float).

There are various ways to interface applications written in other languages with services on the PLCnext Control:
- The application can be built as a shared-object library with an ANSI-C interface. These C-style functions can then be called from a custom ACF component written in C++.
- If you do not want to write any C++ code at all, then your application will need to be able to call C-style functions itself. It can then access Axioline I/O (and the PLC status) through the ANSI-C libraries on the PLCnext Control.
- If the language is also able to call C++ functions, then the application can get access to the complete set of RSC services on the PLCnext Control.

### 8. Package as a PLCnext Store app

There is a category of PLCnext Store app called "Runtime", which is intended for runtime applications just like the one demonstrated in this series. Check out the [runtime applications that are already in the store](https://www.plcnextstore.com/#/search?type=2).

An example of a [runtime app for the PLCnext Store](https://github.com/PLCnext/PLCnextAppExamples/tree/public/DemoApps/DemoPlcnextExtensionsApps/PlcnextSampleRuntimeApp) is shown in the [PLCnextAppExamples](https://github.com/PLCnext/PLCnextAppExamples) Github repository.

---

## Contact us
If you would like help with your idea for a PLCnext Control application, or if you just want to share your ideas or solutions with others, please get in touch on the [PLCnext Community forum](https://www.plcnext-community.net/index.php?option=com_easydiscuss&view=categories&Itemid=221&lang=en).

---

Copyright © 2020 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
