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
#include "HDHRlibFacade.h"

HDHRlibFacade::HDHRlibFacade()
{
	m_remotePortDisc = HDHOMERUN_DISCOVER_UDP_PORT;

	m_sRemoteIPControl = new char[16];
	m_sRemoteIPDisc = new char[16];

	m_cfgProxy = CConfigProxy::GetInstance();
}


HDHRlibFacade::~HDHRlibFacade()
{
	delete []m_sRemoteIPControl;
	delete []m_sRemoteIPDisc;
}

void HDHRlibFacade::setIPHDHR(CString ip)
{
	CStringA ipAux(ip);
	UINT32 uiIP = inet_addr(ipAux);

	m_IPHDHR = htonl(uiIP);
}

int HDHRlibFacade::CreateSocketServUDP_Discovery(CTrace *traceLog)
{
	/* Creation of the socket. */
	m_sockDiscovery = hdhomerun_sock_create_udp();
	if (m_sockDiscovery == HDHOMERUN_SOCK_INVALID) {
		if (traceLog) traceLog->WriteTrace("DISCOVERY  :: Error in creating the Socket server to HDHR\n", ERR);
		return 0;
	}

	/* Bind socket. A local address is associated to the socket */
	if (!hdhomerun_sock_bind(m_sockDiscovery, m_IPHDHR, HDHOMERUN_DISCOVER_UDP_PORT)) {
		hdhomerun_sock_destroy(m_sockDiscovery);
		if (traceLog) traceLog->WriteTrace("DISCOVERY  :: Could not associate (bind) direction to Discovery UDP socket for HDHR\n", ERR);

		return 0;
	}

	if (traceLog) traceLog->WriteTrace("DISCOVERY  :: Created Socket UDP Server for HDHR \n", LEVEL_TRZ_2);

	return 1;
}

void HDHRlibFacade::StopSocketServUDP_Discovery()
{
	shutdown(m_sockDiscovery, SD_BOTH);
	closesocket(m_sockDiscovery);
}

void HDHRlibFacade::TreatIP(char *out, char* ip)
{
	char * tok;
	char tmp[16];
	char ipRes[16];

	memset(tmp, 0, 16);
	memset(ipRes, 0, 16);

	tok = strtok(ip, ".");
	_snprintf(tmp, 4, "%03s", tok);
	strcpy(ipRes, tmp);

	while (tok != NULL)
	{
		tok = strtok(NULL, ".");
		if (tok != NULL)
		{
			_snprintf(tmp, 4, ".%03s", tok);
			strcat(ipRes, tmp);
		}
	}
	//delete[]tok;   // Bug top is pointer only!
	//return ipRes;  // Bug, ipRes is internal string!
	memcpy(out, ipRes, 16);
	/*
	if (trz->IsLevelWriteable(LEVEL_TRZ_6))
	{
		char trace[256];
		_snprintf(trace, trace_length - 2, "CONTROL    :: HDHRlibFacade::TreatIP() input='%s' output='%s' \n", ip, out);
		trz->WriteTrace(trace, LEVEL_TRZ_6);
	}
	*/
}

int HDHRlibFacade::ReceiveDataDiscoveryUDP(CTrace *traceLog)
{
	struct hdhomerun_pkt_t *rx_pkt = hdhomerun_pkt_create();

	if (!rx_pkt)
		return 0;

	size_t length = rx_pkt->limit - rx_pkt->end;

	if (!hdhomerun_sock_recvfrom(m_sockDiscovery, &m_remoteIPDisc, &m_remotePortDisc, rx_pkt->end, &length, 60))
	{
		free(rx_pkt);
		return 0;
	}

	struct sockaddr_in adr;
	u_long ip = htonl(m_remoteIPDisc);
	adr.sin_addr.s_addr = ip;
	//m_sRemoteIPDisc = inet_ntoa(adr.sin_addr);  // Bug, m_sRemoteIPDisc is a pointer to char* new[]!
	//strcpy(m_sRemoteIPDisc, TreatIP(m_sRemoteIPDisc));
	TreatIP(m_sRemoteIPDisc, inet_ntoa(adr.sin_addr));

	//Process the received data.Checked if it is the expected message : UDP Discovery
	
	rx_pkt->end += length;

	uint16_t type;
	if (hdhomerun_pkt_open_frame(rx_pkt, &type) <= 0)
		return 0;
	
	if (type != HDHOMERUN_TYPE_DISCOVER_REQ)
		return -1;

	uint8_t tag;
	size_t len;
	uint8_t *next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
	if (tag != HDHOMERUN_TAG_DEVICE_TYPE)
	{
		free(rx_pkt);
		return -1;
	}

	rx_pkt->pos = next;
	next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
	if (tag != HDHOMERUN_TAG_DEVICE_ID)
	{
		free(rx_pkt);
		return -1;
	}

	free(rx_pkt);
	

	return 1;
}

