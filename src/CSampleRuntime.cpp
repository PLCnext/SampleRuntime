 /******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleRuntime.cpp
 *
 *  Created on: Jan 3, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#include "CSampleRuntime.h"

#include "Arp/System/Rsc/ServiceManager.hpp"
#include "Arp/Plc/AnsiC/ArpPlc.h"
#include "Arp/Plc/AnsiC/Device/Device.h"
#include "Arp/Plc/AnsiC/Domain/PlcManager.h"

using namespace Arp::System::Rsc;

PlcOperation CSampleRuntime::m_zPLCMode = PlcOperation_None;

CSampleRuntime::CSampleRuntime()
              : m_bInitialized(false),
                m_szVendorName(NULL),
                m_byCpuLoad(0),
                m_byMemoryUsage(0),
                m_i8BoardTemp(0)
{
    // announce the status-update callback
    // this is important to get the status of the "firmware-ready"-event PlcOperation_StartWarm
    ArpPlcDomain_SetHandler(PlcOperationHandler);
}

CSampleRuntime::~CSampleRuntime()
{
}

/// @brief: init/connect to the PLCnext services and start the cycle-threads
/// @return	true: success, false: failure
bool CSampleRuntime::Init()
{
    if(m_bInitialized)
    {
        return(true);
    }

    Log::Info("Call of CSampleRuntime::Init");

    bool bRet = false;

    // the firmware needs to be in the state PlcOperation_StartWarm before we can acquire services
    m_pDeviceInfoService = ServiceManager::GetService<IDeviceInfoService>();
    m_pDeviceStatusService = ServiceManager::GetService<IDeviceStatusService>();
    m_pLicenseStatusService = ServiceManager::GetService<ILicenseStatusService>();

    if((m_pDeviceStatusService != NULL) &&
       (m_pDeviceInfoService != NULL) &&
       (m_pLicenseStatusService != NULL)
       )
    {
        if(GetDeviceStatus() == true)
        {
            // check current license
            unsigned int uFirmCode = 0;		// you get FirmCode+ProductCode after creating a new app in the PLCnext store
            unsigned int uProductCode = 0;	// hardcode these numbers in your application
            unsigned int uFeatureCode = 0;	// FeatureCode is not yet supported

            if(m_pLicenseStatusService->GetLicenseStatus(uFirmCode, uProductCode, uFeatureCode) == true)
            {
                // valid license on the device
            }
            else
            {
                // no valid license on the device -> switch to demo mode, do not just quit the app!
            }

            if(m_zRTThread.Init() == true)
            {
                if(m_zSubscriptionThread.Init() == true)
                {
                    m_bInitialized = true;
                    bRet = true;
                }
            }
        }
    }

    return(bRet);
}

/// @brief	start processing of I/O data in the different threads
/// 		e.g. after a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleRuntime::StartProcessing()
{
    Log::Info("Start processing");

    bool bRet = false;
    if(m_zRTThread.StartProcessing() == true)
    {
        if(m_zSubscriptionThread.StartProcessing() == true)
        {
            bRet = true;
        }
    }

    return(bRet);
}

/// @brief	stop processing of I/O data in the different threads
/// 		e.g. if a new PLCnext Engineer Program should be loaded
/// @return	true: success, false: failure
bool CSampleRuntime::StopProcessing()
{
    Log::Info("Stop processing");

    bool bRet = false;
    if(m_zRTThread.StopProcessing() == true)
    {
        if(m_zSubscriptionThread.StopProcessing() == true)
        {
            bRet = true;
        }
    }

    return(bRet);
}

/// @brief: retrieve some information from the PLCnext runtime system.
/// @return true: success, false: failure
bool CSampleRuntime::GetDeviceStatus()
{
    bool bRet = false;

    // get some values from device info service (static data)
    m_rscVendorName = m_pDeviceInfoService->GetItem("General.VendorName");
    m_szVendorName = m_rscVendorName.GetChars();

    // get some values from device status service (dynamic data)
    m_rscCpuLoad = m_pDeviceStatusService->GetItem("Status.Cpu.0.Load.Percent");
    m_rscCpuLoad.CopyTo(m_byCpuLoad);
    m_rscBemoryUsage = m_pDeviceStatusService->GetItem("Status.Memory.Usage.Percent");
    m_rscBemoryUsage.CopyTo(m_byMemoryUsage);
    m_rscBoardTemp = m_pDeviceStatusService->GetItem("Status.Board.Temperature.Centigrade");
    m_rscBoardTemp.CopyTo(m_i8BoardTemp);

    bRet = true;

    return(bRet);
}

/// @brief callback function for the PLC state
/// @param operation current mode of operation
void PlcOperationHandler(enum PlcOperation operation)
{
    CSampleRuntime::m_zPLCMode = operation;

    switch (operation)
    {
        break;
        case PlcOperation_Load:
            Log::Info("Call of PLC Load");
            break;
        case PlcOperation_Setup:
            Log::Info("Call of PLC Setup");
            break;
        case PlcOperation_StartCold:
            Log::Info("Call of PLC Start Cold");
            // when this state-change occurred, the PLCnext runtime is ready to serve requests.
            // Now we can request the needed service-interfaces.
            // Plc may by stopped by system watchdog so that is possible to start plc cold on system start
            if(g_pRT->Init() == false)
            {
                Log::Error("Error during initialization");
            }
            g_pRT->StartProcessing();

            break;
        case PlcOperation_StartWarm:
            Log::Info("Call of PLC Start Warm");

            // when this state-change occurred, the PLCnext runtime is ready to serve requests.
            // Now we can request the needed service-interfaces.
            if(g_pRT->Init() == false)
            {
                Log::Error("Error during initialization");
            }
            g_pRT->StartProcessing();
            break;
        case PlcOperation_StartHot:
            Log::Info("Call of PLC Start Hot");
            g_pRT->StartProcessing();
            break;
        case PlcOperation_Stop:
            Log::Info("Call of PLC Stop");
            g_pRT->StopProcessing();
            break;
        case PlcOperation_Reset:
            Log::Info("Call of PLC Reset");
            g_pRT->StopProcessing();
            break;
        case PlcOperation_Unload:
            Log::Info("Call of PLC Unload");
            g_pRT->StopProcessing();
            break;
        case PlcOperation_None:
            Log::Info("Call of PLC None");
            break;
        default:
            Log::Error("Call of unknown PLC state");
            break;
    }
}

