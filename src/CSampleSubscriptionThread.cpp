 /******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleSubscriptionThread.cpp
 *
 *  Created on: Jul 12, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#include "CSampleSubscriptionThread.h"

#include "Arp/System/Rsc/ServiceManager.hpp"

using namespace Arp::System::Rsc;

// a PLCnext Engineer Project should exist on the device with this simple structure:
// A program-instance named "MyProgramInst" which has three Ports of datatype bool: VarA (IN Port), VarB (IN Port) and VarC (Out Port)
static const char GDSPort1[] = "Arp.Plc.Eclr/MyProgramInst.VarA";
static const char GDSPort2[] = "Arp.Plc.Eclr/MyProgramInst.VarB";
static const char GDSPort3[] = "Arp.Plc.Eclr/MyProgramInst.VarC";

CSampleSubscriptionThread::CSampleSubscriptionThread() :
                m_zCycleThread(),
                m_bInitialized(false),
                m_bDoCycle(false),
                m_uSubscriptionId(0),
                m_gdsPort1(0),
                m_gdsPort2(0),
                m_gdsPort3(0)
{
}

CSampleSubscriptionThread::~CSampleSubscriptionThread()
{
}

/// @brief	Init and start the subscription thread
/// @return	true: success, false: failure
bool CSampleSubscriptionThread::Init()
{
    if(m_bInitialized)
    {
        // already initialized
        return(true);
    }

    Log::Info("Call of CSampleSubscriptionThread::Init");

    bool bRet = false;

    // the firmware needs to be in the state PlcOperation_StartWarm before we can acquire services
    m_pSubscriptionService = ServiceManager::GetService<ISubscriptionService>();
    m_pDataAccessService = ServiceManager::GetService<IDataAccessService>();

    if((m_pSubscriptionService != NULL) &&
       (m_pDataAccessService != NULL))
    {
        // create a simple worker thread for RSC-subscriptions
        if(pthread_create(&m_zCycleThread, NULL, CSampleSubscriptionThread::StaticCycle, this) == 0)
        {
            m_bInitialized = true;
            bRet = true;
        }
        else
        {
            Log::Error("Error calling pthread_create (non-realtime thread)");
        }
    }
    else
    {
        Log::Error("Error getting service (non-realtime thread)");
    }

    return(bRet);
}

/// @brief	The thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleSubscriptionThread::StartProcessing()
{
    Log::Info("Start processing");

    bool bRet = false;
    try
    {
        if(CreateSubscription())
        {
            m_bDoCycle = true;
            bRet = true;
        }
    }
    catch(Arp::Exception &e)
    {
        Log::Error("StartProcessing - Exception occured in CreateSubscription: {0}", e);
    }
    catch(...)
    {
        Log::Error("StartProcessing - Unknown Exception occured");
    } 

    return(bRet);
}

/// @brief	The thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleSubscriptionThread::StopProcessing()
{
    Log::Info("Stop processing");

    bool bRet = false;

    // this is not threadsafe! In a real application you need to wait for end of cycle
    // before deleting the subscription
    m_bDoCycle = false;

    if(DeleteSubscription())
    {
        bRet = true;
    }

    return(bRet);
}

/// @brief		static function for worker thread-entry of the cycle function
/// @param p	pointer to thread object
void* CSampleSubscriptionThread::StaticCycle(void* p)
{
    if(p != NULL)
    {
        ((CSampleSubscriptionThread*)p)->Cycle();
    }
    else
    {
        Log::Error("Null pointer in StaticCycle");
    }
    return(NULL);
}

/// @brief		loop to process values of subscription
void CSampleSubscriptionThread::Cycle()
{
    Log::Info("Call of CSampleSubscriptionThread::Cycle");

    while(true)
    {
        if(m_bDoCycle)
        {
            // you can check the log messages in the local log-file of this application, usually in a subfolder named "Logs"
            Log::Info("************* Subscription-Thread values ******");

            ReadSubscription();

            Log::Info("{0}: Value: {1}", GDSPort1, m_gdsPort1 ? "true" : "false");
            Log::Info("{0}: Value: {1}", GDSPort2, m_gdsPort2 ? "true" : "false");
            Log::Info("{0}: Value: {1}", GDSPort3, m_gdsPort3 ? "true" : "false");
        }

        WAIT100ms;
    }
}

/// @brief		create a GDS subscription for three ports.
/// @return		true: success, false: failure
bool CSampleSubscriptionThread::CreateSubscription()
{
    Log::Info("Call of CSampleSubscriptionThread::CreateSubscription");

    bool bRet = false;

    // create subscription, check the description of SubscriptionKind for the different realtime classes
    m_uSubscriptionId = m_pSubscriptionService->CreateSubscription(SubscriptionKind::HighPerformance);

    if(m_uSubscriptionId != 0)
    {
        // add the variables
        if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort1) == DataAccessError::None)
        {
            if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort2) == DataAccessError::None)
            {
                if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort3) == DataAccessError::None)
                {
                    // subscribe the build subscription and save the returned record info for further reading
                    // of the subscription to know the order of result
                    const uint64 sampleRate = 1000000; // us == 1s
                    if(m_pSubscriptionService->Subscribe(m_uSubscriptionId, sampleRate) == DataAccessError::None)
                    {
                        // get information about the order / layout of the vector of read variable values
                        if(RSCReadVariableInfos(m_zSubscriptionInfos) == DataAccessError::None)
                        {
                            bRet = true;
                        }
                        else
                        {
                            Log::Error("Unable to read variable information");
                        }
                    }
                    else
                    {
                        Log::Error("ISubscriptionservice::Subscribe returned error");
                    }
                }
                else
                {
                    Log::Error("Unable to subscribe variable {0}", GDSPort3);
                }
            }
            else
            {
                Log::Error("Unable to subscribe variable {0}", GDSPort2);
            }
        }
        else
        {
            Log::Error("Unable to subscribe variable {0}", GDSPort1);
        }
    }
    else
    {
        Log::Error("ISubscriptionservice::CreateSubscription returned error");
    }
    return bRet;
}

/// @brief		delete the subscription
/// @return		true: success, false: failure
bool CSampleSubscriptionThread::DeleteSubscription()
{
    Log::Info("Call of CSampleSubscriptionThread::DeleteSubscription");

    bool bRet = false;

    if(m_uSubscriptionId != 0)
    {
        if(m_pSubscriptionService->DeleteSubscription(m_uSubscriptionId) == DataAccessError::None)
        {
            bRet = true;
        }
        else
        {
            Log::Error("ISubscriptionservice::DeleteSubscription returned error");
        }

        m_uSubscriptionId = 0;
    }
    return(bRet);
}

/// @brief		poll the subscribed values and parse them
/// @return		true: success, false: failure
bool CSampleSubscriptionThread::ReadSubscription()
{
    bool bRet = false;

    try
    {
        // Read the subscription
        if(RSCReadVariableValues(m_zSubscriptionValues) == DataAccessError::None)
        {
            // The order of the subscription info vector is the same as the subscription values vector,
            // we can iterate over both in the same loop
            if(m_zSubscriptionValues.size() == m_zSubscriptionInfos.size())	// sanity-check
            {
                for(std::size_t nCount = 0; nCount < m_zSubscriptionValues.size(); ++nCount)
                {
                    if(m_zSubscriptionValues[nCount].GetType() != RscType::Void)
					{

                        if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort1) == 0)
                        {
                            m_zSubscriptionValues[nCount].CopyTo(m_gdsPort1);
                        }
                        if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort2) == 0)
                        {
                            m_zSubscriptionValues[nCount].CopyTo(m_gdsPort2);
                        }
                        if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort3) == 0)
                        {
                            m_zSubscriptionValues[nCount].CopyTo(m_gdsPort3);
                        }
                    }
                    else
					{
						Log::Info("Subscription {0} is not available yet. Initial value of its variable will be used!", m_zSubscriptionInfos[nCount].Name);
					}
                }
                bRet = true;
            }
            else
            {
                Log::Error("Inconsistent size of subscription info and subscription values vector");
            }
        }
        else
        {
            Log::Error("ISubscriptionService::ReadValues returned error");
        }
    }
    catch(Arp::Exception &e)
	{
		Log::Error("ReadSubscription - Arp Exception! {0}", e);
	}
	catch(...)
	{
		Log::Error("ReadSubscription - Unknown Exception occured");
	}


    return(bRet);
}

// create a delegate (callback-function) for reading the vector of subscription values
DataAccessError CSampleSubscriptionThread::RSCReadVariableValues(vector<RscVariant<512>>& values)
{
    ISubscriptionService::ReadValuesValuesDelegate readSubscriptionValuesDelegate =
        ISubscriptionService::ReadValuesValuesDelegate::create([&](IRscReadEnumerator<RscVariant<512>>& readEnumerator)
    {
        values.clear();

        size_t valueCount = readEnumerator.BeginRead();
        values.reserve(valueCount);

        RscVariant<512> current;
        for (size_t i = 0; i < valueCount; i++)
        {
            readEnumerator.ReadNext(current);
            values.push_back(current);
        }
        readEnumerator.EndRead();
    });

    return m_pSubscriptionService->ReadValues(m_uSubscriptionId, readSubscriptionValuesDelegate);
}

// create a delegate (callback-function) for reading the vector of subscription information
DataAccessError CSampleSubscriptionThread::RSCReadVariableInfos(vector<VariableInfo>& values)
{
    ISubscriptionService::GetVariableInfosVariableInfoDelegate getSubscriptionInfosDelegate =
        ISubscriptionService::GetVariableInfosVariableInfoDelegate::create([&](IRscReadEnumerator<VariableInfo>& readEnumerator)
    {
        values.clear();

        size_t valueCount = readEnumerator.BeginRead();
        values.reserve(valueCount);

        VariableInfo current;
        for (size_t i = 0; i < valueCount; i++)
        {
            readEnumerator.ReadNext(current);
            values.push_back(current);
        }
        readEnumerator.EndRead();
    });

    return m_pSubscriptionService->GetVariableInfos(m_uSubscriptionId, getSubscriptionInfosDelegate);
}

 /******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleSubscriptionThread.cpp
 *
 *  Created on: Jul 12, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#include "CSampleSubscriptionThread.h"

#include "Arp/System/Rsc/ServiceManager.hpp"

using namespace Arp::System::Rsc;

// a PLCnext Engineer Project should exist on the device with this simple structure:
// A program-instance named "MyProgramInst" which has three Ports of datatype bool: VarA (IN Port), VarB (IN Port) and VarC (Out Port)
static const char GDSPort1[] = "Arp.Plc.Eclr/MyProgramInst.VarA";
static const char GDSPort2[] = "Arp.Plc.Eclr/MyProgramInst.VarB";
static const char GDSPort3[] = "Arp.Plc.Eclr/MyProgramInst.VarC";

CSampleSubscriptionThread::CSampleSubscriptionThread() :
                m_zCycleThread(),
                m_bInitialized(false),
                m_bDoCycle(false),
                m_uSubscriptionId(0),
                m_gdsPort1(0),
                m_gdsPort2(0),
                m_gdsPort3(0)
{
}

CSampleSubscriptionThread::~CSampleSubscriptionThread()
{
}

/// @brief	Init and start the subscription thread
/// @return	true: success, false: failure
bool CSampleSubscriptionThread::Init()
{
    if(m_bInitialized)
    {
        // already initialized
        return(true);
    }

    Log::Info("Call of CSampleSubscriptionThread::Init");

    bool bRet = false;

    // the firmware needs to be in the state PlcOperation_StartWarm before we can acquire services
    m_pSubscriptionService = ServiceManager::GetService<ISubscriptionService>();
    m_pDataAccessService = ServiceManager::GetService<IDataAccessService>();

    if((m_pSubscriptionService != NULL) &&
       (m_pDataAccessService != NULL))
    {
        // create a simple worker thread for RSC-subscriptions
        if(pthread_create(&m_zCycleThread, NULL, CSampleSubscriptionThread::StaticCycle, this) == 0)
        {
            m_bInitialized = true;
            bRet = true;
        }
        else
        {
            Log::Error("Error calling pthread_create (non-realtime thread)");
        }
    }
    else
    {
        Log::Error("Error getting service (non-realtime thread)");
    }

    return(bRet);
}

/// @brief	The thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleSubscriptionThread::StartProcessing()
{
    Log::Info("Subcription: Start processing");

    bool bRet = false;
    try
    {
        if(CreateSubscription())
        {
            m_bDoCycle = true;
            bRet = true;
        }
    }
    catch(Arp::Exception &e)
    {
        Log::Error("StartProcessing - Exception occured in CreateSubscription: {0}", e);
    }
    catch(...)
    {
        Log::Error("StartProcessing - Unknown Exception occured");
    } 

    return(bRet);
}

/// @brief	The thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleSubscriptionThread::StopProcessing()
{
    Log::Info("Subcription: Stop processing");

    bool bRet = false;

    // this is not threadsafe! In a real application you need to wait for end of cycle
    // before deleting the subscription
    m_bDoCycle = false;

    if(DeleteSubscription())
    {
        bRet = true;
    }

    return(bRet);
}

/// @brief		static function for worker thread-entry of the cycle function
/// @param p	pointer to thread object
void* CSampleSubscriptionThread::StaticCycle(void* p)
{
    if(p != NULL)
    {
        ((CSampleSubscriptionThread*)p)->Cycle();
    }
    else
    {
        Log::Error("Null pointer in StaticCycle");
    }
    return(NULL);
}

/// @brief		loop to process values of subscription
void CSampleSubscriptionThread::Cycle()
{
    Log::Info("Call of CSampleSubscriptionThread::Cycle");

    while(true)
    {
        if(m_bDoCycle)
        {
            // you can check the log messages in the local log-file of this application, usually in a subfolder named "Logs"
            Log::Info("************* Subscription-Thread values ******");

            ReadSubscription();

            Log::Info("{0}: Value: {1}", GDSPort1, m_gdsPort1 ? "true" : "false");
            Log::Info("{0}: Value: {1}", GDSPort2, m_gdsPort2 ? "true" : "false");
            Log::Info("{0}: Value: {1}", GDSPort3, m_gdsPort3 ? "true" : "false");
        }

        WAIT100ms;
    }
}

/// @brief		create a GDS subscription for three ports.
/// @return		true: success, false: failure
bool CSampleSubscriptionThread::CreateSubscription()
{
    Log::Info("Call of CSampleSubscriptionThread::CreateSubscription");

    bool bRet = false;

    // create subscription, check the description of SubscriptionKind for the different realtime classes
    m_uSubscriptionId = m_pSubscriptionService->CreateSubscription(SubscriptionKind::HighPerformance);

    if(m_uSubscriptionId != 0)
    {
        // add the variables
        if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort1) == DataAccessError::None)
        {
            if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort2) == DataAccessError::None)
            {
                if(m_pSubscriptionService->AddVariable(m_uSubscriptionId, GDSPort3) == DataAccessError::None)
                {
                    // subscribe the build subscription and save the returned record info for further reading
                    // of the subscription to know the order of result
                    const uint64 sampleRate = 1000000; // us == 1s
                    if(m_pSubscriptionService->Subscribe(m_uSubscriptionId, sampleRate) == DataAccessError::None)
                    {
                        // get information about the order / layout of the vector of read variable values
                        if(RSCReadVariableInfos(m_zSubscriptionInfos) == DataAccessError::None)
                        {
                            bRet = true;
                        }
                        else
                        {
                            Log::Error("Unable to read variable information");
                        }
                    }
                    else
                    {
                        Log::Error("ISubscriptionservice::Subscribe returned error");
                    }
                }
                else
                {
                    Log::Error("Unable to subscribe variable {0}", GDSPort3);
                }
            }
            else
            {
                Log::Error("Unable to subscribe variable {0}", GDSPort2);
            }
        }
        else
        {
            Log::Error("Unable to subscribe variable {0}", GDSPort1);
        }
    }
    else
    {
        Log::Error("ISubscriptionservice::CreateSubscription returned error");
    }
    return bRet;
}

/// @brief		delete the subscription
/// @return		true: success, false: failure
bool CSampleSubscriptionThread::DeleteSubscription()
{
    Log::Info("Call of CSampleSubscriptionThread::DeleteSubscription");

    bool bRet = false;

    if(m_uSubscriptionId != 0)
    {
        if(m_pSubscriptionService->DeleteSubscription(m_uSubscriptionId) == DataAccessError::None)
        {
            bRet = true;
        }
        else
        {
            Log::Error("ISubscriptionservice::DeleteSubscription returned error");
        }

        m_uSubscriptionId = 0;
    }
    return(bRet);
}

/// @brief		poll the subscribed values and parse them
/// @return		true: success, false: failure
bool CSampleSubscriptionThread::ReadSubscription()
{
    bool bRet = false;

    try
    {
        // Read the subscription
        if(RSCReadVariableValues(m_zSubscriptionValues) == DataAccessError::None)
        {
            // The order of the subscription info vector is the same as the subscription values vector,
            // we can iterate over both in the same loop
            if(m_zSubscriptionValues.size() == m_zSubscriptionInfos.size())	// sanity-check
            {
                for(std::size_t nCount = 0; nCount < m_zSubscriptionValues.size(); ++nCount)
                {
                    if(m_zSubscriptionValues[nCount].GetType() != RscType::Void)
					{

                        if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort1) == 0)
                        {
                            m_zSubscriptionValues[nCount].CopyTo(m_gdsPort1);
                        }
                        if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort2) == 0)
                        {
                            m_zSubscriptionValues[nCount].CopyTo(m_gdsPort2);
                        }
                        if(strcmp(m_zSubscriptionInfos[nCount].Name.CStr(), GDSPort3) == 0)
                        {
                            m_zSubscriptionValues[nCount].CopyTo(m_gdsPort3);
                        }
                    }
                    else
					{
						Log::Info("Subscription {0} is not available yet. Initial value of its variable will be used!", m_zSubscriptionInfos[nCount].Name);
					}
                }
                bRet = true;
            }
            else
            {
                Log::Error("Inconsistent size of subscription info and subscription values vector");
            }
        }
        else
        {
            Log::Error("ISubscriptionService::ReadValues returned error");
        }
    }
    catch(Arp::Exception &e)
	{
		Log::Error("ReadSubscription - Arp Exception! {0}", e);
	}
	catch(...)
	{
		Log::Error("ReadSubscription - Unknown Exception occured");
	}


    return(bRet);
}

// create a delegate (callback-function) for reading the vector of subscription values
DataAccessError CSampleSubscriptionThread::RSCReadVariableValues(vector<RscVariant<512>>& values)
{
    ISubscriptionService::ReadValuesValuesDelegate readSubscriptionValuesDelegate =
        ISubscriptionService::ReadValuesValuesDelegate::create([&](IRscReadEnumerator<RscVariant<512>>& readEnumerator)
    {
        values.clear();

        size_t valueCount = readEnumerator.BeginRead();
        values.reserve(valueCount);

        RscVariant<512> current;
        for (size_t i = 0; i < valueCount; i++)
        {
            readEnumerator.ReadNext(current);
            values.push_back(current);
        }
        readEnumerator.EndRead();
    });

    return m_pSubscriptionService->ReadValues(m_uSubscriptionId, readSubscriptionValuesDelegate);
}

// create a delegate (callback-function) for reading the vector of subscription information
DataAccessError CSampleSubscriptionThread::RSCReadVariableInfos(vector<VariableInfo>& values)
{
    ISubscriptionService::GetVariableInfosVariableInfoDelegate getSubscriptionInfosDelegate =
        ISubscriptionService::GetVariableInfosVariableInfoDelegate::create([&](IRscReadEnumerator<VariableInfo>& readEnumerator)
    {
        values.clear();

        size_t valueCount = readEnumerator.BeginRead();
        values.reserve(valueCount);

        VariableInfo current;
        for (size_t i = 0; i < valueCount; i++)
        {
            readEnumerator.ReadNext(current);
            values.push_back(current);
        }
        readEnumerator.EndRead();
    });

    return m_pSubscriptionService->GetVariableInfos(m_uSubscriptionId, getSubscriptionInfosDelegate);
}

