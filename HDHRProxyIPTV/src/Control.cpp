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
#include "Control.h"

void * CControl::pObject;

CControl::CControl()
{
	m_ControlState = NO_INICIADO;

	m_Traces = new CTrace();
	m_cfgProxy = CConfigProxy::GetInstance();

	m_numClientSockets = 12 + (10 * m_cfgProxy->getTunersNumber());
	m_clientSockets = new InfoClientSocket[m_numClientSockets];

	//Initilize Client Sockets
	for (int i = 0; i < m_numClientSockets; i++)
	{
		m_clientSockets[i].clientSocket = 0;
		m_clientSockets[i].port = 0;
		m_clientSockets[i].IP = CString("");
	}

	wdControlErr = 0;
	wdControlErr_Count = 10;
	
	m_clientSocket = 0; //Current socket
	m_NumMsg = 0;

	m_HDHRClientIP = new char[16];
	strcpy(m_HDHRClientIP, "");
	m_HDHRClientPort = 0;
	strcpy(m_location, CStringA(m_cfgProxy->lineup_location));
	m_libHDHR.setLocation(m_location);

	m_numTuners = m_cfgProxy->getTunersNumber();

	//Initilize Tuners
	m_infoTuners = new CTuner[m_numTuners];

	for (int i = 0; i < m_numTuners; i++)
	{
		m_infoTuners[i].setTuner(i);
		m_infoTuners[i].transportTuner->setTuner(i);
	}

	//Initialization structure for the treatment of messages HDHR
	m_infoMsg = new InfoMessageHDHR;
	m_infoMsg->tipoMsg = ERROR_MSG;
	m_infoMsg->setMsg = 0;
	m_infoMsg->RequestMsg = new char[50];
	m_infoMsg->peticionMsg = new char[50];
	m_infoMsg->setValue = (char*)malloc(200);
	strcpy(m_infoMsg->setValue, "");
	strcpy(m_infoMsg->unknownMsg, "");
	m_infoMsg->numTuner = 0;
	m_infoMsg->seqUpgrade = 0;
	m_infoMsg->upgradeMsg = 0;
	m_infoMsg->IDLockkeyReceived = 0;


	pObject = this;
	SetTimer(NULL, NULL, 10000, TimerProcTuners_Wrapper);
}

CControl::~CControl()
{
	delete m_Traces;

	StopControl();

	for (int i = 0; i < m_numTuners; i++)
		delete m_infoTuners[i].transportTuner;

	delete m_infoTuners;

	if (m_infoMsg->RequestMsg)
		delete[]m_infoMsg->RequestMsg;

	if (m_infoMsg->setValue)
		delete[]m_infoMsg->setValue;

	if (m_infoMsg)
		delete m_infoMsg;
}

void CControl::AssignIP(CString ip)
{
	m_ipHDHR = ip;

	//Copy
	AssignClientIP(ip);
}

int CControl::IsInitialized()
{
	if (m_ControlState == INICIADO)
		return 1;

	return 0;
}

void CControl::AssignIDDevice(CString idD)
{
	CStringA disp(idD); //To convert CString to const char *

	m_idHDHRDevice = strtol(disp, NULL, 16);
}

int CControl::StartHDHRServer()
{
	//Create TCP socket for Control phase
	AssignIP(m_cfgProxy->m_hdhrServerIP);
	if (!m_libHDHR.CreateSocketServTCP_Control(&mainSocket, m_Traces))
		return 0;

	m_ControlState = INICIADO;

	//m_Traces->WriteTrace("CONTROL    :: HDHR server ready to receive Control messages\n", LEVEL_TRZ_2);
	LOGM(TRZ2,0,"HDHR server ready to receive Control messages\n");
	
	//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
	//{
	//	char log_output[1024];
	//	memset(log_output, 0, 1024);
	//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: Listening by [TCP] %s:%d\n", CStringA(m_ipHDHR), HDHOMERUN_CONTROL_TCP_PORT);
	//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
	//}
	LOGM(TRZ2,1,"Listening by [TCP] %s:%d\n", CStringA(m_ipHDHR), HDHOMERUN_CONTROL_TCP_PORT);

	return 1;
}

