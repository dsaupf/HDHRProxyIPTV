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
#include "ConfigProxy.h"
#include "HDHRProxyIPTVDlg.h"

CConfigProxy* CConfigProxy::m_instance = NULL;

CConfigProxy::~CConfigProxy()
{
	delete m_instance;
	m_instance = NULL;
}

void CConfigProxy::ReadFileINI()
{
	CString section;

	//Obtain the current directory
	TCHAR sDirActual[200];
	GetCurrentDirectory(200, sDirActual);

	CString path;
	path.Format(L"%s\\%s", sDirActual, _T(NAME_FILE_INI));
	
	//The name of the INI file is updated. It is libeated the actual and is reserved for the new
	//m_pszProfileName contains the name of the file INI of the application
	AfxGetApp()->m_pszRegistryKey = NULL;
		//m_pszRegistryKey
	free((void*)AfxGetApp()->m_pszProfileName);
	AfxGetApp()->m_pszProfileName = _tcsdup(path);

    // HDHR_DEVICE Section 
	section = _T("HDHR_DEVICE");
	
	m_device_id = AfxGetApp()->GetProfileString(_T("HDHR_DEVICE"), _T("DEVICE_ID"));
	m_device_id.Replace(L"\"", L"");
	if (!m_device_id.Compare(L""))
		m_device_id.Format(_T(ID_DISP_SERV_HDHR));  //If there is no saved data, default value is set

	m_hdhrServerIP = AfxGetApp()->GetProfileString(section, L"SERVER_IP");
	m_hdhrServerIP.Replace(L"\"", L"");
	if (!m_hdhrServerIP.Compare(L""))
	{
		ObtainHostIP(&m_hdhrServerIP);
	}

	m_autostart = AfxGetApp()->GetProfileInt(section, L"AUTOSTART", 0);

	m_TunersNumber = AfxGetApp()->GetProfileInt(section, L"TUNERS_NUM", 2);
	m_TunersNumberToSaveINI = m_TunersNumber;
	if (m_TunersNumber == 0)
		m_TunersNumber = 2;

	if (m_TunersNumber > 9)
		m_TunersNumber = 9;

	m_Lock = AfxGetApp()->GetProfileInt(section, L"LOCK", 0);
	if (m_Lock != 0 && m_Lock != 1)
		m_Lock = 0;

	//CString t = AfxGetApp()->GetProfileString(section, L"MAX_TIME_SEND_DGRAM", _T(MAX_TIME_TO_SEND_S));
	//t.Replace(L"\"", L"");
	//m_maxTimeToSendDgram = atoll(CStringA(t));
	m_maxTimeToSendDgram = MAX_TIME_TO_SEND;
	m_maxTimeToPacket = m_maxTimeToSendDgram / 7;

	m_FullTSReplace = AfxGetApp()->GetProfileString(section, L"FULL_TS_REPLACE");
	m_FullTSReplace.Replace(L"\"", L"");
	if (!m_FullTSReplace.Compare(L""))
		m_FullTSReplace = L"0x0000-0x1FFF";


	// EMU_DEBUG Section
	section = L"EMU_DEBUG";
	CString level;
	level = AfxGetApp()->GetProfileString(section, L"LEVEL");
	level.Replace(L"\"", L"");
	if (!level.Compare(L""))
		level.Format(L"2");  //If there is no saved data , default value is set
	CStringA n(level);
	m_traceLevel = atoi(n);

	m_autoCleanLog = AfxGetApp()->GetProfileInt(section, L"AUTO_CLEAN_LOG", 0);
	if (m_autoCleanLog != 0 && m_autoCleanLog != 1)
		m_autoCleanLog = 0;


	// HDHR_SYS_CONFIG Section 
	section = _T("HDHR_SYS_CONFIG");

	version = AfxGetApp()->GetProfileString(section, _T("SYS_VERSION"), _T(VERSION_VALUE));
	version.Replace(L"\"", L"");
	sys_model = AfxGetApp()->GetProfileString(section, _T("SYS_MODEL"), _T(SYS_MODEL_VALUE));
	sys_model.Replace(L"\"", L"");
	sys_hwmodel = AfxGetApp()->GetProfileString(section, _T("SYS_HWMODEL"), _T(SYS_HWMODEL_VALUE));
	sys_hwmodel.Replace(L"\"", L"");
	sys_copyright = AfxGetApp()->GetProfileString(section, _T("SYS_COPYRIGHT"), _T(SYS_COPYRIGHT_VALUE));
	sys_copyright.Replace(L"\"", L"");
	sys_dvbc_modulation = AfxGetApp()->GetProfileString(section, _T("SYS_DVBC_MODULATION"), _T(SYS_DVBC_MODULATION_VALUE));
	sys_dvbc_modulation.Replace(L"\"", L"");
	sys_channelmap = AfxGetApp()->GetProfileString(section, _T("SYS_CHANNEL_MAP"), _T(SYS_CHANNEL_MAP_VALUE));
	sys_channelmap.Replace(L"\"", L"");
	lineup_location = AfxGetApp()->GetProfileString(section, _T("LINEUP_LOCATION"), _T(LINEUP_LOC_VALUE));
	lineup_location.Replace(L"\"", L"");
	sys_features_ini = AfxGetApp()->GetProfileString(section, _T("SYS_FEATURES"), _T(SYS_FEATURES_VALUE));
	sys_features_ini.Replace(L"\"", L"");
	sys_features = ParserHDHRResponseMultiLine(sys_features_ini);
	sys_debug_ini = AfxGetApp()->GetProfileString(section, _T("SYS_DEBUG"), _T(SYS_DEBUG_VALUE));
	sys_debug_ini.Replace(L"\"", L"");
	sys_debug = ParserHDHRResponseMultiLine(sys_debug_ini);
}

