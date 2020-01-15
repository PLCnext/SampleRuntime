This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 2 - PLCnext Control integration

In the previous article, we built a simple "Hello PLCnext" application and started it manually on a PLCnext Control.

In order for a runtime application to access PLCnext Control services like Axioline I/O, a specific set of ARP components must be running. Most of these components are started automatically by the ARP, but two additional components - required for ANSI-C access - must be started in our application process.

Using an ACF configuration file, we will instruct the PLCnext to automatically start and stop our runtime application with the ARP, and to load the required application-specific PLCnext components.

Then, using an ACF settings file, we will specify a directory where we want application log files to be created and stored.

1. In the project's `data` directory, create a file named `runtime.acf.config`, containing the following text:
   <details>
   <summary>(click to see/hide code)</summary>

   ```xml
   <?xml version="1.0" encoding="UTF-8"?>
   <AcfConfigurationDocument
   xmlns="http://www.phoenixcontact.com/schema/acfconfig"
   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
   xsi:schemaLocation="http://www.phoenixcontact.com/schema/acfconfig.xsd"
   schemaVersion="1.0" >

   <Processes>
      <Process name="runtime"
               binaryPath="/opt/plcnext/projects/runtime/runtime"
               workingDirectory="/opt/plcnext/projects/runtime"
               args="runtime.acf.settings"/>
   </Processes>

   <Libraries>
      <Library name="Arp.Plc.AnsiC.Library" binaryPath="$ARP_BINARY_DIR$/libArp.Plc.AnsiC.so" />
   </Libraries>

   <Components>

      <Component name="Arp.Plc.AnsiC" type="Arp::Plc::AnsiC::AnsiCComponent" library="Arp.Plc.AnsiC.Library" process="runtime">
         <Settings path="" />
      </Component>

      <Component name="Arp.Plc.DomainProxy.IoAnsiCAdaption" type="Arp::Plc::Domain::PlcDomainProxyComponent" library="Arp.Plc.Domain.Library" process="runtime">
         <Settings path="" />
      </Component>

   </Components>

   </AcfConfigurationDocument>
   ```

   </details>

   In this configuration file, the `Processes` element tells the Application Component Framework (ACF) to start and stop our `runtime` application with the plcnext process. The `Libraries` and `Components` elements tell the ACF to start special PLCnext Control components that our application will use to (for example) access Axioline I/O through an ANSI-C interface.

1. In the project's `data` directory, create a file named `runtime.acf.settings`, containing the following text:
   <details>
   <summary>(click to see/hide code)</summary>

   ```xml
   <?xml version="1.0" encoding="UTF-8"?>
   <AcfSettingsDocument
   xmlns="http://www.phoenixcontact.com/schema/acfsettings"
   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
   xsi:schemaLocation="http://www.phoenixcontact.com/schema/acfsettings.xsd"
   schemaVersion="1.0" >

   <RscSettings path="/etc/plcnext/device/System/Rsc/Rsc.settings"/>

   <LogSettings logLevel="Debug" logDir="logs" />

   <EnvironmentVariables>
      <EnvironmentVariable name="ARP_BINARY_DIR" value="/usr/lib" /> <!-- Directory of PLCnext binaries -->
   </EnvironmentVariables>

   </AcfSettingsDocument>
   ```

   </details>

   This file gives the ACF additional information in order to run our application.

   The value of the `logDir` attribute in the `LogSettings` element is the path where log files for this application will be created, relative to the application's working directory.