int CControl::TreatReceivedData()
{
	//char log_output[1024];
	//memset(log_output, 0, 1024);
	int i = 0, err = 0;
	int activity = 0;
	SOCKET sock = 0, acceptSocket = 0;
	char ipClientHDHR[16];
	memset(ipClientHDHR, 0, 16);
	int portClientHDHR = 0;
	int listSocketsFull = 1;

	//FD_SET structure of sockets is clean
	FD_ZERO(&readfds);

	//The main socket is added to the structure FD_SET
	FD_SET(mainSocket, &readfds);

	//Sockets children are added to the structure FD_SET
	for (i = 0; i < m_numClientSockets; i++)
	{
		sock = m_clientSockets[i].clientSocket;

		if (sock > 0)
			FD_SET(sock, &readfds);
	}

	//Expecting while no activity is received of any socket.
	activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

	if (activity == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		//_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: Error at select call (sockets). Error code: %d\n", err);
		//m_Traces->WriteTrace(log_output, ERR);
		LOGM(ERR,0,"Error at select call (sockets). Error code: %d\n", err);

		wdControlErr++;
		if (wdControlErr >= wdControlErr_Count)
		{
			//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
			///	m_Traces->WriteTrace("CONTROL    :: RESTART FORCED by repeated errors in main loop of control treatment sockets\n", LEVEL_TRZ_2);
			LOGM(TRZ2,1,"RESTART FORCED by repeated errors in main loop of control treatment sockets\n");

			wdControlErr = 0;

			StopControl();
			StartHDHRServer();

			//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
			//	m_Traces->WriteTrace("CONTROL    :: RESTARTED CONTROL Process because MAX-SOCKET-ERROR on SELECT\n", LEVEL_TRZ_2);
			LOGM(TRZ2,1,"RESTARTED CONTROL Process because MAX-SOCKET-ERROR on SELECT\n");
		}

		return 0;
	}

	//It receives event in the main socket, there is a new connection
	if (FD_ISSET(mainSocket, &readfds))
	{
		if (!m_libHDHR.Accept(ipClientHDHR, &portClientHDHR, &acceptSocket))
		{
			return 0;
		}

		//New socket is added to the list
		for (i = 0; i < m_numClientSockets; i++)
		{
			if (m_clientSockets[i].clientSocket == 0)
			{
				listSocketsFull = 0;
				m_clientSockets[i].clientSocket = acceptSocket;
				m_clientSockets[i].IP = CString(ipClientHDHR);
				m_clientSockets[i].port = portClientHDHR;

				//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_5))
				//{
				//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: New client socket connection [%s:%d] : Socket %d\n", ipClientHDHR, portClientHDHR, acceptSocket);
				//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_5);
				//}
				LOGM(TRZ5,1,"New client socket connection [%s:%d] : Socket %d\n", ipClientHDHR, portClientHDHR, acceptSocket);
				break;
			}
		}
		if (listSocketsFull)
		{
			//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
			//{
			//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: Client sockets list is FULL [%s:%d] : Socket %d can not be saved and treated.\n", ipClientHDHR, portClientHDHR, acceptSocket);
			//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
			//}
			LOGM(TRZ2,1,"Client sockets list is FULL [%s:%d] : Socket %d can not be saved and treated.\n", ipClientHDHR, portClientHDHR, acceptSocket);
		}
	}

	//Events are treated (reception of data) of the sockets of the list
	for (i = 0; i < m_numClientSockets; i++)
	{
		sock = m_clientSockets[i].clientSocket;

		if ( (sock != 0) && (FD_ISSET(sock, &readfds)) )
		{
/*			cliAddr.sin_addr.s_addr = inet_addr(CStringA(m_clientSockets[i].IP));
			cliAddr.sin_family = AF_INET;
			cliAddr.sin_port = htons(m_clientSockets[i].port);

			//Obtain information of the client socket
			getpeername(sock, (struct sockaddr*)&cliAddr, (int*)sizeof(struct sockaddr_in));
*/			
			setClientSocket(sock); //The socket to be treated in this iteration is saved
			AssignClientIP(m_clientSockets[i].IP);
			setCliHDHRPort(m_clientSockets[i].port);

			if (ReceiveTCPDataHDHR() == 0)
			{
				err = WSAGetLastError();
				if (err == WSAECONNRESET || err == 0) //The client closes the connection
				{
					//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_5))
					//{
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: Client socket disconnected   [%s:%d] : Socket %d\n", ipClientHDHR, portClientHDHR, sock);
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_5);
					//}
					LOGM(TRZ5,1,"Client socket disconnected   [%s:%d] : Socket %d\n", ipClientHDHR, portClientHDHR, sock);

					//The socket is closed and is updated to 0 in the list
					shutdown(sock, SD_BOTH);
					closesocket(sock);
					m_clientSockets[i].clientSocket = 0;
					m_clientSockets[i].port = 0;
					m_clientSockets[i].IP = CString("");

				}
				else
				{
					//_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: Error at RECV (sockets). Error code: %d\n", err);
					//m_Traces->WriteTrace(log_output, ERR);
					LOGM(ERR,0,"Error at RECV (sockets). Error code: %d\n", err);

					//if (err == WSAECONNABORTED)
					if (err > 0)
					{
						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_5))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: Closing socket which had error              [%s:%d] : Socket %d\n", ipClientHDHR, portClientHDHR, sock);
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_5);
						//}
						LOGM(TRZ5,1,"Closing socket which had error              [%s:%d] : Socket %d\n", ipClientHDHR, portClientHDHR, sock);

						//The socket is closed and is updated to 0 in the list
						shutdown(sock, SD_BOTH);
						closesocket(sock);
						m_clientSockets[i].clientSocket = 0;
						m_clientSockets[i].port = 0;
						m_clientSockets[i].IP = CString("");
					}

					wdControlErr++;
					if (wdControlErr >= wdControlErr_Count)
					{
						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//	m_Traces->WriteTrace("CONTROL    :: RESTART FORCED by repeated errors in main loop of control treatment sockets\n", LEVEL_TRZ_2);
						LOGM(TRZ2,1,"RESTART FORCED by repeated errors in main loop of control treatment sockets\n");
						wdControlErr = 0;
						
						StopControl();
						StartHDHRServer();

						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//	m_Traces->WriteTrace("CONTROL    :: RESTARTED CONTROL Process because MAX-SOCKET-ERROR in client socket\n", LEVEL_TRZ_2);
						LOGM(TRZ2,1,"RESTARTED CONTROL Process because MAX-SOCKET-ERROR in client socket\n");
					}

					return 0;
				}
			}
		}
	}

	wdControlErr = 0;

	return 1;
}

