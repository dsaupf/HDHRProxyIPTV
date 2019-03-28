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
#include "Transport.h"

DWORD WINAPI Listening__ThreadTransport(void* p)
{
	((CTransport*)p)->ListeningThreadTransport();
	return 0;
}

DWORD CTransport::ListeningThreadTransport()
{
	while (1)
	{
		if (getErrConnectionIni())
		{
			ReconnectHTTP();
		}
		else
		{
			if (getPerformSend() && !m_refreshFailedConnHTTP)
				TreatReceivedDataTS();
		}
	}
	return 1;
}

int CTransport::StartThreadTransport()
{
	//Creation of Thread for listen and send of Transport Stream
	hThreadTransport = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Listening__ThreadTransport, this, 0, 0);
	SetThreadPriority(hThreadTransport, THREAD_PRIORITY_HIGHEST);

	LOGM(TRZ4,0,"[Tuner %d] Create Transport thread %d\n", m_tuner,hThreadTransport);

	return 1;
}

void CTransport::StopThreadTransport()
{
	if (getState() == 1)
	{
		if (m_typeTransportInput == UDP_TS)
			StopTransportStreamUDP();
		else if (m_typeTransportInput == HTTP_TS)
			StopTransportStreamHTTP();
		setState(0);
	}

	Sleep(150); // wait for closing the open sockets!

	if (hThreadTransport != 0)
	{
		LOGM(TRZ3,0,"[Tuner %d] Calling to KILL a working thread!\n", m_tuner);
		Sleep(50); // wait for flush the LOG!
		TerminateThread(hThreadTransport, 1002);
		hThreadTransport = 0;
	}

	if (m_socketHTTP > 0)
		closesocket(m_socketHTTP);
	m_socketHTTP = -1;
	if (m_socketUDP > 0)
		closesocket(m_socketUDP);
	m_socketUDP = -1;
}

CTransport::CTransport()
{
	m_errConnectionIni = 0;
	receivingDataHTTP = 0;
	m_Traces = new CTrace();
	m_state = 0;  //0 : Uninitiated ; 1 : Initiated
	m_PerformSend = 0;

	m_applyExtPidFiltering = 0;

	m_typeTransportOutput = UDP_TS; //By default will be UDP protocol of output.
	m_typeTransportInput = 0;  //By default UDP, for initialize variable

	cfgProxy = CConfigProxy::GetInstance();

	strcpy(m_ipSend, "");
	m_portSend = 0;

	m_dataGETHTTP.Format(L"");

	hThreadTransport = 0;

	m_failedConnectHTTP = 0;
	m_refreshFailedConnHTTP = 0;

	m_basicRingBuffer = new CRingBufferTS_Basic();

}

CTransport::~CTransport()
{
	this->StopThreadTransport();

	delete m_Traces;
	m_Traces = NULL;

	if (m_basicRingBuffer)
		delete m_basicRingBuffer;
}

int CTransport::InitilizeTransportStream(int channel, CString pidsToFilterList)
{
	int res = 1;

	m_PosChannelInMapList = cfgProxy->ObtainIndexChannel(channel);

	//If the channel is not in the List Mapping it can not start Transport Stream as there are no data

	if (m_PosChannelInMapList == -1)
	{
		LOGM(ERR,0,"[Tuner %d] Could be not initiate TRANSPORT phase. Channel not in MAPPING LIST.\n", m_tuner);
		return -1;
	}

	CString prot = cfgProxy->m_infoChannels[m_PosChannelInMapList].Protocol;
	
	if (!prot.Compare(L"UDP"))
	{
		m_typeTransportInput = UDP_TS;
		if (!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L""))
		{
			LOGM(TRZ1,1,"[Tuner %d] Channel not tuned. Not defined UDPsource information in MappingList for channel %ld.\n", m_tuner, channel);
			return -2;
		}
	}
	else if (!prot.Compare(L"HTTP"))
	{
		m_typeTransportInput = HTTP_TS;
		if (!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L""))
		{
			LOGM(TRZ1,1,"[Tuner %d] Channel not tuned. Not defined URLGet information in MappingList for channel %ld.\n", m_tuner, channel);
			return -2;
		}
	}
	else
	{
		LOGM(TRZ1,1,"[Tuner %d] Channel not tuned. Not defined Protocol information (HTTP/UDP) in MappingList for channel %ld.\n", m_tuner, channel);
		return -2;
	}
	
	if (((!prot.Compare(L"HTTP")) &&
		((!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L"none"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L"NONE"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L""))))
		||
		((!prot.Compare(L"UDP")) &&
		((!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L"none"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L"NONE"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L""))))
		)
	{
		LOGM(TRZ1,1,"[Tuner %d] Channel not tuned. Not defined all Protocol information in MappingList for channel %ld.\n", m_tuner, channel);
		return 0;
	}

	if (m_typeTransportInput == UDP_TS)
	{
		if (InitializeTransportStreamUDP())
		{
			m_PerformSend = 1;

			LOGM(TRZ3,0,"[Tuner %d] HDHR server ready to receive Transport Stream Packages over UDP\n", m_tuner);
			LOGM(TRZ2,1,"[Tuner %d] Listening by [UDP] %s:%d. Sending to [UDP] %s:%d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoUDP, m_ipSend, m_portSend);
		}
		else
		{
			LOGM(ERR,0,"[Tuner %d] Could be not initiate TRANSPORT phase (Protocol UDP)\n", m_tuner);
			LOGM(ERR,1,"[Tuner %d] ERR: [UDP] In: %s:%d ; Out: %s:%d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoUDP, m_ipSend, m_portSend);
			return 0;
		}
	}
	else if (m_typeTransportInput == HTTP_TS)
	{
		if (InitilizeTransportStreamHTTP())
		{
			m_basicRingBuffer->Initialize(pidsToFilterList);
			m_basicRingBuffer->setTuner(m_tuner);

			m_PerformSend = 1;

			LOGM(TRZ3,0,"[Tuner %d] HDHR server ready to receive Transport Stream Packages over HTTP\n", m_tuner);
			LOGM(TRZ2,1,"[Tuner %d] Listening by [HTTP] http://%s:%d/%s. Sending to [UDP] %s:%d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, CStringA(m_dataGETHTTP), m_ipSend, m_portSend);
		}
		else
		{
			LOGM(TRZ1,0,"[Tuner %d] Could be not initiate TRANSPORT phase (Protocol HTTP)\n", m_tuner);
			LOGM(ERR,0,"[Tuner %d] ERR: [HTTP] In: http://%s:%d/%s ; Out: %s:%d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].datosGETHTTP), m_ipSend, m_portSend);
			m_errConnectionIni = 1;
			res = 0;
		}
	}

	if (hThreadTransport == 0)
	{
		StartThreadTransport();
	}

	setState(1);

	return res;
}

int CTransport::InitializeTransportStreamUDP()
{
	int res;
	WSADATA WsaDat;

	//Initilize the use of WinSock
	res = WSAStartup(MAKEWORD(2, 2), &WsaDat);
	if (res)	//If err is 0, ko
	{
		LOGM(ERR,0,"[Tuner %d] Error initializing use of WinSock.\n", m_tuner);
		return 0;
	}

	//Initilize Socket UDP
	m_socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (m_socketUDP == INVALID_SOCKET)
	{
		LOGM(ERR,0,"[Tuner %d] Error in creation of UDP Socket\n", m_tuner);
		return 0;
	}

	int bufsize = (2000000 / 1316) * 1316;
	setsockopt(m_socketUDP, SOL_SOCKET, SO_SNDBUF, (char*)&bufsize, sizeof(bufsize));

	//If the protocol of input is HTTP, only it needs to create the UDP socket for to forward
	//by him the TS received for HTTP, no need to perform the following steps of this function
	if (m_typeTransportInput == HTTP_TS)
		return 1;

	//Especific of server Socket

	//Information of the socket
	SOCKADDR_IN servInf;
	servInf.sin_family = AF_INET;
	if (!strcmp(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), "ANY") ||
		!strcmp(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), "127.0.0.1"))
		servInf.sin_addr.s_addr = htonl(INADDR_ANY);
	else
	{
		servInf.sin_addr.s_addr = inet_addr(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP));
	}

	servInf.sin_port = htons(cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoUDP);  //For the server socket it is the port of the server (from where listen)

	//Associate a local direction to the socket
	res = bind(m_socketUDP, (SOCKADDR*)(&servInf), sizeof(servInf));
	if (res)
	{
		res = WSAGetLastError();
		closesocket(m_socketUDP);
		m_socketUDP = -1;

		LOGM(ERR,0,"[Tuner %d] Has not been able to associate (bind) socket direction. Error %d\n", m_tuner, res);

		return 0;
	}

	return 1;
}