int HDHRlibFacade::SendResponseDiscovery(UINT32 idDispoditive)
{
	// Creation of UDP message
	struct hdhomerun_pkt_t *msgDatos = hdhomerun_pkt_create();

	uint32_t device_type = HDHOMERUN_DEVICE_TYPE_TUNER;

#ifndef HDHOMERUN_TAG_TUNER_COUNT  // old versions of libhdhomerun
#define HDHOMERUN_TAG_TUNER_COUNT 0x10
#endif
	
	hdhomerun_pkt_write_u8(msgDatos, HDHOMERUN_TAG_DEVICE_ID);
	hdhomerun_pkt_write_var_length(msgDatos, 4);
	hdhomerun_pkt_write_u32(msgDatos, (uint32_t)idDispoditive);
	hdhomerun_pkt_write_u8(msgDatos, HDHOMERUN_TAG_DEVICE_TYPE);
	hdhomerun_pkt_write_var_length(msgDatos, 4);
	hdhomerun_pkt_write_u32(msgDatos, device_type);
	hdhomerun_pkt_write_u8(msgDatos, HDHOMERUN_TAG_TUNER_COUNT);
	hdhomerun_pkt_write_var_length(msgDatos, 1);
	hdhomerun_pkt_write_u8(msgDatos, (uint8_t)m_cfgProxy->getTunersNumber());
	hdhomerun_pkt_seal_frame(msgDatos, HDHOMERUN_TYPE_DISCOVER_RPY);

	size_t tamDat = msgDatos->end - msgDatos->start;

	//Send message
	
	//if (!hdhomerun_sock_sendto(m_sockDiscovery, m_IPHDHR, m_remotePortDisc, msgDatos->start, tamDat, 0))
	if (!hdhomerun_sock_sendto(m_sockDiscovery, m_remoteIPDisc, m_remotePortDisc, msgDatos->start, tamDat, 0))
	{
		free(msgDatos);
		return 0;
	}

	free(msgDatos);

	return 1;
}