void CControl::StopControl()
{
	//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
	//	m_Traces->WriteTrace("CONTROL    :: Stop Control Process\n", LEVEL_TRZ_2);
	LOGM(TRZ2,1,"Stop Control Process\n");

	shutdown(mainSocket, SD_BOTH);
	closesocket(mainSocket);

	for (int i = 0; i < m_numTuners; i++)
	{
		if (m_infoTuners[i].getTransportTuner()->getState() == 1)
		{
			if (m_infoTuners[i].getTransportTuner()->getTypeInTransport() == UDP_TS)
				m_infoTuners[i].getTransportTuner()->StopTransportStreamUDP();
			else if (m_infoTuners[i].getTransportTuner()->getTypeInTransport() == HTTP_TS)
				m_infoTuners[i].getTransportTuner()->StopTransportStreamHTTP();

			m_infoTuners[i].getTransportTuner()->StopThreadTransport();

			m_infoTuners[i].getTransportTuner()->setState(0);
		}
		ResetTuner(i);
	}

	m_libHDHR.StopSocketServTCP_Control();

	for (int i = 0; i < m_numClientSockets; i++)
	{
		if (m_clientSockets[i].clientSocket != 0)
		{
			shutdown(m_clientSockets[i].clientSocket, SD_BOTH);
			closesocket(m_clientSockets[i].clientSocket);
			m_clientSockets[i].clientSocket = 0;
			m_clientSockets[i].port = 0;
			m_clientSockets[i].IP = CString("");

		}
	}
}

