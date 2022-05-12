This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 8 - Configure Axioline I/O modules

This example extends the example in [Part 5](../Part-05/README.md) of this series, which used the "Device Status" RSC service to write the PLC board temperature to the application log file.

The "Acyclic Communication" RSC service can be used to read and write configuration information to/from individual Axioline I/O modules. For example, Axioline digital input modules have an adjustable "Filter time" parameter, and Axioline serial communication modules have a configurable protocol (RS-232, 422, 485), baud rate, etc.

There are at least two other ways to write configuration data to Axioline I/O modules:
   - Use the module "Settings" page in PLCnext Engineer.
   - Connect the I/O module to an Axioline Bus Coupler, and use Startup+ software to write configuration data to the module.

Configuration data is retained in the I/O module's solid-state memory, even after power is removed.

### Procedure

This example shows how to read and write configuration information to the serial communication module "AXL F RS UNI 1H" (order number 2688666).

Before using the Acyclic Communication service, please note the following:

- **Do not** write configuration data to an Axioline I/O module more times than is necessary. For example, do not write configuration data on every execution of a cyclic program. Like all solid state memory, the memory in Axioline modules has the capacity for a finite number of read/write cycles.

To configure an Axioline serial communication module from a runtime application:

1. Connect a "AXL F RS UNI 1H" module to the PLCnext Control. In this example, this module must be the first one (i.e. left-most) on the Axioline bus.

1. In the PLCnext Engineer project (prepared in [Part 3](../Part-03/README.md) of this series), replace all the Axioline I/O modules with a single "AXL F RS UNI 1H" module, and download the project to the PLC.