int HDHRlibFacade::CreateSocketServTCP_Control(SOCKET* sock, CTrace *traceLog)
{
	m_sockControl = hdhomerun_sock_create_tcp();

	if (m_sockControl == HDHOMERUN_SOCK_INVALID)
		return 0;

	int yes = 1;
	if (setsockopt(m_sockControl, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(int)) == -1) {
		return 0;
	}

	//IP and port are assigned to the socket
	if (!hdhomerun_sock_bind(m_sockControl, m_IPHDHR, HDHOMERUN_CONTROL_TCP_PORT)) {
		int err = WSAGetLastError();
		hdhomerun_sock_destroy(m_sockControl);
		return 0;
	}
	
	if (listen(m_sockControl, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	*sock = m_sockControl;

	return 1;
}

int HDHRlibFacade::Accept(char* ip, int* port, SOCKET* sock)
{
	struct sockaddr_in serv;
	socklen_t len = sizeof(struct sockaddr_in);

	m_sockControlAccept = accept(m_sockControl, (struct sockaddr *) &serv, &len);

	if (m_sockControlAccept <= 0)
	{
		return 0;
	}
	
	m_remotePortControl = ntohs(serv.sin_port);
	m_iRemoteIPControl = ntohl(serv.sin_addr.s_addr);
	//m_sRemoteIPControl = inet_ntoa(serv.sin_addr);  // Bug, m_sRemoteIPControl is pointer to char* new[];
	strcpy(m_sRemoteIPControl, inet_ntoa(serv.sin_addr));

	//strcpy(ip, m_sRemoteIPControl);
	//strcpy(ip, TreatIP(ip));
	TreatIP(ip, m_sRemoteIPControl);
	*port = m_remotePortControl;
	*sock = m_sockControlAccept;

	return 1;
}

int HDHRlibFacade::ReceiveDataControlTCP(long* numMsg, InfoMessageHDHR** infoMsg, SOCKET socket, CTrace *trz)
{
	m_sockControlAccept = socket;

	struct hdhomerun_pkt_t *rx_pkt = hdhomerun_pkt_create();

	if (!rx_pkt)
		return -1;

	size_t length = rx_pkt->limit - rx_pkt->end;
	
	uint64_t stop_time = getcurrenttime() + 2500;
	uint64_t current_time = getcurrenttime();

	if (!hdhomerun_sock_recv(m_sockControlAccept, rx_pkt->end, &length, stop_time - current_time))
	{
		hdhomerun_pkt_destroy(rx_pkt);
		return 0;
	}
	
	if (*numMsg == 999999)
		*numMsg = 1;
	else
		*numMsg = *numMsg +1;

	(*infoMsg)->numMsg = *numMsg;

	rx_pkt->end += length;


	(*infoMsg)->tipoMsg = ObtainTypeMessage(rx_pkt, trz, infoMsg);

	hdhomerun_pkt_destroy(rx_pkt);

	return 1;
}

//param setMsg indicates whether the received message is of type "set" (1), and not "get".If set=1, the value of the received message is returned in param setValue
int HDHRlibFacade::ObtainTypeMessage(struct hdhomerun_pkt_t *rx_pkt, CTrace *trz, InfoMessageHDHR** infoMsg)
{
	int tipo_msg = ERROR_MSG;
	uint16_t type;
	uint8_t tag;
	size_t len;
	char *value = new char[256];
	memset(value, 0, 256);

	int foundMsg = 0;
	char auxString[1024];
	memset(auxString, 0, 1024);
	uint8_t *next = NULL;
	char log_output[4096];
	memset(log_output, 0, 4096);
	(*infoMsg)->setMsg = 0;
	strcpy((*infoMsg)->setValue, "");
	strcpy((char*)(*infoMsg)->RequestMsg, "");
	(*infoMsg)->IDLockkeyReceived = 0;
	uint32_t  seq = 0;
	uint8_t* data;
	int tam = 0;
	int toHex = 0;
	int ret = hdhomerun_pkt_open_frame(rx_pkt, &type);
	int tunersNum = m_cfgProxy->getTunersNumber();

	if (strlen((char *)rx_pkt->pos) > 50)
	{
		strncpy((char*)(*infoMsg)->RequestMsg, (char *)rx_pkt->pos, 50);
	}
	else
		strcpy((char*)(*infoMsg)->RequestMsg, (char *)rx_pkt->pos);

	if (ret < 0)
		return -1;

	switch (type) {
		
	case HDHOMERUN_TYPE_GETSET_REQ:
		next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
		if (!next)
			return -1;

		if (tag == HDHOMERUN_TAG_GETSET_NAME)
		{
			strcpy(value, (char *)rx_pkt->pos);
			strcpy((char*)(*infoMsg)->peticionMsg, value);
			rx_pkt->pos[len] = 0;
			rx_pkt->pos = next;

			if (strcmp(value, SYS_FEATURES) == 0)
				tipo_msg = FEATURES_MSG;
			else if (strcmp(value, SYS_MODEL) == 0)
				tipo_msg = MODEL_MSG;
			else if (strcmp(value, SYS_DVBC_MODULATION) == 0)
				tipo_msg = SYS_DVBC_MODULATION_MSG;
			else if (strstr(value, "/status") != NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_STATUS, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
					tipo_msg = TUNERX_STATUS_MSG;
			}
			else if (strstr(value, "/target") != NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_TARGET, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
				{
					next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
					if (next)
					{
						strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);

						//It is necessary in case of in the message indicating the tag HDHOMERUN_TAG_GETSET_LOCKKEY, to get it 
						hdhomerun_pkt_read_u32(rx_pkt);
						hdhomerun_pkt_read_u32(rx_pkt);

						rx_pkt->pos[len] = 0;
						rx_pkt->pos = next;

						(*infoMsg)->setMsg = 1;
					}
					tipo_msg = TUNERX_TARGET_MSG;
				}
			}
			else if (strstr(value, "/streaminfo") != NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_STREAMINFO, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
					tipo_msg = TUNERX_STREAMINFO_MSG;
			}
			else if (strstr(value, "/channelmap") != NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_CHANNELMAP, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
				{
					next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
					if (next)
					{
						strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);

						//It is necessary in case of in the message indicating the tag HDHOMERUN_TAG_GETSET_LOCKKEY, to get it 
						hdhomerun_pkt_read_u32(rx_pkt);
						hdhomerun_pkt_read_u32(rx_pkt);
						
						rx_pkt->pos[len] = 0;
						rx_pkt->pos = next;

						(*infoMsg)->setMsg = 1;
					}
					tipo_msg = TUNERX_CHANNELMAP_MSG;
				}
			}
			else if (strstr(value, "/program") != NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_PROGRAM, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}
	
				if (foundMsg)
				{
					next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
					if (next)
					{
						strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);
						//It is necessary in case of in the message indicating the tag HDHOMERUN_TAG_GETSET_LOCKKEY, to get it 
						hdhomerun_pkt_read_u32(rx_pkt);
						hdhomerun_pkt_read_u32(rx_pkt);
						
						rx_pkt->pos[len] = 0;
						rx_pkt->pos = next;

						(*infoMsg)->setMsg = 1;
					}
					tipo_msg = TUNERX_PROGRAM_MSG;
				}
			}
			else if (strcmp(value, SYS_HWMODEL) == 0)
				tipo_msg = SYS_HWMODEL_MSG;
			else if (strcmp(value, SYS_COPYRIGHT) == 0)
				tipo_msg = SYS_COPYRIGHT_MSG;
			else if (strcmp(value, HELP) == 0)
				tipo_msg = HELP_MSG;
			else if (strcmp(value, VERSION) == 0)
				tipo_msg = VERSION_MSG;
			else if (strcmp(value, LINEUP_LOC) == 0)
			{
				next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
				if (next)
				{
					strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);
					rx_pkt->pos[len] = 0;
					rx_pkt->pos = next;

					(*infoMsg)->setMsg = 1;
				}
				tipo_msg = LINEUP_LOC_MSG;
			}
			else if (strcmp(value, SYS_RESTART) == 0)
			{
				next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
				if (next)
				{
					strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);
					rx_pkt->pos[len] = 0;
					rx_pkt->pos = next;

					(*infoMsg)->setMsg = 1;
				}
				tipo_msg = SYS_RESTART_MSG;
			}
			else if (strcmp(value, SYS_DEBUG) == 0)
				tipo_msg = SYS_DEBUG_MSG;

			else if (strstr(value, "/filter") != NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_FILTER, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
				{
					next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
					if (next)
					{
						int lon = strlen((char *)rx_pkt->pos) + 1; //+'\0'

/*
						if (!trz->IsPrintable((char *)rx_pkt->pos))
						{
							if (lon * 5 + 1 > MAX_SIZE_FILTER) {
								if (trz) trz->WriteTrace("CONTROL    :: realloc in HDHRlibFacade::ObtainTypeMessage()\n", LEVEL_TRZ_1);
								(*infoMsg)->setValue = (char*)realloc((char*)(*infoMsg)->setValue, (lon * 5 + 1) * sizeof(char *));
							}
							//(*infoMsg)->setValue = trz->ConvertToHex((char *)rx_pkt->pos);
							strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);

							// toHex = 1;
						}
						else
						{
							if (lon + 1 > MAX_SIZE_FILTER) {
							if (trz) trz->WriteTrace("CONTROL    :: realloc in HDHRlibFacade::ObtainTypeMessage()\n", LEVEL_TRZ_1);
								(*infoMsg)->setValue = (char*)realloc((char*)(*infoMsg)->setValue, (lon + 1) * sizeof(char *));
							}
							strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);
						}
*/
						if (strlen((char *)rx_pkt->pos) <= 1)
						{
							//strcpy((*infoMsg)->setValue, (char *)"0x0000-0x1FFF\0");
							strcpy((*infoMsg)->setValue, (char *)"bypass\0");
							if (trz) trz->WriteTrace("CONTROL    :: UNKNOWN [set filter] in HDHRlibFacade::ObtainTypeMessage() (bypass) \n", LEVEL_TRZ_3);
						}
						else
						{
							if (trz->IsPrintable((char *)rx_pkt->pos))
							{
								strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);
							}
							else
							{
								//strcpy((*infoMsg)->setValue, (char *)"0x0000-0x1FFF\0");
								strcpy((*infoMsg)->setValue, (char *)"bypass\0");
								if (trz) trz->WriteTrace("CONTROL    :: FORMAT ERR [set filter] in HDHRlibFacade::ObtainTypeMessage() (bypass) \n", LEVEL_TRZ_3);
							}
						}

						//It is necessary in case of in the message indicating the tag HDHOMERUN_TAG_GETSET_LOCKKEY, to get it 
						hdhomerun_pkt_read_u32(rx_pkt);
						hdhomerun_pkt_read_u32(rx_pkt);

						rx_pkt->pos[len] = 0;
						rx_pkt->pos = next;

						(*infoMsg)->setMsg = 1;
					}
					tipo_msg = TUNERX_FILTER_MSG;
				}
			}
			else if (strstr(value, "/lockkey") != NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_LOCKKEY, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
				{
					next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
					if (next)
					{
						strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);

						//It is necessary in case of in the message indicating the tag HDHOMERUN_TAG_GETSET_LOCKKEY, to get it 
						hdhomerun_pkt_read_u32(rx_pkt);
						hdhomerun_pkt_read_u32(rx_pkt);
		
						rx_pkt->pos[len] = 0;
						rx_pkt->pos = next;

						(*infoMsg)->setMsg = 1;
					}
					tipo_msg = TUNERX_LOCKKEY_MSG;
				}
			}
			else if (strstr(value, "/debug") != NULL && strstr(value, "/sys/debug") == NULL)
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_DEBUG, i);

					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
					tipo_msg = TUNERX_DEBUG_MSG;
			}
			else if ((strstr(value, "/channel") != NULL) && (strstr(value, "/channelmap") == NULL))
			{
				for (int i = 0; i < tunersNum; i++)
				{
					_snprintf(auxString, sizeof(auxString) - 2, TUNERX_CHANNEL, i);
					
					if (!strcmp(value, auxString))
					{
						(*infoMsg)->numTuner = i;
						foundMsg = 1;
						i = tunersNum;
					}
				}

				if (foundMsg)
				{
					next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);

					if (next)
					{
							strcpy((*infoMsg)->setValue, (char *)rx_pkt->pos);
							//It is necessary in case of in the message indicating the tag HDHOMERUN_TAG_GETSET_LOCKKEY, to get it 
							hdhomerun_pkt_read_u32(rx_pkt);
							hdhomerun_pkt_read_u32(rx_pkt);
							
							rx_pkt->pos[len] = 0;
							rx_pkt->pos = next;

							(*infoMsg)->setMsg = 1;

					}
					tipo_msg = TUNERX_CHANNEL_MSG;
				}
			}
			else
			{
				strcpy((*infoMsg)->unknownMsg, value);
			}
			

			if ((*infoMsg)->setMsg)
			{
				next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
				if (next)
				{
					if (tag == HDHOMERUN_TAG_GETSET_LOCKKEY)
					{
						(*infoMsg)->IDLockkeyReceived = hdhomerun_pkt_read_u32(rx_pkt);
						rx_pkt->pos[len] = 0;
						rx_pkt->pos = next;
					}
				}
			}
		}
		break;
	case HDHOMERUN_TYPE_UPGRADE_REQ:
		data = new uint8_t[256];
		seq = hdhomerun_pkt_read_u32(rx_pkt);		//sequence of upgrade packet. Several packets of 256 bytes are sent
		(*infoMsg)->seqUpgrade = seq;

		if (seq != 4294967295) //Not is the last upgrade message. The last is seq. :0xFFFFFFFF (hexadecimal) -> 4294967295 (decimal)
		{
			next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
			data = rx_pkt->pos;

			(*infoMsg)->upgradeMsg = 1;

			if (trz->IsLevelWriteable(LEVEL_TRZ_4))
			{
				_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: -->    Received HDHR message [%s:%d] {%06d} : Firmware Upgrade [Message sequence %u]\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, (*infoMsg)->seqUpgrade);
				trz->WriteTrace(log_output, LEVEL_TRZ_4);
			}

			trz->WriteTrace("CONTROL    :: Received first message of FIRMWARE UPGRADE\n", LEVEL_TRZ_2);
		}
		else
		{
			(*infoMsg)->upgradeMsg = 0;
			if (trz->IsLevelWriteable(LEVEL_TRZ_4))
			{
				_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: -->    Received HDHR message [%s:%d] {%06d} : Firmware Upgrade [Last Message; Upgrade finished; Message sequence %u (0xFFFFFFFF)]\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, (*infoMsg)->seqUpgrade);
				trz->WriteTrace(log_output, LEVEL_TRZ_4);
			}
			trz->WriteTrace("CONTROL    :: Received last message of FIRMWARE UPGRADE\n", LEVEL_TRZ_2);
		}

		tipo_msg = HDHR_UPGRADE_MSG;
		break;
	default:
		if ((*infoMsg)->upgradeMsg == 1)
		{
			seq = hdhomerun_pkt_read_u32(rx_pkt);		//sequence of upgrade packet. Several packets of 256 bytes are sent
			(*infoMsg)->seqUpgrade = seq;

			data = new uint8_t[256];
			next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
			data = rx_pkt->pos;

			if (trz->IsLevelWriteable(LEVEL_TRZ_5))
			{
				_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    :: -->    Received HDHR message [%s:%d] {%06d} : UPGRADING Firmware  [Message sequence %u]\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, (*infoMsg)->seqUpgrade);
				trz->WriteTrace(log_output, LEVEL_TRZ_5);
			}
			tipo_msg = HDHR_UPGRADE_FILE_MSG;
		}
		break;
	}

	if (trz && trz->getTraceLevel() != NO_TRACE && tipo_msg != HDHR_UPGRADE_MSG && tipo_msg != HDHR_UPGRADE_FILE_MSG && (*infoMsg)->upgradeMsg != 1)
	{
		char tip[4];
		int trace_length = 200 + strlen((*infoMsg)->setValue) + strlen((*infoMsg)->RequestMsg);
		char *trace = new char[trace_length];
		memset(trace, 0, trace_length);

		//strcpy(m_sRemoteIPControl, TreatIP(m_sRemoteIPControl));
		TreatIP(m_sRemoteIPControl, m_sRemoteIPControl);
		
		if (tipo_msg != ERROR_MSG)
		{
			int pos = strcspn((*infoMsg)->RequestMsg, "/");
			for (int i = 0; i < pos; i++)
				(*infoMsg)->RequestMsg[i] = '.';
		}
		else
		{
			for (int i = 0; i < 2; i++)
				(*infoMsg)->RequestMsg[i] = '.';
		}

		if (tipo_msg == ERROR_MSG)
		{
			if (trz->IsLevelWriteable(LEVEL_TRZ_2))
			{
				_snprintf(trace, trace_length - 2, "CONTROL    :: UNKNOWN HDHR MESSAGE         [%s:%d] : %s\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->RequestMsg);
				trz->WriteTrace(trace, LEVEL_TRZ_2);
			}
			if (trz->IsLevelWriteable(LEVEL_TRZ_4))
			{
				_snprintf(trace, trace_length - 2, "CONTROL    :: -->    RECEIVED UNKNOWN HDHR MESSAGE [%s:%d] {%06d} : %s\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, (*infoMsg)->RequestMsg);
				trz->WriteTrace(trace, LEVEL_TRZ_4);
			}
		}
		else
		{
			if (type == HDHOMERUN_TYPE_GETSET_REQ)
				{
					if ((*infoMsg)->setMsg)
					{
						strcpy(tip, "set");

						if (trz->IsLevelWriteable(LEVEL_TRZ_4))
						{
							_snprintf(trace, trace_length - 2, "CONTROL    :: -->    Received HDHR message [%s:%d] {%06d} : %s %s\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, tip, (*infoMsg)->RequestMsg);
							trz->WriteTrace(trace, LEVEL_TRZ_4);
						}
						if (toHex)
						{
							if (trz->IsLevelWriteable(LEVEL_TRZ_2))
							{
								_snprintf(trace, trace_length - 2, "CONTROL    :: Content of filter message is not printable. It has been converted to Hexadecimal (%s)\n", (*infoMsg)->setValue);
								trz->WriteTrace(trace, LEVEL_TRZ_2);
							}
						}

						//For the messages with a response in more than 1 lines the content begins at new line
						if (trz->IsLevelWriteable(LEVEL_TRZ_6))
						{
							if (tipo_msg == FEATURES_MSG || tipo_msg == TUNERX_STREAMINFO_MSG)
								_snprintf(trace, trace_length - 2, "CONTROL    ::     \\- CONTENT HDHR message  [%s:%d] {%06d} :\n[%s]\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, (*infoMsg)->setValue);
							else
								_snprintf(trace, trace_length - 2, "CONTROL    ::     \\- CONTENT HDHR message  [%s:%d] {%06d} : [%s]\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, (*infoMsg)->setValue);
							trz->WriteTrace(trace, LEVEL_TRZ_6);
						}
					}
					else
					{
						strcpy(tip, "get");
						if (trz->IsLevelWriteable(LEVEL_TRZ_4))
						{
							_snprintf(trace, trace_length - 2, "CONTROL    :: -->    Received HDHR message [%s:%d] {%06d} : %s %s\n", m_sRemoteIPControl, m_remotePortControl, (*infoMsg)->numMsg, tip, (*infoMsg)->RequestMsg);
							trz->WriteTrace(trace, LEVEL_TRZ_4);
						}
					}
				}
		}

		if (trace)
			delete []trace;
	}

	if (value)
		delete []value;

	(*infoMsg)->tipoMsg = tipo_msg;
	
	return tipo_msg;
}


int HDHRlibFacade::SendResponseControl(InfoMessageHDHR* infoMsg, CTuner* infTuner, SOCKET socket, CTrace *trz)
{
	m_sockControlAccept = socket;

	struct hdhomerun_pkt_t * msg = NULL;
	
	if (CreateMessageResponseHDHR(infoMsg, &msg, infTuner, trz))
	{
		if (infoMsg->upgradeMsg == 1) //Messages of the firmware file
			return 1;

		if (!hdhomerun_sock_send(m_sockControlAccept, msg->start, msg->end - msg->start, 2500)) {
			free(msg);
			return 0;
		}

		if (trz)
		{
			uint16_t type;

			int ret = hdhomerun_pkt_open_frame(msg, &type);

			char* tipmsg = new char[strlen((char *)msg->pos)+1];
			strcpy(tipmsg, (char *)msg->pos);

			char log_output[4096];
			memset(log_output, 0, 4096);
			
			if (infoMsg->tipoMsg != HDHR_UPGRADE_MSG && infoMsg->tipoMsg != HDHR_UPGRADE_FILE_MSG)
			{
				if (infoMsg->tipoMsg != ERROR_MSG)
				{
					int pos = strcspn(tipmsg, "/");
					for (int i = 0; i < pos; i++)
						tipmsg[i] = '.';
				}
				else
				{
					for (int i = 0; i < 2; i++)
						tipmsg[i] = '.';
				}
			}
			if (infoMsg->tipoMsg == ERROR_MSG)
			{
				if (trz->IsLevelWriteable(LEVEL_TRZ_4))
				{
					_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::    <-- SEND UNKNOWN HDHR MESSAGE     [%s:%d] {%06ld} : %s\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg, tipmsg);
					trz->WriteTrace(log_output, LEVEL_TRZ_4);
				}
			}
			else if (infoMsg->tipoMsg == HDHR_UPGRADE_MSG)
			{
				if (trz->IsLevelWriteable(LEVEL_TRZ_4))
				{
					if (infoMsg->seqUpgrade == 4294967295)
						_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::    <-- Send HDHR message     [%s:%d] {%06ld} : Response. Firmware Upgrade finished\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg);
					else
						_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::    <-- Send HDHR message     [%s:%d] {%06ld} : Response. Firmware Upgrade initiated\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg);

					trz->WriteTrace(log_output, LEVEL_TRZ_4);
				}
			}
			else if (infoMsg->tipoMsg == HDHR_UPGRADE_FILE_MSG)
			{
				if (trz->IsLevelWriteable(LEVEL_TRZ_4))
				{
					_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::    <-- Send HDHR message     [%s:%d] {%06ld} : Response. Firmware Upgrade File.\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg);
					trz->WriteTrace(log_output, LEVEL_TRZ_4);
				}
			}
			else
			{
				if (trz->IsLevelWriteable(LEVEL_TRZ_4))
				{
					_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::    <-- Send HDHR message     [%s:%d] {%06ld} : %s\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg, tipmsg);
					trz->WriteTrace(log_output, LEVEL_TRZ_4);
				}
			}

			uint8_t tag;
			size_t len;
			uint8_t *next = hdhomerun_pkt_read_tlv(msg, &tag, &len);
			msg->pos = next;

/*			if (msg->pos)
			{
				int tam = strlen((char*)msg->pos);
				if (600 < tam+100)
				{
					if (trz) trz->WriteTrace("CONTROL    :: realloc in HDHRlibFacade::SendResponseControl()\n", LEVEL_TRZ_1);
					log_output = (char*)realloc(log_output, (strlen((char *)msg->pos) + 100) * sizeof(char *));
				}
			}
*/

			if (infoMsg->tipoMsg != HDHR_UPGRADE_MSG && infoMsg->tipoMsg != HDHR_UPGRADE_FILE_MSG)
			{
				for (int i = 0; i < 2; i++)
					msg->pos[i] = '.';
				if (msg->pos[2] == '\x1' || msg->pos[2] == '\x2')
					msg->pos[2] = '.';
			}
	
			if (trz->IsLevelWriteable(LEVEL_TRZ_6))
			{
				if (infoMsg->tipoMsg == ERROR_MSG)
					_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::     \\- CONTENT HDHR MESSAGE          [%s:%d] {%06ld} : [%s]\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg, (char *)msg->pos);
				else if (infoMsg->tipoMsg != HDHR_UPGRADE_MSG && infoMsg->tipoMsg != HDHR_UPGRADE_FILE_MSG)
				{
					if (infoMsg->tipoMsg == FEATURES_MSG || infoMsg->tipoMsg == TUNERX_STREAMINFO_MSG)
						_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::     \\- CONTENT HDHR message  [%s:%d] {%06ld} :\n[%s]\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg, (char *)msg->pos);
					else
						_snprintf(log_output, sizeof(log_output) - 2, "CONTROL    ::     \\- CONTENT HDHR message  [%s:%d] {%06ld} : [%s]\n", m_sRemoteIPControl, m_remotePortControl, infoMsg->numMsg, (char *)msg->pos);
				}

				if (infoMsg->tipoMsg != HDHR_UPGRADE_MSG && infoMsg->tipoMsg != HDHR_UPGRADE_FILE_MSG)
					trz->WriteTrace(log_output, LEVEL_TRZ_6);
			}

			if (tipmsg)
				delete []tipmsg;