//Function to update data to be displayed at the window
int CControl::ObtainInfoCLi(int tuner)
{
	int i = 0, find = 0;
	int index=0;
	CString aux;
	aux.Format(L"");

	switch (m_infoTuners[tuner].getState())
	{
	case STANDBY:
		m_cfgProxy->m_infoActualSelCli->state.Format(L"STANDBY");
		break;
	case TUNED_CHAN:
		m_cfgProxy->m_infoActualSelCli->state.Format(L"TUNED CHANNEL");
		break;
	case FILTERING:
		m_cfgProxy->m_infoActualSelCli->state.Format(L"FILTERING");
		break;
	case STREAMING:
		m_cfgProxy->m_infoActualSelCli->state.Format(L"STREAMING : Q[%d, %d, %d]", m_infoTuners[tuner].ss, m_infoTuners[tuner].snq, m_infoTuners[tuner].seq);
		break;
	default:
		break;
	}

	aux = CString(m_infoTuners[tuner].getTarget());
	m_cfgProxy->m_infoActualSelCli->target.Format(L"%s", aux);
//	aux = CString(m_HDHRClientIP);
	if (m_infoTuners[tuner].getChannelNotInMapList() != 0 && m_infoTuners[tuner].getState() == STANDBY)
	{
		m_cfgProxy->m_infoActualSelCli->channelNotInMapList = 1;
		m_cfgProxy->m_infoActualSelCli->channel = m_infoTuners[tuner].getChannelNotInMapList();
	}
	else
	{
		m_cfgProxy->m_infoActualSelCli->channelNotInMapList = 0;
		m_cfgProxy->m_infoActualSelCli->channel = m_infoTuners[tuner].getChannel();
	}

	//PIDs active list:
	if (m_infoTuners[tuner].getProgram() != 0)		//pids by program
	{
		if (m_infoTuners[tuner].getPidsToFiltering().Compare(L""))
			m_cfgProxy->m_infoActualSelCli->pids = m_infoTuners[tuner].getPidsToFiltering();
		else
			m_cfgProxy->m_infoActualSelCli->pids.Format(L"0x0000-0x1FFF");
	}
	else
	{
		m_cfgProxy->m_infoActualSelCli->pids = m_infoTuners[tuner].getFilter();
	}

	//Internal PIDs filter:
	if (m_cfgProxy->getInternalPIDFilteringOfChannel(m_infoTuners[tuner].getChannel()))
	{
		if (m_infoTuners[tuner].getProgram() != 0)
		{
			if (m_infoTuners[tuner].getPidsToFiltering().Compare(L""))
				m_cfgProxy->m_infoActualSelCli->pidsProgram.Format(m_infoTuners[tuner].getPidsToFiltering());
			else
				m_cfgProxy->m_infoActualSelCli->pidsProgram.Format(L"Disabled filtering: Pass all pids");
		}
		else
		{
			if (m_infoTuners[tuner].getPidsToFiltering().Compare(L""))
				m_cfgProxy->m_infoActualSelCli->pidsProgram.Format(m_infoTuners[tuner].getPidsToFiltering());
			else
				m_cfgProxy->m_infoActualSelCli->pidsProgram.Format(L"Disabled filtering: Pass all pids");
		}
	}
	else
	{
		if (m_infoTuners[tuner].getState() == STANDBY)
			m_cfgProxy->m_infoActualSelCli->pidsProgram.Format(L"");
		else
			m_cfgProxy->m_infoActualSelCli->pidsProgram.Format(L"Not active");
	}

	m_cfgProxy->m_infoActualSelCli->program.Format(L"%d : ", m_infoTuners[tuner].getProgram());

	int idxCH = m_cfgProxy->ObtainIndexChannel(m_infoTuners[tuner].getChannel());
	if ((m_infoTuners[tuner].getProgram() != 0 || (m_infoTuners[tuner].getProgram() == 0 && m_infoTuners[tuner].canal != 0))
			&& idxCH != -1
			&& (m_cfgProxy->m_infoChannels[idxCH].Program_table.Compare(L"-1"))
			&& (m_cfgProxy->m_infoChannels[idxCH].Program_table.Compare(L"none"))
			&& (m_cfgProxy->m_infoChannels[idxCH].Program_table.Compare(L"NONE")))
		m_cfgProxy->m_infoActualSelCli->program.Append(m_cfgProxy->m_infoChannels[idxCH].Program_table_file);


	if (m_infoTuners[tuner].lockkey != 0)
		m_cfgProxy->m_infoActualSelCli->lockkey.Format(L"%s (%lu)", CString(m_infoTuners[tuner].IPLockkey), m_infoTuners[tuner].getLockkey());
	else
		m_cfgProxy->m_infoActualSelCli->lockkey.Format(L"");


	if (!m_infoTuners[tuner].getTransportTuner()->getPerformSend() || idxCH == -1)
	{
		m_cfgProxy->m_infoActualSelCli->protocolTS.Format(L"");
		m_cfgProxy->m_infoActualSelCli->udpTS.Format(L"");
		m_cfgProxy->m_infoActualSelCli->httpTS.Format(L"");
	}
	else
	{
		switch (m_infoTuners[tuner].getTransportTuner()->getTypeInTransport())
		{
		case UDP_TS:
			m_cfgProxy->m_infoActualSelCli->protocolTS.Format(L"UDP");
			if (m_cfgProxy->m_infoChannels[idxCH].ipUDP.Compare(L""))
				m_cfgProxy->m_infoActualSelCli->udpTS.Format(L"%s:%d", m_cfgProxy->m_infoChannels[idxCH].ipUDP, m_cfgProxy->m_infoChannels[idxCH].puertoUDP);
			break;
		case HTTP_TS:
			m_cfgProxy->m_infoActualSelCli->protocolTS.Format(L"HTTP");
			m_cfgProxy->m_infoActualSelCli->httpTS.Format(L"");
			m_cfgProxy->m_infoActualSelCli->httpTS.Append(m_cfgProxy->m_infoChannels[idxCH].URLGet_ExtPidFilt);
			break;
		default:
			m_cfgProxy->m_infoActualSelCli->protocolTS.Format(L"");
			m_cfgProxy->m_infoActualSelCli->udpTS.Format(L"");
			m_cfgProxy->m_infoActualSelCli->httpTS.Format(L"");
			break;
		}
	}

	if (m_infoTuners[tuner].getState() == STREAMING)
	{
		m_cfgProxy->m_infoActualSelCli->readbuffer.Format(L"%d", m_infoTuners[tuner].getReadBufferStatus());
		m_cfgProxy->m_infoActualSelCli->ringbuffer.Format(L"%d", m_infoTuners[tuner].getRingBufferStatus());
	}
	else
	{
		m_cfgProxy->m_infoActualSelCli->readbuffer.Format(L"");
		m_cfgProxy->m_infoActualSelCli->ringbuffer.Format(L"");
	}
	return 1;
}

void CControl::AssignClientIP(CString ip)
{
	m_ipHDHR = ip;
	strcpy(m_HDHRClientIP, CStringA(ip));

	m_libHDHR.setIPHDHR(ip);
	strcpy(m_libHDHR.m_sRemoteIPControl, CStringA(ip));
}

