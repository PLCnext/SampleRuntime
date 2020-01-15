This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 6 - Creating real-time threads

So far, our application has been doing all its work on a single thread that is running with a "standard" (non-real-time) priority. At the end of each execution cycle, our application sleeps for (approximately) 1 second, giving cycle times that are completely non-deterministic. This is fine for applications with low performance requirements, but many industrial automation applications require deterministic, real-time performance.

PLCnext Control includes the ability to schedule threads with real-time priorities, resulting in deterministic cycle times with very little jitter. This feature is used by the PLCnext Control's own Execution and Synchronisation Manager (ESM), and this feature can also be used in our own application.

Most industrial applications will include real-time and non-real-time threads. Real-time threads should only be used for time-critical functions and - just like in traditional IEC programming - consideration must be given as to whether any operation on a real-time thread will block for long enough to over-run the required cycle time.

Some operations that are not suitable for real-time threads include:
- Access to RSC services on the PLCnext Control.
- File I/O, including calls to the `Arp::Log` function on the PLCnext Control.

In this article, we will move the cyclic I/O processing on to a real-time thread.

1. Modify the file `runtime.cpp` so it looks like the following:
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
   #include <thread>
   #include <pthread.h>

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

   // This function will be executed on a real-time thread
   void* realTimeCycle(void* p)
   {
      using clock = std::chrono::steady_clock;

      // Set the cycle time
      const auto cycle_time = std::chrono::microseconds{100000};
      auto next_cycle = clock::now() + cycle_time;

      // Declare a process data item
      uint8_t value = 0;

      while (true)
      {
         if (processing)
         {
            // Read process inputs
            readInputData((char*)&value, sizeof(value));

            // Perform application-specific processing
            // In this case, simply invert the process data bits
            value = ~value;

            // Write process outputs
            writeOutputData((char*)&value, sizeof(value));
         }

         // Wait for the next cycle
         std::this_thread::sleep_until(next_cycle);
         next_cycle += cycle_time;
      }
   }

   // This function creates a real-time thread and then
   // continues with non-real-time operations
   int main(int argc, char** argv)
   {
      // Variables required to create the real-time thread
      pthread_t realTimeThread;
      struct sched_param param;
      param.sched_priority = 80;
      pthread_attr_t attr;

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

      // Create the real-time thread
      if(pthread_attr_init(&attr) == 0)
      {
         if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) == 0)
         {
            if(pthread_attr_setschedparam(&attr, &param) == 0)
            {
               if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0)
               {
                  // This call will fail due to insufficient permissions,
                  // if the process was not configured with the correct capabilities.
                  // Make sure to set the capabilities on the executable
                  // after *every* deployment to the target!
                  if(pthread_create(&realTimeThread, &attr, realTimeCycle, NULL) != 0)
                  {
                     Log::Error("Error calling pthread_create (realtime thread)");
                  }
               }
               else
               {
                  Log::Error("Error calling pthread_attr_setinheritsched");
               }
            }
            else
            {
               Log::Error("Error calling pthread_attr_setschedparam");
            }
         }
         else
         {
            Log::Error("Error calling pthread_attr_setschedpolicy");
         }
      }
      else
      {
         Log::Error("Error calling pthread_attr_init");
      }

      // Continue with non-real-time operations
      while (true)
      {
         if (initialised)
         {
            // Get the board temperature and write it to file
            rscBoardTemp = deviceStatusService->GetItem("Status.Board.Temperature.Centigrade");
            rscBoardTemp.CopyTo(boardTemp);
            Log::Info("Current PLC board temperature is {0:d}°C", boardTemp);
         }

         // Wait a short time before repeating
         sleep(1);
      }
   }
   ```

   </details>

   Notes on the above code:
   - Real-time operations have been moved in to a separate function called `realTimeCycle`.
   - The real-time thread is created and configured in the `main` function, which then continues with non-real-time operations.
   - Attempting to set the thread priority will fail unless the executable has been configured with the correct capabilities (see steps below).
   - A real-time priority in the range 67 to 82 should be selected to avoid conflicts with the PLCnext runtime. This is the same range used by ESM tasks. 
   - In this example, the real-time thread is executed with a fixed cycle time of 100 milliseconds, entirely independently of the cycle time of the main (non-real-time) thread. Note that it is currently recommended to exchange data with the Axioline GDS buffer no more than once per millisecond.
   - Neither thread safety nor thread synchronisation have been considered in this example. These are important issues that must be considered in real-world projects.

1. Build the project to generate the `runtime` executable.

1. Copy the executable to the PLC.

   ```bash
   scp deploy/AXCF2152_20.0.0.24752/Release/bin/runtime admin@192.168.1.10:~/projects/runtime
   ```

   Note: If you receive a "Text file busy" message in response to this command, then the file is probably locked by the PLCnext Control. In this case, stop the plcnext process on the PLC with the command `sudo /etc/init.d/plcnext stop` before copying the file.

   It is assumed that the ACF config and settings files (described in a previous article) are already on the PLC.

1. Open a secure shell session on the PLC:

   ```bash
   ssh admin@192.168.1.10
   ```

1. Set the capabilites on the executable:

   ```bash
   sudo setcap cap_net_bind_service,cap_net_admin,cap_net_raw,cap_sys_boot,cap_sys_nice,cap_sys_time+ep projects/runtime/runtime
   ```

   This is required for the application to be able to set the real-time thread priority.

1. Restart the plcnext process:

   ```bash
   sudo /etc/init.d/plcnext restart
   ```

1. Run htop to check the priority of all processes.

   ```bash
   htop
   ```

   You should see an instance of your application running with a priority of -81.

---

Copyright © 2020 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
