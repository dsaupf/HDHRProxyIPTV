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
#include "afx.h"

#include "Parameterization.h"
#include "direct.h"

class CTrace;
class CHDHRProxyIPTVDlg;
class CGestionProxy;

#define NAME_FILE_INI     "HDHRProxyIPTV.ini"
#define NAME_FILE_MAPLIST "HDHRProxyIPTV_MappingList.ini"

#define MAX_TIME_TO_SEND	35000000		//Max time to send a TS packet (1316). Relative to RingBuffer
#define MAX_TIME_TO_SEND_S	"35000000"

typedef struct
{
	int program;
	CString filterList;
} FilterProgram;

typedef struct
{
	long channel;
	long LowFreq;
	long HighFreq;
	CString URLGet;
	CString URLGet_ExtPidFilt;
	CString Protocol;
	CString UDPsource;
	CString InternalForcingPIDs;
	CString InternalPIDFiltering;
	CString ExternalPIDFiltering;
	CString Program_table;
	CString Program_table_file;
	FilterProgram FilterProgramList[50];
	int NumPrograms;
	CString ipHTTP;
	int puertoHTTP;
	CString datosGETHTTP;
	CString ipUDP;
	int puertoUDP;
	int signalStrength;
	int signalQuality;
	int symbolQuality;
	long networkRate;
} ChannelMappingList;

typedef struct
{
	CString state;
	long channel;
	int channelNotInMapList;
	CString pids;	//PIDs active list
	CString target;
	CString protocolTS; 
	CString udpTS;
	CString httpTS;
	CString lockkey;
	CString program;
	CString pidsProgram;	//Internal PIDs filter
	CString readbuffer;
	CString ringbuffer;
} InfoActualSelectCli;


//Clase Singleton, there will be only one instance of it
class CConfigProxy
{
private:
	static CConfigProxy* m_instance;
	CHDHRProxyIPTVDlg* m_dlg;
	CGestionProxy* m_gestProxy;

	CConfigProxy() {  };

public:
	void setDlg(CHDHRProxyIPTVDlg* dlg)
	{
		m_dlg = dlg;
	}

	CHDHRProxyIPTVDlg* getDlg()
	{
		return m_dlg;
	}
	void setGestProxy(CGestionProxy* gest) { m_gestProxy = gest; }
	CGestionProxy* getGestProxy() { return m_gestProxy; }

	CTrace* m_trace;
	CString m_device_id;
	CString m_hdhrServerIP;
	CString m_hdhrServerPuerto;
	int m_traceLevel;
	int m_tipoProgTabla;
	int m_autostart;  // 1 / 0
	int m_TunersNumber;
	int m_TunersNumberToSaveINI;
	int m_Lock;  //1: lock (lockkey) is applied; 0: No lock is applied even comes message of set lockkey
	int m_autoCleanLog;
	__int64 m_maxTimeToSendDgram;
	__int64 m_maxTimeToPacket;
	CString m_FullTSReplace;

	//Response to request of HDHR Clients that are part of configuration of de HDHR Server (device)
	CString sys_model;		//response to /sys/model
	CString sys_hwmodel;	//response to /sys/hwmodel
	CString version;		//response to /sys/version
	CString sys_copyright;	//response to /sys/copyright
	CString sys_features_ini;	  //response to /sys/features. Format in the Config INI file
	CString sys_features;	//response to /sys/features. Format to send
	CString sys_dvbc_modulation;  //response to /sys/dvbc_modulation
	CString sys_channelmap;	//response to /tunerX/channelmap
	CString sys_debug_ini;
	CString sys_debug;

	CString lineup_location;	  //response to /lineup/location
	
	CString m_multicast;
	CString m_servSATIP;
	CString m_port;

	FILE *m_fileINI;

	//Mapping List Data
	int m_numChannels;
	ChannelMappingList* m_infoChannels;

	//Used to temporarily store data to be displayed in the selected client information, to access information from the dialogue
	InfoActualSelectCli* m_infoActualSelCli;

	void ReadFileMappingList();

	static CConfigProxy* GetInstance()
	{
		if (m_instance == NULL)
		{
			m_instance = new CConfigProxy();
		}

		return m_instance;
	}
	
	~CConfigProxy();

	void ReadFileINI();
	int setValueItemInFicheroINI(CString seccion, CString item, CString valor);
	void ObtainHostIP(CString *ip);
	CString ParserHDHRResponseMultiLine(CString value);

	void setMulticast(CString mc) { m_multicast = mc; }
	CString getMulticast() { return m_multicast; }
	void setPort(CString p) { m_port = p; }
	CString getPort() { return m_port; }
	void setServSATIP(CString p) { m_servSATIP = p; }
	CString getServSATIP() { return m_servSATIP; }
	__int64 getMaxTimeToSendDgram() { return m_maxTimeToSendDgram; }
	__int64 getMaxTimeToPacket() { return m_maxTimeToPacket; }
	CString getFullTSReplace() { return m_FullTSReplace; }

	int SaveItems();
	CString ParserProgramTable(CString progTable, ChannelMappingList* mapList);

	int ObtainIndexChannel(int canal);
	CString ObtainProgsTableByChannel(int canal);

	/////////////////////////////////
	//Functions to access the graphical interface
	void AddClientToInterface();
	int ObtainClientInterface();
	int ObtainTunerInterface();
	void UpdateClientTOInterface(int tuner);

	void ReloadMappingList();

	int getTunersNumber() { return m_TunersNumber; }
	int getLock() { return m_Lock; }
	CString findPidsOfProgram(long channel, int program);
	int getInternalPIDFilteringOfChannel(long channel);
	int getExternalPIDFilteringOfChannel(long channel);

	void AccessToRestartProxy();

#define LOGM(level, only, text, ...) LOGG(m_trace, "CONFIG     ::", level, only, text, ##__VA_ARGS__)
};
