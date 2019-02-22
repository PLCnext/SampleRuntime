/******************************************************************************
 *
 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
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

#include <vector>
#include <pthread.h>
#include "Arp/Plc/AnsiC/Domain/PlcOperationHandler.h"
#include "Arp/Device/Interface/Services/IDeviceStatusService.hpp"
#include "Arp/Device/Interface/Services/IDeviceInfoService.hpp"
#include "Arp/Plc/Domain/Services/IPlcManagerService.hpp"
#include "Arp/Services/NotificationLogger/Services/INotificationLoggerService.hpp"
#include "Arp/Plc/Gds/Services/ISubscriptionService.hpp"
#include "Arp/Plc/Gds/Services/IDataAccessService.hpp"

using namespace std;
using namespace Arp;
using namespace Arp::Plc::Domain::Services;
using namespace Arp::Device::Interface::Services;
using namespace Arp::Services::NotificationLogger::Services;
using namespace Arp::Plc::Gds::Services;

#define WAIT100ms 	usleep(100000);		// 100ms
#define WAIT1s 		usleep(1000000);	// 1s
#define WAIT10s 	usleep(10000000);	// 10s

typedef void * (*THREADFUNCPTR)(void *);

// callback-function for state changes
void PlcOperationHandler(enum PlcOperation operation);

class CSampleRuntime;
extern CSampleRuntime* g_pRT;

class CSampleRuntime {
public:
	CSampleRuntime();
	virtual ~CSampleRuntime();

	static PlcOperation m_zPLCMode;

	bool Init();
	static void* StaticCycle(void* p);
	void Cycle();

	// start and stop our own processing
	bool StartProcessing();
	bool StopProcessing();
private:
    // workerthread for cycle
    pthread_t m_zCycleThread;

    bool m_bInitialized;	// class already initialized?
	bool m_bDoCycle;		// shall the cycle run?

    IDeviceInfoService::Ptr m_pDeviceInfoService;
    IDeviceStatusService::Ptr m_pDeviceStatusService;
    IPlcManagerService::Ptr m_pPLCManagerService;
    INotificationLoggerService::Ptr m_pNotificationLoggerService;
    ISubscriptionService::Ptr m_pSubscriptionService;
    IDataAccessService::Ptr m_pDataAccessService;

    bool GetDeviceStatus();

    RscVariant<512> m_rscVendorName;
    RscVariant<512> m_rscCpuLoad;
    RscVariant<512> m_rscBemoryUsage;
    RscVariant<512> m_rscBoardTemp;
    const char* m_szVendorName;
    byte m_byCpuLoad;
	byte m_byMemoryUsage;
	int8 m_i8BoardTemp;

	// example usage of GDS subscription
	bool CreateSubscription();
	bool DeleteSubscription();
	bool ReadSubscription();
	DataAccessError RSCReadVariableValues(vector<RscVariant<512>>& values);
	DataAccessError RSCReadVariableInfos(vector<VariableInfo>& values);

    // subscriptions
    uint32 m_uSubscriptionId;
    vector<RscVariant<512>> m_zSubscriptionValues;
	vector<VariableInfo> m_zSubscriptionInfos;

    // data fields
    bool m_gdsPort1;
    bool m_gdsPort2;
    bool m_gdsPort3;

    // example usage of direct access to fieldbus-frame
    bool ReadWriteIoData(void);
    bool ReadAxioSystemVariable(void);
    bool ReadProfinetIOPS(void);
    bool ReadInputData(const char* fbIoSystemName, const char* portName, char* pValue, size_t valueSize);
    bool WriteOutputData(const char* fbIoSystemName, const char* portName, const char* pValue, size_t valueSize);
};

#endif /* CSAMPLERUNTIME_H_ */
