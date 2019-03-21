/*
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
*
* Copyright (c) 2014-2015 Vanesa Dominguez
*
* Contributor(s): Daniel Soto
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

#include "stdafx.h"
#include "RingBufferTS_Basic.h"

CRingBufferTS_Basic::CRingBufferTS_Basic()
{
	m_cfgProxy = CConfigProxy::GetInstance();

	memset(m_buffer, 0, sizeof(m_buffer));
	m_posWrite = 0;
	m_posRead = 0;
	m_bufferSize = sizeof(m_buffer); //78960;
	m_freeSpace = m_bufferSize;
	m_BusySpace = 0;
	m_pidsToFilterList.Format(L"");
	m_applyPidFiltering = 0;

	memset(m_buffer_output, 0, sizeof(m_buffer_output));
	m_numTSPacketsOutput = 0;

	m_Traces = new CTrace();

	semaphoreTreatingPIDs = CreateSemaphore(
		NULL,           // default security attributes
		1,  // initial count
		1,  // maximum count
		NULL);          // unnamed semaphore

	for (int i = 0; i < 1024; i++)
		m_pidsList[i] = 0;

	m_filteredPacketsSent = 0;
	m_filteredOutPacketsDescarted = 0;
	m_nullPacketsSent = 0;
	m_receivedPackets = 0;
	m_lockaheads = 0;
	m_timetoCheckAnalysis = 3000000000; //In nanoseconds = 3 seconds
	GetLocalTime(&m_timeLastAnalysis);
}

CRingBufferTS_Basic::~CRingBufferTS_Basic()
{
	if (m_Traces)
		delete m_Traces;
}

void CRingBufferTS_Basic::Initialize(CString pidsToFilterList)
{
	strcpy(m_buffer, "");
	m_posWrite = 0;
	m_posRead = 0;
	m_freeSpace = m_bufferSize;
	m_BusySpace = 0;

	m_pidsToFilterList = pidsToFilterList;
}

int CRingBufferTS_Basic::Insert(char* data, int size)
{
	if ( size  < 0 ) return 0;
	if ( size == 0 ) return 1;

	if (size >= m_freeSpace)
	{
		if (m_BusySpace + size < m_bufferSize)
		{
			memmove(&m_buffer[0], &m_buffer[m_posRead], m_BusySpace);

			m_posWrite = m_BusySpace;
			m_posRead = 0;
			m_freeSpace = m_bufferSize - m_BusySpace;
		}
		else
			return 0;

	}

	memcpy(&m_buffer[m_posWrite], data, size);

	m_posWrite += size;
	m_freeSpace -= size;
	m_BusySpace += size;

	m_receivedPackets += size / 188;

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
	{
		char log_output[1024];
		memset(log_output, 0, 1024);
		_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: [Tuner %d] Saved in Ring Buffer  <<< %5d bytes.\n", m_tuner,size);
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
	}

	return 1;
}

int CRingBufferTS_Basic::GetTSPacket(char* data)
{
	int size = 188;  //Size of a Tranport Stream packet: 188 bytes

	//if (m_BusySpace > size)
	if (m_BusySpace >= size)
	{
		//Initial data on buffer is sync, it can obtain a correct packet 188 bytes
		//if (m_buffer[m_posRead] == 'G' && m_buffer[m_posRead + 188] == 'G')
		// Only one packet output, then only check if this one is a TS packet!
		if (m_buffer[m_posRead] == 'G')
		{
			GetData(data, size);
		}
		else
		{
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
			{
				char log_output[1024];
				memset(log_output, 0, 1024);
				_snprintf(log_output, sizeof(log_output) - 2, "ERROR      :: RING BUFFER DETECTED NO SYNC. Discard packet position at buffer: %d; Length of data at buffer: %d\n", m_posRead, m_BusySpace);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
			}
			char discard[188];
			GetData(discard, size);
			return 0;
		}
	}
	else
		return 0;

	return size;
}

int CRingBufferTS_Basic::GetData(char* data, int size)
{
	if (m_BusySpace >= size)
	{
		memcpy(data, &m_buffer[m_posRead], size);
		m_posRead += size;
		m_BusySpace -= size;
	}
	else
		return 0;

	return size;
}

int CRingBufferTS_Basic::GetMultipleTSPacket(char* data, int numMaxPackets, unsigned int numPadding[1])
{
	char packet[188];
	int numTSPackets = 0;
	char log_output[1024];
	memset(log_output, 0, 1024);
	int pass_all_pids = 0;
	int numNulls = 0;

	//m_Traces->WriteTrace("TRANSPORT  :: **** START TREATMENT OF GETTING Packets of buffer to send ****\n", LEVEL_TRZ_6);

	if (numMaxPackets <= 0) return 0;

	if (!m_applyPidFiltering || !m_pidsToFilterList.Compare(L""))
	{
		//Not necessary apply internal PID Filtering
		pass_all_pids = 1;
	}
	else
	{
		// For internal pid filtering check if time to send packets
		if ((GetBusySpaceBuf() < MAX_SIZE_DATAGRAM_TO_SEND * 50) && (!CheckTimeToSend()))
			return 0;
	}

	int i = 0;
	for (i = 0; i < numMaxPackets; i++)
	{
		if (GetTSPacket(packet))
		{
			if (pass_all_pids || PIDFiltering(packet))
			{
				// Pass packet!
				memcpy(&data[numTSPackets * 188], packet, 188);
				numTSPackets++;
				m_filteredPacketsSent++;
			}
			else
			{
				// Filtered out!
				m_filteredOutPacketsDescarted++;
			}

			if ((!pass_all_pids) && (numTSPackets >= NUM_PACKETS_TO_SEND))
			{
				// When filtering, STOP when first group of packets is completed
				break;
			}
		}
		else
		{
			if (numTSPackets >= NUM_PACKETS_TO_SEND)
			{
				break;
			}
		}
	}

	if ((!pass_all_pids) && (numTSPackets < NUM_PACKETS_TO_SEND))
		SaveTimeToSend();

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
	{
		_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: [Tuner %d] Read from Ring Buffer >>> %5d bytes; %02d packets.    More Info Buffer: Read Position[%05d]; Write Position[%05d]; Free Space[%05d]\n",
			m_tuner, numTSPackets * 188, numTSPackets, m_posRead, m_posWrite, m_freeSpace);
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
	}

#ifdef AddPaddingToTSPacket
	int numDummyPackets = (NUM_PACKETS_TO_SEND - (numTSPackets % NUM_PACKETS_TO_SEND));
	if ((numTSPackets != 0) && (numDummyPackets < NUM_PACKETS_TO_SEND))
	{
		GenerateNullPaddingTSPackets(&data[numTSPackets * 188], numDummyPackets);

		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_5))
		{
			_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: [Tuner %d] Adding %d null padding TS Packets to arrive at 7 packets to send (%d total packets).\n", m_tuner, numDummyPackets, numTSPackets);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_5);
		}

		numNulls++;
		numTSPackets += numDummyPackets;
		m_nullPacketsSent += numDummyPackets;
	}
#endif

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
		CheckTimeToAnalyzeData();

	//m_Traces->WriteTrace("TRANSPORT  :: ****  END  TREATMENT OF GETTING Packets of buffer to send ****\n", LEVEL_TRZ_6);

	numPadding[0] = numNulls;
	return numTSPackets * 188;
}

void CRingBufferTS_Basic::InitHTTPMessage()
{
	//Initialize m_timeLastSending
	GetLocalTime(&m_timeLastSending);
}

void CRingBufferTS_Basic::GenerateNullPaddingTSPackets(char* data, int numPackets)
{
	char datagram[188];
	int i = 0;

	//Generate a null padding TS packet 

	//SYNC byte (0x47 == 'G')
	datagram[0] = 'G'; 
	//2 bytes: PID Number. Value 0x1FFF -> Null Packet (used for fixed bandwidth padding)
	datagram[1] = '\x1f';
	datagram[2] = '\xff';
	//Special byte 0x1C. (It's a Fake Continuity Counter)
	datagram[3] = '\x1c';

	memset(&data[4], 0xFF, 184);

	//Return number of null packets required
	for (i = 0; i < numPackets; i++)
	{
		memcpy(&data[i*188], datagram, 188);
	}
}

int CRingBufferTS_Basic::PIDFiltering(char* data)
{
	unsigned short pid = ObtainPID((const unsigned char *)data);

	if (pid > PID_MAX)
		return 0;

	if (IsPIDEnabled(pid))
	{
		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
		{
			char log_output[4096];
			memset(log_output, 0, 4096);
			_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: [Tuner %d] PID FILTERING packet to  PASS  with PID: %d (Add to output buffer)\n", m_tuner, pid);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
		}
		return 1;
	}

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
	{
		char log_output[4096];
		memset(log_output, 0, 4096);
		_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: [Tuner %d] PID FILTERING packet to FILTER with PID: %d\n", m_tuner, pid);
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
	}
	return 0;
}

void CRingBufferTS_Basic::SemaphoreWait()
{
	DWORD dwWaitResult;
	BOOL bContinue = TRUE;

	while (bContinue)
	{
		// Try to enter the semaphore gate.
		dwWaitResult = WaitForSingleObject(
			semaphoreTreatingPIDs,   // handle to semaphore
			0L);           // zero-second time-out interval

		switch (dwWaitResult)
		{
			// The semaphore object was signaled. Semaphore is free to continue
		case WAIT_OBJECT_0:
			bContinue = FALSE;
			break;
			// The semaphore was nonsignaled, so a time-out occurred. Not continue until semaphore was signaled (free)
		case WAIT_TIMEOUT:
			m_Traces->WriteTrace("DBG        :: SemaphoreWait : WAIT Semaphore\n", LEVEL_TRZ_4);
			break;
		default:
			m_Traces->WriteTrace("DBG        :: SemaphoreWait : Other result\n", LEVEL_TRZ_4);
			break;
		}

	}
}

void CRingBufferTS_Basic::SemaphoreSignal()
{
	// Release the semaphore when task is finished (when entry at this function)

	if (!ReleaseSemaphore(
		semaphoreTreatingPIDs,  // handle to semaphore
		1,            // increase count by one
		NULL))
	{
	}
}

//get:1 ; set:0
CString CRingBufferTS_Basic::GetSetPIFFilteringData(int getset, CString pidsToFilterList, int internalPFiltering)
{
	CString tmpPIDlist;
	tmpPIDlist.Format(L"");

	SemaphoreWait();

	if (getset)
		tmpPIDlist = m_pidsToFilterList;
	else
	{
		UpdatePIFFilteringData(pidsToFilterList, internalPFiltering);
	}

	SemaphoreSignal();

	return tmpPIDlist;
}

void CRingBufferTS_Basic::UpdatePIFFilteringData(CString pidsToFilterList, int internalPFiltering)
{
	setPidsToFilterList(pidsToFilterList);

	setApplyPidFiltering(internalPFiltering);
		

	for (int i = 0; i < 8192; i++)
		DisablePID(i);

	int index = 0, index2 = 0;
	index = pidsToFilterList.Find(L",", 0);
	if (index != -1)
	{
		EnablePID(atoi(CStringA(pidsToFilterList.Left(index))));
		index2 = pidsToFilterList.Find(L",", index + 1);

		while (index2 != -1)
		{
			EnablePID(atoi(CStringA(pidsToFilterList.Mid(index + 1, index2 - index - 1))));

			index = index2;
			index2 = pidsToFilterList.Find(L",", index + 1);
		}
		EnablePID(atoi(CStringA(pidsToFilterList.Right(pidsToFilterList.GetLength() - index - 1))));
	}

}

void CRingBufferTS_Basic::SaveTimeToSend()
{
	GetLocalTime(&m_timeLastSending);
}

int CRingBufferTS_Basic::CheckTimeToSend()
{
	SYSTEMTIME curr;
	GetLocalTime(&curr);
	
	if (m_timeLastSending.wYear == 0) //Not yet initialize m_timeLastSending
		return 0;

	_int64 nanoSecs = CompareSystemTime(m_timeLastSending, curr);

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
	{
		char log_output[4096];
		memset(log_output, 0, 4096);
		_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: [Tuner %d] (DBG) Check Time To Send: Nanoseconds passed: %I64d / MAX_TIME_TO_SEND: %d\n", m_tuner, nanoSecs, m_cfgProxy->getMaxTimeToSendDgram());
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
	}

	if (nanoSecs >= m_cfgProxy->getMaxTimeToSendDgram())
		return 1;
	
	return 0;
}

//Returns differences in nanoseconds.
_int64 CRingBufferTS_Basic::CompareSystemTime(SYSTEMTIME st1, SYSTEMTIME st2)
{
	__int64 res;
	timeunion ft1;
	timeunion ft2;
	SystemTimeToFileTime(&st1, &ft1.fileTime);
	SystemTimeToFileTime(&st2, &ft2.fileTime);

	ft1.ul.LowPart = ft1.fileTime.dwLowDateTime;
	ft1.ul.HighPart = ft1.fileTime.dwHighDateTime;
	
	ft2.ul.LowPart = ft2.fileTime.dwLowDateTime;
	ft2.ul.HighPart = ft2.fileTime.dwHighDateTime;

	//Values of FILETIME and QuadPart are expressed in 100 nano seconds 

/*		1 Minute = 60 Seconds
		1 Second = 1000 Miliseconds
		1 Milliseconds = 1000 Microseconds
		1 Microseconds = 1000 Nanoseconds
*/

