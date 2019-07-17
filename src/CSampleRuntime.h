/******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleRuntime.h
 *
 *  Created on: Jan 3, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#ifndef CSAMPLERUNTIME_H_
#define CSAMPLERUNTIME_H_

#include "Utility.h"
#include "CSampleRTThread.h"
#include "CSampleSubscriptionThread.h"

#include <pthread.h>
#include "Arp/Device/Interface/Services/IDeviceStatusService.hpp"
#include "Arp/Device/Interface/Services/IDeviceInfoService.hpp"
#include "Arp/System/Lm/Services/ILicenseStatusService.hpp"
#include "Arp/System/Lm/Services/LicenseStatusServiceProxyFactory.hpp"
#include "Arp/Plc/AnsiC/Domain/PlcOperationHandler.h"

using namespace std;
using namespace Arp;
using namespace Arp::Device::Interface::Services;
using namespace Arp::System::Lm::Services;

// callback-function for state changes
void PlcOperationHandler(enum PlcOperation operation);

class CSampleRuntime;
extern CSampleRuntime* g_pRT;	// global pointer to sample runtime to make it call-able from callback-function

class CSampleRuntime
{
public:
    CSampleRuntime();
    virtual ~CSampleRuntime();

    static PlcOperation m_zPLCMode;	// current mode of operation

    bool Init();
    bool StartProcessing();
    bool StopProcessing();

private:
    bool m_bInitialized;	// already initializes?

    CSampleRTThread m_zRTThread;
    CSampleSubscriptionThread m_zSubscriptionThread;

    // RSC services
    IDeviceInfoService::Ptr m_pDeviceInfoService;
    IDeviceStatusService::Ptr m_pDeviceStatusService;
    ILicenseStatusService::Ptr m_pLicenseStatusService;

    bool GetDeviceStatus();

    // some static and dynamic data of the PLCnext runtime
    RscVariant<512> m_rscVendorName;
    RscVariant<512> m_rscCpuLoad;
    RscVariant<512> m_rscBemoryUsage;
    RscVariant<512> m_rscBoardTemp;
    const char* m_szVendorName;
    byte m_byCpuLoad;
    byte m_byMemoryUsage;
    int8 m_i8BoardTemp;
};

#endif /* CSAMPLERUNTIME_H_ */
