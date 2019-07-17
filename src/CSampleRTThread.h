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

#ifndef CSAMPLERTTHREAD_H_
#define CSAMPLERTTHREAD_H_

#include <pthread.h>
#include <string>
#include <map>

#include "Arp/System/Core/Arp.h"
#include "Arp/System/Commons/Logging.h"
#include "Arp/Plc/AnsiC/Gds/DataLayout.h"
#include "Arp/Plc/AnsiC/Io/FbIoSystem.h"
#include "Arp/Plc/AnsiC/Io/Axio.h"
#include "Utility.h"

using namespace Arp;
using namespace std;

// time calculation helpers
void timeAdd(struct timespec& zValue, long lAdd);
int timeCmp(struct timespec& zFirst, struct timespec& zSecond);

///	structure to handle a metadata and value of a single I/O
struct RAWIO
{
    // definition of I/O
    string strID;					// ID of I/O
    size_t nOffset = 0;				// offset in bus-frame in byte
    unsigned char ucBitMask = 0;	// bitmask in case of a boolean value
    bool bIsBool = false;			// true, if it is a boolean

    // current value
    unsigned char* pValue = NULL;	// pointer to data, if it is no boolean
    size_t zSize = 0;				// data size in bytes
    bool bValue = false;			// value if it is a boolean
};

class CSampleRTThread
{
public:
    CSampleRTThread();
    virtual ~CSampleRTThread();

    bool Init();
    static void* RTStaticCycle(void* p);
    void RTCycle();
    static void* StaticLoggingCycle(void* p);
    void LoggingCycle();

    bool StartProcessing();
    bool StopProcessing();

private:
    // workerthread for cycle
    pthread_t m_zRTCycleThread;
    pthread_t m_zLoggingThread;	// log status of I/Os in extra thread

    bool m_bInitialized;	// class already initialized?
    bool m_bDoCycle;		// shall the subscription cycle run?
    bool m_bFirstRTCycle;	// is it the first cycle?

    // GDS buffers for raw I/O access
    TGdsBuffer* m_pGdsInBuffer;
    TGdsBuffer* m_pGdsOutBuffer;

    // some sample I/O IDs
    String m_strInByte;
    String m_strIn04;
    String m_strIn05;
    String m_strIn06;
    String m_strIn07;
    String m_strOut04;
    String m_strOut05;
    String m_strOut06;
    String m_strOut07;

    // maps with ID of I/Os for easy access
    std::map<std::string, RAWIO> m_zInputsMap;
    std::map<std::string, RAWIO> m_zOutputsMap;

    void LogIO(RAWIO& zRawIO);
    bool AddInput(std::string strID, size_t zSize, bool bIsBool);
    bool AddOutput(std::string strID, size_t zSize, bool bIsBool);

    // example usage of direct access to fieldbus-frame
    bool ReadInputData(void);
    bool DoLogic(void);
    bool WriteOutputData(void);

    bool ReadInputValue(const char* pFrame, RAWIO& zIO);
    bool WriteOutputValue(char* pFrame, RAWIO& zIO);
};

#endif /* CSAMPLERTTHREAD_H_ */