void CControl::TreatTypeHDHRMessage(InfoMessageHDHR* infoMsg)
{
	//char log_output[1024];
	//memset(log_output, 0, 1024);

	switch (infoMsg->tipoMsg)
	{
	case TUNERX_LOCKKEY_MSG:
		if (infoMsg->setMsg)
		{
			if (m_cfgProxy->getLock())
			{
				strcpy(m_infoTuners[infoMsg->numTuner].IPLockkey, m_HDHRClientIP);

				if (!strcmp(infoMsg->setValue, "none") || !strcmp(infoMsg->setValue, "force"))
				{
					m_infoTuners[infoMsg->numTuner].setLockkey(0);

					if (!strcmp(infoMsg->setValue, "force"))
					{
						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Client %s force unlock (lockkey: %lu)\n", infoMsg->numTuner, m_infoTuners[infoMsg->numTuner].IPLockkey, m_infoTuners[infoMsg->numTuner].getLockkey());
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Client %s force unlock (lockkey: %lu)\n", infoMsg->numTuner, m_infoTuners[infoMsg->numTuner].IPLockkey, m_infoTuners[infoMsg->numTuner].getLockkey());
					}
					else
					{
						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Client %s unlock (lockkey: %lu)\n", infoMsg->numTuner, m_infoTuners[infoMsg->numTuner].IPLockkey, m_infoTuners[infoMsg->numTuner].lockkey);
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Client %s unlock (lockkey: %lu)\n", infoMsg->numTuner, m_infoTuners[infoMsg->numTuner].IPLockkey, m_infoTuners[infoMsg->numTuner].lockkey);
					}
				}
				else
				{
					m_infoTuners[infoMsg->numTuner].setLockkey((uint32_t)strtoul(infoMsg->setValue, NULL, 10));

					//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
					//{
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Client %s lock (lockkey: %lu)\n", infoMsg->numTuner, m_infoTuners[infoMsg->numTuner].IPLockkey, m_infoTuners[infoMsg->numTuner].getLockkey());
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
					//}
					LOGM(TRZ2,1,"[Tuner %d] Client %s lock (lockkey: %lu)\n", infoMsg->numTuner, m_infoTuners[infoMsg->numTuner].IPLockkey, m_infoTuners[infoMsg->numTuner].getLockkey());
				}
			}
		}
		break;
	case LINEUP_LOC_MSG:
		if (infoMsg->setMsg)
		{
			strcpy(m_location, infoMsg->setValue);
			m_libHDHR.setLocation(m_location);
			m_cfgProxy->lineup_location = CString(m_location);
		}
		break;
	case SYS_RESTART_MSG:
		if (infoMsg->setMsg)
		{
			if (!strcmp(infoMsg->setValue, "self"))
			{
				//m_Traces->WriteTrace("CONTROL    :: Restart proxy by request message /sys/restart self\n", LEVEL_TRZ_2);
				LOGM(TRZ2,0,"Restart proxy by request message /sys/restart self\n");

				//In this case it responses to client before treat de request because this make a restart of the proxy
				m_libHDHR.SendResponseControl(m_infoMsg, m_infoTuners, m_clientSocket, m_Traces);

				RestartProxy();
			}
			else if (!strcmp(infoMsg->setValue, "clean-log"))
			{
				m_Traces->CleanTrace();
				//m_Traces->WriteTrace("CLEANING LOG\n", LEVEL_TRZ_1);
				LOGM(TRZ1,0,"CLEANING LOG\n");
			}
			else if (strstr(infoMsg->setValue, "tuner") != NULL)
			{
				for (int i = 0; i < this->m_numTuners; i++)
				{
					char auxString[100];
					memset(auxString, 0, 100);
					_snprintf(auxString, sizeof(auxString) - 2, "tuner%d", i);

					if (!strcmp(infoMsg->setValue, auxString))
					{
						ResetTuner(i);
					}
				}
			}
			else if (!strcmp(infoMsg->setValue, "mapping-list"))
			{
				m_cfgProxy->ReloadMappingList();
			}
		}
		
		break;
	case TUNERX_CHANNELMAP_MSG:
		if (infoMsg->setMsg)
		{
			m_infoTuners[infoMsg->numTuner].setChannelmap(infoMsg->setValue);
		}
		break;
	case TUNERX_CHANNEL_MSG:
		if (infoMsg->setMsg)
		{
			if (!strcmp(infoMsg->setValue, NONE))
			{
				if (m_infoTuners[infoMsg->numTuner].getChannel() != 0 || m_infoTuners[infoMsg->numTuner].getState() != STANDBY)
				{
					m_infoTuners[infoMsg->numTuner].ChangeStateToStandby();

					//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
					//{
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
					//}
					LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
				}
			}
			else
			{
				//Obtain channel
				char ch[25];
				char* chp;
				//char *ch = new char[25];  //Bug!
				strcpy(ch, infoMsg->setValue);
				strtok(ch, ":");
				chp = strtok(NULL, ":");
				long chan;
				
				if (chp != NULL)
					chan = atoi(chp);
				else
					chan = atoi(infoMsg->setValue);

				//Treat if the channel is not in the MappingList. If so, it goes to state STANDBY
				if (m_cfgProxy->ObtainIndexChannel(chan) == -1)
				{
					LOGM(TRZ2,1,"[Tuner %d] CHN not found in MappingList [%s] : Response to set channel message [Channel/Freq %ld]\n", infoMsg->numTuner, m_HDHRClientIP, chan);

					m_infoTuners[infoMsg->numTuner].ChangeStateToStandby();
					m_infoTuners[infoMsg->numTuner].setChannelNotInMapList(chan);

					//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
					//{
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] CHN not found in MappingList [%s] : Response to set channel message [Channel/Freq %ld]\n", infoMsg->numTuner, m_HDHRClientIP, chan);
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
					//}
					LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
				}
				else
				{
					m_infoTuners[infoMsg->numTuner].setChannelNotInMapList(0);

					if (m_infoTuners[infoMsg->numTuner].canal != chan || m_infoTuners[infoMsg->numTuner].getState() != TUNED_CHAN)
					{
						m_infoTuners[infoMsg->numTuner].ChangeStateToTunedChan(chan);

						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: TNCH [%s] : Channel/freq: %ld\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].getChannel());
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Change State: TNCH [%s] : Channel/freq: %ld\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].getChannel());
					}
				}
			}
		}
		break;
	case TUNERX_PROGRAM_MSG:
		if (m_infoTuners[infoMsg->numTuner].getState() == STANDBY)
			break;
		if (infoMsg->setMsg)
		{
			if (strcmp(infoMsg->setValue, NONE) && strcmp(infoMsg->setValue, "0"))
			{
				if (m_infoTuners[infoMsg->numTuner].getProgram() != atoi(infoMsg->setValue) || m_infoTuners[infoMsg->numTuner].getState() != FILTERING)
				{
					if (m_infoTuners[infoMsg->numTuner].getState() != STREAMING)
					{
						m_infoTuners[infoMsg->numTuner].ChangeStateToFilteringByProgram(atoi(infoMsg->setValue));

						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: FLTR [%s] : Program filtering: %d ; filtered PIDs of program: %s\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].getProgram(), CStringA(m_infoTuners[infoMsg->numTuner].getPidsToFiltering()));
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Change State: FLTR [%s] : Program filtering: %d ; filtered PIDs of program: %s\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].getProgram(), CStringA(m_infoTuners[infoMsg->numTuner].getPidsToFiltering()));
					}
					else
						m_infoTuners[infoMsg->numTuner].ChangePIDsByProgramInStreaming(atoi(infoMsg->setValue), m_HDHRClientIP);
				}
			}
			else
			{
				m_infoTuners[infoMsg->numTuner].ChangeStateToFilteringByProgram(0);

				if (m_infoTuners[infoMsg->numTuner].getChannel() == 0)
				{
					//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
					//{
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
					//}
					LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
				}
				else
				{
/*					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
					{
						_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: Change State of Tuner%d: TNCH [%s] : Channel/freq: %ld\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].getChannel());
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
					}
*/
					LOGM(TRZ2,1,"[Tuner %d] Change State: TNCH [%s] : Channel/freq: ---\n", infoMsg->numTuner, m_HDHRClientIP); //, m_infoTuners[infoMsg->numTuner].getChannel());
				}
			}
		}
		break;
	case TUNERX_TARGET_MSG:
		if (m_infoTuners[infoMsg->numTuner].getState() == STANDBY)
			break;
		if (infoMsg->setMsg)
		{
			if (strcmp(infoMsg->setValue, NONE) && infoMsg->setValue && strlen(infoMsg->setValue) != 0 && strcmp(infoMsg->setValue, ""))
			{
				if (strcmp(m_infoTuners[infoMsg->numTuner].getTarget(), infoMsg->setValue) || m_infoTuners[infoMsg->numTuner].getState() != STREAMING)
				{
					if (m_infoTuners[infoMsg->numTuner].getChannel() != 0)
					{
						m_infoTuners[infoMsg->numTuner].ChangeStateToStreaming(infoMsg->setValue);

						if (m_infoTuners[infoMsg->numTuner].getState() == STREAMING)
						{
							//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
							//{
							//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STRM [%s] : Target: %s\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].getTarget());
							//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
							//}
							LOGM(TRZ2,1,"[Tuner %d] Change State: STRM [%s] : Target: %s\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].getTarget());
						}
						else if (m_infoTuners[infoMsg->numTuner].getState() == STANDBY)
						{
							//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_1))
							//{
							//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d]                    [%s] : NOT possible change state to STREAMING, channel undefined or none\n", infoMsg->numTuner, m_HDHRClientIP);
							//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_1);
							//}
							LOGM(TRZ1,1,"[Tuner %d]                    [%s] : NOT possible change state to STREAMING, channel undefined or none\n", infoMsg->numTuner, m_HDHRClientIP);
							//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
							//{
							//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STANDBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
							//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
							//}
							LOGM(TRZ2,1,"[Tuner %d] Change State: STANDBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
						}
						else if (m_infoTuners[infoMsg->numTuner].getState() == FILTERING)
						{
							//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
							//{
							//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: FLTR [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
							//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
							//}
							LOGM(TRZ2,1,"[Tuner %d] Change State: FLTR [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
						}
					}
					else
					{
						m_infoTuners[infoMsg->numTuner].ChangeStateToStandby();

						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_1))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d]                    [%s] : NOT possible change state to STREAMING, channel undefined or none\n", infoMsg->numTuner, m_HDHRClientIP);
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_1);
						//}
						LOGM(TRZ1,1,"[Tuner %d]                    [%s] : NOT possible change state to STREAMING, channel undefined or none\n", infoMsg->numTuner, m_HDHRClientIP);
						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STANDBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Change State: STANDBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
					}
				}
			}
			else
			{
				m_infoTuners[infoMsg->numTuner].ChangeStateToStreaming("none");

				if (m_infoTuners[infoMsg->numTuner].getProgram() == 0 && !strcmp(m_infoTuners[infoMsg->numTuner].getFilter(), "0x0000-0x1FFF"))
				{
					if (m_infoTuners[infoMsg->numTuner].getChannel() == 0)
					{
						m_infoTuners[infoMsg->numTuner].ChangeStateToStandby();

						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
					}
					else
					{
						m_infoTuners[infoMsg->numTuner].ChangeStateToTunedChan(m_infoTuners[infoMsg->numTuner].getChannel());

						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: TNCH [%s] : Channel/freq: %ld\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].canal);
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Change State: TNCH [%s] : Channel/freq: %ld\n", infoMsg->numTuner, m_HDHRClientIP, m_infoTuners[infoMsg->numTuner].canal);
					}
				}
				else
				{
					if (m_infoTuners[infoMsg->numTuner].getState() != FILTERING)
					{
						m_infoTuners[infoMsg->numTuner].setState(FILTERING);

						//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
						//{
						//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: FLTR [%s]: Target is setting to none\n", infoMsg->numTuner, m_HDHRClientIP);
						//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
						//}
						LOGM(TRZ2,1,"[Tuner %d] Change State: FLTR [%s]: Target is setting to none\n", infoMsg->numTuner, m_HDHRClientIP);
					}
				}
			}
		}
		break;
	case TUNERX_FILTER_MSG:
		if (m_infoTuners[infoMsg->numTuner].getState() == STANDBY)
			break;
		if (infoMsg->setMsg && infoMsg->setValue)
		{
				if ((strlen(infoMsg->setValue) == 6) && (strcmp(infoMsg->setValue, "bypass") == 0))
				{
					//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_4))
					//{
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: 'bypass' set filter \n", infoMsg->numTuner);
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);
					//}
					LOGM(TRZ4,1,"[Tuner %d] Change State: 'bypass' set filter \n", infoMsg->numTuner);
					break;
				}

				if (m_infoTuners[infoMsg->numTuner].canal != 0 && m_cfgProxy->ObtainIndexChannel(m_infoTuners[infoMsg->numTuner].canal) != -1)
				{
					if (m_infoTuners[infoMsg->numTuner].getState() != STREAMING)
						m_infoTuners[infoMsg->numTuner].ChangeStateToFilteringByFilter(infoMsg->setValue, m_HDHRClientIP);
					else
						m_infoTuners[infoMsg->numTuner].ChangePIDsByFilterInStreaming(infoMsg->setValue, m_HDHRClientIP);
				}
				else
				{
					m_infoTuners[infoMsg->numTuner].ChangeStateToStandby();

					//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
					//{
					//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
					//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
					//}
					LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
				}
		}
		break;
	case TUNERX_STATUS_MSG:  //This message is not a set one but to send the status is checked, in case of the state is STREAMING by HTTP, if data is being received and if it have to update the status of the signal.
		if (m_infoTuners[infoMsg->numTuner].getState() == STREAMING
			&& m_infoTuners[infoMsg->numTuner].getTransportTuner()->getTypeInTransport() == HTTP_TS)
		{
			if (m_infoTuners[infoMsg->numTuner].getTransportTuner()->getReceivingDataHTTP())
			{
				int ch = m_cfgProxy->ObtainIndexChannel(m_infoTuners[infoMsg->numTuner].getChannel());
				if (ch != -1)
				{
					m_infoTuners[infoMsg->numTuner].setSS(m_cfgProxy->m_infoChannels[ch].signalStrength);
					m_infoTuners[infoMsg->numTuner].setSNQ(m_cfgProxy->m_infoChannels[ch].signalQuality);
					m_infoTuners[infoMsg->numTuner].setSEQ(m_cfgProxy->m_infoChannels[ch].symbolQuality);
					m_infoTuners[infoMsg->numTuner].setBPS(m_cfgProxy->m_infoChannels[ch].networkRate);
					m_infoTuners[infoMsg->numTuner].setPPS(m_cfgProxy->m_infoChannels[ch].networkRate);
					//m_infoTuners[infoMsg->numTuner].setPPS(0);
				}
			}
			else
			{
				int changeSignals = 0;
				if (m_infoTuners[infoMsg->numTuner].ss != 0 || m_infoTuners[infoMsg->numTuner].snq != 0 || m_infoTuners[infoMsg->numTuner].seq)
					changeSignals = 1;

				m_infoTuners[infoMsg->numTuner].setSS(0);
				m_infoTuners[infoMsg->numTuner].setSNQ(0);
				m_infoTuners[infoMsg->numTuner].setSEQ(0);
				m_infoTuners[infoMsg->numTuner].setBPS(0);
				m_infoTuners[infoMsg->numTuner].setPPS(0);

				if (changeSignals)
					m_cfgProxy->UpdateClientTOInterface(infoMsg->numTuner);
			}
		}

		break;
	}

	if (m_infoTuners[infoMsg->numTuner].getTransportTuner()->getfailedConnectHTTP() &&
		m_infoTuners[infoMsg->numTuner].getTransportTuner()->getrefreshFailedConnHTTP())
	{
		m_infoTuners[infoMsg->numTuner].setSS(1);
		m_infoTuners[infoMsg->numTuner].setSNQ(0);
		m_infoTuners[infoMsg->numTuner].setSEQ(0);
		m_infoTuners[infoMsg->numTuner].setBPS(0);
		m_infoTuners[infoMsg->numTuner].setPPS(0);
		m_infoTuners[infoMsg->numTuner].ChangeStateToStandby();

		//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
		//{
		//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
		//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
		//}
		LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s]\n", infoMsg->numTuner, m_HDHRClientIP);
		m_infoTuners[infoMsg->numTuner].getTransportTuner()->setrefreshFailedConnHTTP(0);

		m_cfgProxy->UpdateClientTOInterface(infoMsg->numTuner);
	}

	if (infoMsg->setMsg)
	{
		m_cfgProxy->UpdateClientTOInterface(infoMsg->numTuner);
	}
}

