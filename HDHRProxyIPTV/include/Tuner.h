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

#include "hdhomerun_os_windows.h"
#include "Transport.h"
#include "Trace.h"
#include <time.h>
#include <stdio.h>
#include "winsock2.h"

#define MAX_SIZE_FILTER 32536

class CTuner
{
public:
	int tuner;
	int state; //State in which the tuner is : STANDBY; TUNED_CHAN; FILTERING; STREAMING
	long canal;
	long channelNotInMapList;
	char lock[15];
	int ss;
	int snq;
	int seq;
	long bps;
	int pps;
	int program;
	char filter[MAX_SIZE_FILTER];
	char target[50];
	char channelmap[12];
	uint32_t lockkey;
	char IPLockkey[16];
	CString pidsToFiltering; //Internal PID filtering
	CTransport* transportTuner;

	int initiazedByCli;
	SYSTEMTIME timer1;  //Timer to control no events
	int timer1_secs;

	CTrace* m_traces;
	CConfigProxy* m_cfgProxy;

	CTuner();
	~CTuner();

	long getChannelNotInMapList() { return channelNotInMapList;}
	void setChannelNotInMapList(long chan) { channelNotInMapList = chan; }
	int getInitiazedByCli() { return initiazedByCli; }
	void setInitiazedByCli(int val) { initiazedByCli = val; }
	long getChannel() { return canal; }
	int getProgram() { return program; }
	int getState() { return state; }
	void setState(int st) { state = st; }
	void setTuner(int ntuner) { tuner = ntuner; transportTuner->setTuner(ntuner); }
	CTransport* getTransportTuner() { return transportTuner; }
	char* getTarget() { return target; }
	char* getFilter() { return filter; }
	CString getPidsToFiltering() { return pidsToFiltering; };
	uint32_t getLockkey() { return lockkey; }
	void setLockkey(uint32_t lock) { lockkey = lock; }
	void setChannelmap(char* chmap) { strcpy(channelmap, chmap); }
	
	void setSS(int s) { ss = s; }
	void setSNQ(int s) { snq = s; }
	void setSEQ(int s) { seq = s; }
	void setBPS(long s) { bps = s; }
	void setPPS(int s) { pps = s; }

	void ChangeStateToStandby();
	void ChangeStateToTunedChan(long chan);
	void ChangeStateToFilteringByProgram(int prog);
	int ChangeStateToStreaming(char* targ);
	void ChangeStateToFilteringByFilter(char* filt, char* ip);
	void ChangePIDsByFilterInStreaming(char* filt, char* ip);
	void ChangePIDsByProgramInStreaming(int prog, char* ip);
	CString ConvertHexPidsListToNum(CString filterpids);
	int getReadBufferStatus();
	int getRingBufferStatus();

	void SetTimers();
	int CheckTimer();
	_int64 CTuner::CompareSystemTime(SYSTEMTIME st1, SYSTEMTIME st2);

#define LOGM(level, only, text, ...) LOGG(m_traces, "CTRL-TUNER ::", level, only, text, ##__VA_ARGS__)    
};
