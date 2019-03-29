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
#include "Discovery.h"

CDiscovery::CDiscovery()
{
	m_countMsg = 0;
	m_Traces = new CTrace();
	m_Traces->setTraceLevel(TRZ1); //By default the minimum trace level is set
}

CDiscovery::~CDiscovery()
{
	delete m_Traces;
}

int CDiscovery::InicializeListenCliHDHR()
{
	if (!m_libHDHR.CreateSocketServUDP_Discovery())
		return 0;

	LOGM(TRZ2,0,"HDHR server ready to receive Discovery messages\n");
	LOGM(TRZ3,1,"Listening by [UDP] %s:%d\n", CStringA(m_ipHDHR), HDHOMERUN_DISCOVER_UDP_PORT);
	return 1;
}

void CDiscovery::StopDiscovery()
{
	m_libHDHR.StopSocketServUDP_Discovery();
}

int CDiscovery::TreatReceivedData()
{
	int res = 0;

	res = m_libHDHR.ReceiveDataDiscoveryUDP(m_Traces);
	//Message

	if (res)
	{
		m_countMsg++;

		LOGM(TRZ4,1,"-->    Received message      [%s:%d] {%06d} : UDP Discovery\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR(), m_countMsg);

		if (!m_libHDHR.SendResponseDiscovery(m_idHDHRDevice)) //ID_DISP_SERV_HDHR o cambio por el usuario
		{
			return KO;
		}

		LOGM(TRZ4,1,"   <-- Send message          [%s:%d] {%06ld} : UDP Discovery\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR(), m_countMsg);
		LOGM(TRZ3,1,"Send UDP Discovery message   [%s:%d]\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR());
		return SEND_OK;
	}

	return OK;
}

void CDiscovery::AssignIDDevice(CString idD)
{
	CStringA disp(idD); //To convert CString to const char *

	m_idHDHRDevice = strtol(disp, NULL, 16);

}

void CDiscovery::AssignIPHDHR(CString ipHDHR)
{
	m_ipHDHR = ipHDHR; 

	m_libHDHR.setIPHDHR(ipHDHR);
}