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

#pragma once

#include "ConfigProxy.h"
#include "Trace.h"
#include "stdio.h"
#include "string"
#include <iostream> 
#include <sstream>  
#include <cstdint>
#include<time.h>


/*Define to add padding data when the number of TS packets is lower than 7 */
#define AddPaddingToTSPacket

#define MAX_SIZE_DATAGRAM_TO_SEND 7*188 //1316
#define NUM_PACKETS_TO_SEND 7			//Packets of transport stream to send by UDP to the client
#define PID_MAX	8191

union timeunion {
	FILETIME fileTime;
	ULARGE_INTEGER ul;
};

class CRingBufferTS_Basic
{
	
public:
	CConfigProxy* m_cfgProxy;

	char m_buffer[78960];	//60*1316
	char m_buffer_output[MAX_SIZE_DATAGRAM_TO_SEND];
	int m_numTSPacketsOutput;
	int m_posWrite;
	int m_posRead;
	int m_freeSpace;
	int m_BusySpace;
	int m_bufferSize;
	CString m_pidsToFilterList;
	CString getPidsTOFilter() { return m_pidsToFilterList; }

	int m_applyPidFiltering;
	int getapplyPidFiltering() { return m_applyPidFiltering;  }
	CTrace* m_Traces;

	SYSTEMTIME m_timeLastSending;

	int m_tuner;
	int getTuner() { return m_tuner; }
	void setTuner(int tun) { m_tuner = tun; }

	HANDLE semaphoreTreatingPIDs;
	void SemaphoreWait();
	void SemaphoreSignal();

	CRingBufferTS_Basic();
	~CRingBufferTS_Basic();

	int GetBusySpaceBuf() { return m_BusySpace; }
	int GetFreeSpaceBuf() { return m_freeSpace; }
	int GetUsedSpace() { return ((m_BusySpace * 100)/ 78960); }  // Size (78960) * 100;

	void Initialize(CString pidsToFilterList);
	int Insert(char* data, int size);
	int GetTSPacket(char* data);  // Return next valid 188 bytes packet
	int GetData(char* data, int size); // Return "size" bytes of buffer
	int GetMultipleTSPacket(char* data, int numMaxPackets); // Return 1-numMaxPackets valid 188 bytes packets
	void GenerateNullPaddingTSPackets(char* data, int numPackets);
	int PIDFiltering(char* data);
	void InitHTTPMessage();

	void setPidsToFilterList(CString pids) { m_pidsToFilterList = pids; }
	void setApplyPidFiltering(int pfilt) { m_applyPidFiltering = pfilt; }

	void UpdatePIFFilteringData(CString pidsToFilterList, int internalPFiltering);
	void SaveTimeToSend();
	int CheckTimeToSend();
	_int64 CompareSystemTime(SYSTEMTIME st1, SYSTEMTIME st2);
	void SubstractTimeToPacket();
	void setTimeLastSending() { GetLocalTime(&m_timeLastSending); };
	CString GetSetPIFFilteringData(int getset, CString pidsToFilterList, int internalPFiltering);

	unsigned char m_pidsList[1024]; //1024 bytes = 8192 bits = total num of valid PIDs
	inline void EnablePID(unsigned short pid);
	inline void DisablePID(unsigned short pid);
	inline int IsPIDEnabled(unsigned short pid);
	inline unsigned short ObtainPID(const unsigned char * tspacket);

	int m_filteredPacketsSent;
	int m_filteredOutPacketsDescarted;
	int m_nullPacketsSent;
	int m_receivedPackets;
	int m_lockaheads;
	_int64 m_timetoCheckAnalysis;
	SYSTEMTIME m_timeLastAnalysis;
	void setimeLastAnalysis() { GetLocalTime(&m_timeLastAnalysis); };
	int CheckTimeToAnalyzeData();
};