//	__int64 res = (ft2.ul.QuadPart - ft1.ul.QuadPart) / 10000; // Convert to milliseconds
//	__int64 res = (ft2.ul.QuadPart - ft1.ul.QuadPart) * 100; // Convert to nanoseconds

	res = (ft2.ul.QuadPart - ft1.ul.QuadPart) * 100;

	return res;			
}

void CRingBufferTS_Basic::SubstractTimeToPacket()
{
	timeunion ft1;

	SystemTimeToFileTime(&m_timeLastSending, &ft1.fileTime);

	ft1.ul.LowPart = ft1.fileTime.dwLowDateTime;
	ft1.ul.HighPart = ft1.fileTime.dwHighDateTime;

	__int64 nanosecs = (ft1.ul.QuadPart * 100) - m_cfgProxy->getMaxTimeToPacket();
	ft1.ul.QuadPart = nanosecs / 100;

	memcpy(&ft1.fileTime, &ft1.ul, sizeof(ft1.fileTime));
	
	FileTimeToSystemTime(&ft1.fileTime, &m_timeLastSending);
}

inline
void CRingBufferTS_Basic::EnablePID(unsigned short pid)
{
	unsigned int index = pid / 8;
	unsigned int bit = pid % 8;
	m_pidsList[index] |= 1 << bit;
}

