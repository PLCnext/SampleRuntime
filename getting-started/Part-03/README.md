This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 3 - Reading and writing Axioline I/O

A requirement of most PLCnext runtime applications is that they must exchange process data with input and output modules on the local Axioline bus.

The Axioline bus is controlled by a PLCnext Control system component, and this Axioline component must be configured using Technology Independent Configuration (.tic) files.

In this article, we will use [PLCnext Engineer](http://phoenixcontact.net/product/1046008) to generate a set of TIC files for a specific arrangement of Axioline I/O modules. Note that it is possible to configure the Axioline bus without using PLCnext Engineer - a description of how to do this is beyond the scope of this series, but will be covered in a future technical article in the PLCnext Community.

1. In PLCnext Engineer, [create a new project](https://youtu.be/I-FeT3p6cGA) that includes:
   - A PLCnext Control with the correct firmware version.
   - Axioline I/O modules corresponding to your physical hardware arrangement.

   Make sure that there are no Programs or Tasks scheduled to run on either ESM1 or ESM2, and no connections in the PLCnext Port List.

1. Download the PLCnext Engineer project to the PLC.

   This creates a set of .tic files on the PLC that results in the automatic configuration of the Axioline bus when the plcnext process starts.

1. Determine I/O port names from the .tic files created by PLCnext Engineer.

   Examine the .tic file(s) in the PLC directory `/opt/plcnext/projects/PCWE/Io/Arp.Io.AxlC`. The files of interest have long, cryptic file names (e.g. `1da1f65d-b76f-4364-bfc4-f59474ccfdad.tic`). In these files, look for elements labelled "IO:Port", and note down the "NodeId", "Name" and "DataType" of each port. These will be needed for our application.

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

   #include <syslog.h>
   #include <unistd.h>

   using namespace Arp;

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

      Log::Info("Hello PLCnext");

      // Wait for Axioline configuration to be completed
      // before attempting to access I/O
      sleep(30);

      // Declare a process data item
      uint8_t value = 0;

      while (true)
      {
         // Read process inputs
         readInputData((char*)&value, sizeof(value));

         Log::Info("Read value of: {0:#04x}", value);

         // Perform application-specific processing
         // In this case, simply invert the process data bits
         value = ~value;

         // Write process outputs
         writeOutputData((char*)&value, sizeof(value));

         // Wait a short time before repeating
         sleep(1);
      }
   }
   ```

   </details>


   Notes on the above code:
   - After the call to `ArpSystemModule_Load`, we must wait for the Axioline bus configuration to be completed before attempting to access I/O data. In this case we use a timer, but there is a smarter way to do this - as we shall see in a later article.
   - In this example, the I/O read and write operations are performed in separate functions.
   - The required format of I/O port names is {Bus Type}/{NodeId}.{Name}, where {Bus Type} = "Arp.Io.AxlC", and {NodeId} and {Name} were obtained from the .tic file on the PLC.
   - In this example, we have hard-coded the I/O details (including the port name) in the read and write functions, but in a real application these types of functions should be general-purpose.
   - The read/write process takes place in a number of steps:
      1. Get a pointer to the Global Data Space (GDS) buffer.
      1. Get the offset to the I/O variable in the GDS.
      1. Lock the GDS buffer for reading or writing.
      1. Copy data to or from the GDS.
      1. Free resources.
   
      Since the layout of the GDS is fixed during the startup of the plcnext process, it is not necessary to repeatedly retrieve references to a given port, as is done in this example. Instead, GDS references could be retrived once for each port, and then cached in a collection. This is demonstrated in the "Sample Runtime" application, later in this series.
   
   An important point to note is that, in this project, I/O data is transferred automatically from the GDS to Axioline I/O modules every 500μs. This should be considered for real-time tasks with very short cycle times (in the order of milliseconds).

1. Modify the relevant section of the CMakeLists.txt file, so it looks like the following:
   ```cmake
   ################# add link targets ####################################################

   find_package(ArpDevice REQUIRED)
   find_package(ArpProgramming REQUIRED)

   target_link_libraries(runtime PRIVATE ArpDevice ArpProgramming
                        Arp.System.ModuleLib Arp.System.Module
                        Arp.Plc.AnsiC)

   #######################################################################################
   ```
   The `Arp.Plc.AnsiC` library implements the ANSI-C function used to access I/O process data.

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
   After approximately 30 seconds, you should see the digital outputs set to the inverse of the digital input values.

---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