void CConfigProxy::ObtainHostIP(CString *ip)
{
	struct sockaddr_in host;
	char nombre[255];

	gethostname(nombre, 255);
	host.sin_addr = *(struct in_addr*) gethostbyname(nombre)->h_addr;
	*ip = inet_ntoa(host.sin_addr);
}


int CConfigProxy::SaveItems()
{
	int showMessageRestart = 0;
	CString seccion;

	//Obtain current directory
	TCHAR sDirActual[200];
	GetCurrentDirectory(200, sDirActual);

	CString path;
	path.Format(L"%s\\%s", sDirActual, _T(NAME_FILE_INI));

	//The name of the INI file is updated.It is liberated the current and is reserved for the new
	//m_pszProfileName contains the name of the file INI of the application
	AfxGetApp()->m_pszRegistryKey = NULL; // If the value is NULL it is stored in the file, if not it is stored in the Windows Registry		
										  
	free((void*)AfxGetApp()->m_pszProfileName);
	AfxGetApp()->m_pszProfileName = _tcsdup(path);

	//  HDHR_DEVICE Section
	seccion = L"HDHR_DEVICE";

	if (!AfxGetApp()->WriteProfileString(seccion, _T("DEVICE_ID"), m_device_id))
		return 0;

	if (!AfxGetApp()->WriteProfileString(seccion, _T("SERVER_IP"), m_hdhrServerIP))
		return 0;
	
	int currentAutoStart = AfxGetApp()->GetProfileInt(seccion, L"AUTOSTART", 0);
	if (currentAutoStart != m_autostart)
		showMessageRestart = 1;
	if (!AfxGetApp()->WriteProfileInt(seccion, _T("AUTOSTART"), m_autostart))
		return 0;

	int currentTunersNum = AfxGetApp()->GetProfileInt(seccion, L"TUNERS_NUM", 2);
	if (currentTunersNum != m_TunersNumberToSaveINI)
		showMessageRestart = 1;
	if (!AfxGetApp()->WriteProfileInt(seccion, _T("TUNERS_NUM"), m_TunersNumberToSaveINI))
		return 0;

	if (!AfxGetApp()->WriteProfileInt(seccion, _T("LOCK"), m_Lock))
		return 0;

	//CString t;
	//t.Format(L"%I64d", m_maxTimeToSendDgram);
	//if (!AfxGetApp()->WriteProfileString(seccion, _T("MAX_TIME_SEND_DGRAM"), t))
	//	return 0;

	// EMU_DEBUG Section
	seccion = L"EMU_DEBUG";
	CString level;
	level.Format(L"%d", m_traceLevel);
	if (!AfxGetApp()->WriteProfileString(seccion, _T("LEVEL"), level))
		return 0;

	if (!AfxGetApp()->WriteProfileInt(seccion, _T("AUTO_CLEAN_LOG"), m_autoCleanLog))
		return 0;

	// HDHR_SYS_CONFIG Section 
	seccion = _T("HDHR_SYS_CONFIG");

	sys_model.Insert(0, L"\"");
	sys_model.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_MODEL"), sys_model))
		return 0;
	sys_model.Replace(L"\"", L"");

	version.Insert(0, L"\"");
	version.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_VERSION"), version))
		return 0;
	version.Replace(L"\"", L"");

	sys_hwmodel.Insert(0, L"\"");
	sys_hwmodel.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_HWMODEL"), sys_hwmodel))
		return 0;
	sys_hwmodel.Replace(L"\"", L"");

	sys_copyright.Insert(0, L"\"");
	sys_copyright.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_COPYRIGHT"), sys_copyright))
		return 0;
	sys_copyright.Replace(L"\"", L"");

	sys_dvbc_modulation.Insert(0, L"\"");
	sys_dvbc_modulation.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_DVBC_MODULATION"), sys_dvbc_modulation))
		return 0;
	sys_dvbc_modulation.Replace(L"\"", L"");

	sys_channelmap.Insert(0, L"\"");
	sys_channelmap.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_CHANNEL_MAP"), sys_channelmap))
		return 0;
	sys_channelmap.Replace(L"\"", L"");

	lineup_location.Insert(0, L"\"");
	lineup_location.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("LINEUP_LOCATION"), lineup_location))
		return 0;
	lineup_location.Replace(L"\"", L"");

	sys_features_ini.Insert(0, L"\"");
	sys_features_ini.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_FEATURES"), sys_features_ini))
		return 0;
	sys_features_ini.Replace(L"\"", L"");

	sys_debug_ini.Insert(0, L"\"");
	sys_debug_ini.Append(L"\"");
	if (!AfxGetApp()->WriteProfileString(seccion, _T("SYS_DEBUG"), sys_debug_ini))
		return 0;
	sys_debug_ini.Replace(L"\"", L"");

	if (showMessageRestart)
		return 2;

	return 1;
}

