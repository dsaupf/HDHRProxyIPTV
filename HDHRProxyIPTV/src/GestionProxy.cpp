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
#include "GestionProxy.h"
#include "ConfigProxy.h"
#include "resource.h"

DWORD WINAPI Listening_ThreadDiscovery(void* p)
{
	((CGestionProxy*)p)->ListeningThreadDiscovery();
	return 0;
}

DWORD WINAPI Listening_ThreadControl(void* p)
{
	((CGestionProxy*)p)->ListeningThreadControl();
	return 0;
}

CGestionProxy::CGestionProxy()
{
	m_cfgProxy = CConfigProxy::GetInstance();
	m_cfgProxy->setGestProxy(this);

	m_initializedProxy = 0;

	m_Traces = new CTrace();
	m_Traces->setTraceLevel(TRZ1); //	By default the minimum trace level is set

	m_discovery = new CDiscovery();
	m_control = new CControl();
	
}

CGestionProxy::~CGestionProxy()
{
	if (m_Traces)
		delete m_Traces;

	delete m_discovery;
	delete m_control;
}

DWORD CGestionProxy::ListeningThreadDiscovery()
{
	while (m_initializedProxy)
	{
		if (m_discovery->TreatReceivedData() == SEND_OK)
		{
			if (!m_control->IsInitialized())
			{
				if (m_control->StartHDHRServer())
				{
					//Creating Thread to listen for HDHR client requests
					HANDLE hAccept = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Listening_ThreadControl, this, 0, 0);
				}
			}
		}
	}
	DWORD res =0;
	return res;
}

DWORD CGestionProxy::ListeningThreadControl()
{
	while (1)
	{
		m_control->TreatReceivedData();
	}

	return 1;
}

int CGestionProxy::InitializeProxy(int trace, CString idDisp, CString ipHDHR)
{
	m_initializedProxy = 1;
	//Traces are activated to save in log
	m_Traces->InitializeTrace(trace);
	
	//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_1))
	//{
    //char nvl[1024];
    //memset(nvl, 0, 1024);
    //_snprintf(nvl, sizeof(nvl) - 2, "Trace Level: %d\n", trace);
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"Trace Level: %d\n", trace);

    //m_Traces->WriteTrace("******************* Start HDHR IPTV Proxy *******************\n", LEVEL_TRZ_1);
    LOGM(TRZ1,0,"******************* Start HDHR IPTV Proxy *******************\n");
    //m_Traces->WriteTrace("*** Version HDHRProxyIPTV Application: v1.0.3l\n", LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Version HDHRProxyIPTV Application: v1.0.3n\n");
    CStringA idDispTmp(idDisp);
    CStringA ipHDHRTmp(ipHDHR);
    //_snprintf(nvl, sizeof(nvl) - 2, "*** Device ID: %s\n", idDispTmp);
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Device ID: %s\n", idDispTmp);
    //_snprintf(nvl, sizeof(nvl) - 2, "*** Server IP: %s\n", ipHDHRTmp);
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Server IP: %s\n", ipHDHRTmp);
    //_snprintf(nvl, sizeof(nvl) - 2, "*** Server IP: %d\n", getDiscovery()->ObtainHDHRServPort());
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Server PORT: %d\n", getDiscovery()->ObtainHDHRServPort());
    //_snprintf(nvl, sizeof(nvl) - 2, "*** Number of Tuners configured: %d\n", m_cfgProxy->getTunersNumber());
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Number of Tuners configured: %d\n", m_cfgProxy->getTunersNumber());
    //_snprintf(nvl, sizeof(nvl) - 2, "*** Lock configured: %d\n", m_cfgProxy->getLock());
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Lock configured: %d\n", m_cfgProxy->getLock());

    TCHAR sDirActual[200];
    GetCurrentDirectory(200, sDirActual);
    CString path;
    path.Format(L"%s\\%s", sDirActual, _T(NAME_FILE_MAPLIST));
    //_snprintf(nvl, sizeof(nvl) - 2, "*** Mapping List File: %s\n", CStringA(path));
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Mapping List File: %s\n", CStringA(path));
    
    CString chans,ch;
    chans.Format(L"[");
    for (int i=0; i < m_cfgProxy->m_numChannels; i++)
    {
        ch.Format(L"%d", m_cfgProxy->m_infoChannels[i].channel);
        chans.Append(ch);
        if (i<m_cfgProxy->m_numChannels - 1)
            chans.Append(L";");
    }
    chans.Append(L"]");
    //_snprintf(nvl, sizeof(nvl) - 2, "*** Number of channels in Mapping List File: %d %s\n", m_cfgProxy->m_numChannels, CStringA(chans));
    //m_Traces->WriteTrace(nvl, LEVEL_TRZ_1);
    LOGM(TRZ1,0,"*** Number of channels in Mapping List File: %d %s\n", m_cfgProxy->m_numChannels, CStringA(chans));
	//}

	//The first is to start phase Discovery
	if (!StartDiscovery(trace, idDisp, ipHDHR))
		return 0;

	/* Control */
	StartControl(trace, idDisp, ipHDHR);

	return 1;
}

int CGestionProxy::StartDiscovery(int trace, CString idDisp, CString ipHDHR)
{
	m_discovery->ChangeTraceLevel(trace);
	m_discovery->AssignIDDevice(idDisp);
	m_discovery->AssignIPHDHR(ipHDHR);

	if (!m_discovery->InicializeListenCliHDHR())
	{
		//m_Traces->WriteTrace("Could be not initiate DISCOVERY phase", LEVEL_TRZ_1);
		LOGM(TRZ1,0,"Could be not initiate DISCOVERY phase");

		return 0;
	}

	//Creating Thread to listen for HDHR client requests
	hThreadDiscovery = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Listening_ThreadDiscovery, this, 0, 0);

	return 1;
}

int CGestionProxy::StartControl(int trace, CString idDisp, CString ipHDHR)
{
	m_control->ChangeTraceLevel(trace);
	m_control->AssignIDDevice(idDisp);
	m_control->AssignIP(ipHDHR);

	if (!m_control->StartHDHRServer())
	{
		//m_Traces->WriteTrace("Could be not initiate CONTROL phase", LEVEL_TRZ_1);
		LOGM(TRZ1,0,"Could be not initiate CONTROL phase");
		return 0;
	}

	//Creating Thread to listen for HDHR client requests
	hThreadControl = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Listening_ThreadControl, this, 0, 0);

	return 1;
}

void CGestionProxy::StopProxy()
{
	m_initializedProxy = 0;
	m_discovery->StopDiscovery();
	TerminateThread(hThreadDiscovery, 1000);

	TerminateThread(hThreadControl, 1001);
	m_control->StopControl();


	//m_Traces->WriteTrace("******************* Stop Proxy SATIP2HDHR *******************\n", LEVEL_TRZ_1);
	LOGM(TRZ1,0,"******************* Stop Proxy SATIP2HDHR *******************\n");
	m_Traces->StopTrace();
}

void CGestionProxy::CleanLog()
{
	m_Traces->CleanTrace();
}

void CGestionProxy::UpdateInfoCliAtInterface(int tuner)
{
	m_cfgProxy->AddClientToInterface();

	m_control->ObtainInfoCLi(tuner);
}

void CGestionProxy::ResetTuner(int tuner)
{
	m_control->ResetTuner(tuner);
}

void CGestionProxy::ForceUnlockTuner(int tuner)
{
	m_control->ForceUnlockTuner(tuner);
}