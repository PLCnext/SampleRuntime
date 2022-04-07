 /******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleRTThread.cpp
 *
 *  Created on: Jul 12, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#include "CSampleRTThread.h"

#define ARP_IO_AXIO "Arp.Io.AxlC"	// ID of AXIO IO Component
#define ARP_IO_PN	"Arp.Io.PnC"	// ID of PROFINET IO Component

#define RTCYCLETIME 1000			// Cycletime of RT-Thread in us. Use only multiple of 500

CSampleRTThread::CSampleRTThread()
      : m_zRTCycleThread(),
        m_zLoggingThread(),
        m_bInitialized(false),
        m_bDoCycle(false),
        m_bFirstRTCycle(true),
        m_pGdsInBuffer(NULL),
        m_pGdsOutBuffer(NULL)
{
}

CSampleRTThread::~CSampleRTThread()
{
}

/// @brief	Init and start the RT- and logging-thread
/// @return	true: success, false: failure
bool CSampleRTThread::Init()
{
    if(m_bInitialized)
    {
        // already initialized
        return(true);
    }

    Log::Info("CSampleRTThread::Init"); /******************************************************************************
 *
 *  Copyright (c) 2019 Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CSampleRTThread.cpp
 *
 *  Created on: Jul 12, 2019
 *      Author: Steffen Schlette
 *
 ******************************************************************************/

#include "CSampleRTThread.h"

#define ARP_IO_AXIO "Arp.Io.AxlC"	// ID of AXIO IO Component
#define ARP_IO_PN	"Arp.Io.PnC"	// ID of PROFINET IO Component

#define RTCYCLETIME 1000			// Cycletime of RT-Thread in us. Use only multiple of 500

CSampleRTThread::CSampleRTThread()
      : m_zRTCycleThread(),
        m_zLoggingThread(),
        m_bInitialized(false),
        m_bDoCycle(false),
        m_bFirstRTCycle(true),
        m_pGdsInBuffer(NULL),
        m_pGdsOutBuffer(NULL)
{
}

CSampleRTThread::~CSampleRTThread()
{
}

/// @brief	Init and start the RT- and logging-thread
/// @return	true: success, false: failure
bool CSampleRTThread::Init()
{
    if(m_bInitialized)
    {
        // already initialized
        return(true);
    }

    Log::Info("CSampleRTThread::Init");

    bool bRet = false;

    // create a realtime worker thread for AXIO access
    // select a priority in the range of ESM-tasks (67 to 82) to avoid conflicting
    // with the PLCnext runtime. If the AXIO-Bus is used with a realtime priority,
    // it is not recommended to use any ESM tasks in parallel!
    // 1. it will lead to unexpected AXIO update-rates. The AXIO update will run with 500us,
    //    if no cyclic ESM-task is defined. Otherwise the update-rate is calculated on the basis
    //    of the defined cyclic ESM-tasks
    // 2. realtime-tasks might use the AXIO-bus concurrently which could lead to jitter or watchdogs
    //    even with non-cyclic tasks like idle-tasks

    struct sched_param param;
    param.sched_priority = 80;
    pthread_attr_t attr;

    if(pthread_attr_init(&attr) == 0)
    {
        if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) == 0)
        {
            if(pthread_attr_setschedparam(&attr, &param) == 0)
            {
                if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0)
                {
                    // this call will fail due to lacking permissions, if the process was not configured with
                    // the correct capabilities. You can check the needed capabilities at the
                    // PLCnext runtime:
                    //
                    // root@axcf2152:~# getcap /usr/bin/Arp.System.Application
                    // /usr/bin/Arp.System.Application = cap_net_bind_service,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_sys_boot,cap_sys_nice,cap_sys_time+ep
                    //
                    // and set the same capabilities to the application
                    // root@axcf2152:~# setcap cap_net_bind_service,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_sys_boot,cap_sys_nice,cap_sys_time+ep /opt/PLCnextSampleRuntime/PLCnextSampleRuntime
                    //
                    // for convenience, the debug-script will automatically set the capabilities after download
                    if(pthread_create(&m_zRTCycleThread, &attr, CSampleRTThread::RTStaticCycle, this) == 0)
                    {
                        // create a simple worker thread for RSC-subscriptions
                        if(pthread_create(&m_zLoggingThread, NULL, CSampleRTThread::StaticLoggingCycle, this) == 0)
                        {
                            m_bInitialized = true;
                            bRet = true;
                        }
                        else
                        {
                            Log::Error("Error calling pthread_create (realtime logging thread)");
                        }
                    }
                    else
                    {
                        Log::Error("Error calling pthread_create (realtime thread)");
                    }
                }
                else
                {
                    Log::Error("Error calling pthread_attr_setinheritsched");
                }
            }
            else
            {
                Log::Error("Error calling pthread_attr_setschedparam");
            }
        }
        else
        {
            Log::Error("Error calling pthread_attr_setschedpolicy");
        }
    }
    else
    {
        Log::Error("Error calling pthread_attr_init");
    }

    return(bRet);
}