void CTransport::TreatReceivedDataTS()
{
	int attemptsReconn = 0;
	int reconnect = 0;
	int err = 0;

	if (m_typeTransportInput == UDP_TS)
		TreatReceivedDataUDP();
	else if (m_typeTransportInput == HTTP_TS)
	{
		if (TreatReceivedDataHTTP() == -1) //Close
		{
			LOGM(TRZ3,1,"[Tuner %d] Try to reconnect connection HTTP.\n", m_tuner);

			while (attemptsReconn < 50 && !reconnect)
			{
				SendNullPackets();
				reconnect = ReconnectConnectionHTTP();
				attemptsReconn++;
			}

			if (!reconnect)
			{
				if (getState() != 0)
				{
					LOGM(TRZ2,1,"[Tuner %d] Not possible to reconnect connection HTTP.\n", m_tuner);
				}
				m_failedConnectHTTP = 1;
				m_refreshFailedConnHTTP = 1;
				setPerformSend(0, m_tuner);
			}
			else
			{
				if (getState() != 0)
				{
					LOGM(TRZ2,1,"[Tuner %d] Connection HTTP is reconnected.\n", m_tuner);
				}
			}
		}
	}
}

int CTransport::TreatReceivedDataUDP()
{
	if (!m_PerformSend)
		return 1;

	int res;
	char *RecvBuf = new char[1316];
	int BufLen = 1316;
	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	//Returns the number of bytes received, if not is error
	res = recvfrom(m_socketUDP, RecvBuf, BufLen, 0, (SOCKADDR *)& SenderAddr, &SenderAddrSize);
	if (res == SOCKET_ERROR) {
			if (WSAGetLastError() != 10054)
			{
				LOGM(ERR,0,"[Tuner %d] Error %d at UDP data reception\n", m_tuner, WSAGetLastError());
				delete[] RecvBuf;
				return 0;
			}
	}
	else
	{
		//Send Datagram:
		sockaddr_in RecvAddr;
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(m_portSend);
		RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);

		int ntry = 0;
		while (ntry<5)
		{
			res = sendto(m_socketUDP,
				RecvBuf,
				BufLen,
				0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
			if (res == SOCKET_ERROR)
			{
				Sleep(50);
				ntry++;
			}
			else break;
		}
		if (ntry>=5) {
			LOGM(ERR,0,"[Tuner %d] Error=%d sending UDP data.\n", m_tuner, WSAGetLastError());
			delete[] RecvBuf;
			return 0;
		}
	}

	return 1;
}

void CTransport::StopTransportStreamUDP()
{
	m_PerformSend = 0;

	if (getState() == 1)
	{
		shutdown(m_socketUDP, SD_BOTH);
		//closesocket(m_socketUDP);
		//m_socketUDP = -1;
	}
}