1. Modify the file `Runtime.cpp` so it looks like the following:
   <details>
   <summary>(click to see/hide code)</summary>

   ```cpp
   //
   // Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
   // Licensed under the MIT. See LICENSE file in the project root for full license information.
   // SPDX-License-Identifier: MIT
   //
   #include "Arp/System/ModuleLib/Module.h"
   #include "Arp/System/Commons/Logging.h"
   #include "Arp/Plc/AnsiC/Domain/PlcOperationHandler.h"
   #include "Arp/System/Rsc/ServiceManager.hpp"
   #include "Arp/Io/Axioline/Services/IAcyclicCommunicationService.hpp"
   #include "Arp/Io/Axioline/Services/PdiParam.hpp"
   #include "Arp/Io/Axioline/Services/PdiResult.hpp"

   #include <syslog.h>
   #include <libgen.h>

   using namespace Arp;
   using namespace Arp::System::Rsc;
   using namespace Arp::Io::Axioline::Services;
   using namespace Arp::System::Commons::Diagnostics::Logging;

   bool initialised = false;  // The RSC service is available
   bool configured = false;   // The serial module is configured
   bool processing = false;   // Axioline I/O is available

   IAcyclicCommunicationService::Ptr acyclicCommunicationService;  // Reference to the RSC service
   PdiParam pdiParam;            // PDI parameters
   vector<uint8> writeData(16);  // Data written to the serial module
   vector<uint8> readData;   // Data read from the serial module
   PdiResult transferResult;     // Result of the PDI data transfer

   // Configure serial module
   void configSerial()
   {
      Log::Info("Call of configSerial");

      if(acyclicCommunicationService != NULL)
      {
         // The basic communication settings - baud rate, data bits, parity and
         // stop bits - are written to an AXL F RS UNI 1H module installed on the local Axioline bus.

         // Module access data.
         pdiParam.Slot = 1;  // Serial module is the first module on the Axioline bus
         pdiParam.Subslot = 0;
         pdiParam.Index = 0x0080; // Parameter table
         pdiParam.Subindex = 0;

         // Serial module configuration data.
         // For the meaning of configuration codes, refer to data sheet 8533_en_04 "AXL F RS UNI 1H", pp 26-30.
         writeData[0] = 0x40; // DTR control via Process Data; RS-232 interface; Transparent protocol.
         writeData[1] = 0x74; // Baud rate (9600), Data bits (8), Parity (None) and Stop bits (1).
         writeData[2] = 0x0D; // First delimiter = CR.
         writeData[3] = 0x0A; // Second delimiter = LF.
         writeData[4] = 0x24; // If a character is received with an error (e.g. parity error), the character '$' is stored.
         writeData[5] = 0x00; // Not used.
         writeData[6] = 0x00; // Not used.
         writeData[7] = 0x00; // Not used.
         writeData[8] = 0x00; // Reserved.
         writeData[9] = 0x00; // Data exchange via Process Data.
         writeData[10] = 0x00; // Lead time.
         writeData[11] = 0x00; // Lag time.
         writeData[12] = 0x00; // Reserved.
         writeData[13] = 0x00; // Reserved.
         writeData[14] = 0x00; // Reserved.
         writeData[15] = 0x00; // Reserved.

         // Write configuration data to the serial module
         transferResult = acyclicCommunicationService->PdiWrite(pdiParam, writeData);

         if (transferResult.ErrorCode == 0)
         {
            Log::Info("Successfully configured serial module");
            configured = true;

            // Read configuration data back from the module
            transferResult = acyclicCommunicationService->PdiRead(pdiParam, readData);

            if (transferResult.ErrorCode == 0)
            {
               if (readData.size() != 16)
               {
                  Log::Error("Unexpected response from serial module. Expected 16 bytes, got {0}", readData.size());
               }
               else
               {
                  Log::Info("Configuration: 0x{0:04X}, 0x{1:04X}, 0x{2:04X}, 0x{3:04X}, ...", readData[0], readData[1], readData[2], readData[3]);
               }
            }
            else
            {
            // For the meaning of error codes, refer to user manual 8663_en_03 "AXL F SYS DIAG", Table 2-3.
            Log::Error("Could not read configuration from serial module. Error code: 0x{0:04X} Additional info: 0x{1:04X}", transferResult.ErrorCode, transferResult.AddInfo);
            configured = false;
            }
         }
         else
         {
            // For the meaning of error codes, refer to user manual 8663_en_03 "AXL F SYS DIAG", Table 2-3.
            Log::Error("Could not configure serial module. Error code: 0x{0:04X} Additional info: 0x{1:04X}", transferResult.ErrorCode, transferResult.AddInfo);
            configured = false;
         }
      }
      else
      {
         Log::Error("Could not configure serial module - RSC service not available");
         configured = false;
      }
   }

   // Initialise PLCnext RSC services
   void initServices()
   {
      if(!initialised)
      {
         Log::Info("Call of initServices");

         acyclicCommunicationService = ServiceManager::GetService<IAcyclicCommunicationService>();

         if(acyclicCommunicationService != NULL)
         {
            Log::Info("Subscribed to Acyclic Communication service");
            initialised = true;
         }
         else
         {
            Log::Error("Could not subscribe to Acyclic Communication service");
         }

         configSerial();
      }
   }

   // Callback function for the PLC state
   void plcOperationHandler(enum PlcOperation operation)
   {
      switch (operation)
      {
         case PlcOperation_Load:
            Log::Info("Call of PLC Load");
            break;

         case PlcOperation_Setup:
            Log::Info("Call of PLC Setup");
            break;

         case PlcOperation_StartCold:
            Log::Info("Call of PLC Start Cold");
            processing = true;
            break;

         case PlcOperation_StartWarm:
            Log::Info("Call of PLC Start Warm");
            // When this state-change occurs, the firmware is ready to serve requests.
            initServices();
            processing = true;
            break;

         case PlcOperation_StartHot:
            Log::Info("Call of PLC Start Hot");
            processing = true;
            break;

         case PlcOperation_Stop:
            Log::Info("Call of PLC Stop");
            processing = false;
            break;

         case PlcOperation_Reset:
            Log::Info("Call of PLC Reset");
            processing = false;
            break;

         case PlcOperation_Unload:
            Log::Info("Call of PLC Unload");
            processing = false;
            break;

         case PlcOperation_None:
            Log::Info("Call of PLC None");
            break;

         default:
            Log::Error("Call of unknown PLC state");
            break;
      }
   }

   int main(int argc, char** argv)
   {
      // Register the status update callback
      // This is important to get the status of the "firmware ready" event, "PlcOperation_StartWarm"
      ArpPlcDomain_SetHandler(plcOperationHandler);

      // Ask plcnext for access to its services
      // Use syslog for logging until the PLCnext logger is ready
      openlog ("runtime", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

      // Process command line arguments
      string acfSettingsRelPath("");

      if(argc != 2)
      {
         syslog (LOG_ERR, "Invalid command line arguments. Only relative path to the acf.settings file must be passed");
         return -1;
      }
      else
      {
         acfSettingsRelPath = argv[1];
         syslog(LOG_INFO, string("Arg Acf settings file path: " + acfSettingsRelPath).c_str());
      }

      char szExePath[PATH_MAX];
      ssize_t count = readlink("/proc/self/exe", szExePath, PATH_MAX);
      string strDirPath;
      if (count != -1) {
         strDirPath = dirname(szExePath);
      }
      string strSettingsFile(strDirPath);
         strSettingsFile += "/" + acfSettingsRelPath;
      syslog(LOG_INFO, string("Acf settings file path: " + strSettingsFile).c_str());

      // Intialize PLCnext module application
      // Arguments:
      //  arpBinaryDir:    Path to Arp binaries
      //  applicationName: Arbitrary Name of Application
      //  acfSettingsPath: Path to *.acf.settings document to set application up
      if (ArpSystemModule_Load("/usr/lib", "runtime", strSettingsFile.c_str()) != 0)
      {
         syslog (LOG_ERR, "Could not load Arp System Module");
         return -1;
      }
      syslog (LOG_INFO, "Loaded Arp System Module");
      closelog();

      // Cyclic processing
      while (true)
      {
         if (processing)
         {
            // Exchange data with serial module here
         }
         // Wait a short time before repeating
         sleep(1);
      }
   }
   ```

   </details>

   Notes on the above code:
   - This example uses the blocking functions `PdiRead` and `PdiWrite`. It is not recommended to call these functions from a high-frequency cyclic task (even once!), otherwise the required cycle time is likely to be exceeded.
   - In this example, after the  serial communication module is configured, no process data is exchanged with the module (this is a necessary requirement for a practical serial communication application). Process data exchange would typically take place an endless loop, for example in the `while(true)` loop in the `main` function.

