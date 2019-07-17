This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control.

## Part 9 - Disable unnecessary system services

There are many services enabled by default on a PLCnext Control, however a particular runtime application may only use a small number of these services. For example, both Profinet and Ethernet/IP™ fieldbus services are enabled by default on an AXC F 2152 and, while it is not necessary to disable them, it does save some system resources if these services are not started.

This procedure shows how to disable any or all of the following four services on a PLCnext Control:

| Service      | Required for                                          |
| ------------ | ----------------------------------------------------- |
| Profinet     | Communication to Profinet controllers and/or devices. |
| Ethernet/IP™ | Communication to Ethernet/IP™ scanners.               |
| HMI          | Browser-based HMI hosted from the PLCnext Control     |
| OPC-UA       | Communication with OPC-UA clients                     |

---

**IMPORTANT NOTE**

Disabling services on a PLCnext Control is done at the users own risk.

Phoenix Contact performs all system tests with the default configuration. Any changes to this configuration may adversely affect the stability of the system.

Users should always perform complete and thorough tests on their PLCnext Control application to ensure the required performance level.

---

### Procedure

1. Create a file with the extension `.acf.config`, containing the following text:

    ```xml
    <?xml version="1.0" encoding="UTF-8"?>
    <AcfSettingsDocument
    xmlns="http://www.phoenixcontact.com/schema/acfsettings"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://www.phoenixcontact.com/schema/acfsettings.xsd"
    schemaVersion="1.0" >

    <EnvironmentVariables>
        <EnvironmentVariable name="ARP_PROFINET_SUPPORT" value="false" overridden="true" />
        <EnvironmentVariable name="ARP_ETHERNETIP_SUPPORT" value="false" overridden="true" />
        <EnvironmentVariable name="ARP_EHMI_SUPPORT" value="false" overridden="true" />
        <EnvironmentVariable name="ARP_OPC_UA_SUPPORT" value="false" overridden="true" />
    </EnvironmentVariables>

    </AcfSettingsDocument>
    ```

    In this configuration file:
    - Set any or all of the `value` attributes to `false` to disable the corresponding service.
    - Since all the services in this example are enabled by default, it is not strictly necessary to include a line with `value="true"`. However, if your application relies on one of these services to be running, then you should explicitly enable it here, to ensure that the service is not disabled by another application. 

1. Copy this new file to the PLC directory `/opt/plcnext/appshome`

1. Restart the plcnext process:
   ```
   sudo /etc/init.d/plcnext restart
   ```
The disabled PLCnext Control services will not be started. If Profinet is disabled on an AXC F 2152, the red BF-C and BF-D LEDs will not be lit.

---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.