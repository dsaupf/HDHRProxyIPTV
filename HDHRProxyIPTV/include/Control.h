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

#include "Transport.h"
#include "InfoHDHR.h"
#include "HDHRlibFacade.h"
#include "Tuner.h"

#define NO_INICIADO	0
#define INICIADO	1

typedef struct
{
	SOCKET clientSocket;
	CString IP;
	int port;
} InfoClientSocket;

class CControl
{
private:
	static void * pObject;
public:
	HANDLE hThreadTimerTuner;

	char m_location[10];
	long m_NumMsg;
	int m_numTuners;
	CTuner* m_infoTuners;
	InfoMessageHDHR* m_infoMsg;

	SOCKET m_clientSocket;
	char* m_HDHRClientIP;
	int m_HDHRClientPort;

	int m_ControlState;
	CString m_ipHDHR;
	UINT32 m_idHDHRDevice;

	int wdControlErr;  //Control if continued error occurs in the main control loop
	int wdControlErr_Count;

	int m_numClientSockets;
	SOCKET mainSocket;	//Main Socket
	InfoClientSocket *m_clientSockets; //List of client's sockets of which it accepts theirs connection
	fd_set readfds;

	HDHRlibFacade m_libHDHR;	//Instance class that directly accesses to the library libHomeRun
	
	CConfigProxy* m_cfgProxy;
	CTrace *m_Traces;

	CControl();
	~CControl();

	int IsInitialized();
	int StartHDHRServer();
	void AssignIDDevice(CString idD);
	void AssignIP(CString ip);
	int TreatReceivedData();
	void StopControl();

	int ObtainInfoCLi(int tuner);

	SOCKET getClientSocket() { return m_clientSocket; }
	void setClientSocket(SOCKET clientSocket) { m_clientSocket = clientSocket; }

	int getCliHDHRPort() { return m_HDHRClientPort; }
	void setCliHDHRPort(int port) { m_HDHRClientPort = port; m_libHDHR.m_remotePortControl = port; }

	void ChangeTraceLevel(int nivTrz) { m_Traces->setTraceLevel(nivTrz); }

	void AssignClientIP(CString ip);
	void TreatTypeHDHRMessage(InfoMessageHDHR* infoMsg);
	int ReceiveTCPDataHDHR();

	DWORD ListeningThreadControlTimersTuners();

	void ResetTuner(int tuner);
	void ForceUnlockTuner(int tuner);

	void RestartProxy();

	static void CALLBACK TimerProcTuners_Wrapper(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
	void CALLBACK TimerProcTuners(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

#define LOGM(level, only, text, ...) LOGG(m_Traces, "CONTROL    ::", level, only, text, ##__VA_ARGS__)
};
