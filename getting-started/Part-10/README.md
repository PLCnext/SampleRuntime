This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 10 - OPC UA

OPC UA is the industry standard for exchanging non-real-time data between industrial automation software components. It would therefore be useful if data in a PLCnext runtime application could be accessed via an OPC UA server.

An OPC UA server for a custom runtime application could be implemented in a number of ways. However, since there is already a full-featured OPC UA server included with every PLCnext Control, it makes sense to use this OPC UA server for applications running on the same PLC.

This article provides a procedure for the following two tasks:

1. Creating custom data items in the OPC UA server address space.

1. Exchanging data between the runtime and custom data items in the OPC UA server address space.

### Technical Background

On a PLCnext Control, the OPC UA server address space is populated by the Program Library Manager (PLM) using information in `.*meta` files associated with PLM components and programs. Since we cannot interact with the PLM directly from a runtime application, we must create a separate shared object library containing a PLM component that defines data items from our runtime application that should appear in the OPC UA server address space. We must then instruct the PLCnext runtime to create the PLM component at startup.

A PLM component that populates the OPC UA address space also creates a GDS port variable for each OPC UA data item that it creates. Each of these GDS port variable is linked to its associated OPC UA data item, so applications that can read and write GDS variables can use these variables to (indirectly) read and write the corresponding OPC UA data items.

The PLCnext Command-Line Interface (CLI) includes a template for a PLM project, so we will use this as the starting point in the creation of a PLM component that we will use to create OPC UA data items and their associated GDS variables.

### Procedure

1. Using the PLCnext CLI, create a new project based on the standard template and set the target for the build. In this example, the project is called `RuntimeOpc`, but you can call it whatever you want.
   
   ```
   plcncli new project -n RuntimeOpc
   cd RuntimeOpc
   plcncli set target --add -n AXCF2152 -v 2020.0
   ```

1. The CLI default project includes source files for a real-time C++ program. This example does not use real-time PLM programs, and so these program source files can be deleted.

   ```
   cd src
   rm RuntimeOpcProgram.*
   ```

1. Open the Component .hpp file in your favourite editor, and add the following GDS Port definition in the section indicated by the auto-generated comment:

   ```cpp
    //#attributes(Hidden|Opc)
    struct DATA {
        //#attributes(Input)
        int32 Runtime_RW = 0;
        //#attributes(Output)
        int32 Runtime_RO = 0;
    };

    //#port
    DATA data;
   ```

   This will create two OPC UA data items; one read/write (Input) and one read-only (Output), each holding an integer value. Corresponding GDS variables will also be created using this information.

   This `struct` can include any number of elements. The data type of each element must be taken from the list in Appendix A of the PLCnext Technology User Manual (version 2019.6).

1. Auto-generate the remaining C++ source code, and configuration (`*.meta`) files, for the project. From the root directory of the RuntimeOpc project:

   ```
   plcncli generate code
   plcncli generate config
   ```

1. Build the project. From the root directory of the project:

   ```
   plcncli build
   ```

1. In the project root directory, create a new directory called `data`.

1. In the `data` directory, create a new file named `RuntimeOpc.plm.config`, containing the following text:

   ```xml
    <?xml version="1.0" encoding="utf-8"?>
    <PlmConfigurationDocument xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" schemaVersion="1.3" xmlns="http://www.phoenixcontact.com/schema/plmconfig">
    <Libraries>
        <Library name="RuntimeOpc" binaryPath="/opt/plcnext/projects/RuntimeOpc/libRuntimeOpc.so" />
    </Libraries>
    <Components>
        <Component name="Runtime" type="RuntimeOpc.RuntimeOpcComponent" library="RuntimeOpc">
        <Settings path="" />
        </Component>
    </Components>
    </PlmConfigurationDocument>
   ```

   This instructs the PLCnext runtime to create one instance of the RuntimeOpcComponent component, called Runtime, from the RuntimeOpc shared object library that we have just built.

1. Download the application to the PLC. From the project root directory copy the following files:

   - Shared object library containing the PLM component.

   ```
   ssh admin@192.168.1.10 'mkdir -p projects/RuntimeOpc'
   scp bin/AXCF2152_20.0.0.24752/Release/lib/libRuntimeOpc.so admin@192.168.1.10:~/projects/RuntimeOpc
   ```

   - PLM metadata files.

   ```
   scp -r intermediate/config/* admin@192.168.1.10:~/projects/RuntimeOpc
   ```

   - PLM configuration file.

   ```
   scp data/RuntimeOpc.plm.config admin@192.168.1.10:~/projects/Default/Plc/Plm
   ```

1. On the PLC, edit the file `/opt/plcnext/projects/Default/Plc/Plm/Plm.config`. In the `<Includes>` section, do not delete or change any existing lines, but add the following new lines:
   ```xml
   <!-- Include all other plm.config files in this folder -->
   <Include path="*.plm.config" />
   ```

1. On the PLC, edit the file `/opt/plcnext/projects/PCWE/Services/OpcUA/PCWE.opcua.config`. In the `<GdsPortsToProvide>` section, change the value `<None />` to `<All />`. That section of the file should then look like:

   ```xml
   <GdsPortsToProvide>
     <All />
   </GdsPortsToProvide>
   ```

1. Restart the PLCnext runtime:

   ```
   sudo /etc/init.d/plcnext restart
   ```

1. In an OPC UA client like UaExpert from Unified Automation:
   - In the Address Space pane, open the branch PLCnext -> Runtime
   - Add the two new data items to the Data Access View pane.
   - Attempt to change the value of both data items. It is not possible to change the value of the Output (read-only) data item.

These OPC UA variables can now be used by the runtime application.

### Next steps

GDS variables corresponding to the new OPC UA data items have been created and are available to any application that can access the Global Data Space. The runtime application can exchange data with these GDS variables using the "Data Access" and/or "Subscription" RSC services.

---

Copyright © 2020 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
