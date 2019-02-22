 /******************************************************************************
 *
 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleRuntime.cpp
 *
 *  Created on: Jan 3, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <libgen.h>
#include <linux/limits.h>
#include <unistd.h>
#include <vector>
#include "boost/range/combine.hpp"
#include "Arp/System/Core/Arp.h"
#include "Arp/System/ModuleLib/Module.h"
#include "Arp/System/Acf/ComponentBase.hpp"
#include "Arp/System/Acf/IApplication.hpp"
#include "Arp/System/Acf/IControllerComponent.hpp"
#include "Arp/System/Rsc/ServiceManager.hpp"
#include "Arp/System/Commons/Logging.h"
#include "Arp/System/Commons/Threading/WorkerThread.hpp"
#include "Arp/Plc/AnsiC/ArpPlc.h"
#include "Arp/Plc/AnsiC/Device/Device.h"
#include "Arp/Plc/AnsiC/Domain/PlcManager.h"

#include "Arp/Plc/AnsiC/Gds/DataLayout.h"
#include "Arp/Plc/AnsiC/Io/FbIoSystem.h"
#include "Arp/Plc/AnsiC/Io/Axio.h"

#include "CSampleRuntime.h"


using namespace Arp;
using namespace Arp::System::Acf;
using namespace Arp::System::Rsc;
using namespace Arp::Device::Interface::Services;


PlcOperation CSampleRuntime::m_zPLCMode = PlcOperation_None;

// a PLCnext Engineer Project should exist on the device with this simple structure:
// A program-instance named "MyProgramInst" which has three Ports of datatype bool: VarA (IN Port), VarB (IN Port) and VarC (Out Port)
static const char GDSPort1[] = "Arp.Plc.Eclr/MyProgramInst.VarA";
static const char GDSPort2[] = "Arp.Plc.Eclr/MyProgramInst.VarB";
static const char GDSPort3[] = "Arp.Plc.Eclr/MyProgramInst.VarC";

#define ARP_IO_AXIO "Arp.Io.AxlC"
#define ARP_IO_PN	"Arp.Io.PnC"

CSampleRuntime::CSampleRuntime()
			  : m_zCycleThread(),
			    m_bInitialized(false),
				m_bDoCycle(false),
				m_szVendorName(NULL),
			    m_byCpuLoad(0),
				m_byMemoryUsage(0),
				m_i8BoardTemp(0),
				m_uSubscriptionId(0),
			    m_gdsPort1(0),
			    m_gdsPort2(0),
			    m_gdsPort3(0)
{
	// announce the status-update callback
	// this is important to get the status of the "firmware-ready"-event PlcOperation_StartWarm
	ArpPlcDomain_SetHandler(PlcOperationHandler);
}

CSampleRuntime::~CSampleRuntime()
{

}

// init/connect to the PLCnext services and start the cycle-thread
bool CSampleRuntime::Init()
{
	if(m_bInitialized)
	{
		return(true);
	}

	Log::Info("Call of CSampleRuntime::Init");

	bool bRet = false;

	// the firmware needs to be in the state PlcOperation_StartWarm before we can acquire services
	m_pPLCManagerService = ServiceManager::GetService<IPlcManagerService>();
	m_pDeviceInfoService = ServiceManager::GetService<IDeviceInfoService>();
	m_pDeviceStatusService = ServiceManager::GetService<IDeviceStatusService>();
	m_pNotificationLoggerService = ServiceManager::GetService<INotificationLoggerService>();
	m_pSubscriptionService = ServiceManager::GetService<ISubscriptionService>();
	m_pDataAccessService = ServiceManager::GetService<IDataAccessService>();

	if((m_pPLCManagerService != NULL) &&
	   (m_pDeviceStatusService != NULL) &&
	   (m_pDeviceInfoService != NULL) &&
	   (m_pNotificationLoggerService != NULL) &&
	   (m_pSubscriptionService != NULL) &&
	   (m_pDataAccessService != NULL)
	   )
	{
		if(GetDeviceStatus() == true)
		{
			// create a simple worker thread for our loop
			if(pthread_create(&m_zCycleThread, NULL, CSampleRuntime::StaticCycle, this) == 0)
			{
				m_bInitialized = true;
				bRet = true;
			}
		}
	}

	return(bRet);
}

bool CSampleRuntime::StartProcessing()
{
	Log::Info("Start processing");

	bool bRet = false;
	if(CreateSubscription())
	{
		m_bDoCycle = true;
		bRet = true;
	}

	return(bRet);
}

bool CSampleRuntime::StopProcessing()
{
	Log::Info("Stop processing");

	bool bRet = false;

	// this is not threadsafe! In a real application you need to wait for end of cycle before deleting the subscription
	m_bDoCycle = false;

	if(DeleteSubscription())
	{
		bRet = true;
	}
	return(bRet);
}

// worker thread-entry for the cycle function
void* CSampleRuntime::StaticCycle(void* p)
{
	if(p != NULL)
	{
		((CSampleRuntime*)p)->Cycle();
	}
	return(NULL);
}

// a simple loop. In a real application, this should be done in a worker thread with correct priority
void CSampleRuntime::Cycle()
{
	Log::Info("Call of CSampleRuntime::Cycle");

	for (;;)
	{
		if(m_bDoCycle)
		{
			// you can check the log messages in the local log-file of this application, usually in a subfolder named "Logs"
			Log::Info("************* Another cycle *************");

			ReadSubscription();

			Log::Info("{0}: Value: {1}", GDSPort1, m_gdsPort1 ? "true" : "false");
			Log::Info("{0}: Value: {1}", GDSPort2, m_gdsPort2 ? "true" : "false");
			Log::Info("{0}: Value: {1}", GDSPort3, m_gdsPort3 ? "true" : "false");

			// use the functions for direct fieldbus-frame access
			ReadWriteIoData();
			ReadAxioSystemVariable();
			//ReadProfinetIOPS();	// not used
		}

		WAIT100ms;
	}
}

// retrieve some information from the firmware.
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

// create a GDS subscription for three ports.
bool CSampleRuntime::CreateSubscription()
{
	bool bRet = false;

	// create subscription
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
					// subscribe the build subscription and save the returned record infos for further reading
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

// create a GDS subscription for three ports.
bool CSampleRuntime::DeleteSubscription()
{
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

// poll the subscribed values and parse them
bool CSampleRuntime::ReadSubscription()
{
	bool bRet = false;

	// Read the subscription
	if(RSCReadVariableValues(m_zSubscriptionValues) == DataAccessError::None)
	{
		// The order of the subscription infos vector is the same as the subscription values vector,
		// we can iterate over both.
		if(m_zSubscriptionValues.size() == m_zSubscriptionInfos.size())	// sanity-check
		{
			for(std::size_t nCount = 0; nCount < m_zSubscriptionValues.size(); ++nCount)
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
	return(bRet);
}

// create a delegate (callback-function) for reading the vector of subscription values
DataAccessError CSampleRuntime::RSCReadVariableValues(vector<RscVariant<512>>& values)
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
DataAccessError CSampleRuntime::RSCReadVariableInfos(vector<VariableInfo>& values)
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


bool CSampleRuntime::ReadWriteIoData(void)
{
	bool bRet = false;

    if(ArpPlcAxio_ReadFromAxioToGds(0))
    {
		uint16_t value = 0;

		// create port names. Format: IOSystem/DeviceNumber.NameOfIO
		String strInPort = Formatter::FormatCommon("{}/0.~DI16", ARP_IO_AXIO);
		String strOutPort = Formatter::FormatCommon("{}/1.~DO16", ARP_IO_AXIO);

		if(ReadInputData(ARP_IO_AXIO, strInPort, (char*)&value, sizeof(value)))
		{
			Log::Info("Read Value of {0}: {1:#04x}", strInPort, value);

			WriteOutputData(ARP_IO_AXIO, strOutPort, (char*)&value, sizeof(value));
			if(ArpPlcAxio_WriteFromGdsToAxio(0))
			{
				bRet = true;
			}
			else
			{
				Log::Error("ArpPlcAxio_WriteFromGdsToAxio failed");
			}
		}
    }
    else
    {
    	Log::Error("ArpPlcAxio_ReadFromAxioToGds failed");
    }

    return(bRet);
}

bool CSampleRuntime::ReadAxioSystemVariable(void)
{
	bool bRet = false;

    // Existing axioline system variables:
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_HI         BYTE
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_LOW        BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_HI          BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_LOW         BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_HI        BYTE
    // Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_LOW       BYTE
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PW         BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PF         BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_BUS        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RUN        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_ACT        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RDY        BOOL
    // Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_SYSFAIL    BOOL

	uint16_t axioStatusRegister = 0;
	String strStatusRegister = Formatter::FormatCommon("{}/AXIO_DIAG_STATUS_REG", ARP_IO_AXIO);

	if(ReadInputData(ARP_IO_AXIO, strStatusRegister, (char*)&axioStatusRegister, sizeof(axioStatusRegister)))
    {
    	// if you are wondering about the formatting syntax of the Log-Class, check
    	// http://fmtlib.net/latest/syntax.html
    	Log::Info("Read Value of AXIO_DIAG_STATUS_REG: {0:#04x}", axioStatusRegister);

    	bRet = true;
    }

	return(bRet);
}

bool CSampleRuntime::ReadProfinetIOPS(void)
{
	bool bRet = false;

    uint8_t profinetIOPS = 0;

    // create port names. Format: IOSystem/DeviceNumber.NameOfIO
    String strInPort = Formatter::FormatCommon("{}/9.IPS", ARP_IO_PN);
    String strOutPort = Formatter::FormatCommon("{}/9.OCS", ARP_IO_PN);

    if(ReadInputData(ARP_IO_PN, strInPort, (char*)&profinetIOPS, sizeof(profinetIOPS)))
    {
    	Log::Info("Read Value of {0}: 0x{1:#04x}", strInPort, profinetIOPS);

    	uint8_t profinetIOCS = 0;
		if(ReadInputData(ARP_IO_PN, "Arp.Io.PnC/9.OCS", (char*)&profinetIOCS, sizeof(profinetIOCS)))
		{
			Log::Info("Read Value of {0}: 0x{1:#04x}", strInPort, profinetIOCS);

			bRet = true;
		}
    }

    return(bRet);
}

// read data from a fieldbus input frame
bool CSampleRuntime::ReadInputData(const char* fbIoSystemName, const char* portName, char* pValue, size_t valueSize)
{
	bool bRet = false;

    TGdsBuffer* gdsBuffer = NULL;
    if(ArpPlcIo_GetBufferPtrByPortName(fbIoSystemName, portName, &gdsBuffer))
    {
		size_t offset = 0;
		if(ArpPlcGds_GetVariableOffset(gdsBuffer, portName, &offset))
		{
			// Begin read operation, memory buffer will be locked
			char* readDataBufferPage;
			if(ArpPlcGds_BeginRead(gdsBuffer, &readDataBufferPage))
			{
				// Copy data from gds Buffer
				char* dataAddress = readDataBufferPage + offset;
				memcpy(pValue, dataAddress, valueSize);

				// unlock buffer
				if(ArpPlcGds_EndRead(gdsBuffer))
				{
					bRet = true;
				}
				else
				{
					Log::Error("ArpPlcGds_EndRead failed");
				}
			}
			else
			{
				Log::Error("ArpPlcGds_BeginRead failed");
				ArpPlcGds_EndRead(gdsBuffer);
			}
		}
		else
		{
			Log::Error("ArpPlcGds_GetVariableOffset failed");
		}
    }
    else
    {
    	Log::Error("ArpPlcIo_GetBufferPtrByPortName failed");
    }

    // Release the GdsBuffer and free internal resources
    if(gdsBuffer != NULL)
    {
    	if(!ArpPlcIo_ReleaseGdsBuffer(gdsBuffer))
    	{
    		Log::Error("ArpPlcIo_ReleaseGdsBuffer failed");
    	}
    }

    return(bRet);
}

// write data to a fieldbus output frame
bool CSampleRuntime::WriteOutputData(const char* fbIoSystemName, const char* portName, const char* pValue, size_t valueSize)
{
	bool bRet = false;

    TGdsBuffer* gdsBuffer = NULL;
    if(ArpPlcIo_GetBufferPtrByPortName(fbIoSystemName, portName, &gdsBuffer))
    {
        size_t offset = 0;
        if(ArpPlcGds_GetVariableOffset(gdsBuffer, portName, &offset))
        {
            // Begin write operation, memory buffer will be locked
            char* dataBufferPage = NULL;
            if(ArpPlcGds_BeginWrite(gdsBuffer, &dataBufferPage))
            {
        		// Copy data to gds Buffer
        		char* dataAddress = dataBufferPage + offset;
        		memcpy(dataAddress, pValue, valueSize);

        		// unlock buffer
        		if(ArpPlcGds_EndWrite(gdsBuffer))
        		{
        			bRet = true;
        		}
        		else
        		{
        			Log::Error("ArpPlcGds_EndWrite failed");
        		}
            }
            else
            {
            	Log::Error("ArpPlcGds_BeginWrite failed");
                ArpPlcGds_EndWrite(gdsBuffer);
            }
        }
        else
        {
        	Log::Error("ArpPlcGds_GetVariableOffset failed");
        }
    }
    else
    {
    	Log::Error("ArpPlcIo_GetBufferPtrByPortName failed");
    }

    // Release the GdsBuffer and free internal resources
    if(gdsBuffer != NULL)
    {
    	if(!ArpPlcIo_ReleaseGdsBuffer(gdsBuffer))
    	{
    		Log::Error("ArpPlcIo_ReleaseGdsBuffer failed");
    	}
    }

    return(bRet);
}

// callback function for the PLC state
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
        	g_pRT->StartProcessing();

        	break;
        case PlcOperation_StartWarm:
        	Log::Info("Call of PLC Start Warm");

        	// when this state-change occurred, the firmware is ready to serve requests.
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