void CTransport::AssignDataSend(char* target)
{
	int infoProt = 0;
	CString aux(target);
	CString ip;
	CString port;

	int index = aux.Find(L":", 0);
	ip = aux.Left(index);
	ip.MakeUpper();

	if (!ip.Compare(L"UDP"))
	{
		m_typeTransportOutput = UDP_TS;
		infoProt = 1;
	}
	else if (!ip.Compare(L"RTP"))
	{
		m_typeTransportOutput = RTP_TS;
		infoProt = 1;
	}
	
	if (infoProt)
	{
		//format: udp_o_rtp://ip:port
		index = index + 3;
		int index2 = aux.Find(L":", index + 1);
		ip = aux.Mid(index, index2 - index);
		port = aux.Mid(index2 + 1, aux.GetLength()- index2);
	}
	else
	{
		//format: ip:port (TSReader)
		port = aux.Right(aux.GetLength() - index-1);
		m_typeTransportOutput = UDP_TS;  //If  the transport protocol is not informed, it will be by default UDP
	}

	CStringA ip2(ip);
	CStringA port2(port);

	strcpy(m_ipSend, ip2);
	m_portSend = atoi(port2);
	
	if (m_typeTransportOutput == RTP_TS)
	{
		LOGM(TRZ3,1,"[Tuner %d] Sending to [RTP] %s:%d\n", m_tuner, m_ipSend, m_portSend);
	}
	else
	{
		LOGM(TRZ3,1,"[Tuner %d] Sending to [UDP] %s:%d\n", m_tuner, m_ipSend, m_portSend);
	}
}

