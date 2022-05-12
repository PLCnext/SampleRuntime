This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 2 - PLCnext Control integration

In the previous article, a simple "Hello World" application was run on a PLCnext Control device.

In order for a runtime application to access PLCnext Control services like Axioline I/O, a specific set of Automation Runtime Platform (ARP - i.e. PLCnext) components must be running. Most of these components are started automatically by the ARP, but two additional components - required for ANSI-C access - must be started in our application process.

Using an Application Component Framework (ACF) configuration file, we will instruct the PLCnext to automatically start and stop our runtime application with the ARP, and to load the required application-specific PLCnext components.

Then, using an ACF settings file, we will specify a directory where we want application log files to be created and stored.

The required files will be created automatically using a new PLCnext CLI project template.

1. Install the "runtime" PLCnext CLI project template

   Copy the `RuntimeTemplate` directory and its contents from this repository to the PLCnext CLI Templates directory on your development machine, e.g.

   ```bash
   cp -r RuntimeTemplate ~/plcncli/Templates
   ```

1. Include the new template in a CLI Templates file

   For example, create a file called `CustomTemplates.xml` in the PLCnext CLI `Templates` directory, containing the following:

   ```xml
   <?xml version="1.0" encoding="utf-8"?>
   <Templates xmlns="http://www.phoenixcontact.com/schema/clitemplates">
	   <Include type="Template">RuntimeTemplate/TemplateDescription.xml</Include>
   </Templates>
   ```

1. If a new Templates file was created, register it with the PLCnext CLI

   ```bash
   plcncli set setting TemplateLocations ./Templates/CustomTemplates.xml --add
   ```

1. Check that the new template has been installed correctly.

   ```bash
   plcncli new
   ```

   You should see `runtime_project` listed as an option: 
   ```text
   plcncli 22.0.0 LTS (22.0.0.952)
   Copyright (c) 2018 PHOENIX CONTACT GmbH & Co. KG

   ERROR(S):
     No verb selected.
      :
     runtime_project      Create a new runtime project.
      :
   ```

1. Create a new project called `Runtime`

   ```bash
   cd ~
   plcncli new runtime_project --name Runtime
   ```

   This will automatically create a number of files, including:

   **data/Runtime.acf.config**

   In this configuration file, the `Processes` element tells the Application Component Framework (ACF) to start and stop our `Runtime` application with the plcnext process. The `Libraries` and `Components` elements tell the ACF to start special PLCnext Control components that our application will use to (for example) access Axioline I/O through an ANSI-C interface.

   **data/Runtime.acf.settings**

   This file gives the ACF additional information in order to run our application.

   The value of the `logDir` attribute in the `LogSettings` element is the path where log files for this application will be created, relative to the application's working directory.

   **src/Runtime.cpp**

   - Runtime applications must call the PLCnext Control function `ArpSystemModule_Load`, which notifies the ACF that the runtime application has started successfully. Through this call, the application also provides the path to an `.acf.settings` file. If this function is not called, the runtime application will not be able to access any PLCnext services, and it will be stopped after a short timeout period.

   - The template assumes that the name of the `.acf.settings` file is passed as a command-line argument. The absolute path to this file is added by the code.

   - Runtime applications must never end. The PLCnext Control expects runtime applications to keep working until it tells them to stop. This template includes an infinite loop that simply sleeps for 1 second while waiting for the PLCnext Control to kill the runtime process.

   - Like all other PLCnext Control functions, the logging function is not available until after the call to `ArpSystemModule_Load`, and until then this template uses `syslog` to log messages.

1. Set the application target(s)

   ```bash
   cd ~/Runtime
   plcncli set target --name AXCF2152 --version 2022.0 --add
   ```

1. Build the project to generate the `Runtime` executable.

   ```bash
   plcncli build
   ```

1. Deploy the executable to the PLC.

   ```bash
   ssh admin@192.168.1.10 'mkdir -p projects/Runtime'
   scp bin/AXCF2152_22.0.4.144/Release/Runtime admin@192.168.1.10:~/projects/Runtime
   ```

   Note: If you receive a "Text file busy" message in response to this command, then the file is probably locked by the PLCnext Control. In this case, stop the plcnext process on the PLC with the command `sudo /etc/init.d/plcnext stop` before copying the file.

1. Deploy the `Runtime.acf.settings` file to your project directory on the PLC.

   ```bash
   scp data/Runtime.acf.settings admin@192.168.1.10:~/projects/Runtime
   ```

   The destination directory is the one we specified in the call to `ArpSystemModule_Load`.

1. Deploy the `Runtime.acf.config` file to the PLC's `Default` project directory.

   ```bash
   scp data/Runtime.acf.config admin@192.168.1.10:~/projects/Default
   ```

   All `.acf.config` files in this directory are processed by the ACF when the plcnext process starts up, via the `Default.acf.config` file in the same directory.

1. Open a secure shell session on the PLC:

   ```bash
   ssh admin@192.168.1.10
   ```

1. Create the `logs` directory for our application:

   ```bash
   mkdir /opt/plcnext/projects/Runtime/logs
   ```

1. Restart the plcnext process:

   ```bash
   sudo /etc/init.d/plcnext restart
   ```

1. Give the plcnext process a short time to start our application, and then check that our application has started successfully by examining the plcnext log file:

   ```bash
   cat /opt/plcnext/logs/Output.log | grep Runtime
   ```

   The result should be something like:

   ```text
   12.05.22 13:35:28.218 Arp.System.Acf.Internal.Sm.ProcessesController               INFO  - Process 'Runtime' started successfully.
   12.05.22 13:35:33.577 Arp.System.Acf.Internal.Sm.ProcessesController               INFO  - Library 'Arp.Plc.AnsiC.Library' in process 'Runtime' loaded.
   12.05.22 13:35:33.583 Arp.System.Acf.Internal.Sm.ProcessesController               INFO  - Library 'Arp.Plc.Domain.Library' in process 'Runtime' loaded.
   12.05.22 13:35:33.594 Arp.System.Acf.Internal.Sm.ProcessesController               INFO  - Library 'Arp.System.UmRscAuthorizator.Library' in process 'Runtime' loaded.
   12.05.22 13:35:33.734 Arp.System.Acf.Internal.Sm.ComponentsController              INFO  - Component 'Arp.Plc.AnsiC' in process 'Runtime' created.
   12.05.22 13:35:33.735 Arp.System.Acf.Internal.Sm.ComponentsController              INFO  - Component 'Arp.Plc.DomainProxy.IoAnsiCAdaption' in process 'Runtime' created.
   12.05.22 13:35:33.737 Arp.System.Acf.Internal.Sm.ComponentsController              INFO  - Component 'Arp.System.UmRscAuthorizator@Runtime' in process 'Runtime' created.
   ```

1. Check the contents of the application log file:

   ```bash
   cat /opt/plcnext/projects/Runtime/logs/Runtime.log
   ```

   You should see a number of formatted output messages, including a message similar to the following:

   ```text
   12.05.22 13:35:27.719 root     INFO    - Hello PLCnext
   ```

---

Copyright © 2020-2022 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
