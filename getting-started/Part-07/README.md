This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 7 - "Sample Runtime" application

Sample Runtime is a complete C++ project that implements a PLCnext runtime application. It utilises all the features used in previous articles in this series, and introduces the following new features:

1. Licence Manager RSC service - used to protect against the use of the application on unauthorised devices.

1. Subscription RSC service - used to receive notifications each time there is a change in the value of a subscribed GDS variables.

1. Axioline status register - used to monitor the status of the Axioline bus.

1. Enhanced real-time task management, including monitoring cycle time over-runs.

The source files for this application are located in the [src](https://github.com/PLCnext/SampleRuntime/tree/master/src) directory of this repository.

Support files for this application - which were used in earlier articles in this series - are also available in this repository:
- `.acf.config` and `.acf.settings` files are located in the [data](https://github.com/PLCnext/SampleRuntime/tree/master/data) folder.
- The CMakeLists.txt file is located in the [root](https://github.com/PLCnext/SampleRuntime) of the repository.
- The build script is located in the [tools](https://github.com/PLCnext/SampleRuntime/tree/master/tools) folder.

### Building and installing the application

Once you you have completed all parts of the series up to this point, you can build the Sample Runtime application as follows:

1. In your project's `src` directory, delete the file `runtime.cpp`.

1. Copy all the source files for the Sample Runtime project into your project's `src` directory.

1. Add a reference to the library `Arp.System.Lm.Services` in the CMakeLists.txt file. This provides the Licence Manager RSC service implementation.

1. Build the project to generate the `runtime` executable.

To run this example on a PLCnext Control:

1. On a Windows 10 machine, download the PLCnext Engineer project from the [PLCnEngTestProject](https://github.com/PLCnext/SampleRuntime/tree/master/PLCnEngTestProject) directory.

   This project includes an IEC 61131 project with GDS ports that are used by the the Sample Runtime.

1. In PLCnext Engineer, open the project, set the IP address of the PLC, and adjust the Axioline hardware configuration if necessary.

1. Download the PLCnext Engineer project to the PLC.

1. Copy the `runtime` executable to the PLC.

   ```bash
   scp deploy/AXCF2152_20.0.0.24752/Release/bin/runtime admin@192.168.1.10:~/projects/runtime
   ```

   Note: If you receive a "Text file busy" message in response to this command, then the file is probably locked by the PLCnext Control. In this case, stop the plcnext process on the PLC with the command `sudo /etc/init.d/plcnext stop` before copying the file.

   It is assumed that the ACF config and settings files are already on the PLC.

1. Open a secure shell session on the PLC:

   ```bash
   ssh admin@192.168.1.10
   ```

1. Set the capabilites on the executable:

   ```bash
   sudo setcap cap_net_bind_service,cap_net_admin,cap_net_raw,cap_sys_boot,cap_sys_nice,cap_sys_time+ep projects/runtime/runtime
   ```

1. Restart the plcnext process:

   ```bash
   sudo /etc/init.d/plcnext restart
   ```

1. Check the contents of the application log file:

   ```bash
   cat /opt/plcnext/projects/runtime/logs/runtime.log
   ```

   You should see messages containing I/O values being written to the log files 10 times each second.

1. Activate the fifth input on the digital input card.

   You should see the sixth digital output come on.

The Sample Runtime application is now running.

Note that there is a shell script in the  [tools](https://github.com/PLCnext/SampleRuntime/tree/master/tools) folder called `start_debug.sh`, which can be used as a template for automating the deployment, configuration and startup of the runtime application during project development.

---

### Structure of the application

The Sample Runtime application includes the following source files:

| File | Contents |
| --- | --- |
| PLCnextSampleRuntime.cpp | The `main` entry point for the application |
| CSampleRuntime.cpp / .h: | `CSampleRuntime` class |
| CSampleSubscriptionThread.cpp / .h: | `CSampleSubscriptionThread` class |
| CSampleRTThread.cpp / .h: | `CSampleRTThread` class |
| Utility.h: | Common definitions |

When the Sample Runtime application starts, the `main` function calls `ArpSystemModule_Load` and then creates a single instance of `CSampleRuntime`.

The remaining sections describe the three classes used in this application. It is recommended that this be read alongside the corresponding source code.

---

### CSampleRuntime

This class is responsible for starting worker threads and handling PLC state changes. It contains one `CSampleSubscriptionThread` object, and one `CSampleRTThread` object.

When constructed, a CSampleRuntime object simply calls `ArpPlcDomain_SetHandler` to register the function named `PlcOperationHandler`. Subsequent operations are performed when the PLCnext Control calls `PlcOperationHandler` to signal a change of state:
- **Start Warm**: On this event, `CSampleRuntime` performs some [initialisation](#initialisation), then tells the `CSampleSubscriptionThread` object and the `CSampleRTThread` object to start processing.

- **Start Cold** or **Start Hot**: `CSampleRuntime` tells the `CSampleSubscriptionThread` object and the `CSampleRTThread` object to start processing.

- **Stop**, **Reset** or **Unload**: `CSampleRuntime` tells the `CSampleSubscriptionThread` object and the `CSampleRTThread` object to stop processing.

### Initialisation

When the PLC signals the "Start Warm" event, the following initialisation steps occur:
- The `CSampleRuntime` object subscribes to a number of RSC services:
   - Device Info service.
   - Device Status service.
   - Licence Status service.
   
- Data is read from the Device Info and Device Status services:
   - PLC vendor name
   - Load on one CPU core
   - Memory usage
   - PLC board temperature
   
   In this application, data is simply read during initialisation as a demonstration of how to use these services.

- The Licence Status service is used to check that there is a valid licence for this application on this PLC. This mechanism can be used to protect against the use of the application on unauthorised devices. Currently, the only way to install an application licence on a PLC is by installing the application from the PLCnext Store.

   In this example, the application cannot be instaled from PLCnext Store (because it doesn't exist there!), and so no valid licence will be found on the PLC. This application simply ignores the result of the licence check, but a real-world application can (for example) shut down or continue to operate in a "restricted" mode if no valid licence is found.

- The `CSampleRTThread` object is initialised. This involves the creation of two worker threads:
   - A real-time thread, which executes the `RTStaticCycle` member function.
   - A default priority thread, which executes the `StaticLoggingCycle` member function.

   These two member functions are described below.

- The `CSampleSubscriptionThread` object is initialised. This involves the creation of a single worker thread that executes the `StaticCycle` member function. This member function is described below.

---

### CSampleRTThread

During initialisation (in the `Init` member function), the `CSampleRTThread` object creates a real-time thread for executing time-critical operations. It also creates a non-real-time thread to execute "slow" operations that, if run on the real-time thread, would affect the performance of the time-critical parts of the application.

When commanded to start processing (via the `StartProcessing` member function), the object:
- Gets pointers to the Axioline Input and Output buffers in the Global Data Space.

- Uses the `AddInput` and `AddOutput` functions to populate two `std::map`s with instances of the `RAWIO` struct. This struct, defined in `CSampleRTThread.h`, contains all the data required to access a single Axioline I/O point, including the offset to the I/O data in the GDS buffer.

- Adds a special Axioline input variable called `AXIO_DIAG_STATUS_REG`. This variables contains the current status of the Axioline bus, and can be used for diagnostics and error detection, e.g. to detect when an Axioline module has failed. Details of how to interpret values for this variable are given in the document "UM EN AXL F SYS DIAG", available for download from the Phoenix Contact website.

   Note that, since the structure of the Global Data Space is fixed during the startup of the PLCnext runtime, information about the location of I/O in the Global Data Space only needs to be obtained once, rather than every scan cycle. This provides a significant efficiency improvement over the way that I/O reads and writes were handled in the example shown earlier in this series.

Cyclic processing on the real-time thread is perfomed by the `RTStaticCycle` member function, which in turn calls the `RTCycle` member function. The main purpose of the `RTCycle` function is to schedule the start of the next scan cycle using the `clock_nanosleep` function. This provides a precise period for the processing of real-time operations. This function also checks for "real-time violations", i.e. any instances where the execution of the function takes longer than the specified cycle time.

During each scan cycle, the `RTCycle` function also calls these three functions:
- `ReadInputData`
- `DoLogic`
- `WriteOutputData`

`DoLogic` is the core of the real-time application, where process-specific logic is implemented. In this case, some basic binary operations are performed on a few digital inputs and outputs.

Cyclic processing on the non-real-time thread is performed by the `StaticLoggingCycle` member function, which in turn calls the `LoggingCycle` member function. This simply writes the current value of each I/O variable to the application log file, approximately every 100 milliseconds.

---

### CSampleSubscriptionThread

This class demonstrates the use of the "Subscription" RSC service for reading GDS variables. This service is an alternative to the "Data Access" RSC service, which can be used to "poll" the values of GDS variables. The Subscripton service, on the other hand, will notify the RSC client when the value of any subscribed GDS variables changes, eliminating the need for polling.

This application uses the Subscription service to read the value of three GDS variables. These variables have been declared in the PLCnext Engineer project that accompanies this example.

The Subscription service differs from most other RSC services, in that it employs delegates (also known as callback functions) to send notifications to subscribers.

In order to use the Subscription service:

1. Declare a shared pointer to store a service handle:

   ```cpp
   ISubscriptionService::Ptr m_pSubscriptionService;
   ```

2. Get the handle for an ISubscriptionService method:

   ```cpp
   CSampleRuntime::Init() {
      m_pSubscriptionService = ServiceManager::GetService<ISubscriptionService>(); //get ISubscriptionService
   }
   ```

3. Create a subscription for the port variables: 

   ```cpp
   CreateSubscription() {
      m_uSubscriptionId = m_pSubscriptionService->CreateSubscription(SubscriptionKind::HighPerformance);
      m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort1)
   }
   ```

4. Read and assign port variables:

   ```cpp
   CSampleRuntime::ReadSubscription() {
      RSCReadVariableValues(m_zSubscriptionValues)
      m_zSubscriptionValues[nCount].CopyTo(m_gdsPort1)
   }
   ```

5. Delete the subscription:

   ```cpp
   CSampleRuntime::DeleteSubscription() {
      m_pSubscriptionService->DeleteSubscription(m_uSubscriptionId)
   }
   ```

---

## How to get support

You can get support in the forum of the [PLCnext Community](https://www.plcnext-community.net/index.php?option=com_easydiscuss&view=categories&Itemid=221&lang=en).

---

Copyright © 2020 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
