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

	//m_Traces->WriteTrace("DISCOVERY  :: HDHR server ready to receive Discovery messages\n", LEVEL_TRZ_2);
	LOGM(TRZ2,0,"HDHR server ready to receive Discovery messages\n");
	//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
	//{
	//	char log_output[1024];
	//	memset(log_output, 0, 1024);
	//	_snprintf(log_output, sizeof(log_output) - 2, "DISCOVERY  :: Listening by [UDP] %s:%d\n", CStringA(m_ipHDHR), HDHOMERUN_DISCOVER_UDP_PORT);
	//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
	//}
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
	//char log_output[1024];
	//memset(log_output, 0, 1024);

	res = m_libHDHR.ReceiveDataDiscoveryUDP(m_Traces);
	//Message

	if (res)
	{
		m_countMsg++;

		//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_4))
		//{
		//	_snprintf(log_output, sizeof(log_output) - 2, "DISCOVERY  :: -->    Received message      [%s:%d] {%06d} : UDP Discovery\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR(), m_countMsg);
		//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);
		//}
		LOGM(TRZ4,1,"-->    Received message      [%s:%d] {%06d} : UDP Discovery\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR(), m_countMsg);

		if (!m_libHDHR.SendResponseDiscovery(m_idHDHRDevice)) //ID_DISP_SERV_HDHR o cambio por el usuario
		{
			return KO;
		}

		//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_4))
		//{
		//	_snprintf(log_output, sizeof(log_output) - 2, "DISCOVERY  ::    <-- Send message          [%s:%d] {%06ld} : UDP Discovery\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR(), m_countMsg);
		//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);
		//}
		LOGM(TRZ4,1,"   <-- Send message          [%s:%d] {%06ld} : UDP Discovery\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR(), m_countMsg);
		//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
		//{
		//	_snprintf(log_output, sizeof(log_output) - 2, "DISCOVERY  :: Send UDP Discovery message   [%s:%d]\n", m_libHDHR.getDiscIPClientHDHR(), m_libHDHR.getPortClientHDHR());
		//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
		//}
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