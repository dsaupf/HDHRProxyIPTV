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

#include "hdhomerun.h"
#include "Parameterization.h"
#include "Trace.h"
#include "winsock2.h"
#include "ConfigProxy.h"
#include "InfoHDHR.h"
#include "Tuner.h"

#define HDHR_CONTROL_CONNECT_TIMEOUT 2500

class HDHRlibFacade
{
public:
	CConfigProxy* m_cfgProxy;
	hdhomerun_sock_t m_sockDiscovery; //UDP server Socket for Discovery phase
	uint32_t m_IPHDHR; //IP of HDHR Client to which send the messages in Discovery phase
	uint16_t m_remotePortDisc; //Client remote port where send Discovery response
	uint32_t m_remoteIPDisc; //IP of client in Discovery phase
	char *m_sRemoteIPDisc; //IP of client in Discovery phase
	uint16_t m_remotePortControl; //remote client port where send Control response
	uint32_t m_iRemoteIPControl; //IP of client in Control phase
	char *m_sRemoteIPControl; //IP of client in Control phase
	hdhomerun_sock_t m_sockControl; //TCP server Socket for Control phase
	hdhomerun_sock_t m_sockControlAccept;
	char m_location[10];
	void setLocation(char* loc) { strcpy(m_location, loc); }

	HDHRlibFacade();
	~HDHRlibFacade();

	int getPortServHDHR() { return HDHOMERUN_DISCOVER_UDP_PORT; }
	uint16_t getPortClientHDHR() { return m_remotePortDisc; }
	char *getDiscIPClientHDHR() {return m_sRemoteIPDisc; }
	void setIPHDHR(CString ip);
	void TreatIP(char *out, char* ip);
	int CreateSocketServUDP_Discovery(CTrace *traceLog = NULL);
	void StopSocketServUDP_Discovery();
	int ReceiveDataDiscoveryUDP(CTrace *traceLog = NULL);
	int SendResponseDiscovery(UINT32 idDispoditive);
	int CreateSocketServTCP_Control(SOCKET* sock, CTrace *traceLog);
	int ReceiveDataControlTCP(long* numMsg, InfoMessageHDHR** infoMsg, SOCKET socket, CTrace *trz = NULL);
	int Accept(char* ip, int* port, SOCKET* sock);
	int SendResponseControl(InfoMessageHDHR* infoMsg, CTuner* infTuner, SOCKET socket, CTrace *trz = NULL);
	int CreateMessageResponseHDHR(InfoMessageHDHR* infoReqMsg, struct hdhomerun_pkt_t **msg, CTuner* infTuner, CTrace* trz);
	int ObtainTypeMessage(struct hdhomerun_pkt_t *rx_pkt, CTrace *trz, InfoMessageHDHR** infoMsg);
	void StopSocketServTCP_Control();

#define LOGD(trace, level, only, text, ...) if (trace) { LOGG(trace, "HDHR-LIB-D ::", level, only, text, ##__VA_ARGS__) }
#define LOGC(trace, level, only, text, ...) if (trace) { LOGG(trace, "HDHR-LIB   ::", level, only, text, ##__VA_ARGS__) }
};
