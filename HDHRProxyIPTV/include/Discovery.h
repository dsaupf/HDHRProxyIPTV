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

#include "HDHRlibFacade.h"
#include "Trace.h"

#define KO		0
#define OK		1
#define SEND_OK 2

class CDiscovery
{
public:
	long m_countMsg;
	HDHRlibFacade m_libHDHR;	//Instance class that directly accesses to library libHomeRun
	CTrace *m_Traces;
	UINT32 m_idHDHRDevice;
	CString m_ipHDHR;

	CDiscovery();
	~CDiscovery();

	void ChangeTraceLevel(int nivTrz) { m_Traces->setTraceLevel(nivTrz); }
	void AssignIDDevice(CString idD);
	void AssignIPHDHR(CString ipHDHR);
	int InicializeListenCliHDHR();
	void StopDiscovery();
	int TreatReceivedData();
	int ObtainHDHRServPort() { return m_libHDHR.getPortServHDHR(); }

#define LOGM(level, only, text, ...) LOGG(m_Traces, "DISCOVERY  ::", level, only, text, ##__VA_ARGS__)
};