//			delete []valmsg;  // Will be deleted when remove msg
		}
	}

	if (msg)
		delete msg;

	return 1;
}

int HDHRlibFacade::CreateMessageResponseHDHR(InfoMessageHDHR* infoReqMsg, struct hdhomerun_pkt_t **msg, CTuner* infTuner, CTrace* trz)
{
	int res = 0;
	char *name, *value;
	int name_len, value_len;
	int len = 0;
	
	name = (char*)malloc(500);
	value = (char*)malloc(1024);
	memset(name, 0, 500);
	memset(value, 0, 1024);

	*msg = hdhomerun_pkt_create();

	switch (infoReqMsg->tipoMsg)
	{
	case ERROR_MSG:
		len = strlen(infoReqMsg->unknownMsg)+1;
		if (len > 100)
		{
			if (trz) trz->WriteTrace("CONTROL    :: realloc in HDHRlibFacade::CreateMessageResponseHDHR()\n", LEVEL_TRZ_1);
			name = (char*)realloc(name, (len+1) * sizeof(char *));
		}
		strncpy(name, infoReqMsg->unknownMsg, len);
		name_len = (int)strlen(name) + 1;
		strncpy(value, ERROR_VALUE, 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_ERROR_MESSAGE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);
		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);

		res = 1;
		break;
	case ERROR_LOCKKEY_MSG:
		strncpy(name, infoReqMsg->unknownMsg, 500 - 2);
		name_len = (int)strlen(name) + 1;

		//_snprintf(value, 1024 - 2, "ERROR: resource locked by %s", m_sRemoteIPControl);
		_snprintf(value, 1024 - 2, "ERROR: resource locked by %s", infTuner[infoReqMsg->numTuner].IPLockkey);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_ERROR_MESSAGE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);
		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);

		res = 1;
		break;
	case FEATURES_MSG:
		strncpy(name, SYS_FEATURES, 500 - 2);
		name_len = (int)strlen(name) + 1;
		strncpy(value, CStringA(m_cfgProxy->sys_features), 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case MODEL_MSG:
		strncpy(name, SYS_MODEL, 500 - 2);
		name_len = (int)strlen(name) + 1;
		strncpy(value, CStringA(m_cfgProxy->sys_model), 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case SYS_DVBC_MODULATION_MSG:
		strncpy(name, SYS_DVBC_MODULATION, 500 - 2);
		name_len = (int)strlen(name) + 1;
		strncpy(value, CStringA(m_cfgProxy->sys_dvbc_modulation), 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_STATUS_MSG:
		_snprintf(name, 500 - 2, TUNERX_STATUS, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;
		//_snprintf(value, 1024 - 2, TUNERX_STATUS_VALUE, infTuner[infoReqMsg->numTuner].canal, infTuner[infoReqMsg->numTuner].lock, infTuner[infoReqMsg->numTuner].ss, infTuner[infoReqMsg->numTuner].snq, infTuner[infoReqMsg->numTuner].seq, infTuner[infoReqMsg->numTuner].bps, infTuner[infoReqMsg->numTuner].pps);
		// FIXME: Fake "lock" value at time!
		if (infTuner[infoReqMsg->numTuner].snq > 0)
			_snprintf(value, 1024 - 2, TUNERX_STATUS_VALUE, infTuner[infoReqMsg->numTuner].canal, "qam64", infTuner[infoReqMsg->numTuner].ss, infTuner[infoReqMsg->numTuner].snq, infTuner[infoReqMsg->numTuner].seq, infTuner[infoReqMsg->numTuner].bps, infTuner[infoReqMsg->numTuner].pps);
		else
			_snprintf(value, 1024 - 2, TUNERX_STATUS_VALUE, infTuner[infoReqMsg->numTuner].canal, "none", infTuner[infoReqMsg->numTuner].ss, infTuner[infoReqMsg->numTuner].snq, infTuner[infoReqMsg->numTuner].seq, infTuner[infoReqMsg->numTuner].bps, infTuner[infoReqMsg->numTuner].pps);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_TARGET_MSG:
		_snprintf(name, 500 - 2, TUNERX_TARGET, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;

		strncpy(value, infTuner[infoReqMsg->numTuner].getTarget(), 1024 - 2);
			
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_STREAMINFO_MSG:
		_snprintf(name, 500 - 2, TUNERX_STREAMINFO, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;

		if (infTuner[infoReqMsg->numTuner].canal == 0)
			strncpy(value, NONE, 1024);
		else
		{
			strncpy(value, CStringA(m_cfgProxy->ObtainProgsTableByChannel(infTuner[infoReqMsg->numTuner].canal)), 1024 - 2);

			if (!strcmp(value, ""))
				strncpy(value, NONE, 1024);
		}
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_CHANNELMAP_MSG:
		_snprintf(name, 500 - 2, TUNERX_CHANNELMAP, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;
		strncpy(value, infTuner[infoReqMsg->numTuner].channelmap, 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_PROGRAM_MSG:
		_snprintf(name, 500 - 2, TUNERX_PROGRAM, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;
		_snprintf(value, 1024 - 2, TUNERX_PROGRAM_VALUE, infTuner[infoReqMsg->numTuner].program);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case SYS_HWMODEL_MSG:
		strncpy(name, SYS_HWMODEL, 500 - 2);
		name_len = (int)strlen(name) + 1;
		strncpy(value, CStringA(m_cfgProxy->sys_hwmodel), 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case SYS_COPYRIGHT_MSG:
		strncpy(name, SYS_COPYRIGHT, 500 - 2);
		name_len = (int)strlen(name) + 1;
		strncpy(value, CStringA(m_cfgProxy->sys_copyright), 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case HELP_MSG:
		strncpy(name, HELP, 500 - 2);
		name_len = (int)strlen(name) + 1;
		strncpy(value, HELP_VALUE, 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case VERSION_MSG:
		strncpy(name, VERSION, 500 - 2);
		name_len = (int)strlen(name) + 1;
		strcpy(value, CStringA(m_cfgProxy->version));
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case LINEUP_LOC_MSG:
		strcpy(name, LINEUP_LOC);
		name_len = (int)strlen(name) + 1;
		strncpy(value, CStringA(m_cfgProxy->lineup_location), 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case SYS_RESTART_MSG:
		strcpy(name, SYS_RESTART);
		name_len = (int)strlen(name) + 1;
		strncpy(value, infoReqMsg->setValue, 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case SYS_DEBUG_MSG:
		strcpy(name, SYS_DEBUG);
		name_len = (int)strlen(name) + 1;
		strncpy(value, CStringA(m_cfgProxy->sys_debug), 1024 - 2);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_FILTER_MSG:
		_snprintf(name, 500 - 2, TUNERX_FILTER, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;
		// TODO: Use a function to convert to the right format: Ex. "0x0000 0x0010-0x0012"
		//strncpy(value, CStringA(infTuner[infoReqMsg->numTuner].getPidsToFiltering()), 1024 - 2);
		//if (!strcmp(value, ""))
		//	strncpy(value, "0x0000-0x1fff", 1024);
		strncpy(value, infTuner[infoReqMsg->numTuner].getFilter(), 1024 - 2);
		value[1023] = 0;
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_LOCKKEY_MSG:
		_snprintf(name, 500 - 2, TUNERX_LOCKKEY, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;

		if (infTuner[infoReqMsg->numTuner].lockkey == 0)
			strcpy(value, TUNERX_LOCKKEY_VALUE);
		else
		{
			if (infoReqMsg->setMsg)
				_snprintf(value, 1024 - 2, "%lu", infTuner[infoReqMsg->numTuner].lockkey);
			else
				strncpy(value, m_sRemoteIPControl, 1024 - 2);
		}
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_DEBUG_MSG:
		_snprintf(name, 500 - 2, TUNERX_DEBUG, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;
		_snprintf(value, 1024 - 2, TUNERX_DEBUG_VALUE, infTuner[infoReqMsg->numTuner].canal, infTuner[infoReqMsg->numTuner].lock, infTuner[infoReqMsg->numTuner].ss, infTuner[infoReqMsg->numTuner].snq, infTuner[infoReqMsg->numTuner].seq, infTuner[infoReqMsg->numTuner].bps);
		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case TUNERX_CHANNEL_MSG:
		_snprintf(name, 500 - 2, TUNERX_CHANNEL, infoReqMsg->numTuner);
		name_len = (int)strlen(name) + 1;

		if (infTuner[infoReqMsg->numTuner].canal == 0)
			strcpy(value, NONE);
		else
			_snprintf(value, 1024 - 2, TUNERX_CHANNEL_VALUE, infTuner[infoReqMsg->numTuner].canal);

		value_len = (int)strlen(value) + 1;

		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_NAME);
		hdhomerun_pkt_write_var_length(*msg, name_len);
		hdhomerun_pkt_write_mem(*msg, (void *)name, name_len);
		hdhomerun_pkt_write_u8(*msg, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(*msg, value_len);
		hdhomerun_pkt_write_mem(*msg, (void *)value, value_len);

		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_GETSET_RPY);
		res = 1;
		break;
	case HDHR_UPGRADE_MSG:
	case HDHR_UPGRADE_FILE_MSG:
		hdhomerun_pkt_write_u32(*msg, infoReqMsg->seqUpgrade);
		hdhomerun_pkt_seal_frame(*msg, HDHOMERUN_TYPE_UPGRADE_RPY);
		res = 1;
		break;
	}

	if (name)
		delete name;
	if (value)
		delete value;
	
	return res;
}

void HDHRlibFacade::StopSocketServTCP_Control()
{
	shutdown(m_sockControl, SD_BOTH);
	shutdown(m_sockControlAccept, SD_BOTH);
	hdhomerun_sock_destroy(m_sockControl);
	hdhomerun_sock_destroy(m_sockControlAccept);
}
