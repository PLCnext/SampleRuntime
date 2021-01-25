This is part of a [series of articles](https://github.com/PLCnext/SampleRuntime) that demonstrate how to create a runtime application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 1 - Hello PLCnext

In this article, we will create the structure of the project that we will use throughout this series. We will then create a simple "Hello PLCnext" application, and run it on a PLCnext Control.

We will call our project `runtime`, but you can call it whatever you want.

1. On your Linux host, create a root directory for your project:

   ```bash
   mkdir runtime && cd runtime
   ```

1. Create project sub-directories:

   ```bash
   mkdir src data tools
   ```

1. Start Visual Studio Code (or your IDE of choice):

   ```bash
   code .
   ```

1. In the project's `src` directory, create a file named `runtime.cpp`, containing the following text:

   ```cpp
   //
   // Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
   // Licensed under the MIT. See LICENSE file in the project root for full license information.
   // SPDX-License-Identifier: MIT
   //

   #include <iostream>  

   using namespace std;

   int main()
   {
      cout<<"Hello PLCnext";
      return 0;
   }
   ```

1. In the project's root directory, create a file named `CMakeLists.txt`, containing the following text:
   <details>
   <summary>(click to see/hide code)</summary>

   ```cmake
   cmake_minimum_required(VERSION 3.13)

   project(runtime)

   if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
   endif()

   ################# create target #######################################################

   set (WILDCARD_SOURCE *.cpp)
   set (WILDCARD_HEADER *.h *.hpp *.hxx)

   file(GLOB_RECURSE Headers src/${WILDCARD_HEADER})
   file(GLOB_RECURSE Sources src/${WILDCARD_SOURCE})
   add_executable(runtime ${Headers} ${Sources})

   #######################################################################################

   ################# project include-paths ###############################################

   target_include_directories(runtime
      PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

   #######################################################################################

   ################# include arp cmake module path #######################################

   list(INSERT CMAKE_MODULE_PATH 0 "${ARP_TOOLCHAIN_CMAKE_MODULE_PATH}")

   #######################################################################################

   ################# set link options ####################################################
   # WARNING: Without --no-undefined the linker will not check, whether all necessary    #
   #          libraries are linked. When a library which is necessary is not linked,     #
   #          the firmware will crash and there will be NO indication why it crashed.    #
   #######################################################################################

   target_link_options(runtime PRIVATE LINKER:--no-undefined)

   #######################################################################################

   ################# add link targets ####################################################

   find_package(ArpDevice REQUIRED)
   find_package(ArpProgramming REQUIRED)

   target_link_libraries(runtime PRIVATE ArpDevice ArpProgramming)

   #######################################################################################

   ################# install ############################################################

   string(REGEX REPLACE "^.*\\(([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$" "\\1" _ARP_SHORT_DEVICE_VERSION ${ARP_DEVICE_VERSION})
   install(TARGETS runtime
      LIBRARY DESTINATION ${ARP_DEVICE}_${_ARP_SHORT_DEVICE_VERSION}/$<CONFIG>/lib
      ARCHIVE DESTINATION ${ARP_DEVICE}_${_ARP_SHORT_DEVICE_VERSION}/$<CONFIG>/lib
      RUNTIME DESTINATION ${ARP_DEVICE}_${_ARP_SHORT_DEVICE_VERSION}/$<CONFIG>/bin)
   unset(_ARP_SHORT_DEVICE_VERSION)

   #######################################################################################
   ```

   </details>

1. In the project's `tools` directory, create a file named `build-runtime.sh`, containing the following text:
   <details>
   <summary>(click to see/hide code)</summary>

   ```bash
   #!/bin/bash
   #//
   #// Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
   #// Licensed under the MIT. See LICENSE file in the project root for full license information.
   #// SPDX-License-Identifier: MIT
   #//
   while getopts t:a:n: option
   do
   case "${option}"
   in
   t) TOOLCHAIN=${OPTARG};;
   a) ARPVERSION=${OPTARG};;
   n) TARGETNAME=${OPTARG};;
   esac
   done

   VERSION=$(echo $ARPVERSION | grep -oP [0-9]+[.][0-9]+[.][0-9]+[.][0-9]+)
   echo "Version:${VERSION}"

   # Get the directory of this script
   DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

   echo CMAKE Configure
   cmake --configure -G "Ninja" \
   -DBUILD_TESTING=OFF \
   -DUSE_ARP_DEVICE=ON \
   -DCMAKE_STAGING_PREFIX="${DIR}/../deploy/" \
   -DCMAKE_INSTALL_PREFIX=/usr/local \
   -DCMAKE_PREFIX_PATH="${DIR}/../deploy/${TARGETNAME}_${VERSION}" \
   -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
   "-DARP_TOOLCHAIN_ROOT=${TOOLCHAIN}" \
   "-DARP_DEVICE=${TARGETNAME}" \
   "-DARP_DEVICE_VERSION=${ARPVERSION}" \
   "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN}/toolchain.cmake" \
   -S "${DIR}/../." \
   -B "${DIR}/../build/${TARGETNAME}_${VERSION}"

   cmake --build "${DIR}/../build/${TARGETNAME}_${VERSION}" \
   --config Debug \
   --target all -- -j 3

   cmake --build "${DIR}/../build/${TARGETNAME}_${VERSION}" \
   --config Debug --target install -- -j 3
   ```

   </details>

1. Make the script executable. From the `tools` directory:

   ```bash
   chmod a+x build-runtime.sh
   ```

1. Optional: If using Visual Studio Code, create a directory named `.vscode` in the project's root directory, and create a file named `tasks.json`, containing the following text:
   <details>
   <summary>(click to see/hide code)</summary>

   ```json
   {
      "version": "2.0.0",
      "tasks": [
         {
               "label": "Build runtime for AXCF2152 2020.0",
               "type": "shell",
               "options": {
                  "cwd": "${workspaceFolder}/tools",
                  "env": {
                     "SDKROOT": "/opt/pxc/sdk/AXCF2152/2020.0",
                     "ARP_DEVICE": "AXCF2152",
                     "ARP_DEVICE_VERSION": "2020.0 LTS (20.0.0.24752)",
                  }
               },
               "command": "./build-runtime.sh -t \"${SDKROOT}\" -a \"${ARP_DEVICE_VERSION}\" -n \"${ARP_DEVICE}\"",
               "problemMatcher": []
         }
      ]
   }
   ```

   </details>

   In the above configuration file:
   
   * The `SDKROOT` parameter specifies the full path to the root directory of the SDK. Please change this, if necessary, to the path of the SDK that you are using.
   * The `ARP_DEVICE` and `ARP_DEVICE_VERSION` parameters should specify the SDK device and version. Please change these if necessary.

1. Build the application. In Visual Studio Code, press F1 and execute the task created above. You can configure this as the default build task in Visual Studio Code as follows:

   * Press `F1` to open the command prompt.
   * Type `Configure Default Build Task` and press enter.
   * Choose a build task from the drop-down list.

   By default, this build task can now be invoked using the keyboard shortcut `CTRL`+`SHIFT`+`B`

1. Deploy the executable to the PLC.

   In a terminal window on the host, from the project's root directory (substituting the IP address of your PLC):

   ```bash
   ssh admin@192.168.1.10 'mkdir -p projects/runtime'
   scp deploy/AXCF2152_20.0.0.24752/Release/bin/runtime admin@192.168.1.10:~/projects/runtime
   ```

1. Open a secure shell session on the PLC:

   ```bash
   ssh admin@192.168.1.10
   ```

1. Run the program:

   ```bash
   projects/runtime/runtime
   ```

   You should see the output in the terminal:

   ```text
   Hello PLCnext
   ```

---

Copyright © 2020 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