int CConfigProxy::setValueItemInFicheroINI(CString seccion, CString item, CString valor)
{
	//Obtain current directory
	TCHAR sDirActual[200];
	GetCurrentDirectory(200, sDirActual);

	CString path;
	path.Format(L"%s\\%s", sDirActual, _T(NAME_FILE_INI));

	//The name of the INI file is updated. It is liberated the actual and is reserved for the new
	//m_pszProfileName contains the name of the file INI of the application
	free((void*)AfxGetApp()->m_pszProfileName);
	AfxGetApp()->m_pszProfileName = _tcsdup(path);

	return (WriteProfileString(seccion, item, valor));
}

void CConfigProxy::ReadFileMappingList()
{
	CString seccion, chNum, ch;
	double fch;
	int errFormat = 0;
	

	//Obtain current directory
	TCHAR sDirActual[200];
	GetCurrentDirectory(200, sDirActual);

	CString path;
	path.Format(L"%s\\%s", sDirActual, _T(NAME_FILE_MAPLIST));

	//The name of the INI file is updated. It is libeated the actual and is reserved for the new
	//m_pszProfileName contains the name of the file INI of the application
	AfxGetApp()->m_pszRegistryKey = NULL;
	free((void*)AfxGetApp()->m_pszProfileName);
	AfxGetApp()->m_pszProfileName = _tcsdup(path);

	// MAPPING_LIST Section
	seccion = _T("MAPPING_LIST");

	m_numChannels = AfxGetApp()->GetProfileInt(seccion, L"NUM_CHANNELS", 0);
	if (m_numChannels ==0)
		{
		LOGM(WRNG,0,"Mapping List: NUM_CHANNELS is 0 or not defined\n");
		}

	if (m_numChannels != 0)
		m_infoChannels = new ChannelMappingList[m_numChannels];
	int j = 0;
	
	for (int i = 1; i <= m_numChannels; i++)
	{
		chNum.Format(L"CH%d", i);
		j = i - 1;

		ch = AfxGetApp()->GetProfileString(chNum, L"Channel", 0);
		if (!ch.Compare(L"0") || !ch.Compare(L""))
		{
			LOGM(WRNG,1,"Mapping List: Channel of CH%d is 0 or not defined\n", i);
		}

		if (ch.Find(L".") == -1)
			m_infoChannels[j].channel = atoi(CStringA(ch)); 
		else
		{
			fch = atof(CStringA(ch));
			//ok m_infoChannels[j].channel = fch * 1000000; //Conversion from MHz to Hz
			m_infoChannels[j].channel = fch * 10000000; //Conversion from MHz to Hz
		}

		m_infoChannels[j].LowFreq = atol(CStringA(AfxGetApp()->GetProfileString(chNum, L"LowFreq")));
		if (m_infoChannels[j].LowFreq == 0)
		{
			LOGM(WRNG,1,"Mapping List: LowFreq of CH%d is 0 or not defined\n", i);
		}
		m_infoChannels[j].HighFreq = atol(CStringA(AfxGetApp()->GetProfileString(chNum, L"HighFreq")));
		if (m_infoChannels[j].HighFreq == 0)
		{
			LOGM(WRNG,1,"Mapping List: HighFreq of CH%d is 0 or not defined\n", i);
		}
		m_infoChannels[j].Protocol = AfxGetApp()->GetProfileString(chNum, L"Protocol");
		m_infoChannels[j].URLGet = AfxGetApp()->GetProfileString(chNum, L"URLGet");
		m_infoChannels[j].URLGet_ExtPidFilt = m_infoChannels[j].URLGet;
		
		m_infoChannels[j].UDPsource = AfxGetApp()->GetProfileString(chNum, L"UDPsource");
		m_infoChannels[j].InternalForcingPIDs = AfxGetApp()->GetProfileString(chNum, L"InternalForcingPIDs");
		m_infoChannels[j].InternalPIDFiltering = AfxGetApp()->GetProfileString(chNum, L"InternalPIDFiltering");
		if (m_infoChannels[j].InternalPIDFiltering.Compare(L"Y") &&
			m_infoChannels[j].InternalPIDFiltering.Compare(L"y") &&
			m_infoChannels[j].InternalPIDFiltering.Compare(L"N") &&
			m_infoChannels[j].InternalPIDFiltering.Compare(L"n"))
		{	
			m_infoChannels[j].InternalPIDFiltering.Format(L"N");
			LOGM(WRNG,0,"Mapping List: The value of InternalPIDFiltering must be 'Y' or 'N'. The default value will be 'N'\n");
		}
		m_infoChannels[j].ExternalPIDFiltering = AfxGetApp()->GetProfileString(chNum, L"ExternalPIDFiltering");
		if (m_infoChannels[j].ExternalPIDFiltering.Compare(L"Y") &&
			m_infoChannels[j].ExternalPIDFiltering.Compare(L"y") &&
			m_infoChannels[j].ExternalPIDFiltering.Compare(L"N") &&
			m_infoChannels[j].ExternalPIDFiltering.Compare(L"n"))
		{
			m_infoChannels[j].ExternalPIDFiltering.Format(L"N");
			LOGM(WRNG,0,"Mapping List: The value of ExternalPIDFilteringmust be 'Y' or 'N'. The default value will be 'N'\n");
		}
		
		m_infoChannels[j].signalStrength = AfxGetApp()->GetProfileInt(chNum, L"Signal_Strength", ss_DEF);
		m_infoChannels[j].signalQuality = AfxGetApp()->GetProfileInt(chNum, L"Signal_Quality", snq_DEF);
		m_infoChannels[j].symbolQuality = AfxGetApp()->GetProfileInt(chNum, L"Symbol_Quality", seq_DEF);
		m_infoChannels[j].networkRate = (atol(CStringA(AfxGetApp()->GetProfileString(chNum, L"Network_Rate"))) / 10000) - 100;

		m_infoChannels[j].Program_table = AfxGetApp()->GetProfileString(chNum, L"Program_table");
		m_infoChannels[j].Program_table_file = m_infoChannels[j].Program_table;

		m_infoChannels[j].NumPrograms = 0;

		if ((m_infoChannels[j].Program_table.Compare(L""))
			&& (m_infoChannels[j].Program_table.Compare(L"-1"))
			&& (m_infoChannels[j].Program_table.Compare(L"none"))
			&& (m_infoChannels[j].Program_table.Compare(L"NONE"))
			)
		{
			m_infoChannels[j].Program_table = ParserProgramTable(m_infoChannels[j].Program_table, &m_infoChannels[j]);
			if (!m_infoChannels[j].Program_table.Compare(L""))
			{
				LOGM(WRNG,1,"Mapping List: Program_table of CH%d is not defined\n", i);
			}
		}
		else
		{
			m_infoChannels[j].Program_table.Format(L"");

			LOGM(WRNG,1,"Mapping List: Program_table of CH%d is not defined\n", i);
		}

		//Treatment data transport protocol of Transport Stream

		if (!m_infoChannels[j].Protocol.Compare(L"http"))
			m_infoChannels[j].Protocol.Format(L"HTTP");
		else if (!m_infoChannels[j].Protocol.Compare(L"udp"))
			m_infoChannels[j].Protocol.Format(L"UDP");

		CString aux;
		int index, index2, count;
		errFormat = 0;
		if (!m_infoChannels[j].Protocol.Compare(L"HTTP"))
		{
			//Format: http://IPhttp:PORThttp/udp/IPMULTICAST:PORTudp

			if (!m_infoChannels[j].URLGet.Compare(L"")
				|| !m_infoChannels[j].URLGet.Compare(L"none")
				|| !m_infoChannels[j].URLGet.Compare(L"NONE"))
			{
				m_infoChannels[j].URLGet.Format(L"");
				m_infoChannels[j].URLGet_ExtPidFilt = m_infoChannels[j].URLGet;
				LOGM(WRNG,1,"Mapping List: URLGet of CH%d is not defined. Protocol is HTTP\n", i);
			}
			else
			{
				aux = m_infoChannels[j].URLGet;
				index = aux.Find(L"//", 0);
				if (index == -1) errFormat = 1;
				index2 = aux.Find(L":", index);
				if (index2 == -1) errFormat = 1;
				m_infoChannels[j].ipHTTP = aux.Mid(index + 2, index2 - index - 2);
				index = aux.Find(L"/", index2);
				if (index == -1) errFormat = 1;
				m_infoChannels[j].puertoHTTP = atoi(CStringA(aux.Mid(index2 + 1, index - index2 + 1)));
				m_infoChannels[j].datosGETHTTP = aux.Right(aux.GetLength() - index - 1);

				aux = m_infoChannels[j].ipHTTP;
				count = 0;
				index = 0;
				while (index != -1)
				{
					index = aux.Find(L".", index+1);
					if (index != -1)
						count++;
				}

				if (count != 3)
					errFormat = 1;
				if (errFormat)
				{
					LOGM(WRNG,1,"Mapping List: URLGet of CH%d can have an incorrect format, look over it. Protocol is HTTP\n", i);
				}
			}
		}
		else if (!m_infoChannels[j].Protocol.Compare(L"UDP"))
		{
			//Format: IP:PORT

			if (!m_infoChannels[j].UDPsource.Compare(L"")
				|| !m_infoChannels[j].UDPsource.Compare(L"none")
				|| !m_infoChannels[j].UDPsource.Compare(L"NONE"))
			{
				m_infoChannels[j].UDPsource.Format(L"");
				LOGM(WRNG,1,"Mapping List: UDPsource of CH%d is not defined. Protocol is UDP\n", i);
			}
			else
			{
				aux = m_infoChannels[j].UDPsource;
				index = aux.Find(L":", 0);
				if (index == -1) errFormat = 1;
				m_infoChannels[j].ipUDP = aux.Left(index);
				m_infoChannels[j].puertoUDP = atoi(CStringA(aux.Right(aux.GetLength() - index - 1)));

				aux = m_infoChannels[j].ipUDP;
				count = 0;
				index = 0;
				while (index != -1)
				{
					index = aux.Find(L".", index + 1);
					if (index != -1)
						count++;
				}
				if (count != 3)
					errFormat = 1;

				if (errFormat)
				{
					LOGM(WRNG,1,"Mapping List: UDPsource of CH%d can have an incorrect format, look over it. Protocol is UDP\n", i);
				}
			}
		}
		else
		{
			LOGM(WRNG,1,"Mapping List: Protocol of CH%d is not defined (HTTP/UDP)\n", i);
		}
	}
}

