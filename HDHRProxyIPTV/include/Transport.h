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
#include "hdhomerun_os_windows.h"
#include "RingBufferTS_Basic.h"

#define UDP_TS 1
#define RTP_TS 2
#define HTTP_TS 3

#define BLOCKING_WAIT_TIME 4000  // Miliseconds to block TCP socket read operations!

class CTransport
{
public:

	CTrace *m_Traces;
	int m_state; //added to use in ControlHDHR
	char m_ipSend[16];
	int m_portSend;
	int m_PerformSend;
	int m_typeTransportOutput; //0: UDP_TS; 1: RTP_TS. Protocol for output to HDHR clients
	int m_errConnectionIni;
	int getErrConnectionIni() { return m_errConnectionIni; }
	 
	CConfigProxy* cfgProxy;

	char* dataHttp;
	char* dataUdp;
	int packCount;
	int m_applyExtPidFiltering;
	CString m_dataGETHTTP;

	CRingBufferTS_Basic* m_basicRingBuffer;

	int m_tuner;
	int m_failedConnectHTTP;
	int m_refreshFailedConnHTTP;
	int getfailedConnectHTTP() { return m_failedConnectHTTP; }
	void setfailedConnectHTTP(int value) { m_failedConnectHTTP = value; }
	int getrefreshFailedConnHTTP() { return m_refreshFailedConnHTTP; }
	void setrefreshFailedConnHTTP(int value) { m_refreshFailedConnHTTP = value; }

	char readBuffer[188 * 7 * 24];    // Buffer user for reading from socket; it may contain data from a previous read call!
	int  readBufferSize = sizeof(readBuffer);
	int  readBufferPos = 0;           // Size of valid data on the buffer.
	//int  readBufferMinPos = 7 * 188;  // Position of lookahead valid data on the buffer.
	int  readBufferMinPos = 0;          // lookahead disabled.
	//int kkp = -1;

	CTransport();
	~CTransport();

	CRingBufferTS_Basic* getRingBuffer() { return m_basicRingBuffer; }
	void setTuner(int tun) { m_tuner = tun; m_basicRingBuffer->setTuner(tun); }
	int getTuner() { return m_tuner; }
	int getState() { return m_state; };
	void setState(int est){ m_state = est; }
	int getPerformSend() { return m_PerformSend; }
	void setPerformSend(int send, int tuner);
	void setApplyExtPidFiltering(int extPidFilter){ m_applyExtPidFiltering = extPidFilter; }
	int getTypeInTransport() { return m_typeTransportInput; }

	void ChangeTraceLevel(int nivTrz) { m_Traces->setTraceLevel(nivTrz); }
	int InitializeTransportStreamUDP();
	int TreatReceivedDataUDP();
	void StopTransportStreamUDP();
	
	void AssignDataSend(char* target);

	int receivingDataHTTP;
	int getReceivingDataHTTP() { return receivingDataHTTP; }
	SOCKET m_socketHTTP;
	int InitilizeTransportStreamHTTP();

	int m_typeTransportInput;  //Protocol of input (reception)
	int m_PosChannelInMapList;
	SOCKET m_socketUDP;

	int InitilizeTransportStream(int channel, CString pidsToFilterList);
	int TreatReceivedDataHTTP();
	void TreatReceivedDataTS();
	void StopTransportStreamHTTP();
	int ChangeSourceTS(int channel, CString pidsToFilterList);
	void ChangeFilterPIDsList(CString pidsToFilterList, int internalPFiltering, int externalPFiltering, int channel);

	HANDLE hThreadTransport;
	int StartThreadTransport();
	void StopThreadTransport();
	DWORD ListeningThreadTransport();
	int SendGetHTTPRequest();
	int ReconnectConnectionHTTP();
	int SendNullPackets();
	void ReconnectHTTP();
};