int CControl::ReceiveTCPDataHDHR()
{
	m_infoMsg->tipoMsg = ERROR_MSG;
	m_infoMsg->setMsg = 0;
	strcpy(m_infoMsg->RequestMsg, "");
	strcpy(m_infoMsg->peticionMsg, "");
	strcpy(m_infoMsg->setValue, "");
	strcpy(m_infoMsg->unknownMsg, "");

	if (!m_libHDHR.ReceiveDataControlTCP(&m_NumMsg, &m_infoMsg, m_clientSocket, m_Traces))
	{
		return 0;
	}

	//Every time arrives a client request for a tuner, update the timers of tuner.
	m_infoTuners[m_infoMsg->numTuner].SetTimers();

	if (!m_cfgProxy->getLock())
	{
		TreatTypeHDHRMessage(m_infoMsg);
	}
	else
	{
		if (m_infoMsg->setMsg &&
			(m_infoTuners[m_infoMsg->numTuner].lockkey == 0
			|| m_infoTuners[m_infoMsg->numTuner].lockkey == m_infoMsg->IDLockkeyReceived
			|| ((strstr(m_infoMsg->peticionMsg, "/lockkey") != NULL) && !strcmp(m_infoMsg->setValue, "force"))))
			TreatTypeHDHRMessage(m_infoMsg);
		else
		{
			if (m_infoMsg->setMsg)
			{
				//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_4))
				//{
				//	char log_output[1024];
				//	memset(log_output, 0, 1024);
				//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: lockkey ERROR (current=%lu,sended=%lu) \n", m_infoMsg->numTuner, m_infoTuners[m_infoMsg->numTuner].lockkey, m_infoMsg->IDLockkeyReceived);
				//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);
				//}
				LOGM(TRZ4,1,"[Tuner %d] Change State: lockkey ERROR (current=%lu,sended=%lu) \n", m_infoMsg->numTuner, m_infoTuners[m_infoMsg->numTuner].lockkey, m_infoMsg->IDLockkeyReceived);
				m_infoMsg->tipoMsg = ERROR_LOCKKEY_MSG;
				strcpy(m_infoMsg->unknownMsg, m_infoMsg->peticionMsg);
			}
		}
	}

	m_libHDHR.SendResponseControl(m_infoMsg, m_infoTuners, m_clientSocket, m_Traces);

	return 1;
}