CString CConfigProxy::ParserProgramTable(CString progTable, ChannelMappingList* mapList)
{
	CString table, program, aux, aux2, pids;
	int index1, index2, index3, index4, index5;
	int tam = progTable.GetLength();
	
	index1 = progTable.Find(L"[", 0);
	index2 = progTable.Find(L"]", index1);

	aux = progTable.Mid(index1 + 1, index2 - index1 - 1);

	if (!aux.Compare(L"") || !aux.Compare(L"-1"))
		return CString(L"");

	index3 = aux.Find(L":", 0);
	index4 = aux.Find(L":", index3 + 1);
	index5 = aux.Find(L":", index4 + 1);

	if (index4 != -1)
	{
		aux2 = aux.Left(index3);
		mapList->FilterProgramList[mapList->NumPrograms].program = atoi(CStringA(aux2));
		mapList->NumPrograms++;

		program.Append(aux2);
		program.Append(L": ");
		program.Append(aux.Mid(index3 + 1, index4 - index3 - 1));
		program.Append(L" ");
		if (index5 == -1)
		{
			program.Append(aux.Right(aux.GetLength() - index4 - 1));
			mapList->FilterProgramList[mapList->NumPrograms-1].filterList.Format(L"");
		}
		else
		{
			program.Append(aux.Mid(index4 + 1, index5 - index4- 1));
		}

		table.Append(program);
		
		if (index5 != -1)
		{
			//Obtain PIDs of program
			pids = aux.Right(aux.GetLength() - index5 - 1);
			if (pids.Compare(L"-1") && pids.Compare(L"") && pids.Compare(L"all") && pids.Compare(L"ALL"))
			{
				mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Format(L"%s", pids);
				if (mapList->InternalForcingPIDs.Compare(L""))
				{
					mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Append(L",");
					mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Append(mapList->InternalForcingPIDs);
				}
			}
			else
				mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Format(L"");
		}
	}
	else
	{
		table.Append(aux);
	}

	while (index1 != -1)
	{
		index1 = progTable.Find(L"[", index2);
		index2 = progTable.Find(L"]", index2+1);

		table.Append(L"\r\n");

		aux = progTable.Mid(index1 + 1, index2 - index1 - 1);
		index3 = aux.Find(L":", 0);
		index4 = aux.Find(L":", index3 + 1);
		index5 = aux.Find(L":", index4 + 1);

		if (index4 != -1)
		{
			aux2 = aux.Left(index3);
			mapList->FilterProgramList[mapList->NumPrograms].program = atoi(CStringA(aux2));
			mapList->NumPrograms++;
			
			program.Format(L"");
			program.Append(aux2);
			program.Append(L": ");
			program.Append(aux.Mid(index3 + 1, index4 - index3 - 1));
			program.Append(L" ");

			if (index5 == -1)
			{
				program.Append(aux.Right(aux.GetLength() - index4 - 1));
				mapList->FilterProgramList[mapList->NumPrograms-1].filterList.Format(L"");
			}
			else
			{
				program.Append(aux.Mid(index4 + 1, index5 - index4 - 1));
			}

			table.Append(program);

			if (index5 != -1)
			{
				//Obtain PIDs of program
				pids = aux.Right(aux.GetLength() - index5 - 1);
				if (pids.Compare(L"-1") && pids.Compare(L"") && pids.Compare(L"all") && pids.Compare(L"ALL"))
				{
					mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Format(L"%s", pids);
					if (mapList->InternalForcingPIDs.Compare(L""))
					{
						mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Append(L",");
						mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Append(mapList->InternalForcingPIDs);
					}
				}
				else
					mapList->FilterProgramList[mapList->NumPrograms - 1].filterList.Format(L"");
			}
		}
		else
		{
			table.Append(aux);
		}
	}

	table.Append(L"\r\n");

	return table;
}

