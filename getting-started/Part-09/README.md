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

The [procedure for disabling services](https://www.plcnext-community.net/en/hn-makers-blog/440-how-to-disable-system-components-in-the-plc-for-more-performance.html) is given in the PLCnext Community.

---

Copyright © 2020 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
