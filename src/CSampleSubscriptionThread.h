 /******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleSubscriptionThread.h
 *
 *  Created on: Jul 12, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#include <vector>
#include "Arp/Plc/Gds/Services/ISubscriptionService.hpp"
#include "Arp/Plc/Gds/Services/IDataAccessService.hpp"
#include "Arp/System/Commons/Logging.h"
#include "Utility.h"

using namespace std;
using namespace Arp;
using namespace Arp::Plc::Gds::Services;

#ifndef CSAMPLESUBSCRIPTIONTHREAD_H_
#define CSAMPLESUBSCRIPTIONTHREAD_H_

class CSampleSubscriptionThread
{
public:
    CSampleSubscriptionThread();
    virtual ~CSampleSubscriptionThread();

    bool Init();
    static void* StaticCycle(void* p);
    void Cycle();

    // start and stop our own processing
    bool StartProcessing();
    bool StopProcessing();

private:

    pthread_t m_zCycleThread;

    bool m_bInitialized;	// class already initialized?
    bool m_bDoCycle;		// shall the subscription cycle run?

    ISubscriptionService::Ptr m_pSubscriptionService;
    IDataAccessService::Ptr m_pDataAccessService;

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
};

#endif /* CSAMPLESUBSCRIPTIONTHREAD_H_ */