/// @brief	The realtime thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleRTThread::StartProcessing()
{
    Log::Info("Start RT processing");

    bool bRet = false;

    // get in- and out-buffer of AXIO-bus (check *.tic-files for the ID)
    if(ArpPlcIo_GetBufferPtrByBufferID(ARP_IO_AXIO, "1:IN", &m_pGdsInBuffer))
    {
        if(ArpPlcIo_GetBufferPtrByBufferID(ARP_IO_AXIO, "1:OUT", &m_pGdsOutBuffer))
        {
            // get offsets of I/Os in Buffer
            // create port names. Format: IOSystem/DeviceNumber.NameOfIO
            m_strInByte = Formatter::FormatCommon("{}/0.~DI8", ARP_IO_AXIO);
            m_strIn04 = Formatter::FormatCommon("{}/0.IN04", ARP_IO_AXIO);
            m_strIn05 = Formatter::FormatCommon("{}/0.IN05", ARP_IO_AXIO);
            m_strIn06 = Formatter::FormatCommon("{}/0.IN06", ARP_IO_AXIO);
            m_strIn07 = Formatter::FormatCommon("{}/0.IN07", ARP_IO_AXIO);
            m_strOut04 = Formatter::FormatCommon("{}/0.OUT04", ARP_IO_AXIO);
            m_strOut05 = Formatter::FormatCommon("{}/0.OUT05", ARP_IO_AXIO);
            m_strOut06 = Formatter::FormatCommon("{}/0.OUT06", ARP_IO_AXIO);
            m_strOut07 = Formatter::FormatCommon("{}/0.OUT07", ARP_IO_AXIO);
            AddInput(m_strInByte, 1, false);
            AddInput(m_strIn04, 1, true);
            AddInput(m_strIn05, 1, true);
            AddInput(m_strIn06, 1, true);
            AddInput(m_strIn07, 1, true);
            AddOutput(m_strOut04, 1, true);
            AddOutput(m_strOut05, 1, true);
            AddOutput(m_strOut06, 1, true);
            AddOutput(m_strOut07, 1, true);


            /* Existing axioline system variables:
            Name										Datatype
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_HI         BYTE
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_LOW        BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_HI          BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_LOW         BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_HI        BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_LOW       BYTE
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PW         BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PF         BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_BUS        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RUN        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_ACT        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RDY        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_SYSFAIL    BOOL */

            String strStatusRegister = Formatter::FormatCommon("{}/AXIO_DIAG_STATUS_REG", ARP_IO_AXIO);
            AddInput(strStatusRegister, 2, false);	// 16 bit

            m_bDoCycle = true;
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcIo_GetBufferPtrByBufferID for output buffer");
        }
    }
    else
    {
        Log::Error("Error calling ArpPlcIo_GetBufferPtrByBufferID for input buffer");
    }

#ifdef PROFINET_SYS_VAR_EXAMPLE
    // Access System Variables of the PROFINET Controller
    TGdsBuffer* pGdsPncSysVars = nullptr;
    if(ArpPlcIo_GetBufferPtrByBufferID(ARP_IO_PN, "SysVars", &pGdsPncSysVars))
    {
        String pnioConfigStatusActiveName = Formatter::FormatCommon("{}/PNIO_CONFIG_STATUS_ACTIVE", ARP_IO_PN);
        String pnioForcePrimaryName = Formatter::FormatCommon("{}/PNIO_FORCE_PRIMARY", ARP_IO_PN);

        // Existing PNIO system variables:
        // Name                                     Datatype
        // Arp.Io.PnC/PNIO_SYSTEM_BF                BOOL
        // Arp.Io.PnC/PNIO_SYSTEM_SF                BOOL
        // Arp.Io.PnC/PNIO_MAINTENANCE_DEMANDED     BOOL
        // Arp.Io.PnC/PNIO_MAINTENANCE_REQUIRED     BOOL
        // Arp.Io.PnC/PNIO_CONFIG_STATUS            Bitstring16
        // Arp.Io.PnC/PNIO_CONFIG_STATUS_ACTIVE     BOOL
        // Arp.Io.PnC/PNIO_CONFIG_STATUS_READY      BOOL
        // Arp.Io.PnC/PNIO_CONFIG_STATUS_CFG_FAULT  BOOL
        // Arp.Io.PnC/PNIO_FORCE_FAILSAFE           BOOL
        // Arp.Io.PnC/PNIO_FORCE_PRIMARY            BOOL

        size_t offsetConfigStatusActive = 0;
        unsigned char bitOffsetConfigStatusActive = 0;
        if(ArpPlcGds_GetVariableBitOffset(pGdsPncSysVars, pnioConfigStatusActiveName, &offsetConfigStatusActive, &bitOffsetConfigStatusActive))
        {
            Log::Info("Offset for {0}: {1}.{2}", pnioConfigStatusActiveName, offsetConfigStatusActive, (int)bitOffsetConfigStatusActive);
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableBitOffset for {0}", pnioConfigStatusActiveName);
        }

        size_t offsetForcePrimary = 0;
        unsigned char bitOffsetForcePrimary = 0;
        if(ArpPlcGds_GetVariableBitOffset(pGdsPncSysVars, pnioForcePrimaryName, &offsetForcePrimary, &bitOffsetForcePrimary))
        {
            Log::Info("Offset for {0}: {1}.{2}", pnioForcePrimaryName, offsetForcePrimary, (int)bitOffsetForcePrimary);
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableBitOffset for {0}", pnioForcePrimaryName);
        }

        bool result = ArpPlcIo_ReleaseGdsBuffer(pGdsPncSysVars);
        pGdsPncSysVars = nullptr;

        // Further notes
        // =============
        // Accessing the data:
        // Reading the data from the GDS shall be in a section inside ArpPlcGds_BeginRead() and ArpPlcGds_EndRead() using the buffer handle (like pGdsPncSysVars)
        // Of course this is typically not be done during StartProcessing() so the buffer handle as well as the buffer offsets should be stored somehow.
        // Releasing the data:
        // Typically ArpPlcIo_ReleaseGdsBuffer() should be called when the buffer is not used any more e.g. in StopProcessing()
    }
    else
    {
        Log::Error("Error calling ArpPlcIo_GetBufferPtrByBufferID for PN SysVars buffer");
    }
#endif

    return(bRet);
}