//Returns the position within ChannelMappingList * m_infoChannels in which is the the channel received by parameter
int CConfigProxy::ObtainIndexChannel(int canal)
{
	//Comprove by the channel
	for (int i = 0; i < m_numChannels; i++)
	{
		if (m_infoChannels[i].channel == canal)
			return i;
	}

	//If have not found the index by the channel, is checked by the frequency
	for (int i = 0; i < m_numChannels; i++)
	{
		if (m_infoChannels[i].LowFreq <= canal && m_infoChannels[i].HighFreq >= canal)
			return i;
	}

	//If have not found the index by the frequency, it's checked if frequency is in KHz. TSReader arrives in KHz
	int ch = 0;
	CString c;
	c.Format(L"%d", canal);
	if (c.GetLength() == 8)
	{
		c.Append(L"0");
		ch = atoi(CStringA(c));
		for (int i = 0; i < m_numChannels; i++)
		{
			if (m_infoChannels[i].LowFreq <= ch && m_infoChannels[i].HighFreq >= ch)
				return i;
		}
	}

	return -1;
}

CString CConfigProxy::ObtainProgsTableByChannel(int canal)
{
	int i = 0;

	//Comprove by channel
	for (i = 0; i < m_numChannels; i++)
	{
		if (m_infoChannels[i].channel == canal)
			return m_infoChannels[i].Program_table;
	}

	//If have not found the index by the channel, it's checked by frequency
	for (i = 0; i < m_numChannels; i++)
	{
		if (m_infoChannels[i].LowFreq <= canal && m_infoChannels[i].HighFreq >= canal)
			return m_infoChannels[i].Program_table;
	}

	return CString("");
}

