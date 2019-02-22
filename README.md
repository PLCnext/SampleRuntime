# PLCnext Technology  - SampleRuntime
[![Feature Requests](https://img.shields.io/github/issues/PLCnext/SampleRuntime/feature-request.svg)](https://github.com/PLCnext/SampleRuntime/issues?q=is%3Aopen+is%3Aissue+label%3Afeature-request+sort%3Areactions-%2B1-desc)
[![Bugs](https://img.shields.io/github/issues/PLCnext/SampleRuntime/bug.svg)](https://github.com/PLCnext/SampleRuntime/issues?utf8=✓&q=is%3Aissue+is%3Aopen+label%3Abug)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Web](https://img.shields.io/badge/PLCnext-Website-blue.svg)](https://www.phoenixcontact.com/plcnext)
[![Community](https://img.shields.io/badge/PLCnext-Community-blue.svg)](https://www.plcnext-community.net)

## Guide details
|Description | Value |
|------------ |-----------|
|Created | 17.01.2019|
|Last modified|21.02.2019|
|Controller| AXC F 2152 |
|FW|2019.0 LTS|
|SDK| 2019.0 LTS|

Hardware requirements

Starter kit- AXC F 2152 STARTERKIT - 1046568, consisting of:

	1. I/O module AXL F DI8/1 DO8/1 1H 2701916.
	2. I/O module AXL F AI2 AO2 1H 2702072.
	3. Interface module UM 45-IB-DI/SIM8 2962997.
	4. Patch cable FL CAT5 PATCH 2,0 2832289.


Software requirements

	1. PLCnext Engineer 2019.x LTS.
	2. Linux operating system or Linux VM.
	3. WinSCP (optional).


# 1.0 Sample Runtime v1.1

Sample Runtime is an Eclipse project that contains an additional script for writing the application to and instantiating it on the controller. The script can only be executed on Linux systems.
The example illustrates the implementation of a process called `PLCnextSampleRuntime` with the following applications:

1. Accessing GDS port variables.
First, the port variables are declared in the PLCnext Engineer project and this project is then written to the controller. When the process "PLCnextSampleRuntime" is started, the port variables are accessed using ISubscriptionService.

2. Accessing I/O data and diagnostic status register.
The application shows how to read the digital inputs and how to write data to the digital outputs. It also demonstrates how to access system variables and the Axioline diagnostic registers.

3. Accessing static and dynamic data (VendorName, circuit board temperature, CPU/memory utilization)
In the application, IDeviceInfoService and IDeviceStatusService are used to access the data. The device information is provided by the firmware.


# 1.1 Overview of dependencies

In order for an external application to use the services and the shared memory of the ARP firmware, this application must be started by the PLCnext runtime and it must also initialize part of the ACF. Due to the configuration of the ARP firmware, the application is started in a separate process. 
In order to initialize the ACF, the application must call the ArpSystemModule_Load() function. When the initialization is completed, a number of required components is started in the application process (e.g., Arp.Plc.DomainProxy and Arp.Plc.AnsiC). These components must be defined in the configuration.
When the startup phase is completed, a set of functions enables the application to communicate with the Arp.Plc.AnsiC component. The respective calls are translated into RSC calls to the ARP firmware. Any changes of the controller state are passed on to the application using a callback function.

![Dependencies](/Picture/01_Dependencies.gif)

# 1.2 Firmware configuration

The following aspects must be considered in the ACF configuration:

    - Starting the external application as a child process. 
    - Loading the libraries for Arp.Plc.DomainProxy and the Arp.Plc.AnsiC component.
    - Starting the Arp.Plc.DomainProxy component in the external application process.
    - Starting the Arp.Plc.AnsiC component in the external application process.

This configuration is done using the "include" mechanism of the ACF configuration file called "PLCNextSampleRuntime.acf.config". When the function ArpSystemModule_Load()  is called, an ACF settings file is passed together with the file path to the used  "/usr/lib" libraries. 
The following components are required to execute the application and access the Axioline data:

    Main process:
        Arp.Device.Interface.
        Arp.Device.HmiLed.
        Arp.Plc.Meta.
        Arp.Plc.Esm.
        Arp.Plc.Gds.
        Arp.Plc.Domain.
        Arp.Plc.PlcManager.
        Arp.Plc.Fbm.
    Axioline process:
        Arp.Io.AxlC.
        Arp.Io.FbIo.AxlC.
    External application process:
        Arp.Plc.AnsiC.
        Arp.Plc.DomainProxy.AppName.



# 2.0 Structure of the external application


# 2.1 Initilization

Setting the callback for controller state changes is independent of the communication with the PLCnext runtime. Therefore, the callback can be set using the "ArpPlcDomain_SetHandler()"  function before the ACF is initialized. 
In order to enable the communication with the PLCnext runtime, the ACF must be initialized. This is done by calling the function ArpSystemModule_Load(). In doing so, the binary files directory, the application name and the path to the settings file are passed (ArpSystemModule_Load("/usr/lib", "PLCnextSampleRuntime", strSettingsFile.c_str())). The settings file basically defines the folder of the application"s log file. ArpSystemModule_Load must be called, and the call must be executed when the application is started. The PLCnext runtime will wait for the application to execute this call (time-out after 10 minutes).

# 2.2 Controller state

In case of controller state changes, the callback function (PlcOperationHandler) is called, which has been registered using the function ArpPlcDomain_SetHandler(). The controller state is passed on to this function.


# 3.0 Access to GDS port variables

This example shows how to read data from the GDS using ISubscriptionService. The data access comprises port variables that have been declared in the PLCnext Engineer project. The following functions demonstrate the access:

1. Declaring a shared pointer to store a service handle:
```cpp
	ISubscriptionService::Ptr m_pSubscriptionService;
``` 

2. Getting the handle for an ISubscriptionService method:
```cpp
	CSampleRuntime::Init() {
		m_pSubscriptionService = ServiceManager::GetService<ISubscriptionService>(); //get ISubscriptionService
	}
```

3. Creating a subscription for the port variables: 
```cpp
	CreateSubscription() {
		m_uSubscriptionId = m_pSubscriptionService->CreateSubscription(SubscriptionKind::HighPerformance);
		m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort1)
	}
```

4. Reading and assigning port variables:
```cpp
	CSampleRuntime::ReadSubscription() {
		RSCReadVariableValues(m_zSubscriptionValues)
		m_zSubscriptionValues[nCount].CopyTo(m_gdsPort1)
	}
```

5. Deleting the subscription:
```cpp
	CSampleRuntime::DeleteSubscription() {
		m_pSubscriptionService->DeleteSubscription(m_uSubscriptionId)
	}
```

The complete source code is contained in the sample application "CSampleRuntime.cpp" for better understanding and reconstruction of the functions.


# 4.0 Access to I/O data 

With IEC 61131 applications, the exchange of I/O data between Axioline and the GDS is executed cyclically, managed by the ESM. The ESM updates the I/O data at the beginning and at the end of a defined task. 
With the sample runtime used here, data updates are triggered by calling an appropriate method. For the read/write operation, the method CSampleRuntime::ReadWriteIoData() is implemented. This method uses the functions ArpPlcAxio_ReadFromAxioToGds() and ArpPlcAxio_WriteFromGdsToAxio() in order to explicitly trigger the data exchange between the GDS and Axioline. The data access comprises process data that is defined in a bus configuration file (.tic file; see entry: 0.~DI8 in the .tic file). 
To access the I/O data, first a handle must be created to access the GDS buffer ((ArpPlcIo_GetBufferPtrByPortName()). As the GDS buffer may contain many variables, the next step is to determine the offset values of the variables (ArpPlcGds_GetVariableOffset()). Once the offsets of the relevant variables are known, a memory area of the GDS buffer can be allocated for reading (ArpPlcGds_BeginRead()) or writing ((ArpPlcGds_BeginWrite()). 
The respective memory area will be locked while read/write access is in progress and a pointer to this memory area is returned.  The pointer, which locates the beginning of the memory area, and the offsets of the variables can then be used to start a read or write operation from/to the shared memory. When a read operation is completed, the memory area lock must be released using ArpPlcGds_EndRead(). A successful write operation must be terminated using ArpPlcGds_EndWrite(). The data that has been written is marked as valid. In case the data is to be marked as invalid, ArpPlcGds_EndWriteDataInvalid()  must be used to terminate the operation.
A GDS buffer that is not required anymore must be closed using ArpPlcIo_ReleaseGdsBuffer(). The sample code below shows the most important implementation steps for accessing the I/O data of AXL DI/DO modules.

1. Defining the process data (strInPort and strOutPort) to be accessed and reading/writing the corresponding I/O data using the sub functions ReadInputData and WriteOutputData.
```cpp
	CSampleRuntime::ReadWriteIoData {
	    	ArpPlcAxio_ReadFromAxioToGds(0)  //trigger for AXIO IO data exchange 
		String strInPort = Formatter::FormatCommon("{}/0.~DI8", ARP_IO_AXIO);       //assign the process data to port variable
		String strOutPort = Formatter::FormatCommon("{}/0.~DO8", ARP_IO_AXIO); //assign the process data to port variable
		ReadInputData(ARP_IO_AXIO, strInPort, (char*)&value, sizeof(value)) 	                //read data from Axio frame
		WriteOutputData(ARP_IO_AXIO, strOutPort, (char*)&value, sizeof(value)                    // write data to Axio frame
		ArpPlcAxio_WriteFromGdsToAxio(0) //trigger for AXIO IO data exchange 
	}
```
	
2. Reading process data:
```cpp
	CSampleRuntime::ReadInputData {
		ArpPlcIo_GetBufferPtrByPortName(fbIoSystemName, portName, &gdsBuffer) //Gets the Handle to the GDS-Buffer by the full port name
		ArpPlcGds_GetVariableOffset(gdsBuffer, portName, &offset)) // Calculate the offset of IO variable
		ArpPlcGds_BeginRead(gdsBuffer, &readDataBufferPage)) // Begin read operation, memory buffer will be locked
		memcpy(pValue, dataAddress, valueSize);  //Copy data from gds Buffer
		ArpPlcGds_EndRead(gdsBuffer)  // unlock buffer
		ArpPlcIo_ReleaseGdsBuffer(gdsBuffer)) // Release the GdsBuffer and free internal resources
	}
```

3. Writing process data:
```cpp
	CSampleRuntime::WriteOutputData {
		ArpPlcIo_GetBufferPtrByPortName(fbIoSystemName, portName, &gdsBuffer)  //Gets the Handle to the GDS-Buffer by the full port name
		ArpPlcGds_GetVariableOffset(gdsBuffer, portName, &offset) // Calculate the offset of IO variable
		ArpPlcGds_BeginWrite(gdsBuffer, &dataBufferPage) // Begin write operation, memory buffer will be locked
		memcpy(dataAddress, pValue, valueSize) // Copy data to gds Buffer
		ArpPlcGds_EndWrite(gdsBuffer))         // unlock buffer
		ArpPlcIo_ReleaseGdsBuffer(gdsBuffer)   // Release the GdsBuffer and free internal resources
	}
```
The complete source code is contained in the sample application "CSampleRuntime.cpp" for better understanding and reconstruction of the functions.


# 4.1 Axioline diagnostics

The status of the diagnostic registers of an AXCF2152 controller can be determined using system variables. The system variables of the diagnostic status register and the diagnostic parameter register are accessed using the method "ReadAxioSystemVariable()".
```cpp
	CSampleRuntime::ReadAxioSystemVariable {
		String strStatusRegister = Formatter::FormatCommon("{}/AXIO_DIAG_STATUS_REG", ARP_IO_AXIO);
		ReadInputData(ARP_IO_AXIO, strStatusRegister, (char*)&axioStatusRegister, sizeof(axioStatusRegister)
	}
```
The complete source code is contained in the sample application "CSampleRuntime.cpp" for better understanding and reconstruction of the functions.


# 4.2 Profinet diagnostics

The status of the input or output data of a submodule (IOCS/IOPS) can be determined using the function "ReadProfinetIOPS()". The process data must be taken from the  .tic file (PCWE/IO/Arp.Io.PnC/xxx.tic -> search for "{}/x.IPS", {}/x.OCS)
```cpp
	CSampleRuntime::ReadProfinetIOPS {
	 	String strInPort = Formatter::FormatCommon("{}/9.IPS", ARP_IO_PN);
    		String strOutPort = Formatter::FormatCommon("{}/9.OCS", ARP_IO_PN);
    		ReadInputData(ARP_IO_PN, strInPort, (char*)&profinetIOPS, sizeof(profinetIOPS)
    		ReadInputData(ARP_IO_PN, "Arp.Io.PnC/9.OCS", (char*)&profinetIOCS, sizeof(profinetIOCS)
	}
```


# 4.3 Device status diagnostics

Der state of the controller can be determined as follows using IDeviceInfoService and IDeviceStatusService:
```cpp
	CSampleRuntime::GetDeviceStatus  {
		// get some values from device info service (static data)
		m_rscVendorName = m_pDeviceInfoService->GetItem("General.VendorName");
		m_szVendorName = m_rscVendorName.GetChars();

		// get some values from device status service (dynamic data)
		m_rscCpuLoad = m_pDeviceStatusService->GetItem("Status.Cpu.0.Load.Percent");
		m_rscCpuLoad.CopyTo(m_byCpuLoad);
		m_rscBemoryUsage = m_pDeviceStatusService->GetItem("Status.Memory.Usage.Percent");
		m_rscBemoryUsage.CopyTo(m_byMemoryUsage);
		m_rscBoardTemp = m_pDeviceStatusService->GetItem("Status.Board.Temperature.Centigrade");
		m_rscBoardTemp.CopyTo(m_i8BoardTemp);
	}
```


# 5.0 Installation

The sample project "Sample Runtime v1.zip" has been added as attachment. This example contains an application together with a suitable configuration. 


# 5.1 Installation of Linux packages

Please perform the following installation steps once on the Linux computer:

1. Install "sshpass_1.06-1_amd64.deb" (used by debug script).
2. Install "cmake-3.13.2-Linux-x86_64.sh"(used by the CLI tool, cmake version should be higher than 3.12.2).
3. Install "libunwind8_1.2.1-8_amd64.deb".


# 5.2 Installation of the sample project

Perform the following steps to install the sample application on the Linux computer:

1. Extract the content of "Sample Runtime v1.1.zip" in the Eclipse "workspace" folder.
2. Import the project "PLCnextSampleRuntime" in the Eclipse editor and compile it.
3. The compilation for release and debugging must be without errors.
4. Create the following directory structure on the controller: 

	- /opt/PLCnextSampleRuntime/ (create the folder).
	- /opt/PLCnextSampleRuntime/Default.acf.settings (copy the file from the "Sample Runtime v1.1.zip" file).
	- /opt/PLCnextSampleRuntime/PLCnextSampleRuntime (that's an .exe file that is transferred to the controller via debug scripts).
	- /opt/PLCnextSampleRuntime/Logs (create the folder).
	- Change the file properties of the "Logs" folder (chmod 777 Logs) to allow general overall access. Otherwise the process will not be able to create log files.
	- /opt/PLCnextSampleRuntime/Logs/PLCnextSampleRuntime.log (log file will be created at the first startup of the sample application).
	- /etc/plcnext/device/PLCNextSampleRuntime.acf.config (copy the file from the "Sample Runtime v1.1.zip" file).
  
5. Set the password for the "root" user ($ sudo passwd root).
6. Log in as "root" user ($ su root).
7. Set the entry in "sshd_config" file from "#PermitRootLogin no" to "PermitRootLogin yes" ($ vim /etc/ssh/sshd_config).
8. Reload sshd ($ /etc/init.d/sshd reload)
9. Check the SSH connection from the Eclipse computer/VM to the  AXCF2152 controller ($ ssh -l root IP-of-device).
10. Adapt the settings in the debug script under  "workspace-Folder/PLCnextSampleRuntime/Debug/start_debug" as follows:

	- IP address of the controller.
	- root password.
	
11. Save the debug script and change the access rights for the file to executable ($ chmod 777 start_debug).


# 5.3 Debug settings in Eclipse.

Please find in the plcnext community the debug configuration in Eclipse:

[![Community](https://img.shields.io/badge/PLCnext-Community-blue.svg)](https://www.plcnext-community.net/index.php?option=com_content&view=article&id=178:remote-debugging-attach-to-process&catid=48&Itemid=176&lang=en)


# 5.4 Commissioning

1. Optional procedure: Write the contained PLCnext Engineer project to the controller to create the port variables in the GDS.
2. Compile the Eclipse project as debug version.
3. Run the "start_debug" script ($ ./Debug/start_debug).
4. In Eclipse, start the debug mode that you have configured earlier.
5. Check the output of the application in /opt/PLCnextSampleRuntime/Logs/PLCnextSampleRuntime.log.
6. Check whether the PLCnextSampleRuntime process has been started successfully as child process of the PLCnext firmware (use ps or htop command in the SSH console, for example).


# 5.5 Configuring the sample project "Sample Runtime v1.1" for a specific bus configuration

1. Create a new project in PLCnext Engineer and set up a bus configuration (e.g., an AXL F DI8/1 DO8/1 1H for  AXCF2152 AXIO system bus and an AXCF DI8/3 DO8/3 2H for AXL F BK PN). Then save and compile the project.
2. In the folders "Arp.Io.AxlC" and "Arp.Io.PnC", open the bus configuration files generated by PLCnext Engineer. The folders can be located, for example, in the following directory:    
   "C:\Users\Public\Documents\PLCnext Engineer\2019.0 LTS\Binaries\Test_1~1@binary\RES_EB8795C5FC854B4DA314ED32D2CD62BD" (where "Test_1" is the project name).
3. Edit the source file CSampleRuntime::ReadWriteIoData and adapt the data types to the corresponding process data length of the I/O data. 
   Afterwards, change the NodeID based on the information in the .tic files. For example:

The following modules are replaced:

Is:		AXL F DI8/1 DO8/1 1H.
			AXL F AI2 AO2 1H.
Axioline:   NodeID = "0.~DI8" and NodeID = "0.~DO8"  (see  *.tic file in foder Arp.Io.AxlC). 
       
Change to:		AXL F DI16/4 2F.
       		AXL F DO16/1 1H.
Axioline:   NodeID = "/0.~DI16" and NodeID = "/1.~DO16" (see *.tic file in folder Arp.Io.AxlC).


Changes in the code line of the method: CSampleRuntime::ReadWriteIoData.

Is:	
	```cpp
		uint8_t value = 0;			
	 	String strInPort = Formatter::FormatCommon("{}/0.~DI8", ARP_IO_AXIO);
		String strOutPort = Formatter::FormatCommon("{}/0.~DO8", ARP_IO_AXIO);
	```

Change to:	
	```cpp
		uint16_t value = 0;
	 	String strInPort = Formatter::FormatCommon("{}/0.~DI16", ARP_IO_AXIO);
		String strOutPort = Formatter::FormatCommon("{}/1.~DO16", ARP_IO_AXIO);
	```

	 
In case the AXL modules are connected to an AXL PN BK, the process data can be adapted as follows, same as with Axioline local bus.

Profinet bus coupler: NodeID = "/25" (see *tic file in folder Arp.Io.PnC).


Changes in the code line of the method: CSampleRuntime::ReadProfinetIOPS.

Diagnostics:

Is: 	
	```cpp
		String strInPort = Formatter::FormatCommon("{}/9.IPS", ARP_IO_PN);
		String strOutPort = Formatter::FormatCommon("{}/9.OCS", ARP_IO_PN);
	```
			
Change to: 	
	```cpp
		String strInPort = Formatter::FormatCommon("{}/25.IPS", ARP_IO_PN);
		String strOutPort = Formatter::FormatCommon("{}/25.OCS", ARP_IO_PN);
	```


Data exchange (not implemented):
	```cpp
		String strInPort = Formatter::FormatCommon("{}/25.~DI8", ARP_IO_PN);
		String strOutPort = Formatter::FormatCommon("{}/25.~DO8", ARP_IO_PN);
	```



# 6.0 Header file of the sample application (CSampleRuntime.h)

```cpp
	/******************************************************************************
	 *
	 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
	 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
	 *
	 *  CSampleRuntime.h
	 *
	 *  Created on: Jan 3, 2019
	 *      Author: Steffen Schlette
	 *
	 ******************************************************************************/

	#ifndef CSAMPLERUNTIME_H_
	#define CSAMPLERUNTIME_H_

	#include <vector>
	#include <pthread.h>
	#include "Arp/Plc/AnsiC/Domain/PlcOperationHandler.h"
	#include "Arp/Device/Interface/Services/IDeviceStatusService.hpp"
	#include "Arp/Device/Interface/Services/IDeviceInfoService.hpp"
	#include "Arp/Plc/Domain/Services/IPlcManagerService.hpp"
	#include "Arp/Services/NotificationLogger/Services/INotificationLoggerService.hpp"
	#include "Arp/Plc/Gds/Services/ISubscriptionService.hpp"
	#include "Arp/Plc/Gds/Services/IDataAccessService.hpp"

	using namespace std;
	using namespace Arp;
	using namespace Arp::Plc::Domain::Services;
	using namespace Arp::Device::Interface::Services;
	using namespace Arp::Services::NotificationLogger::Services;
	using namespace Arp::Plc::Gds::Services;

	#define WAIT100ms 	usleep(100000);		// 100ms
	#define WAIT1s 		usleep(1000000);	// 1s
	#define WAIT10s 	usleep(10000000);	// 10s

	typedef void * (*THREADFUNCPTR)(void *);
	
	// callback-function for state changes
	void PlcOperationHandler(enum PlcOperation operation);
	
	class CSampleRuntime;
	extern CSampleRuntime* g_pRT;
	
	class CSampleRuntime {
	public:
		CSampleRuntime();
		virtual ~CSampleRuntime();

	static PlcOperation m_zPLCMode;

	bool Init();
	static void* StaticCycle(void* p);
	void Cycle();

	// start and stop our own processing
	bool StartProcessing();
	bool StopProcessing();
	private:
	// workerthread for cycle
	   pthread_t m_zCycleThread;

	bool m_bInitialized;	// class already initialized?
	bool m_bDoCycle;		// shall the cycle run?

	IDeviceInfoService::Ptr m_pDeviceInfoService;
	IDeviceStatusService::Ptr m_pDeviceStatusService;
	IPlcManagerService::Ptr m_pPLCManagerService;
	INotificationLoggerService::Ptr m_pNotificationLoggerService;
	ISubscriptionService::Ptr m_pSubscriptionService;
	IDataAccessService::Ptr m_pDataAccessService;
	
	bool GetDeviceStatus();
	
	RscVariant<512> m_rscVendorName;
	RscVariant<512> m_rscCpuLoad;
	RscVariant<512> m_rscBemoryUsage;
	RscVariant<512> m_rscBoardTemp;
	const char* m_szVendorName;
	byte m_byCpuLoad;
	byte m_byMemoryUsage;
	int8 m_i8BoardTemp;

	// example usage of GDS subscription
	bool CreateSubscription();
	bool DeleteSubscription();
	bool ReadSubscription();
	DataAccessError RSCReadVariableValues(vector<RscVariant<512>>& values);
	DataAccessError RSCReadVariableInfos(vector<VariableInfo>& values);

    // subscriptions
    uint32 m_uSubscriptionId;
	vector<RscVariant<512>> m_zSubscriptionValues;
	vector<VariableInfo> m_zSubscriptionInfos;

    // data fields
    bool m_gdsPort1;
    bool m_gdsPort2;
    bool m_gdsPort3;

    // example usage of direct access to fieldbus-frame
    bool ReadWriteIoData(void);
    bool ReadAxioSystemVariable(void);
    bool ReadProfinetIOPS(void);
    bool ReadInputData(const char* fbIoSystemName, const char* portName, char* pValue, size_t valueSize);
    bool WriteOutputData(const char* fbIoSystemName, const char* portName, const char* pValue, size_t valueSize);
	};

	#endif /* CSAMPLERUNTIME_H_ */
```


# 6.1 Source code of the sample application (PLCnextSampleRuntime.cpp)

```cpp
	/******************************************************************************
	 *
	 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
	 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
	 *
	 *****************************************************************************/
	
	/*************************************************************************/
	/*  INCLUDES                                                             */
	/*************************************************************************/
	#include <stdint.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <iostream>
	#include <libgen.h>
	#include <linux/limits.h>
	#include <unistd.h>
	#include <syslog.h>
	#include "Arp/System/ModuleLib/Module.h"
	
	#include "CSampleRuntime.h"
	
	using namespace std;
	
	CSampleRuntime* g_pRT;
	
	int main() {

	// we use syslog for logging until the PLCnext logger isn't ready
	openlog ("PLCnextSampleRuntime", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	syslog (LOG_INFO, "PLCnextSampleRuntime started");

	char szExePath[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", szExePath, PATH_MAX);
	string strDirPath;
	if (count != -1) {
		strDirPath = dirname(szExePath);
	}

    // First initialize PLCnext module application
    // Arguments:
    //  arpBinaryDir:    Path to Arp binaries
    //  applicationName: Arbitrary Name of Application
    //  acfSettingsPath: Path to *.acf.settings document to set application up
	string strSettingsFile(strDirPath);
	strSettingsFile += "/Default.acf.settings";

    if (ArpSystemModule_Load("/usr/lib", "PLCnextSampleRuntime", strSettingsFile.c_str()) != 0)
    {
    	syslog (LOG_ERR, "Could not load Arp System Module Application");
        return -1;
    }
	syslog (LOG_INFO, "Loaded Arp System Module Application");
	closelog();

	g_pRT = new CSampleRuntime();

	// loop forever
	for (;;)
	{
		WAIT1s;
	}

	return 0;
	}
```


# 6.3 Source code of the sample application (CSampleRuntime.cpp)

```cpp
	/******************************************************************************
	 *
	 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
	 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
	 *
	 *  CSampleRuntime.cpp
	 *
	 *  Created on: Jan 3, 2019
	 *      Author: Steffen Schlette
	 *
	 ******************************************************************************/
	
	#include <stdint.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <iostream>
	#include <libgen.h>
	#include <linux/limits.h>
	#include <unistd.h>
	#include <vector>
	#include "boost/range/combine.hpp"
	#include "Arp/System/Core/Arp.h"
	#include "Arp/System/ModuleLib/Module.h"
	#include "Arp/System/Acf/ComponentBase.hpp"
	#include "Arp/System/Acf/IApplication.hpp"
	#include "Arp/System/Acf/IControllerComponent.hpp"
	#include "Arp/System/Rsc/ServiceManager.hpp"
	#include "Arp/System/Commons/Logging.h"
	#include "Arp/System/Commons/Threading/WorkerThread.hpp"
	#include "Arp/Plc/AnsiC/ArpPlc.h"
	#include "Arp/Plc/AnsiC/Device/Device.h"
	#include "Arp/Plc/AnsiC/Domain/PlcManager.h"
	
	#include "Arp/Plc/AnsiC/Gds/DataLayout.h"
	#include "Arp/Plc/AnsiC/Io/FbIoSystem.h"
	#include "Arp/Plc/AnsiC/Io/Axio.h"
	
	#include "CSampleRuntime.h"
	
	
	using namespace Arp;
	using namespace Arp::System::Acf;
	using namespace Arp::System::Rsc;
	using namespace Arp::Device::Interface::Services;
	
	
	PlcOperation CSampleRuntime::m_zPLCMode = PlcOperation_None;
	
	// a PLCnext Engineer Project should exist on the device with this simple structure:
	// A program-instance named "MyProgramInst" which has three Ports of datatype bool: VarA (IN Port), VarB (IN Port) and VarC (Out Port)
	static const char GDSPort1[] = "Arp.Plc.Eclr/MyProgramInst.VarA";
	static const char GDSPort2[] = "Arp.Plc.Eclr/MyProgramInst.VarB";
	static const char GDSPort3[] = "Arp.Plc.Eclr/MyProgramInst.VarC";
	
	#define ARP_IO_AXIO "Arp.Io.AxlC"
	#define ARP_IO_PN	"Arp.Io.PnC"
	
	CSampleRuntime::CSampleRuntime()
				  : m_zCycleThread(),
				    m_bInitialized(false),
					m_bDoCycle(false),
					m_szVendorName(NULL),
				    m_byCpuLoad(0),
					m_byMemoryUsage(0),
					m_i8BoardTemp(0),
					m_uSubscriptionId(0),
				    m_gdsPort1(0),
				    m_gdsPort2(0),
				    m_gdsPort3(0)
	{
		// announce the status-update callback
		// this is important to get the status of the "firmware-ready"-event PlcOperation_StartWarm
		ArpPlcDomain_SetHandler(PlcOperationHandler);
	}
	
	CSampleRuntime::~CSampleRuntime()
	{
	
	}
	
	// init/connect to the PLCnext services and start the cycle-thread
	bool CSampleRuntime::Init()
	{
		if(m_bInitialized)
		{
			return(true);
		}

	Log::Info("Call of CSampleRuntime::Init");

	bool bRet = false;

	// the firmware needs to be in the state PlcOperation_StartWarm before we can acquire services
	m_pPLCManagerService = ServiceManager::GetService<IPlcManagerService>();
	m_pDeviceInfoService = ServiceManager::GetService<IDeviceInfoService>();
	m_pDeviceStatusService = ServiceManager::GetService<IDeviceStatusService>();
	m_pNotificationLoggerService = ServiceManager::GetService<INotificationLoggerService>();
	m_pSubscriptionService = ServiceManager::GetService<ISubscriptionService>();
	m_pDataAccessService = ServiceManager::GetService<IDataAccessService>();

	if((m_pPLCManagerService != NULL) &&
	   (m_pDeviceStatusService != NULL) &&
	   (m_pDeviceInfoService != NULL) &&
	   (m_pNotificationLoggerService != NULL) &&
	   (m_pSubscriptionService != NULL) &&
	   (m_pDataAccessService != NULL)
	   )
	{
		if(GetDeviceStatus() == true)
		{
			if(CreateSubscription() == true)
			{
				// create a simple worker thread for our loop
				if(pthread_create(&m_zCycleThread, NULL, CSampleRuntime::StaticCycle, this) == 0)
				{
					m_bInitialized = true;
					bRet = true;
				}
			}
		}
	}

	return(bRet);
	}
	
	bool CSampleRuntime::StartProcessing()
	{
		Log::Info("Start processing");

	bool bRet = false;
	if(CreateSubscription())
	{
		m_bDoCycle = true;
		bRet = true;
	}

	return(bRet);
	}
	
	bool CSampleRuntime::StopProcessing()
	{
		Log::Info("Stop processing");
	
		bool bRet = false;
	
		// this is not threadsafe! In a real application you need to wait for end of cycle before deleting the subscription
		m_bDoCycle = false;
	
		if(DeleteSubscription())
		{
			bRet = true;
		}
		return(bRet);
	}

	// worker thread-entry for the cycle function
	void* CSampleRuntime::StaticCycle(void* p)
	{
		if(p != NULL)
		{
			((CSampleRuntime*)p)->Cycle();
		}
		return(NULL);
	}
	
	// a simple loop. In a real application, this should be done in a worker thread with correct priority
	void CSampleRuntime::Cycle()
	{
		Log::Info("Call of CSampleRuntime::Cycle");

	for (;;)
	{
		if(m_bDoCycle)
		{
			// you can check the log messages in the local log-file of this application, usually in a subfolder named "Logs"
			Log::Info("************* Another cycle *************");

			ReadSubscription();

			Log::Info("{0}: Value: {1}", GDSPort1, m_gdsPort1 ? "true" : "false");
			Log::Info("{0}: Value: {1}", GDSPort2, m_gdsPort2 ? "true" : "false");
			Log::Info("{0}: Value: {1}", GDSPort3, m_gdsPort3 ? "true" : "false");

			// use the functions for direct fieldbus-frame access
			ReadWriteIoData();
			ReadAxioSystemVariable();
			//ReadProfinetIOPS();	// not used
		}

		WAIT100ms;
	}
	}

	// retrieve some information from the firmware.
	bool CSampleRuntime::GetDeviceStatus()
	{
		bool bRet = false;

	// get some values from device info service (static data)
	m_rscVendorName = m_pDeviceInfoService->GetItem("General.VendorName");
	m_szVendorName = m_rscVendorName.GetChars();

	// get some values from device status service (dynamic data)
	m_rscCpuLoad = m_pDeviceStatusService->GetItem("Status.Cpu.0.Load.Percent");
	m_rscCpuLoad.CopyTo(m_byCpuLoad);
	m_rscBemoryUsage = m_pDeviceStatusService->GetItem("Status.Memory.Usage.Percent");
	m_rscBemoryUsage.CopyTo(m_byMemoryUsage);
	m_rscBoardTemp = m_pDeviceStatusService->GetItem("Status.Board.Temperature.Centigrade");
	m_rscBoardTemp.CopyTo(m_i8BoardTemp);

	bRet = true;

	return(bRet);
	}

	// create a GDS subscription for three ports.
	bool CSampleRuntime::CreateSubscription()
	{
		bool bRet = false;

	// create subscription
	m_uSubscriptionId = m_pSubscriptionService->CreateSubscription(SubscriptionKind::HighPerformance);

	if(m_uSubscriptionId != 0)
	{
		// add the variables
		if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort1) == DataAccessError::None)
		{
			if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort2) == DataAccessError::None)
			{
				if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort3) == DataAccessError::None)
				{
					// subscribe the build subscription and save the returned record infos for further reading
					// of the subscription to know the order of result
					const uint64 sampleRate = 1000000; // us == 1s
					if(m_pSubscriptionService->Subscribe(m_uSubscriptionId, sampleRate) == DataAccessError::None)
					{
						// get information about the order / layout of the vector of read variable values
						if(RSCReadVariableInfos(m_zSubscriptionInfos) == DataAccessError::None)
						{
							bRet = true;
						}
						else
						{
							Log::Error("Unable to read variable information");
						}
					}
					else
					{
						Log::Error("ISubscriptionservice::Subscribe returned error");
					}
				}
				else
				{
					Log::Error("Unable to subscribe variable {0}", GDSPort3);
				}
			}
			else
			{
				Log::Error("Unable to subscribe variable {0}", GDSPort2);
			}
		}
		else
		{
			Log::Error("Unable to subscribe variable {0}", GDSPort1);
		}
	}
	else
	{
		Log::Error("ISubscriptionservice::CreateSubscription returned error");
	}
	return bRet;
	}

	// create a GDS subscription for three ports.
	bool CSampleRuntime::DeleteSubscription()
	{
		bool bRet = false;

	if(m_uSubscriptionId != 0)
	{
		if(m_pSubscriptionService->DeleteSubscription(m_uSubscriptionId) == DataAccessError::None)
		{
			bRet = true;
		}
		else
		{
			Log::Error("ISubscriptionservice::DeleteSubscription returned error");
		}

		m_uSubscriptionId = 0;
	}
	return(bRet);
	}
	
	// poll the subscribed values and parse them
	bool CSampleRuntime::ReadSubscription()
	{
		bool bRet = false;

	// Read the subscription
	if(RSCReadVariableValues(m_zSubscriptionValues) == DataAccessError::None)
	{
		// The order of the subscription infos vector is the same as the subscription values vector,
		// we can iterate over both.
		if(m_zSubscriptionValues.size() == m_zSubscriptionInfos.size())	// sanity-check
		{
			for(std::size_t nCount = 0; nCount < m_zSubscriptionValues.size(); ++nCount)
			{
				if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort1) == 0)
				{
					 m_zSubscriptionValues[nCount].CopyTo(m_gdsPort1);
				}
				if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort2) == 0)
				{
					 m_zSubscriptionValues[nCount].CopyTo(m_gdsPort2);
				}
				if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort3) == 0)
				{
					 m_zSubscriptionValues[nCount].CopyTo(m_gdsPort3);
				}
			}
			bRet = true;
		}
		else
		{
			Log::Error("Inconsistent size of subscription info and subscription values vector");
		}
	}
	else
	{
		Log::Error("ISubscriptionService::ReadValues returned error");
	}
	return(bRet);
	}
	
	// create a delegate (callback-function) for reading the vector of subscription values
	DataAccessError CSampleRuntime::RSCReadVariableValues(vector<RscVariant<512>>& values)
	{
	    ISubscriptionService::ReadValuesValuesDelegate readSubscriptionValuesDelegate =
	        ISubscriptionService::ReadValuesValuesDelegate::create([&](IRscReadEnumerator<RscVariant<512>>& readEnumerator)
	    {
	    	values.clear();

    	size_t valueCount = readEnumerator.BeginRead();
        values.reserve(valueCount);

        RscVariant<512> current;
        for (size_t i = 0; i < valueCount; i++)
        {
            readEnumerator.ReadNext(current);
            values.push_back(current);
        }
        readEnumerator.EndRead();
    });

    return m_pSubscriptionService->ReadValues(m_uSubscriptionId, readSubscriptionValuesDelegate);
	}
	
	// create a delegate (callback-function) for reading the vector of subscription information
	DataAccessError CSampleRuntime::RSCReadVariableInfos(vector<VariableInfo>& values)
	{
	    ISubscriptionService::GetVariableInfosVariableInfoDelegate getSubscriptionInfosDelegate =
	        ISubscriptionService::GetVariableInfosVariableInfoDelegate::create([&](IRscReadEnumerator<VariableInfo>& readEnumerator)
	    {
	    	values.clear();

    	size_t valueCount = readEnumerator.BeginRead();
        values.reserve(valueCount);

        VariableInfo current;
        for (size_t i = 0; i < valueCount; i++)
        {
            readEnumerator.ReadNext(current);
            values.push_back(current);
        }
        readEnumerator.EndRead();
    });

    return m_pSubscriptionService->GetVariableInfos(m_uSubscriptionId, getSubscriptionInfosDelegate);
	}


	bool CSampleRuntime::ReadWriteIoData(void)
	{
		bool bRet = false;

	/*
    if(ArpPlcAxio_ReadFromAxioToGds(0))
    {
		Uint8_t value = 0;

		// create port names. Format: IOSystem/DeviceNumber.NameOfIO
		String strInPort = Formatter::FormatCommon("{}/0.~DI8", ARP_IO_AXIO);
		String strOutPort = Formatter::FormatCommon("{}/0.~DO8", ARP_IO_AXIO);

		if(ReadInputData(ARP_IO_AXIO, strInPort, (char*)&value, sizeof(value)))
		{
			Log::Info("Read Value of {0}: {1:#04x}", strInPort, value);

			WriteOutputData(ARP_IO_AXIO, strOutPort, (char*)&value, sizeof(value));
			if(ArpPlcAxio_WriteFromGdsToAxio(0))
			{
				bRet = true;
			}
			else
			{
				Log::Error("ArpPlcAxio_WriteFromGdsToAxio failed");
			}
		}
    }*/

	if(ArpPlcAxio_ReadFromAxioToGds(0))
	    {
			uint8_t value = 0;

			// create port names. Format: IOSystem/DeviceNumber.NameOfIO
			String strInPort = Formatter::FormatCommon("{}/0.~DI8", ARP_IO_AXIO);
			String strOutPort = Formatter::FormatCommon("{}/0.~DO8", ARP_IO_AXIO);

			if(ReadInputData(ARP_IO_AXIO, strInPort, (char*)&value, sizeof(value)))
			{
				Log::Info("Read Value of {0}: {1:#04x}", strInPort, value);

				WriteOutputData(ARP_IO_AXIO, strOutPort, (char*)&value, sizeof(value));
				if(ArpPlcAxio_WriteFromGdsToAxio(0))
				{
					bRet = true;
				}
				else
				{
					Log::Error("ArpPlcAxio_WriteFromGdsToAxio failed");
				}
			}
	    }

    else
    {
    	Log::Error("ArpPlcAxio_ReadFromAxioToGds failed");
    }

    return(bRet);
	}
	
	bool CSampleRuntime::ReadAxioSystemVariable(void)
	{
		bool bRet = false;

    // Existing axioline system variables:
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_HI         BYTE
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_LOW        BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_HI          BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_LOW         BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_HI        BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_LOW       BYTE
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PW         BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PF         BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_BUS        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RUN        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_ACT        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RDY        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_SYSFAIL    BOOL

	uint16_t axioStatusRegister = 0;
	String strStatusRegister = Formatter::FormatCommon("{}/AXIO_DIAG_STATUS_REG", ARP_IO_AXIO);

	if(ReadInputData(ARP_IO_AXIO, strStatusRegister, (char*)&axioStatusRegister, sizeof(axioStatusRegister)))
    {
    	// if you are wondering about the formatting syntax of the Log-Class, check
    	// http://fmtlib.net/latest/syntax.html
    	Log::Info("Read Value of AXIO_DIAG_STATUS_REG: {0:#04x}", axioStatusRegister);

    	bRet = true;
    }

	return(bRet);
	}
	
	bool CSampleRuntime::ReadProfinetIOPS(void)
	{
		bool bRet = false;

    uint8_t profinetIOPS = 0;

    // create port names. Format: IOSystem/DeviceNumber.NameOfIO
    String strInPort = Formatter::FormatCommon("{}/9.IPS", ARP_IO_PN);
    String strOutPort = Formatter::FormatCommon("{}/9.OCS", ARP_IO_PN);

    if(ReadInputData(ARP_IO_PN, strInPort, (char*)&profinetIOPS, sizeof(profinetIOPS)))
    {
    	Log::Info("Read Value of {0}: 0x{1:#04x}", strInPort, profinetIOPS);

    	uint8_t profinetIOCS = 0;
		if(ReadInputData(ARP_IO_PN, "Arp.Io.PnC/9.OCS", (char*)&profinetIOCS, sizeof(profinetIOCS)))
		{
			Log::Info("Read Value of {0}: 0x{1:#04x}", strInPort, profinetIOCS);

			bRet = true;
		}
    }

    return(bRet);
	}
	
	// read data from a fieldbus input frame
	bool CSampleRuntime::ReadInputData(const char* fbIoSystemName, const char* portName, char* pValue, size_t valueSize)
	{
		bool bRet = false;

    TGdsBuffer* gdsBuffer = NULL;
    if(ArpPlcIo_GetBufferPtrByPortName(fbIoSystemName, portName, &gdsBuffer))
    {
		size_t offset = 0;
		if(ArpPlcGds_GetVariableOffset(gdsBuffer, portName, &offset))
		{
			// Begin read operation, memory buffer will be locked
			char* readDataBufferPage;
			if(ArpPlcGds_BeginRead(gdsBuffer, &readDataBufferPage))
			{
				// Copy data from gds Buffer
				char* dataAddress = readDataBufferPage + offset;
				memcpy(pValue, dataAddress, valueSize);

				// unlock buffer
				if(ArpPlcGds_EndRead(gdsBuffer))
				{
					bRet = true;
				}
				else
				{
					Log::Error("ArpPlcGds_EndRead failed");
				}
			}
			else
			{
				Log::Error("ArpPlcGds_BeginRead failed");
				ArpPlcGds_EndRead(gdsBuffer);
			}
		}
		else
		{
			Log::Error("ArpPlcGds_GetVariableOffset failed");
		}
    }
    else
    {
    	Log::Error("ArpPlcIo_GetBufferPtrByPortName failed");
    }

    // Release the GdsBuffer and free internal resources
    if(gdsBuffer != NULL)
    {
    	if(!ArpPlcIo_ReleaseGdsBuffer(gdsBuffer))
    	{
    		Log::Error("ArpPlcIo_ReleaseGdsBuffer failed");
    	}
    }

    return(bRet);
	}

	// write data to a fieldbus output frame
	bool CSampleRuntime::WriteOutputData(const char* fbIoSystemName, const char* portName, const char* pValue, size_t valueSize)
	{
		bool bRet = false;

    TGdsBuffer* gdsBuffer = NULL;
    if(ArpPlcIo_GetBufferPtrByPortName(fbIoSystemName, portName, &gdsBuffer))
    {
        size_t offset = 0;
        if(ArpPlcGds_GetVariableOffset(gdsBuffer, portName, &offset))
        {
            // Begin write operation, memory buffer will be locked
            char* dataBufferPage = NULL;
            if(ArpPlcGds_BeginWrite(gdsBuffer, &dataBufferPage))
            {
        		// Copy data to gds Buffer
        		char* dataAddress = dataBufferPage + offset;
        		memcpy(dataAddress, pValue, valueSize);

        		// unlock buffer
        		if(ArpPlcGds_EndWrite(gdsBuffer))
        		{
        			bRet = true;
        		}
        		else
        		{
        			Log::Error("ArpPlcGds_EndWrite failed");
        		}
            }
            else
            {
            	Log::Error("ArpPlcGds_BeginWrite failed");
                ArpPlcGds_EndWrite(gdsBuffer);
            }
        }
        else
        {
        	Log::Error("ArpPlcGds_GetVariableOffset failed");
        }
    }
    else
    {
    	Log::Error("ArpPlcIo_GetBufferPtrByPortName failed");
    }

    // Release the GdsBuffer and free internal resources
    if(gdsBuffer != NULL)
    {
    	if(!ArpPlcIo_ReleaseGdsBuffer(gdsBuffer))
    	{
    		Log::Error("ArpPlcIo_ReleaseGdsBuffer failed");
    	}
    }

    return(bRet);
	}
	
	// callback function for the PLC state
	void PlcOperationHandler(enum PlcOperation operation)
	{
		CSampleRuntime::m_zPLCMode = operation;

	switch (operation)
    {
        break;
        case PlcOperation_Load:
            Log::Info("Call of PLC Load");
            break;
        case PlcOperation_Setup:
        	Log::Info("Call of PLC Setup");
            break;
        case PlcOperation_StartCold:
        	Log::Info("Call of PLC Start Cold");
        	g_pRT->StartProcessing();

        	break;
        case PlcOperation_StartWarm:
        	Log::Info("Call of PLC Start Warm");

        	// when this state-change occurred, the firmware is ready to serve requests.
        	// Now we can request the needed service-interfaces.
        	if(g_pRT->Init() == false)
        	{
        		Log::Error("Error during initialization");
        	}
        	g_pRT->StartProcessing();

        	break;
        case PlcOperation_StartHot:
        	Log::Info("Call of PLC Start Hot");
        	g_pRT->StartProcessing();
            break;
        case PlcOperation_Stop:
        	Log::Info("Call of PLC Stop");
        	g_pRT->StopProcessing();
            break;
        case PlcOperation_Reset:
        	Log::Info("Call of PLC Reset");
        	g_pRT->StopProcessing();
            break;
        case PlcOperation_Unload:
        	Log::Info("Call of PLC Unload");
        	g_pRT->StopProcessing();
            break;
        case PlcOperation_None:
        	Log::Info("Call of PLC None");
        	break;
        default:
        	Log::Error("Call of unknown PLC state");
            break;
    }
	}
```