/// @brief	The realtime thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleRTThread::StopProcessing()
{
    Log::Info("Stop RT processing");

    bool bRet = false;

    // this is not threadsafe! In a real application you need to wait for end of cycle before deleting the objects
    m_bDoCycle = false;

    ArpPlcIo_ReleaseGdsBuffer(m_pGdsInBuffer);
    m_pGdsInBuffer = NULL;
    ArpPlcIo_ReleaseGdsBuffer(m_pGdsOutBuffer);
    m_pGdsOutBuffer = NULL;

    // clear lists of inputs and outputs and free resources
    std::map<std::string, RAWIO>::iterator it = m_zInputsMap.begin();
    while(it != m_zInputsMap.end())
    {
        free(it->second.pValue);
        it++;
    }

    it = m_zOutputsMap.begin();
    while(it != m_zOutputsMap.end())
    {
        free(it->second.pValue);
        it++;
    }

    m_zInputsMap.clear();
    m_zOutputsMap.clear();

    bRet = true;

    return(bRet);
}

/// @brief		static function for worker thread-entry of the realtime cycle function
/// @param p	pointer to thread object
void* CSampleRTThread::RTStaticCycle(void* p)
{
    if(p != NULL)
    {
        ((CSampleRTThread*)p)->RTCycle();
    }
    else
    {
        Log::Error("Null pointer in RTStaticCycle");
    }
    return(NULL);
}

/// @brief	realtime loop to process IOs of the AXIO bus
void CSampleRTThread::RTCycle()
{
    Log::Info("Call of CSampleRTThread::RTCycle");

    timespec zCycleTime;

    // CLOCK_MONOTONIC is used as time-base to avoid drifts and jumps in time
    // the realtime cycle interval is calculated with an absolute time from the beginning
    // of the program, not a relative time from cycle to cycle which could drift over time
    if(clock_gettime(CLOCK_MONOTONIC, &zCycleTime) == 0)
    {
        while(true)
        {
            timespec zCurrentTime;
            clock_gettime(CLOCK_MONOTONIC, &zCurrentTime);

            if(m_bFirstRTCycle == false)
            {
                // calculate monotonic cycle intervals
                timeAdd(zCycleTime, RTCYCLETIME);

                // check, if we have a realtime violation
                if(timeCmp(zCurrentTime, zCycleTime) > 0)
                {
                    // realtime violation, just log and recover in this example
                    Log::Error("Error realtime violation in realtime cycle");
                    Log::Error("current time: {0} sec {1} nsec cycle start: {2} sec {3} nsec ", zCurrentTime.tv_sec, zCurrentTime.tv_nsec, zCycleTime.tv_sec, zCycleTime.tv_nsec);

                    zCycleTime = zCurrentTime;
                    timeAdd(zCycleTime, RTCYCLETIME);	// calculate wakeup-time for next cycle
                }
            }
            else
            {
                // in the first cycle we wait for the next full second
                // plus a fixed offset (TBD)
                zCycleTime.tv_sec +=1;
                zCycleTime.tv_nsec = 450000;
                m_bFirstRTCycle = false;
            }

            // schedule next cycle
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &zCycleTime, NULL);

            if(m_bDoCycle)
            {
                // do some processing
                ReadInputData();
                DoLogic();
                WriteOutputData();
            }
        }
    }
    else
    {
        Log::Error("Error getting monotonic clock");
    }
}

/// @brief		static function for worker thread-entry of the realtime cycle function
/// @param p	pointer to thread object
void* CSampleRTThread::StaticLoggingCycle(void* p)
{
    if(p != NULL)
    {
        ((CSampleRTThread*)p)->LoggingCycle();
    }
    return(NULL);
}

///	@brief	thread for logging the realtime I/O data, this cannot be done in the realtime thread
/// 		without violating the realtime
void CSampleRTThread::LoggingCycle()
{
    while(true)
    {
        if(m_bDoCycle)
        {
            //Log::Info("************* RT-Thread values ****************");

            // log status of I/Os of RT-thread
            // you can check the log messages in the local log-file of this application, usually in a subfolder named "Logs"
            std::map<std::string, RAWIO>::iterator it = m_zInputsMap.begin();
            while(it != m_zInputsMap.end())
            {
                RAWIO& zRawIO = it->second;
                LogIO(zRawIO);

                it++;
            }

            it = m_zOutputsMap.begin();
            while(it != m_zOutputsMap.end())
            {
                RAWIO& zRawIO = it->second;
                LogIO(zRawIO);

                it++;
            }
        }
        WAIT100ms
    }
}

/// @brief			log a single I/O
/// @param zRawIO	reference to I/O
void CSampleRTThread::LogIO(RAWIO& zRawIO)
{
    if(zRawIO.bIsBool)
    {
        Log::Info("{0}: {1}", zRawIO.strID, zRawIO.bValue);
    }
    else
    {
        // log first byte of value
        // if you are wondering about the formatting syntax of the Log-Class, check
        // http://fmtlib.net/latest/syntax.html
        unsigned char ucValue = *(zRawIO.pValue);
        Log::Info("{0}: {1:#02x}", zRawIO.strID, ucValue);
    }
}