void CControl::ResetTuner(int tuner)
{
	//char log_output[1024];
	//memset(log_output, 0, 1024);

	//Reset tuner: Change to STANDBY state
	m_infoTuners[tuner].ChangeStateToStandby();
	m_infoTuners[tuner].setChannelNotInMapList(0);

	m_cfgProxy->UpdateClientTOInterface(tuner);

	//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
	//{
	//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s] : RESET TUNER from HDHRProxyIPTV App\n", tuner, m_HDHRClientIP);
	//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
	//}
	LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s] : RESET TUNER from HDHRProxyIPTV App\n", tuner, m_HDHRClientIP);
}

void CControl::ForceUnlockTuner(int tuner)
{
	if (m_infoTuners[tuner].getLockkey() != 0)
	{
		//char log_output[1024];
		//memset(log_output, 0, 1024);

		m_infoTuners[tuner].setLockkey(0);

		m_cfgProxy->UpdateClientTOInterface(tuner);

		//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
		//{
		//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Force unlock by HDHRProxyIPTV App\n", tuner);
		//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
		//}
		LOGM(TRZ2,1,"[Tuner %d] Force unlock by HDHRProxyIPTV App\n", tuner);
	}
}

void CControl::RestartProxy()
{

	for (int i = 0; i < m_numTuners; i++)
	{
		m_infoTuners[i].ChangeStateToStandby();
		m_cfgProxy->UpdateClientTOInterface(i);
	}
	
	m_cfgProxy->AccessToRestartProxy();
}

void CALLBACK CControl::TimerProcTuners_Wrapper(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	CControl* controlObject = (CControl*)pObject;
	controlObject->TimerProcTuners(hwnd, uMsg, idEvent, dwTime);
}

void CALLBACK CControl::TimerProcTuners(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	//char log_output[1024];
	//memset(log_output, 0, 1024);

	for (int i = 0; i < m_numTuners; i++)
	{
		if (m_infoTuners[i].CheckTimer())
		{
			//if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
			//{
			//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] TIMEOUT in received request from clients.\n", i);
			//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
			//	_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: [Tuner %d] Change State: STBY [%s]: By timeout\n", i, m_HDHRClientIP);
			//	m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
			//}
			LOGM(TRZ2,1,"[Tuner %d] TIMEOUT in received request from clients.\n", i);
			LOGM(TRZ2,1,"[Tuner %d] Change State: STBY [%s]: By timeout\n", i, m_HDHRClientIP);
			m_cfgProxy->UpdateClientTOInterface(i);
		}
	}
}