1. Build the project to generate the `Runtime` executable.

   ```bash
   plcncli build
   ```

1. Deploy the executable to the PLC.

   ```bash
   scp bin/AXCF2152_22.0.4.144/Release/Runtime admin@192.168.1.10:~/projects/Runtime
   ```

   Note: If you receive a "Text file busy" message in response to this command, then the file is probably locked by the PLCnext Control. In this case, stop the plcnext process on the PLC with the command `sudo /etc/init.d/plcnext stop` before copying the file.

   It is assumed that the ACF config and settings files (described in a previous article) are already on the PLC.

1. Open a secure shell session on the PLC:

   ```bash
   ssh admin@192.168.1.10
   ```

1. Restart the plcnext process:

   ```bash
   sudo /etc/init.d/plcnext restart
   ```

1. Check the log file. You should see messages similar to the following, indicating successful writing and reading of configuration data:

   ```text
   23.07.19 06:02:12.466 root   INFO    - Subscribed to Acyclic Communication service
   23.07.19 06:02:12.481 root   INFO    - Successfully configured serial module
   23.07.19 06:02:12.494 root   INFO    - Configuration: 0x0040, 0x0074, 0x000D, 0x000A, ...
   ```

1. On the serial communication module, the orange LED labelled "33" should be illuminated, indicating that the module is configured for RS-232 communication.

---

Copyright © 2020-2022 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