/// @brief			add one input to the list of inputs
/// @param strID	identifier of input (check *.tic-files for the name)
/// @param zSize	size in bytes
/// @param bIsBool	true, if it is a single-bit value
/// @return			true: success, false: failure
bool CSampleRTThread::AddInput(std::string strID, size_t zSize, bool bIsBool)
{
    bool bRet = false;
    RAWIO zIO;
    zIO.strID = strID;
    zIO.bIsBool = bIsBool;
    zIO.zSize = zSize;
    zIO.pValue = (unsigned char*)malloc(zSize);
    memset(zIO.pValue, 0, zSize);

    if(bIsBool)
    {
        // get byte- and bit-offset in AXIO-frame
        unsigned char ucBitOffset;
        if(ArpPlcGds_GetVariableBitOffset(m_pGdsInBuffer, String(zIO.strID), &(zIO.nOffset), &(ucBitOffset)))
        {
            zIO.ucBitMask = 1 << ucBitOffset;
            m_zInputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableBitOffset for {0}", strID);
        }
    }
    else
    {
        // get byte-offset in AXIO-frame
        if(ArpPlcGds_GetVariableOffset(m_pGdsInBuffer, String(zIO.strID), &(zIO.nOffset)))
        {
            m_zInputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableOffset for {0}", strID);
        }
    }

    return(bRet);
}

/// @brief			add one output to the list of outputs
/// @param strID	identifier of output (check *.tic-files for the name)
/// @param zSize	size in bytes
/// @param bIsBool	true, if it is a single-bit value
/// @return			true: success, false: failure
bool CSampleRTThread::AddOutput(std::string strID, size_t zSize, bool bIsBool)
{
    bool bRet = false;
    RAWIO zIO;
    zIO.strID = strID;
    zIO.bIsBool = bIsBool;
    zIO.zSize = zSize;
    zIO.pValue = (unsigned char*)malloc(zSize);
    memset(zIO.pValue, 0, zSize);

    if(bIsBool)
    {
        // get byte- and bit-offset in AXIO-frame
        unsigned char ucBitOffset;
        if(ArpPlcGds_GetVariableBitOffset(m_pGdsOutBuffer, String(zIO.strID), &(zIO.nOffset), &(ucBitOffset)))
        {
            zIO.ucBitMask = 1 << ucBitOffset;
            m_zOutputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableBitOffset for {0}", strID);
        }
    }
    else
    {
        // get byte-offset in AXIO-frame
        if(ArpPlcGds_GetVariableOffset(m_pGdsOutBuffer, String(zIO.strID), &(zIO.nOffset)))
        {
            m_zOutputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableOffset for {0}", strID);
        }
    }

    return(bRet);
}

/// @brief		read inputs from AXIO frame
/// @return		true: success, false: failure
bool CSampleRTThread::ReadInputData(void)
{
    bool bRet = true;

    char* pFrame = NULL;

    // begin read operation, memory buffer will be locked
    if(ArpPlcGds_BeginRead(m_pGdsInBuffer, &pFrame))
    {
        std::map<std::string, RAWIO>::iterator it = m_zInputsMap.begin();
        while(it != m_zInputsMap.end())
        {
            if(ReadInputValue(pFrame, it->second) == false)
            {
                bRet = false;
            }

            // logging of IO values is done in Non-RT thread to not violate realtime

            it++;
        }

        // unlock buffer
        if(ArpPlcGds_EndRead(m_pGdsInBuffer))
        {
        }
        else
        {
            Log::Error("ArpPlcGds_EndRead failed");
            bRet = false;
        }
    }
    else
    {
        // returned false, data is not (yet) valid
        ArpPlcGds_EndRead(m_pGdsInBuffer);
        bRet = false;
    }

    return(bRet);
}

/// @brief		do some processing
/// @return		true: success, false: failure
bool CSampleRTThread::DoLogic(void)
{
    bool bRet = false;

    // an AND logic
    if(m_zInputsMap[m_strIn04].bValue == true && m_zInputsMap[m_strIn05].bValue == true)
    {
        m_zOutputsMap[m_strOut05].bValue = true;
    }
    else
    {
        m_zOutputsMap[m_strOut05].bValue = false;
    }

    // useful for realtime measurements with an oscilloscope

    // create a toggle
    m_zOutputsMap[m_strOut04].bValue = !m_zOutputsMap[m_strOut04].bValue;

    // read one input and forward it to an output
    m_zOutputsMap[m_strOut06].bValue = m_zInputsMap[m_strIn04].bValue;


    return(bRet);
}

/// @brief		write outputs to an AXIO frame
/// @return		true: success, false: failure
bool CSampleRTThread::WriteOutputData(void)
{
    bool bRet = true;

    char* pFrame;
    if(ArpPlcGds_BeginWrite(m_pGdsOutBuffer, &pFrame))
    {
        std::map<std::string, RAWIO>::iterator it = m_zOutputsMap.begin();
        while(it != m_zOutputsMap.end())
        {
            if(WriteOutputValue(pFrame, it->second) == false)
            {
                bRet = false;
            }

            it++;
        }

        // unlock buffer
        if(ArpPlcGds_EndWrite(m_pGdsOutBuffer))
        {
        }
        else
        {
            Log::Error("ArpPlcGds_EndWrite failed");
            bRet = false;
        }
    }
    else
    {
        Log::Error("ArpPlcGds_BeginWrite failed");
        bRet = false;
    }

    return(bRet);
}