///////////////////////////////////////

void CConfigProxy::AddClientToInterface()
{
	if (!m_infoActualSelCli)
		m_infoActualSelCli = new InfoActualSelectCli;
}

int CConfigProxy::ObtainClientInterface()
{
	return m_dlg->m_CurrentCli;
}

int CConfigProxy::ObtainTunerInterface()
{
	return m_dlg->m_CurrentTuner;
}

void CConfigProxy::UpdateClientTOInterface(int tuner)
{
	if (tuner == m_dlg->m_CurrentTuner)
	{
		m_dlg->UpdateInfoTuner();
	}
}

CString CConfigProxy::findPidsOfProgram(long channel, int program)
{
	int ch = ObtainIndexChannel(channel);

	for (int i = 0; i < m_infoChannels[ch].NumPrograms; i++)
	{
		if (m_infoChannels[ch].FilterProgramList[i].program == program)
			return m_infoChannels[ch].FilterProgramList[i].filterList;
	}

	return CString(L"");
}

int CConfigProxy::getInternalPIDFilteringOfChannel(long channel)
{
	int ch = ObtainIndexChannel(channel);

	if (ch == -1)
		return 0;

	return ((m_infoChannels[ch].InternalPIDFiltering.Compare(L"Y")
		&& m_infoChannels[ch].InternalPIDFiltering.Compare(L"y")) ? 0 : 1);
}