int CTransport::InitilizeTransportStreamHTTP()
{
	WSADATA wsa;

	m_errConnectionIni = 0;
	m_failedConnectHTTP = 0;
	m_refreshFailedConnHTTP = 0;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LOGM(ERR,0,"[Tuner %d] Error initializing use of WinSock [HTTP].\n", m_tuner);
		return 0;
	}

	//Initilize UDP Socket to resend by its the received TS from HTTP
	if (!InitializeTransportStreamUDP())
		return 0;

	//Initilize Socket for HTTP. It will be a TCP socket
	
	if ((m_socketHTTP = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		LOGM(ERR,0,"[Tuner %d] Error in creation of HTTP(TCP) Socket\n", m_tuner);
		return 0;
	}

	DWORD timeout = BLOCKING_WAIT_TIME;  // Blocking (waiting) time.
	setsockopt(m_socketHTTP, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(DWORD));
	int bufsize = (2000000 / 1316) * 1316;
	setsockopt(m_socketHTTP, SOL_SOCKET, SO_RCVBUF, (char*)&bufsize, sizeof(bufsize));

	struct sockaddr_in addr;
	addr.sin_addr.s_addr = inet_addr(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP);

	//Connect to remote server
	if (connect(m_socketHTTP, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		int err = WSAGetLastError();
		LOGM(ERR,0,"[Tuner %d] Can not connect to HTTP://%s:%d. Error code %d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, err);
		shutdown(m_socketHTTP, SD_BOTH);
		closesocket(m_socketHTTP);
		m_socketHTTP = -1;
		return 0;
	}

	return 1;
}

int CTransport::SendGetHTTPRequest()
{
	char *msg = new char[5200];
	memset(msg, 0, 5200);
	_snprintf(msg, 5200 - 2, "GET /%s HTTP/1.1\r\n\r\n", CStringA(m_dataGETHTTP));

	LOGM(TRZ3,1,"[Tuner %d] Resending Request HTTP message(GET): \"GET /%s HTTP/1.1\"\n", m_tuner, CStringA(m_dataGETHTTP));

	if (send(m_socketHTTP, msg, strlen(msg), 0) < 0)
	{
		return 0;
	}
	delete[]msg;

	return 1;
}

int CTransport::TreatReceivedDataHTTP()
{
	char msg[5000];
	memset(msg, 0, 5000);
	char nullPaddingPckt[MAX_SIZE_DATAGRAM_TO_SEND];
	char dataR[MAX_SIZE_DATAGRAM_TO_SEND*25];
	int recv_size = 1316;
	int isBody = 0;
	int tamSend = 0;
	int isSync = 0;
	int maxSync = 0;
	int notstream = 0;
	int res = 0;
	int err = 0;

	sockaddr_in RecvAddr;
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_portSend);
	RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);

	// Prepare the nullPaddingPckt only one time!
	m_basicRingBuffer->GenerateNullPaddingTSPackets(nullPaddingPckt, NUM_PACKETS_TO_SEND);

	_snprintf(msg, sizeof(msg) - 2, "GET /%s HTTP/1.1\r\n\r\n", CStringA(m_dataGETHTTP));
	
	LOGM(TRZ3,1,"[Tuner %d] Sending Request HTTP message(GET): \"GET /%s HTTP/1.1\"\n", m_tuner, CStringA(m_dataGETHTTP));

	if (send(m_socketHTTP, msg, strlen(msg), 0) < 0)
	{
		err = WSAGetLastError();
		if (err == WSAECONNRESET || err == 0) //The connection is closed
		{
			shutdown(m_socketHTTP, SD_BOTH);
			closesocket(m_socketHTTP);
			m_socketHTTP = -1;
			return -1;
		}
		return 0;
	}

	//Send an initial UDP datagram (7 TS packets with null padding). It's like say OK to client.
	int ntry = 0;
	while (ntry<5)
	{
		res = sendto(m_socketUDP,
			nullPaddingPckt,
			MAX_SIZE_DATAGRAM_TO_SEND,
			0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
		if (res == SOCKET_ERROR)
		{
			Sleep(50);
			ntry++;
		}
		else break;
	}
	if (ntry>=5)
	{
		LOGM(ERR,0,"[Tuner %d] Error=%d sending initial packet to UDP with padding %s:%d\n", m_tuner, WSAGetLastError(), m_ipSend, m_portSend);

		return 0;
	}
	
	LOGM(TRZ4,1,"[Tuner %d] Send an initial UDP datagram with %d null padding packets to client.\n", m_tuner, NUM_PACKETS_TO_SEND);

	// Begin to receive and send TS packets
	// Initialize the buffer
	readBufferPos = 0;
	memset(readBuffer, 0, readBufferSize);
	int notValidTS = 0 ;  // Indicates if data in Read Buffer is invalid
	SYSTEMTIME lastPrintUpdate;
	GetLocalTime(&lastPrintUpdate);

	while (getPerformSend())
	{

		// Part I: Read data from TCP socket (HTTP stream)
		if (!isBody)
		{
			recv_size = readBufferSize - readBufferPos;
			notValidTS = 0;
		}
		else
		{
			// Check if data pending on buffer... for reading the end of the incomplete packet!
			if (readBufferPos - readBufferMinPos> 0)
			{
				// If data plus lookahead is waiting in the buffer then SYNC is lost!
				notValidTS = 1;
				recv_size = ( ((readBufferPos / MAX_SIZE_DATAGRAM_TO_SEND)+1) * MAX_SIZE_DATAGRAM_TO_SEND ) - readBufferPos;
			}
			else
			{
				notValidTS = 0;  // Assume by default that new data is good
				recv_size = MAX_SIZE_DATAGRAM_TO_SEND;
			}

			if (recv_size > (readBufferSize - readBufferPos))
				recv_size = readBufferSize - readBufferPos;
		}

		LOGM(TRZ6,1,"[Tuner %d] Next recv_size:           %5d bytes.                             (Read Buffer size: %05d).\n", m_tuner, recv_size, readBufferPos);

		int ok_read = -1;
		int r_offset = 0;
		int r_size = 0;
		int ret = 0;
		while (ok_read < 1)
		{
			if (!getPerformSend()) return -1;
			if ((ret = recv(m_socketHTTP, &readBuffer[readBufferPos + r_size], r_offset > 0 ? 188 - r_offset : recv_size, r_offset > 0 ? MSG_WAITALL : 0)) < 0)  // Blocking read!
			{
				err = WSAGetLastError();
				if (((err == WSAEINTR) || (err == WSAETIMEDOUT)) && (ok_read < 0))
				{
					// Timeout expire!
					LOGM(TRZ6,1,"[Tuner %d] Failed read in Stream Socket: %d,%d. Try to continue.\n", m_tuner, r_size, err);
					ok_read++;
					//shutdown(m_socketHTTP, SD_BOTH);
					//closesocket(m_socketHTTP);
					//m_socketHTTP = -1;
					//return -1;
				}
				else
				{
					// Another Error in socket!
					LOGM(TRZ4,1,"[Tuner %d] Error in Stream Socket: %d:%d. Closing connection.\n", m_tuner, r_size, err);
					shutdown(m_socketHTTP, SD_BOTH);
					closesocket(m_socketHTTP);
					m_socketHTTP = -1;
					return -1;
				}
			}
			r_size += ret;
			int total_r = r_size + readBufferPos;

			if (!isBody || r_size == 0)
			{
				ok_read = 1;
			}
			else if (total_r % 188 == 0)
			{
				ok_read = 1;
			}
			else
			{
				r_offset = total_r % 188;
				LOGM(TRZ4,1,"[Tuner %d] Partial packet read in Socket retry to read for %5d bytes pending.\n", m_tuner, 188 - r_offset);
			}
	        }
		recv_size = r_size;
		if (r_offset > 0)
		{
			LOGM(TRZ4,1,"[Tuner %d] Continue after Partial reading from Socket, new %5d bytes readed (Read Buffer size: %05d).\n", m_tuner, recv_size, readBufferPos + recv_size);
		}

/*
		int kk;
		for (kk=0; (kk * 188) < recv_size ; kk++)
		{
			const unsigned char* ppp = (unsigned char*)&readBuffer[readBufferPos + (kk * 188)];
			if (ppp[1] == '\x00' && ppp[2] == '\xa3')
			{
				const int cc = ppp[3] & 15;
				//if (kkp < 0)
				//    kkp=cc-1;
				kkp++;
				if (kkp > 15) kkp=0;
				if (kkp != cc)
				{
					LOGM(TRZ3,0,"[Tuner %d] CC Error! pre=%d curr=%d (kk=%d) recv=%d\n", m_tuner, kkp, cc, kk, recv_size);
//					m_basicRingBuffer->m_lockaheads++;
				}
				kkp = cc;
			}
		}
*/

		// Update GUI counters
		SYSTEMTIME curr;
		GetLocalTime(&curr);
		_int64 nanoSecs = m_basicRingBuffer->CompareSystemTime(lastPrintUpdate, curr);
		if (nanoSecs > 1000000000)  // 1sec.
		{
			cfgProxy->UpdateClientTOInterface(m_tuner);
			GetLocalTime(&lastPrintUpdate);
		}

		if ( recv_size == 0 )  // Timeout of read call
		{
			LOGM(TRZ4,0,"[Tuner %d] Read from Stream Socket timeout!\n", m_tuner);
			receivingDataHTTP = 0;
			notstream++;

			//Send out UDP NULL datagram (7 TS packets with null padding). It's like say ALIVE to client.
			int ntry = 0;
			while (ntry<5)
			{
				if (!getPerformSend()) return -1;
				res = sendto(m_socketUDP,
					nullPaddingPckt,
					MAX_SIZE_DATAGRAM_TO_SEND,
					0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
				if (res == SOCKET_ERROR)
				{
					Sleep(50);
					ntry++;
				}
				else break;
			}
			if (ntry>=5)
			{
				LOGM(ERR,0,"[Tuner %d] Error=%d sending alive NULL packet to UDP with padding %s:%d\n", m_tuner, WSAGetLastError(), m_ipSend, m_portSend);

				return 0;
			}
			
			if ( notstream >= 20 )  // If not receiving the streaming for several time then exit!
			{
				LOGM(TRZ3,0,"[Tuner %d] Closing Stream Socket because several timeouts.\n", m_tuner);
				shutdown(m_socketHTTP, SD_BOTH);
				closesocket(m_socketHTTP);
				m_socketHTTP = -1;
				return -1;
			}

			continue;
		}
		LOGM(TRZ5,1,"[Tuner %d] Received from TCP Socket: %5d bytes.                             (Read Buffer size: %05d).\n", m_tuner, recv_size, readBufferPos + recv_size);

		// Incoming data from socket, update internal data
		receivingDataHTTP = 1;
		notstream=0;
		readBufferPos += recv_size;
		if ( readBufferPos > readBufferSize )
		{
			LOGM(TRZ2,0,"[Tuner %d] ERROR Read Buffer overflow! (empty half of the buffer).\n", m_tuner);
			readBufferPos = (readBufferSize / 2) - 1;
			memmove(&readBuffer[0], &readBuffer[readBufferPos], readBufferPos);
			memset(&readBuffer[readBufferPos], 0, readBufferSize - readBufferPos);
			
			continue;
		}
		
		if ( !isBody )
		{
			// Process the HTTP Header response...
			int HTTPHeaderEnd = -1;
			
			// Search for 'HTTP/1.*' response
			if (strstr(readBuffer, "HTTP/1.0 ") != NULL || strstr(readBuffer, "HTTP/1.1 ") != NULL)
			{
				// OK: Other end of the socket speaks HTTP.
				HTTPHeaderEnd = 0;
			}

			// Search for '\r\n\r\n' OR Binary data OR EOF
			// FIXME: '\r\n\r\n' not implemented, if body is text this code fails!
			if ( HTTPHeaderEnd > -1 )
			{
				int i;
				for ( i=0; i<readBufferPos; i++ )
				{
					if (!isprint(readBuffer[i]))
					{
						if ((readBuffer[i] != '\n') && (readBuffer[i] != '\r'))
							break;
					}
				}
				HTTPHeaderEnd = i;
			}
			
			if ( HTTPHeaderEnd > 0 )
			{
				LOGM(TRZ4,1,"[Tuner %d] Header of HTTP response received, size: %d bytes.\n", m_tuner, HTTPHeaderEnd);

				LOGM(TRZ6,1,"[Tuner %d] Complete header of HTTP response >>> \n%s\n<<< (EOF HTTP response)\n", m_tuner, readBuffer);
			}
			
			if ( HTTPHeaderEnd < 0 ) // Non Valid HTTP Header found, then closing connection
			{
				LOGM(TRZ3,1,"[Tuner %d] Closing Stream Socket because non valid HTTP response.\n", m_tuner);

				LOGM(TRZ6,1,"[Tuner %d] Complete readed data from TCP socket >>> \n%s\n<<< (EOF TCP data)\n", m_tuner, readBuffer);

				shutdown(m_socketHTTP, SD_BOTH);
				closesocket(m_socketHTTP);
				m_socketHTTP = -1;
				return -1;
			}
			else
			{
				memmove(&readBuffer[0], &readBuffer[HTTPHeaderEnd+1], readBufferPos-HTTPHeaderEnd);
				readBufferPos = readBufferPos-HTTPHeaderEnd;
				isBody = 1;
				LOGM(TRZ6,0,"[Tuner %d] Starting to receive the stream.\n", m_tuner);
				// Init internal timers
				m_basicRingBuffer->SaveTimeToSend();
				m_basicRingBuffer->InitHTTPMessage();
			}
		}
		else  // Is body
		{
			// Process the stream...
			
			// Check TS SYNC!
			// if notValidTS=1 then previous waiting data in the buffer.
			int newSyncPos = 0;      // After resync, here will be the possible sync mark.

			// FAST check for packets of 188bytes with correct header
			if ( readBufferPos < 188 ) notValidTS=2;
			else if ( (readBufferPos % 188) != 0 )  notValidTS = 3;
			else
			{
				int i;
				for (i = 0; i < (readBufferPos / 188); i++)
				{
					if (readBuffer[188 * i] != 'G') notValidTS = 4;
				}
			}
			
			if ( notValidTS > 0 )
			{
				if      (notValidTS == 1) { LOGM(TRZ4,0,"[Tuner %d] Incoming data: garbage or need to Resynchronize (SYNC LOST) [ waiting data ].\n", m_tuner); }
				else if (notValidTS == 2) { LOGM(TRZ4,0,"[Tuner %d] Incoming data: garbage or need to Resynchronize (SYNC LOST) [  length<188  ].\n", m_tuner); }
				else if (notValidTS == 3) { LOGM(TRZ4,0,"[Tuner %d] Incoming data: garbage or need to Resynchronize (SYNC LOST) [ !length%188  ].\n", m_tuner); }
				else                      { LOGM(TRZ3,0,"[Tuner %d] Incoming data: garbage or need to Resynchronize (SYNC LOST) [fail sync mark].\n", m_tuner); }

				LOGM(TRZ4,1,"[Tuner %d] re-SYNC: Starting to search >>>                                    (Read Buffer size: %05d).\n", m_tuner, readBufferPos);
				notValidTS = 1;

				// Search for ANY valid packet in the Buffer
				int i;
				for (i = 0; i < readBufferPos; i++)
				{
					if (readBuffer[i] == 'G')
					{
						if ((i + 188 + 188) >= readBufferPos)
						{
							// Insuficient data for resync
							notValidTS = 1;
							newSyncPos = i;
							LOGM(TRZ4,1,"[Tuner %d] re-SYNC: Low data, SYNC possible @%05d                            (Read Buffer size: %05d).\n", m_tuner, i, readBufferPos);
							break;
						}
						else
						{
							if ((readBuffer[i + 188] == 'G') && (readBuffer[i + 188 + 188] == 'G'))
							{
								// SYNC OK
								notValidTS = 0;
								newSyncPos = i;
								LOGM(TRZ4,1,"[Tuner %d] re-SYNC: Inconming data OK at %5d                                (Read Buffer size: %05d).\n", m_tuner, i, readBufferPos);
								break;
							}
							// Advance to next position! 
						}
					}
					// Advance to next position!
				}

				if ((newSyncPos >= readBufferPos) && (readBufferPos > 0))
				{
					// Not found any SYNC mark! Discard all data!
					LOGM(TRZ4,1,"[Tuner %d] re-SYNC: Not found SYNC on buffer, erasing %5d bytes of data.\n", m_tuner, readBufferPos);
					readBufferPos = 0;
					memset(&readBuffer[0], 0, readBufferSize);
				}
				else
				{
					// Remove ancient data positioning at new SYNC mark!
					memmove(&readBuffer[0], &readBuffer[newSyncPos], readBufferPos - newSyncPos);
					readBufferPos = readBufferPos - newSyncPos;
					LOGM(TRZ4,1,"[Tuner %d] re-SYNC: Removing %5d bytes at start                             (Read Buffer size: %05d).\n", m_tuner, newSyncPos, readBufferPos);
					memset(&readBuffer[readBufferPos], 0, readBufferSize - readBufferPos);
				}

				if (!notValidTS)  // SYNC Mark found (2 valid packets minimum): Check if all packets are valid, or parts of them at the end.
				{
					int complet = readBufferPos / 188;
					int incomplet = readBufferPos % 188;
					int j;
					
					for (j = 0; j < complet; j++)
					{
						if (readBuffer[188*j] != 'G')
						{
							// Remaining data not valid
							notValidTS = 1;
							break;
						}
					}

					// Evaluate results...
					if ((j == complet) && (notValidTS == 0)) // All OK!
					{
						newSyncPos = 0;
						LOGM(TRZ4,0,"[Tuner %d] re-SYNC: >>> Completed ALL OK!\n", m_tuner);

						if ((incomplet != 0) && (readBuffer[188 * j] == 'G'))
						{
							// Save remaining data
							newSyncPos = incomplet;
							incomplet = 0;
						}
					}
					if ((j < complet) || (incomplet > 0))
					{
						//  Garbage data! Remove it.
						newSyncPos = 0;
						int last_good = 188 * j;
						LOGM(TRZ4,1,"[Tuner %d] re-SYNC: Deleting %5d bytes, from %5d to %5d                 (Read Buffer size: %05d).\n", m_tuner, readBufferPos - last_good, last_good, readBufferPos, last_good );
						readBufferPos = last_good;
						memset(&readBuffer[readBufferPos], 0, readBufferSize - readBufferPos);
					}
					// End of checks when resync!
					notValidTS = 0; // Whatever is in the buffer is now valid.
				}
				// End of checks when detected is not valid!
			}

			if (notValidTS)
			{
				// Not found sync!
				maxSync++;
				LOGM(TRZ5,0,"[Tuner %d] re-SYNC: Not found mark, retrying... \n", m_tuner);

				// Send new UDP datagram with null padding, as keepalive to client.
				int ntry = 0;
				while (ntry<5)
				{
					if (!getPerformSend()) return -1;
					res = sendto(m_socketUDP,
						nullPaddingPckt,
						MAX_SIZE_DATAGRAM_TO_SEND,
						0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
					if (res == SOCKET_ERROR)
					{
						Sleep(50);
						ntry++;
					}
					else break;
				}
				if (ntry>=5)
				{
					LOGM(ERR,0,"[Tuner %d] Error=%d sending initial packet to UDP with padding %s:%d\n", m_tuner, WSAGetLastError(), m_ipSend, m_portSend);

					return 0;
				}

				if (maxSync > 10)
				{
					// Closed socket, excesive errors!
					LOGM(TRZ3,0,"[Tuner %d] re-SYNC: Closing Stream Socket because impossible to sync.\n", m_tuner);
					shutdown(m_socketHTTP, SD_BOTH);
					closesocket(m_socketHTTP);
					m_socketHTTP = -1;
					return -1;
				}
			}
			else
			{
				maxSync = 0; // reset counter!

				// Save data in the Ring Buffer

				int partial = readBufferPos % 188;
				int endPos = 0;

				if (readBufferPos <= readBufferMinPos)
				{
					// Do nothing, only "save" data in the lookahead part of the read buffer!
					endPos = 0;
					LOGM(TRZ6,0,"[Tuner %d] Filling lookahead part of read buffer.\n", m_tuner);
				}
				else if (partial == 0)
				{
					// Normal read, push new data in the Ring Buffer, except lookahead space.
					endPos = readBufferPos - readBufferMinPos;
				}
				else
				{
					// Read with errors, try to use the lookahead buffer.
					endPos = readBufferMinPos;
					m_basicRingBuffer->m_lockaheads++;
					LOGM(TRZ5,0,"[Tuner %d] Using the lookahead buffer!\n", m_tuner);
				}

				if ((endPos > 0) && (endPos <= readBufferPos))
				{
					// Flush read buffer and push it in Ring Buffer.
					if (endPos > readBufferMinPos)
						endPos = endPos - partial; // Insert only 188*Nbytes in RingBuffer.
					if (m_basicRingBuffer->Insert(&readBuffer[0], endPos) > 0)
					{
						// Insert OK
						LOGM(TRZ5,1,"[Tuner %d] Put in Ring Buffer:       %5d bytes.                             (Read Buffer size: %05d).\n", m_tuner, endPos, readBufferPos - endPos);
						memmove(&readBuffer[0], &readBuffer[endPos], readBufferPos - endPos);
						readBufferPos = readBufferPos - endPos;
					}
					else
					{
						// Ring_Buffer full, NO insert
						LOGM(TRZ5,0,"[Tuner %d] Ring Buffer full, insert data call fails!\n", m_tuner);
					}
				}
			}
		}

		// Part II: Send data over UDP socket

		if (1)
		//if (m_basicRingBuffer->CheckTimeToSend())
		{
			//m_basicRingBuffer->SaveTimeToSend();

			//tamSend = m_basicRingBuffer->GetMultipleTSPacket(dataR, 7*2);  // Write at double speed (*2)
			int packets_to_read;
			packets_to_read = 7 * CHUNK_SIZE;  // Process only a chunk of packets in each loop! (dataR size 7*25)
			unsigned int numPadding[1] = { 0 };
			tamSend = m_basicRingBuffer->GetMultipleTSPacket(dataR, packets_to_read, numPadding);

			//if (strlen(dataR) && tamSend)  // if data to send...
			if (tamSend)  // if data to send...
			{
				int offset = 0;
				while (tamSend > 1)
				{
					// Send in blocks of 1316 or less!
					int udp_send = 1316;
					if (tamSend <= udp_send) udp_send = tamSend;
					tamSend -= udp_send;

					if (udp_send < 1316)
					{
						LOGM(TRZ3,0,"[Tuner %d] PROBLEM  :  Sent to client less than 7 packets!!!\n", m_tuner);
					}

					//This terrible hack tries to overcome the problem when the IP stack reorders/swap UDP packets
					FILE* nulfile = fopen("NUL:", "w"); fprintf(nulfile," "); fclose(nulfile);

					//Send UDP Datagram:
					int ntry = 0;
					while (ntry<5)
					{
						if (!getPerformSend()) return -1;
						res = sendto(m_socketUDP,
							dataR + offset,
							udp_send,
							0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
						if (res == SOCKET_ERROR)
						{
							LOGM(TRZ3,0,"[Tuner %d] Write socket re-try.\n", m_tuner);
							Sleep(25);
							ntry++;
						}
						else break;
					}
					if (ntry>=5)
					{
						LOGM(ERR,0,"[Tuner %d] Sent to [UDP://%s:%d] : Error=%d sending packet! \n", m_tuner, m_ipSend, m_portSend, WSAGetLastError());

						return 0;
					}

					//The hack triggers the file(socket) event loop two times: before and after send each UDP packet!
					nulfile = fopen("NUL:", "w"); fprintf(nulfile," "); fclose(nulfile);

					LOGM(TRZ5,1,"[Tuner %d] Sent to [UDP://%s:%d] : %5d bytes (waiting:%05d).\n", m_tuner, m_ipSend, m_portSend, udp_send, tamSend);
					offset += udp_send;

					if (numPadding[0] > 0)
					{
						// Slow down a bit!
						//Sleep(15);
						m_basicRingBuffer->m_lockaheads++;
						LOGM(TRZ4,0,"[Tuner %d] Slow down a bit.\n", m_tuner);
					}
				}
			}
		}
		
		// Continue the loop
	}

	return 1;
}

void CTransport::StopTransportStreamHTTP()
{
	m_PerformSend = 0;

	if (getState() == 1)
	{
		m_basicRingBuffer->SemaphoreSignal();

		LOGM(TRZ5,0,"[Tuner %d] Closing HTTP socket.\n", m_tuner);

		if (m_socketHTTP > 0)
		{
			shutdown(m_socketHTTP, SD_BOTH);
			//closesocket(m_socketHTTP);
			//m_socketHTTP = -1;
		}

		if (m_socketUDP > 0)
		{
			shutdown(m_socketUDP, SD_BOTH);
			//closesocket(m_socketUDP);
			//m_socketUDP = -1;
		}

		LOGM(TRZ6,0,"[Tuner %d] End Closing HTTP socket.\n", m_tuner);
	}
}

int CTransport::ChangeSourceTS(int channel, CString pidsToFilterList)
{
	int res;

	LOGM(TRZ3,0,"[Tuner %d] Closing open Stream Sockets because change of Source.\n", m_tuner);
	if (m_typeTransportInput == UDP_TS)
	{
		StopTransportStreamUDP();
	}
	else if (m_typeTransportInput == HTTP_TS)
	{
		StopTransportStreamHTTP();
	}

	res = InitilizeTransportStream(channel, pidsToFilterList);

	if (res <= 0)
	{
		if (res == -1)
		{
			LOGM(TRZ1,0,"[Tuner %d] Could be not initiate TRANSPORT phase. Channel not in MAPPING LIST.\n", m_tuner);
			return -1;
		}
		else
		{
			LOGM(TRZ1,0,"[Tuner %d] Could be not initiate TRANSPORT phase\n", m_tuner);
			if (res == -2)
				return-2;
			else
				return 0;
		}
	}

	return 1;
}

void CTransport::setPerformSend(int send, int tuner)
{
	int sending = m_PerformSend;
	
	m_PerformSend = send;

	if (sending == 1 && send == 0)
	{
		receivingDataHTTP = 0;

		//The sockets are closed
		if (m_typeTransportInput == UDP_TS)
		{
			StopTransportStreamUDP();
		}
		else if (m_typeTransportInput == HTTP_TS)
		{
			StopTransportStreamHTTP();
		}

		StopThreadTransport();

		LOGM(TRZ2,1,"[Tuner %d] Stop sending TS Packets to Target.\n", m_tuner); // stop restreaming!
	}
};

void CTransport::ChangeFilterPIDsList(CString pidsToFilterList, int internalPFiltering, int externalPFiltering, int channel)
{
	int index, index2;

	m_basicRingBuffer->GetSetPIFFilteringData(0, pidsToFilterList, internalPFiltering);
	setApplyExtPidFiltering(externalPFiltering);

	m_PosChannelInMapList = cfgProxy->ObtainIndexChannel(channel);

	if (m_PosChannelInMapList != -1)
	{
		//External PID Filtering

		m_dataGETHTTP = cfgProxy->m_infoChannels[m_PosChannelInMapList].datosGETHTTP;

		cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet;

		if (externalPFiltering)
		{
			CString tmp;
			index = m_dataGETHTTP.Find(L"&pids=", 0);
			if (index != -1)
			{
				index2 = m_dataGETHTTP.Find(L"&", index+1);
				if (index2 == -1)
				{
					m_dataGETHTTP = m_dataGETHTTP.Left(index);
				}
				else
				{
					tmp = m_dataGETHTTP;
					m_dataGETHTTP = tmp.Left(index);
					m_dataGETHTTP.Append(tmp.Right(tmp.GetLength() - index2));
				}
			}

			index = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Find(L"&pids=", 0);
			if (index != -1)
			{
				index2 = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Find(L"&", index + 1);
				if (index2 == -1)
				{
					cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Left(index);
				}
				else
				{
					tmp = m_dataGETHTTP;
					cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt = tmp.Left(index);
					cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(tmp.Right(tmp.GetLength() - index2));
				}
			}


			if (!pidsToFilterList.Compare(L"")  || !pidsToFilterList.Compare(L"0x0000-0x1FFF"))
			{
				// FIXME: Hack for devices without pids=all support!
				//m_dataGETHTTP.Append(L"&pids=all");
				//cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(L"&pids=all");
				//m_dataGETHTTP.Append(L"&pids=0,1,3,16,17,18");
				//cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(L"&pids=0,1,3,16,17,18");
				CString replaceList;
				if (!cfgProxy->m_FullTSReplace.Compare(L"0x0000-0x1FFF"))
					replaceList = L"all";
				else
					replaceList = cfgProxy->getFullTSReplace();
				//m_dataGETHTTP.Append(L"&pids=" + cfgProxy->getFullTSReplace());
				m_dataGETHTTP.Append(L"&pids=" + replaceList);
				cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(L"&pids=" + replaceList);
			}
			else
			{
				m_dataGETHTTP.Append(L"&pids=");
				m_dataGETHTTP.Append(pidsToFilterList);
				cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(L"&pids=");
				cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(pidsToFilterList);
			}
		}
	}

}


int CTransport::ReconnectConnectionHTTP()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LOGM(ERR,0,"[Tuner %d] Error initializing use of WinSock [HTTP].\n", m_tuner);
		return 0;
	}

	//Initilize Socket for HTTP. It will be a TCP socket

	if ((m_socketHTTP = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		LOGM(ERR,0,"[Tuner %d] Error in creation of HTTP(TCP) Socket\n", m_tuner);
		return 0;
	}

	DWORD timeout = BLOCKING_WAIT_TIME;  // Blocking (waiting) time.
	setsockopt(m_socketHTTP, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(DWORD));
	int bufsize = (2000000 / 1316) * 1316;
	setsockopt(m_socketHTTP, SOL_SOCKET, SO_RCVBUF, (char*)&bufsize, sizeof(bufsize));

	struct sockaddr_in addr;
	addr.sin_addr.s_addr = inet_addr(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP);

	//ReConnect to remote server
	if (connect(m_socketHTTP, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		int err = WSAGetLastError();
		
		LOGM(ERR,1,"[Tuner %d] Can not reconnect to http://%s:%d. Error code %d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, err);
		return 0;
	}

	m_failedConnectHTTP = 0;
	m_refreshFailedConnHTTP = 0;
	return 1;
}


int CTransport::SendNullPackets()
{
	int res = 0;
	//Send an UDP datagram (7 TS packets with null padding).
	char* nullPaddingPckt = new char[MAX_SIZE_DATAGRAM_TO_SEND];
	strcpy(nullPaddingPckt, "");
	m_basicRingBuffer->GenerateNullPaddingTSPackets(nullPaddingPckt, NUM_PACKETS_TO_SEND);

	sockaddr_in RecvAddr;
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_portSend);
	RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);

	int ntry = 0;
	while (ntry<5)
	{
		res = sendto(m_socketUDP,
			nullPaddingPckt,
			MAX_SIZE_DATAGRAM_TO_SEND,
			0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
		if (res == SOCKET_ERROR)
		{
			Sleep(50);
			ntry++;
		}
		else break;
	}
	if (ntry>=5)
	{
		if (getState() != 0)
		{
			LOGM(ERR,0,"[Tuner %d] Error=%d sending null packet to UDP with padding %s:%d\n", m_tuner, WSAGetLastError(), m_ipSend, m_portSend);
		}

		return 0;
	}
	
	LOGM(TRZ4,1,"[Tuner %d] Send an UDP datagram with %d null padding packets to client.\n", m_tuner, NUM_PACKETS_TO_SEND);

	delete[]nullPaddingPckt;

}

void CTransport::ReconnectHTTP()
{
	int attemptsReconn = 0;
	int reconnect = 0;

	shutdown(m_socketHTTP, SD_BOTH);
	closesocket(m_socketHTTP);
	m_socketHTTP = -1;

	LOGM(TRZ3,1,"[Tuner %d] Try to reconnect connection HTTP.\n", m_tuner);

	while (attemptsReconn < 20 && !reconnect)
	{
		SendNullPackets();
		reconnect = ReconnectConnectionHTTP();
		attemptsReconn++;
	}

	if (!reconnect)
	{
		if (getState() != 0)
		{
			LOGM(TRZ2,1,"[Tuner %d] Not possible to reconnect connection HTTP.\n", m_tuner);
		}
		m_failedConnectHTTP = 1;
		m_refreshFailedConnHTTP = 1;
		setPerformSend(0, m_tuner);
	}
	else
	{
		if (getState() != 0)
		{
			LOGM(TRZ2,1,"[Tuner %d] Connection HTTP is reconnected.\n", m_tuner);
		}
	}
	m_errConnectionIni = 0;

}