/// @brief			read data from a fieldbus input frame
/// @param pFrame	frame pointer
/// @param zIO		reference to RAWIO
/// @return			true: success, false: failure
bool CSampleRTThread::ReadInputValue(const char* pFrame, RAWIO& zIO)
{
    bool bRet = true;

    // Copy data from gds Buffer
    const char* pDataAddress = pFrame + zIO.nOffset;

    // is it a single-bit-value?
    if(zIO.bIsBool)
    {
        // get value of bit
        zIO.bValue = ((*pDataAddress) & zIO.ucBitMask) != 0;
    }
    else
    {
        // get number of bytes
        memcpy(zIO.pValue, pDataAddress, zIO.zSize);
    }

    return(bRet);
}

/// @brief			write data to a fieldbus output frame
/// @param pFrame	frame pointer
/// @param zIO		reference to RAWIO
/// @return			true: success, false: failure
bool CSampleRTThread::WriteOutputValue(char* pFrame, RAWIO& zIO)
{
    bool bRet = true;

    // Copy data to gds Buffer
    char* pDataAddress = pFrame + zIO.nOffset;

    // is it a single-bit-value?
    if(zIO.bIsBool)
    {
        // set value of bit
        if(zIO.bValue == true)
        {
            (*pDataAddress) |= zIO.ucBitMask;
        }
        else
        {
            (*pDataAddress) &= ~zIO.ucBitMask;
        }
    }
    else
    {
        // set number of bytes
        memcpy(pDataAddress, zIO.pValue, zIO.zSize);
    }

    return(bRet);
}

/// @brief			addition helper function for missing time calculations
/// @param zValue	reference to time value
/// @param lAdd		add value to time
void timeAdd(struct timespec& zValue, long lAdd)
{
    zValue.tv_nsec += (lAdd * 1E3);

    if(zValue.tv_nsec > 1E9)
    {
        zValue.tv_nsec = zValue.tv_nsec - 1E9;
        zValue.tv_sec += 1;
    }
}

/// @brief			compare helper function for missing time calculations
/// @param zFirst	reference to time value
/// @param zSecond	reference to time value
int timeCmp(struct timespec& zFirst, struct timespec& zSecond)
{
    if(zFirst.tv_sec > zSecond.tv_sec)
    {
        return 1;
    }
    else if(zFirst.tv_sec < zSecond.tv_sec)
    {
        return -1;
    }
    else	// equal
    {
        if(zFirst.tv_nsec > zSecond.tv_nsec)
        {
            return 1;
        }
        else if(zFirst.tv_nsec < zSecond.tv_nsec)
        {
            return -1;
        }
        else	// equal
        {
            return 0;
        }
    }
}


    bool bRet = false;

    // create a realtime worker thread for AXIO access
    // select a priority in the range of ESM-tasks (67 to 82) to avoid conflicting
    // with the PLCnext runtime. If the AXIO-Bus is used with a realtime priority,
    // it is not recommended to use any ESM tasks in parallel!
    // 1. it will lead to unexpected AXIO update-rates. The AXIO update will run with 500us,
    //    if no cyclic ESM-task is defined. Otherwise the update-rate is calculated on the basis
    //    of the defined cyclic ESM-tasks
    // 2. realtime-tasks might use the AXIO-bus concurrently which could lead to jitter or watchdogs
    //    even with non-cyclic tasks like idle-tasks

    struct sched_param param;
    param.sched_priority = 80;
    pthread_attr_t attr;

    if(pthread_attr_init(&attr) == 0)
    {
        if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) == 0)
        {
            if(pthread_attr_setschedparam(&attr, &param) == 0)
            {
                if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0)
                {
                    // this call will fail due to lacking permissions, if the process was not configured with
                    // the correct capabilities. You can check the needed capabilities at the
                    // PLCnext runtime:
                    //
                    // root@axcf2152:~# getcap /usr/bin/Arp.System.Application
                    // /usr/bin/Arp.System.Application = cap_net_bind_service,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_sys_boot,cap_sys_nice,cap_sys_time+ep
                    //
                    // and set the same capabilities to the application
                    // root@axcf2152:~# setcap cap_net_bind_service,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_sys_boot,cap_sys_nice,cap_sys_time+ep /opt/PLCnextSampleRuntime/PLCnextSampleRuntime
                    //
                    // for convenience, the debug-script will automatically set the capabilities after download
                    if(pthread_create(&m_zRTCycleThread, &attr, CSampleRTThread::RTStaticCycle, this) == 0)
                    {
                        // create a simple worker thread for RSC-subscriptions
                        if(pthread_create(&m_zLoggingThread, NULL, CSampleRTThread::StaticLoggingCycle, this) == 0)
                        {
                            m_bInitialized = true;
                            bRet = true;
                        }
                        else
                        {
                            Log::Error("Error calling pthread_create (realtime logging thread)");
                        }
                    }
                    else
                    {
                        Log::Error("Error calling pthread_create (realtime thread)");
                    }
                }
                else
                {
                    Log::Error("Error calling pthread_attr_setinheritsched");
                }
            }
            else
            {
                Log::Error("Error calling pthread_attr_setschedparam");
            }
        }
        else
        {
            Log::Error("Error calling pthread_attr_setschedpolicy");
        }
    }
    else
    {
        Log::Error("Error calling pthread_attr_init");
    }

    return(bRet);
}

