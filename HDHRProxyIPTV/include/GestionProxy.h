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

#include "Trace.h"
#include "Discovery.h"
#include "Control.h"

class CGestionProxy
{
public:
	CConfigProxy* m_cfgProxy;
	CTrace *m_Traces;
	int m_initializedProxy;
	CDiscovery *m_discovery;
	CControl *m_control;
	HANDLE hThreadDiscovery;
	HANDLE hThreadControl;

	CGestionProxy();
	~CGestionProxy();

	CDiscovery *getDiscovery() { return m_discovery; }
	CControl *getControl() { return m_control; }

	DWORD ListeningThreadDiscovery();
	DWORD ListeningThreadControl();
	int InitializeProxy(int trace, CString idDisp, CString ipHDHR);
	int StartDiscovery(int trace, CString idDisp, CString ipHDHR);
	int StartControl(int trace, CString idDisp, CString ipHDHR);
	void StopProxy();
	void CleanLog();

	void UpdateInfoCliAtInterface(int tuner);
	void ResetTuner(int tuner);
	void ForceUnlockTuner(int tuner);

#define LOGM(level, only, text, ...) LOGG(m_Traces, "-- TOP EXE --", level, only, text, ##__VA_ARGS__)    
};