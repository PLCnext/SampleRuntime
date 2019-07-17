This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 4 - Getting PLCnext Control status via callbacks

It is possible to get notifications of PLCnext Control state changes via a callback function. In this article, we will use these notifications to start and stop process data exchange with the Axioline I/O modules.

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

   #include <syslog.h>
   #include <unistd.h>

   using namespace Arp;

   bool processing = false;

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
            // TODO: request pointers to RSC services, if necessary.
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

   int main()
   {
      // Register the status update callback
      // This is important to get the status of the "firmware ready" event, "PlcOperation_StartWarm"
      ArpPlcDomain_SetHandler(plcOperationHandler);

      // Ask plcnext for access to its services
      // Use syslog for logging until the PLCnext logger is ready
      openlog ("runtime", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
      if (ArpSystemModule_Load("/usr/lib", "runtime", "/opt/plcnext/projects/runtime/runtime.acf.settings") != 0)
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
   - The startup delay timer from the earlier example has been removed.
   - The boolean variable `processing` has been added. This is used to enable and disable I/O processing in the `main` function.
   - A callback function `plcOperationHandler` has been defined, which sets and resets the `processing` flag based on the state of the PLC.
   - The callback function is registered with the PLCnext Control **before** the `ArpSystemModule_Load` function is called. This ensures that all PLC state changes will be captured during startup.

1. Build the project to generate the `runtime` executable.

1. Copy the executable to the PLC.
   ```
   scp deploy/AXCF2152_19.6.0.20989/Release/bin/runtime admin@192.168.1.10:~/projects/runtime
   ```
   Note: If you receive a "Text file busy" message in response to this command, then the file is probably locked by the PLCnext Control. In this case, stop the plcnext process on the PLC with the command `sudo /etc/init.d/plcnext stop` before copying the file.

   It is assumed that the ACF config and settings files (described in a previous article) are already on the PLC.

1. Open a secure shell session on the PLC:
   ```
   ssh admin@192.168.1.10
   ```

1. Restart the plcnext process:
   ```
   sudo /etc/init.d/plcnext restart
   ```

1. Check the log file to see messages for each PLC state change.
   ```
   cat /opt/plcnext/projects/runtime/logs/runtime.log
   ```

---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.