/// @brief	The realtime thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleRTThread::StartProcessing()
{
    Log::Info("Start RT processing");

    bool bRet = false;

    // get in- and out-buffer of AXIO-bus (check *.tic-files for the ID)
    if(ArpPlcIo_GetBufferPtrByBufferID(ARP_IO_AXIO, "1:IN", &m_pGdsInBuffer))
    {
        if(ArpPlcIo_GetBufferPtrByBufferID(ARP_IO_AXIO, "1:OUT", &m_pGdsOutBuffer))
        {
            // get offsets of I/Os in Buffer
            // create port names. Format: IOSystem/DeviceNumber.NameOfIO
            m_strInByte = Formatter::FormatCommon("{}/0.~DI8", ARP_IO_AXIO);
            m_strIn04 = Formatter::FormatCommon("{}/0.IN04", ARP_IO_AXIO);
            m_strIn05 = Formatter::FormatCommon("{}/0.IN05", ARP_IO_AXIO);
            m_strIn06 = Formatter::FormatCommon("{}/0.IN06", ARP_IO_AXIO);
            m_strIn07 = Formatter::FormatCommon("{}/0.IN07", ARP_IO_AXIO);
            m_strOut04 = Formatter::FormatCommon("{}/0.OUT04", ARP_IO_AXIO);
            m_strOut05 = Formatter::FormatCommon("{}/0.OUT05", ARP_IO_AXIO);
            m_strOut06 = Formatter::FormatCommon("{}/0.OUT06", ARP_IO_AXIO);
            m_strOut07 = Formatter::FormatCommon("{}/0.OUT07", ARP_IO_AXIO);
            AddInput(m_strInByte, 1, false);
            AddInput(m_strIn04, 1, true);
            AddInput(m_strIn05, 1, true);
            AddInput(m_strIn06, 1, true);
            AddInput(m_strIn07, 1, true);
            AddOutput(m_strOut04, 1, true);
            AddOutput(m_strOut05, 1, true);
            AddOutput(m_strOut06, 1, true);
            AddOutput(m_strOut07, 1, true);


            /* Existing axioline system variables:
            Name										Datatype
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_HI         BYTE
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_LOW        BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_HI          BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_REG_LOW         BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_HI        BYTE
            Arp.Io.AxlC/AXIO_DIAG_PARAM_2_REG_LOW       BYTE
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PW         BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_PF         BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_BUS        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RUN        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_ACT        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_RDY        BOOL
            Arp.Io.AxlC/AXIO_DIAG_STATUS_REG_SYSFAIL    BOOL */

            String strStatusRegister = Formatter::FormatCommon("{}/AXIO_DIAG_STATUS_REG", ARP_IO_AXIO);
            AddInput(strStatusRegister, 2, false);	// 16 bit

            m_bDoCycle = true;
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcIo_GetBufferPtrByBufferID for output buffer");
        }
    }
    else
    {
        Log::Error("Error calling ArpPlcIo_GetBufferPtrByBufferID for input buffer");
    }

    return(bRet);
}

/// @brief	The realtime thread will run continuously after creation but the processing of
/// 		I/Os can be started and stopped e.g. if a new PLCnext Engineer Program was loaded
/// @return	true: success, false: failure
bool CSampleRTThread::StopProcessing()
{
    Log::Info("Stop RT processing");

    bool bRet = false;

    // this is not threadsafe! In a real application you need to wait for end of cycle before deleting the objects
    m_bDoCycle = false;

    m_pGdsInBuffer = NULL;
    m_pGdsOutBuffer = NULL;

    // clear lists of inputs and outputs and free resources
    std::map<std::string, RAWIO>::iterator it = m_zInputsMap.begin();
    while(it != m_zInputsMap.end())
    {
        free(it->second.pValue);
        it++;
    }

    it = m_zOutputsMap.begin();
    while(it != m_zOutputsMap.end())
    {
        free(it->second.pValue);
        it++;
    }

    m_zInputsMap.clear();
    m_zOutputsMap.clear();

    bRet = true;

    return(bRet);
}

/// @brief		static function for worker thread-entry of the realtime cycle function
/// @param p	pointer to thread object
void* CSampleRTThread::RTStaticCycle(void* p)
{
    if(p != NULL)
    {
        ((CSampleRTThread*)p)->RTCycle();
    }
    else
    {
        Log::Error("Null pointer in RTStaticCycle");
    }
    return(NULL);
}

/// @brief	realtime loop to process IOs of the AXIO bus
void CSampleRTThread::RTCycle()
{
    Log::Info("Call of CSampleRTThread::RTCycle");

    timespec zCycleTime;

    // CLOCK_MONOTONIC is used as time-base to avoid drifts and jumps in time
    // the realtime cycle interval is calculated with an absolute time from the beginning
    // of the program, not a relative time from cycle to cycle which could drift over time
    if(clock_gettime(CLOCK_MONOTONIC, &zCycleTime) == 0)
    {
        while(true)
        {
            timespec zCurrentTime;
            clock_gettime(CLOCK_MONOTONIC, &zCurrentTime);

            if(m_bFirstRTCycle == false)
            {
                // calculate monotonic cycle intervals
                timeAdd(zCycleTime, RTCYCLETIME);

                // check, if we have a realtime violation
                if(timeCmp(zCurrentTime, zCycleTime) > 0)
                {
                    // realtime violation, just log and recover in this example
                    Log::Error("Error realtime violation in realtime cycle");
                    Log::Error("current time: {0} sec {1} nsec cycle start: {2} sec {3} nsec ", zCurrentTime.tv_sec, zCurrentTime.tv_nsec, zCycleTime.tv_sec, zCycleTime.tv_nsec);

                    zCycleTime = zCurrentTime;
                    timeAdd(zCycleTime, RTCYCLETIME);	// calculate wakeup-time for next cycle
                }
            }
            else
            {
                // in the first cycle we wait for the next full second
                // plus a fixed offset (TBD)
                zCycleTime.tv_sec +=1;
                zCycleTime.tv_nsec = 450000;
                m_bFirstRTCycle = false;
            }

            // schedule next cycle
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &zCycleTime, NULL);

            if(m_bDoCycle)
            {
                // do some processing
                ReadInputData();
                DoLogic();
                WriteOutputData();
            }
        }
    }
    else
    {
        Log::Error("Error getting monotonic clock");
    }
}