1. In our `runtime.cpp` file, we must implement the following features:
   - Our application must call the PLCnext Control function `ArpSystemModule_Load`, which notifies the ACF that our application has started successfully. Through this call, we also provide the path to our `.acf.settings` file. If this function is not called, our application will not be able to access any PLCnext services, and it will be stopped after a short timeout period.

   - Our application must never end. The PLCnext Control expects our application to keep working until it tells us to stop.

   The modified source file should look like this:
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
   #include <syslog.h>
   #include <unistd.h>
   #include <libgen.h>

   using namespace std;
   using namespace Arp::System::Commons::Diagnostics::Logging;

   int main(int argc, char** argv)
   {
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

      Log::Info("Hello PLCnext");

      while (true)
      {
         // For now, let's catch up on some
         sleep(1);
         // plcnext will kill this process as it shuts down.
      }
   }
   ```

   </details>

   Notes on the above code:
   - The name of the `.acf.settings` file is passed as a command-line argument, and then the absolute path to this file is added.
   - The call to `std::cout` in the earlier example has been replaced with a call to the PLCnext Control function `Log::Info`. This logging function provides formatted output, as we shall see below. 
   - Like all other PLCnext Control functions, the logging function is not available until after the call to `ArpSystemModule_Load`, and until then we use `syslog` to log messages.
   - We have included an infinite loop that simply sleeps for 1 second while waiting for the PLCnext Control to kill the runtime process.

1. Modify the relevant section of the CMakeLists.txt file, so it looks like the following:

   ```cmake
   ################# add link targets ####################################################

   find_package(ArpDevice REQUIRED)
   find_package(ArpProgramming REQUIRED)

   target_link_libraries(runtime PRIVATE ArpDevice ArpProgramming
                         Arp.System.ModuleLib Arp.System.Module)

   #######################################################################################
   ```

   The two Arp.System libraries implement the `ArpSystemModule_Load` function.

1. Build the project to generate the `runtime` executable.

1. Deploy the executable to the PLC.

   ```bash
   scp deploy/AXCF2152_20.0.0.24752/Release/bin/runtime admin@192.168.1.10:~/projects/runtime
   ```

   Note: If you receive a "Text file busy" message in response to this command, then the file is probably locked by the PLCnext Control. In this case, stop the plcnext process on the PLC with the command `sudo /etc/init.d/plcnext stop` before copying the file.

1. Deploy the `runtime.acf.settings` file to your project directory on the PLC.

   ```bash
   scp data/runtime.acf.settings admin@192.168.1.10:~/projects/runtime
   ```

   The destination directory is the one we specified in the call to `ArpSystemModule_Load`.

1. Deploy the `runtime.acf.config` file to the PLC's `Default` project directory.

   ```bash
   scp data/runtime.acf.config admin@192.168.1.10:~/projects/Default
   ```

   All `.acf.config` files in this directory are processed by the ACF when the plcnext process starts up, via the `Default.acf.config` file in the same directory.

1. Open a secure shell session on the PLC:

   ```bash
   ssh admin@192.168.1.10
   ```

1. Create the `logs` directory for our application:

   ```bash
   mkdir /opt/plcnext/projects/runtime/logs
   ```

1. Restart the plcnext process:

   ```bash
   sudo /etc/init.d/plcnext restart
   ```

1. Give the plcnext process a short time to start our application, and then check that our application has started successfully by examining the plcnext log file:

   ```bash
   cat /opt/plcnext/logs/Output.log | grep runtime
   ```

   The result should be something like:

   ```text
   15.07.19 11:13:04.306 Arp.System.Acf.Internal.Sm.ProcessesController               INFO  - Process 'runtime' started successfully.
   15.07.19 11:13:06.498 Arp.System.Acf.Internal.Sm.ProcessesController               INFO  - Library 'Arp.Plc.AnsiC.Library' in process 'runtime' loaded.
   15.07.19 11:13:06.506 Arp.System.Acf.Internal.Sm.ProcessesController               INFO  - Library 'Arp.Plc.Domain.Library' in process 'runtime' loaded.
   15.07.19 11:13:06.702 Arp.System.Acf.Internal.Sm.ComponentsController              INFO  - Component 'Arp.System.UmRscAuthorizator.runtime' in process 'MainProcess' created.
   15.07.19 11:13:06.707 Arp.System.Acf.Internal.Sm.ComponentsController              INFO  - Component 'Arp.Plc.AnsiC' in process 'runtime' created.
   15.07.19 11:13:06.712 Arp.System.Acf.Internal.Sm.ComponentsController              INFO  - Component 'Arp.Plc.DomainProxy.IoAnsiCAdaption' in process 'runtime' created.
   ```

1. Check the contents of the application log file:

   ```bash
   cat /opt/plcnext/projects/runtime/logs/runtime.log
   ```

   You should see a number of formatted output messages, including a message similar to the following:

   ```text
   16.07.19 04:19:23.355 root     INFO    - Hello PLCnext
   ```

---

Copyright © 2020 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
