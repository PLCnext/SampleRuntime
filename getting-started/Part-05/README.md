This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 5 - Using RSC Services

There are a number of examples that demonstrate how to subscribe to RSC Services from PLCnext C++ Function Extension components. These components, which inherit from the PLCnext class `ComponentBase`, include a `SubscribeServices` method that is called by the PLCnext Control when it is ready to receive subscriptions to RSC services.

For runtime applications, which do not include a class that inherits from `ComponentBase`, the right time to subscribe to RSC services is when the PLC notifies us of the "Start Warm" state.

The example below subscribes to the "Device Status" RSC service, which provides live values for a number of PLC variables, and uses this service to write the PLC board temperature to the application log file.

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
   #include "Arp/Plc/AnsiC/Gds/DataLayout.h"
   #include "Arp/Plc/AnsiC/Io/FbIoSystem.h"
   #include "Arp/Plc/AnsiC/Io/Axio.h"
   #include "Arp/Plc/AnsiC/Domain/PlcOperationHandler.h"
   #include "Arp/System/Rsc/ServiceManager.hpp"
   #include "Arp/Device/Interface/Services/IDeviceStatusService.hpp"

   #include <syslog.h>
   #include <unistd.h>
   #include <libgen.h>

   using namespace Arp;
   using namespace Arp::System::Rsc;
   using namespace Arp::Device::Interface::Services;
   using namespace Arp::System::Commons::Diagnostics::Logging;

   bool initialised = false;  // The RSC service is available
   bool processing = false;   // Axioline I/O is available

   IDeviceStatusService::Ptr deviceStatusService;  // Reference to the RSC service
   RscVariant<512> rscBoardTemp;                   // The value returned from the RSC Service
   int8 boardTemp;                                 // The value as an integer

   // Initialise PLCnext RSC services
   void initServices()
   {
      if(!initialised)
      {
         Log::Info("Call of initServices");

         deviceStatusService = ServiceManager::GetService<IDeviceStatusService>();

         if(deviceStatusService != NULL)
         {
            Log::Info("Subscribed to Device Status service");
            initialised = true;
         }
         else
         {
            Log::Error("Could not subscribe to Device Status service");
         }
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

   // Read data from a fieldbus input frame
   void readInputData(char* pValue, size_t valueSize)
   {
      TGdsBuffer* gdsBuffer = NULL;

      if(ArpPlcIo_GetBufferPtrByPortName("Arp.Io.AxlC", "Arp.Io.AxlC/0.~DI8", &gdsBuffer))
      {
         size_t offset = 0;
         if(ArpPlcGds_GetVariableOffset(gdsBuffer, "Arp.Io.AxlC/0.~DI8", &offset))
         {
            // Begin read operation, memory buffer will be locked
            char* readDataBufferPage;
            if(ArpPlcGds_BeginRead(gdsBuffer, &readDataBufferPage))
            {
               // Copy data from GDS buffer
               char* dataAddress = readDataBufferPage + offset;
               memcpy(pValue, dataAddress, valueSize);

               // Unlock buffer
               if(!ArpPlcGds_EndRead(gdsBuffer))
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

      // Release the GDS buffer and free internal resources
      if(gdsBuffer != NULL)
      {
         if(!ArpPlcIo_ReleaseGdsBuffer(gdsBuffer))
         {
            Log::Error("ArpPlcIo_ReleaseGdsBuffer failed");
         }
      }
   }

   // Write data to a fieldbus output frame
   void writeOutputData(const char* pValue, size_t valueSize)
   {
      TGdsBuffer* gdsBuffer = NULL;
      if(ArpPlcIo_GetBufferPtrByPortName("Arp.Io.AxlC", "Arp.Io.AxlC/0.~DO8", &gdsBuffer))
      {
         size_t offset = 0;
         if(ArpPlcGds_GetVariableOffset(gdsBuffer, "Arp.Io.AxlC/0.~DO8", &offset))
         {
            // Begin write operation, memory buffer will be locked
            char* dataBufferPage = NULL;
            if(ArpPlcGds_BeginWrite(gdsBuffer, &dataBufferPage))
            {
               // Copy data to GDS buffer
               char* dataAddress = dataBufferPage + offset;
               memcpy(dataAddress, pValue, valueSize);

               // Unlock buffer
               if(!ArpPlcGds_EndWrite(gdsBuffer))
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

      // Release the GDS buffer and free internal resources
      if(gdsBuffer != NULL)
      {
         if(!ArpPlcIo_ReleaseGdsBuffer(gdsBuffer))
         {
            Log::Error("ArpPlcIo_ReleaseGdsBuffer failed");
         }
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

      // Declare a process data item
      uint8_t value = 0;

      while (true)
      {
         if (initialised)
         {
            // Get the board temperature and write it to file
            rscBoardTemp = deviceStatusService->GetItem("Status.Board.Temperature.Centigrade");
            rscBoardTemp.CopyTo(boardTemp);
            Log::Info("Current PLC board temperature is {0:d}°C", boardTemp);
         }

         if (processing)
         {
            // Read process inputs
            readInputData((char*)&value, sizeof(value));

            // Log::Info("Read value of: {0:#04x}", value);

            // Perform application-specific processing
            // In this case, simply invert the process data bits
            value = ~value;

            // Write process outputs
            writeOutputData((char*)&value, sizeof(value));
         }

         // Wait a short time before repeating
         sleep(1);
      }
   }
   ```

   </details>

   Notes on the above code:
   - The boolean variable `initialised` has been added. This is used to enable and disable calls to RSC services in the `main` function.
   - The function `initServices` has been defined, which subscribes to the Device Status RSC service.
   - In the `plcOperationHandler` callback function, the `initServices` function is called from the "Start Warm" case.
   - In the `main` functions infinite `while` loop, the current board PLC temperature is obtained from the Device Status service and written to the application log file.

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

1. Check the log file to see messages containing the current PLC board temperature.

   ```bash
   cat /opt/plcnext/projects/Runtime/logs/Runtime.log
   ```

---

Copyright © 2020-2022 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