/// @brief		static function for worker thread-entry of the realtime cycle function
/// @param p	pointer to thread object
void* CSampleRTThread::StaticLoggingCycle(void* p)
{
    if(p != NULL)
    {
        ((CSampleRTThread*)p)->LoggingCycle();
    }
    return(NULL);
}

///	@brief	thread for logging the realtime I/O data, this cannot be done in the realtime thread
/// 		without violating the realtime
void CSampleRTThread::LoggingCycle()
{
    while(true)
    {
        if(m_bDoCycle)
        {
            //Log::Info("************* RT-Thread values ****************");

            // log status of I/Os of RT-thread
            // you can check the log messages in the local log-file of this application, usually in a subfolder named "Logs"
            std::map<std::string, RAWIO>::iterator it = m_zInputsMap.begin();
            while(it != m_zInputsMap.end())
            {
                RAWIO& zRawIO = it->second;
                LogIO(zRawIO);

                it++;
            }

            it = m_zOutputsMap.begin();
            while(it != m_zOutputsMap.end())
            {
                RAWIO& zRawIO = it->second;
                LogIO(zRawIO);

                it++;
            }
        }
        WAIT100ms
    }
}

/// @brief			log a single I/O
/// @param zRawIO	reference to I/O
void CSampleRTThread::LogIO(RAWIO& zRawIO)
{
    if(zRawIO.bIsBool)
    {
        Log::Info("{0}: {1}", zRawIO.strID, zRawIO.bValue);
    }
    else
    {
        // log first byte of value
        // if you are wondering about the formatting syntax of the Log-Class, check
        // http://fmtlib.net/latest/syntax.html
        unsigned char ucValue = *(zRawIO.pValue);
        Log::Info("{0}: {1:#02x}", zRawIO.strID, ucValue);
    }
}

/// @brief			add one input to the list of inputs
/// @param strID	identifier of input (check *.tic-files for the name)
/// @param zSize	size in bytes
/// @param bIsBool	true, if it is a single-bit value
/// @return			true: success, false: failure
bool CSampleRTThread::AddInput(std::string strID, size_t zSize, bool bIsBool)
{
    bool bRet = false;
    RAWIO zIO;
    zIO.strID = strID;
    zIO.bIsBool = bIsBool;
    zIO.zSize = zSize;
    zIO.pValue = (unsigned char*)malloc(zSize);
    memset(zIO.pValue, 0, zSize);

    if(bIsBool)
    {
        // get byte- and bit-offset in AXIO-frame
        unsigned char ucBitOffset;
        if(ArpPlcGds_GetVariableBitOffset(m_pGdsInBuffer, String(zIO.strID), &(zIO.nOffset), &(ucBitOffset)))
        {
            zIO.ucBitMask = 1 << ucBitOffset;
            m_zInputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableBitOffset for {0}", strID);
        }
    }
    else
    {
        // get byte-offset in AXIO-frame
        if(ArpPlcGds_GetVariableOffset(m_pGdsInBuffer, String(zIO.strID), &(zIO.nOffset)))
        {
            m_zInputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableOffset for {0}", strID);
        }
    }

    return(bRet);
}

/// @brief			add one output to the list of outputs
/// @param strID	identifier of output (check *.tic-files for the name)
/// @param zSize	size in bytes
/// @param bIsBool	true, if it is a single-bit value
/// @return			true: success, false: failure
bool CSampleRTThread::AddOutput(std::string strID, size_t zSize, bool bIsBool)
{
    bool bRet = false;
    RAWIO zIO;
    zIO.strID = strID;
    zIO.bIsBool = bIsBool;
    zIO.zSize = zSize;
    zIO.pValue = (unsigned char*)malloc(zSize);
    memset(zIO.pValue, 0, zSize);

    if(bIsBool)
    {
        // get byte- and bit-offset in AXIO-frame
        unsigned char ucBitOffset;
        if(ArpPlcGds_GetVariableBitOffset(m_pGdsOutBuffer, String(zIO.strID), &(zIO.nOffset), &(ucBitOffset)))
        {
            zIO.ucBitMask = 1 << ucBitOffset;
            m_zOutputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableBitOffset for {0}", strID);
        }
    }
    else
    {
        // get byte-offset in AXIO-frame
        if(ArpPlcGds_GetVariableOffset(m_pGdsOutBuffer, String(zIO.strID), &(zIO.nOffset)))
        {
            m_zOutputsMap.insert(std::make_pair(strID, zIO));
            bRet = true;
        }
        else
        {
            Log::Error("Error calling ArpPlcGds_GetVariableOffset for {0}", strID);
        }
    }

    return(bRet);
}

