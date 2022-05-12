//
// Copyright (c) 2019-2022 Phoenix Contact GmbH & Co. KG. All rights reserved.
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
   // Use syslog for logging until the PLCnext logger is ready
   openlog ("$(name)", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

   // Process command line arguments
   string acfSettingsRelPath("");

   if(argc != 2)
   {
      syslog (LOG_ERR, "Invalid command line arguments. Only the relative path to the acf.settings file must be passed");
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

   // Intialise PLCnext module application
   // Arguments:
   //  arpBinaryDir:    Path to Arp binaries
   //  applicationName: The name of the process defined in $(name).acf.config
   //  acfSettingsPath: Path to *.acf.settings document to set application up
   if (ArpSystemModule_Load("/usr/lib", "$(name)", strSettingsFile.c_str()) != 0)
   {
      syslog (LOG_ERR, "Could not load Arp System Module");
      return -1;
   }
   syslog (LOG_INFO, "Loaded Arp System Module");
   closelog();

   Log::Info("Hello PLCnext");

   // Endless loop with sleep.
   // When the plcnext process shuts down, it will also kill this process.
   while (true)
   {
      sleep(1);
   }
}