int CConfigProxy::getExternalPIDFilteringOfChannel(long channel)
{
	int ch = ObtainIndexChannel(channel);

	if (ch == -1)
		return 0;

	return ((m_infoChannels[ch].ExternalPIDFiltering.Compare(L"Y") 
		&& m_infoChannels[ch].ExternalPIDFiltering.Compare(L"y")) ? 0 : 1);
}

CString CConfigProxy::ParserHDHRResponseMultiLine(CString value)
{
	CString resultString, aux;
	int index1, index2;

	resultString.Format(L"");

	index1 = value.Find(L"[", 0);
	index2 = value.Find(L"]", index1);

	aux = value.Mid(index1 + 1, index2 - index1 - 1);

	if (!aux.Compare(L"") || !aux.Compare(L"-1"))
		return CString(L"");

	resultString.Append(aux);

	while (index1 != -1)
	{
		index1 = value.Find(L"[", index2);
		index2 = value.Find(L"]", index2 + 1);
		
		aux = value.Mid(index1 + 1, index2 - index1 - 1);

		if (aux.Compare(L"") && aux.Compare(L"-1"))
		{
			resultString.Append(L"\n");
			resultString.Append(aux);
		}
	}

	return resultString;
}

void CConfigProxy::ReloadMappingList()
{
	m_dlg->OnBnClickedButtonReloadMappingList();
}

void CConfigProxy::AccessToRestartProxy()
{
	//Stop Discovery and Control. Control thread don't stop because it is called from this thread

	m_gestProxy->getDiscovery()->StopDiscovery();
	TerminateThread(m_gestProxy->hThreadDiscovery, 1000);

	m_gestProxy->getControl()->StopControl();

	//Start Discovery and Control
	m_gestProxy->StartDiscovery(m_traceLevel, m_device_id, m_hdhrServerIP);
	m_gestProxy->getControl()->StartHDHRServer();  // StartControl(m_traceLevel, m_device_id, m_hdhrServerIP);

}