/// @brief		read inputs from AXIO frame
/// @return		true: success, false: failure
bool CSampleRTThread::ReadInputData(void)
{
    bool bRet = true;

    char* pFrame = NULL;

    // begin read operation, memory buffer will be locked
    if(ArpPlcGds_BeginRead(m_pGdsInBuffer, &pFrame))
    {
        std::map<std::string, RAWIO>::iterator it = m_zInputsMap.begin();
        while(it != m_zInputsMap.end())
        {
            if(ReadInputValue(pFrame, it->second) == false)
            {
                bRet = false;
            }

            // logging of IO values is done in Non-RT thread to not violate realtime

            it++;
        }

        // unlock buffer
        if(ArpPlcGds_EndRead(m_pGdsInBuffer))
        {
        }
        else
        {
            Log::Error("ArpPlcGds_EndRead failed");
            bRet = false;
        }
    }
    else
    {
        // returned false, data is not (yet) valid
        ArpPlcGds_EndRead(m_pGdsInBuffer);
        bRet = false;
    }

    return(bRet);
}

/// @brief		do some processing
/// @return		true: success, false: failure
bool CSampleRTThread::DoLogic(void)
{
    bool bRet = false;

    // an AND logic
    if(m_zInputsMap[m_strIn04].bValue == true && m_zInputsMap[m_strIn05].bValue == true)
    {
        m_zOutputsMap[m_strOut05].bValue = true;
    }
    else
    {
        m_zOutputsMap[m_strOut05].bValue = false;
    }

    // useful for realtime measurements with an oscilloscope

    // create a toggle
    m_zOutputsMap[m_strOut04].bValue = !m_zOutputsMap[m_strOut04].bValue;

    // read one input and forward it to an output
    m_zOutputsMap[m_strOut06].bValue = m_zInputsMap[m_strIn04].bValue;


    return(bRet);
}

/// @brief		write outputs to an AXIO frame
/// @return		true: success, false: failure
bool CSampleRTThread::WriteOutputData(void)
{
    bool bRet = true;

    char* pFrame;
    if(ArpPlcGds_BeginWrite(m_pGdsOutBuffer, &pFrame))
    {
        std::map<std::string, RAWIO>::iterator it = m_zOutputsMap.begin();
        while(it != m_zOutputsMap.end())
        {
            if(WriteOutputValue(pFrame, it->second) == false)
            {
                bRet = false;
            }

            it++;
        }

        // unlock buffer
        if(ArpPlcGds_EndWrite(m_pGdsOutBuffer))
        {
        }
        else
        {
            Log::Error("ArpPlcGds_EndWrite failed");
            bRet = false;
        }
    }
    else
    {
        Log::Error("ArpPlcGds_BeginWrite failed");
        bRet = false;
    }

    return(bRet);
}

/// @brief			read data from a fieldbus input frame
/// @param pFrame	frame pointer
/// @param zIO		reference to RAWIO
/// @return			true: success, false: failure
bool CSampleRTThread::ReadInputValue(const char* pFrame, RAWIO& zIO)
{
    bool bRet = true;

    // Copy data from gds Buffer
    const char* pDataAddress = pFrame + zIO.nOffset;

    // is it a single-bit-value?
    if(zIO.bIsBool)
    {
        // get value of bit
        zIO.bValue = ((*pDataAddress) & zIO.ucBitMask) != 0;
    }
    else
    {
        // get number of bytes
        memcpy(zIO.pValue, pDataAddress, zIO.zSize);
    }

    return(bRet);
}

/// @brief			write data to a fieldbus output frame
/// @param pFrame	frame pointer
/// @param zIO		reference to RAWIO
/// @return			true: success, false: failure
bool CSampleRTThread::WriteOutputValue(char* pFrame, RAWIO& zIO)
{
    bool bRet = true;

    // Copy data to gds Buffer
    char* pDataAddress = pFrame + zIO.nOffset;

    // is it a single-bit-value?
    if(zIO.bIsBool)
    {
        // set value of bit
        if(zIO.bValue == true)
        {
            (*pDataAddress) |= zIO.ucBitMask;
        }
        else
        {
            (*pDataAddress) &= ~zIO.ucBitMask;
        }
    }
    else
    {
        // set number of bytes
        memcpy(pDataAddress, zIO.pValue, zIO.zSize);
    }

    return(bRet);
}

/// @brief			addition helper function for missing time calculations
/// @param zValue	reference to time value
/// @param lAdd		add value to time
void timeAdd(struct timespec& zValue, long lAdd)
{
    zValue.tv_nsec += (lAdd * 1E3);

    if(zValue.tv_nsec > 1E9)
    {
        zValue.tv_nsec = zValue.tv_nsec - 1E9;
        zValue.tv_sec += 1;
    }
}

/// @brief			compare helper function for missing time calculations
/// @param zFirst	reference to time value
/// @param zSecond	reference to time value
int timeCmp(struct timespec& zFirst, struct timespec& zSecond)
{
    if(zFirst.tv_sec > zSecond.tv_sec)
    {
        return 1;
    }
    else if(zFirst.tv_sec < zSecond.tv_sec)
    {
        return -1;
    }
    else	// equal
    {
        if(zFirst.tv_nsec > zSecond.tv_nsec)
        {
            return 1;
        }
        else if(zFirst.tv_nsec < zSecond.tv_nsec)
        {
            return -1;
        }
        else	// equal
        {
            return 0;
        }
    }
}