inline
void CRingBufferTS_Basic::DisablePID(unsigned short pid)
{
	unsigned int index = pid / 8;
	unsigned int bit = pid % 8;
	m_pidsList[index] &= ~(1 << bit);
}

inline
int CRingBufferTS_Basic::IsPIDEnabled(unsigned short pid)
{
	unsigned int index = pid / 8;
	unsigned int bit = pid % 8;
	return (m_pidsList[index] & (1 << bit)) ? 1 : 0;
}

inline
unsigned short CRingBufferTS_Basic::ObtainPID(const unsigned char * tspacket)
{
	return ((tspacket[1] & 0x1f) << 8) + tspacket[2];
}

int CRingBufferTS_Basic::CheckTimeToAnalyzeData()
{
	char trace[1024];
	memset(trace, 0, 1024);
	SYSTEMTIME curr;
	GetLocalTime(&curr);

	if (m_timeLastAnalysis.wYear == 0) //Not yet initialize m_timeLastSending
		return 0;

	_int64 nanoSecs = CompareSystemTime(m_timeLastAnalysis, curr);
	
	if (nanoSecs >= m_timetoCheckAnalysis)
	{
		if (!m_applyPidFiltering || !m_pidsToFilterList.Compare(L""))
		{
			if (m_receivedPackets < m_filteredPacketsSent)
			{
					m_receivedPackets = m_filteredPacketsSent;
			}
			_snprintf(trace, sizeof(trace) - 2, "TRANSPORT  :: [Tuner %d] ALL DATA  : Packets: sent data %05d, send padding %4d, total received %05d, total lookaheads %3d\n", m_tuner, m_filteredPacketsSent, m_nullPacketsSent, m_receivedPackets, m_lockaheads);
		}
		else
			_snprintf(trace, sizeof(trace) - 2, "TRANSPORT  :: [Tuner %d] FILTERING : Packets: sent data %05d, send padding %4d, filtered out %05d, total received %05d, total sended %d, total lookaheads %3d\n", m_tuner, m_filteredPacketsSent, m_nullPacketsSent, /*m_filteredOutPacketsDescarted*/m_receivedPackets - m_filteredPacketsSent, m_receivedPackets, m_filteredPacketsSent + m_nullPacketsSent, m_lockaheads);
		
		m_Traces->WriteTrace(trace, LEVEL_TRZ_3);

		m_filteredPacketsSent = 0;
		m_filteredOutPacketsDescarted = 0;
		m_nullPacketsSent = 0;
		m_receivedPackets = 0;
		m_lockaheads = 0;

		GetLocalTime(&m_timeLastAnalysis);

		return 1;
	}

	return 0;
}
