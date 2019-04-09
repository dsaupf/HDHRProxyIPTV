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
#include "Trace.h"

CTrace::CTrace()
{
	m_traceFile = NULL;
	m_cfgProxy = CConfigProxy::GetInstance();
	//Initialize trace level default
	m_traceLevel = 2;
}

CTrace::~CTrace()
{
	if (m_traceFile)
	{
		fclose(m_traceFile);
		delete m_traceFile;
	}
}

void CTrace::InitializeTrace(int nivelTrz)
{
	char cdatetime[60];

	m_traceLevel = nivelTrz;

	if (m_traceLevel != NO_TRACE)
	{
		m_traceFile = fopen(TRACE_FILE_NAME, "a+");

		if (m_traceFile)
			WriteTrace("Start LOG of Proxy traces\n", TRZ2);
		else
		{
			ObtainCurrentDateTime(cdatetime, sizeof(cdatetime));
			fprintf(m_traceFile, "%s ## *ERROR* :: It has not generated the trace file\n", cdatetime);
		}

		fclose(m_traceFile);
	}
}

void CTrace::StopTrace()
{
	if (m_traceLevel != NO_TRACE)
	{
		WriteTrace("Close LOG file\n", TRZ2);
		fclose(m_traceFile);
	}
}

void CTrace::ObtainCurrentDateTime(char* datetime, int size)
{
	SYSTEMTIME st;
	//char* dateTime = new char[40];
	memset(datetime, 0, size);
	
	GetLocalTime(&st);

	_snprintf(datetime, size - 2, "%04d/%02d/%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	//return dateTime;
}

void CTrace::WriteTraceForceLevel(char *traza, int level)
{
	int cfgTraza = m_cfgProxy->m_traceLevel;

	m_cfgProxy->m_traceLevel = level;

	WriteTrace(traza, level);

	//Initial value is restored
	m_cfgProxy->m_traceLevel = cfgTraza;
}

void CTrace::WriteTrace(char *traza, int level)
{
	m_traceLevel = m_cfgProxy->m_traceLevel;

	if (m_traceLevel == NO_TRACE)
		return;

	if (level > m_traceLevel && level != ERR && level != WRNG)
		return;

	CRITICAL_SECTION m_cs; //Critical section
	InitializeCriticalSection(&m_cs); //Initialize criticaL section

	EnterCriticalSection(&m_cs);

	if (1)
	{
		char cdatetime[60];
		ObtainCurrentDateTime(cdatetime, sizeof(cdatetime));

		m_traceFile = fopen(TRACE_FILE_NAME, "a+");

		if (level == ERR)
			fprintf(m_traceFile, "%s ## *ERROR* %s", cdatetime, traza);
		else if (level == WRNG)
			fprintf(m_traceFile, "%s ## WARNING %s", cdatetime, traza);
		else
			fprintf(m_traceFile, "%s ## DEBUG %d %s", cdatetime, level, traza);

		fclose(m_traceFile);
	}

	//END CRITICAL SECTION
	if (&m_cs != NULL)
	{
		LeaveCriticalSection(&m_cs);
		DeleteCriticalSection(&m_cs);
	}
}

void CTrace::CleanTrace()
{
	m_traceLevel = NO_TRACE;

	m_traceFile = fopen(TRACE_FILE_NAME, "w");

	fclose(m_traceFile);
}

int CTrace::IsPrintable(char* cad)
{
	for (int i = 0; i < strlen(cad); i++)
	{
		if (!isprint(cad[i]))
			return 0;
	}
	return 1;
}


char* CTrace::ConvertToHex(char* cad)
{
	int tam = strlen(cad) * 7 + 1;
	char* hexstr = new char[tam+7+2];
	int i = 0;
	_snprintf(hexstr + i * 7, 7, "0x%04x ", cad[i]);

	for (i = 1; i<strlen(cad); i++) {
		if (cad[i] == ' ')
			_snprintf(hexstr + i * 7, 7, "       ");
		else
			_snprintf(hexstr + i * 7, 7, " 0x%04x", cad[i]);
	}
	hexstr[i*7] = 0;

	CString aux(hexstr);

	aux.Replace(L"         ", L" ");
	aux.Replace(L"        ", L" ");
	aux.Replace(L"       ", L" ");
	aux.Replace(L"      ", L" ");
	aux.Replace(L"     ", L" ");
	aux.Replace(L"  ", L" ");

	strncpy(hexstr, CStringA(aux), tam);

	return hexstr;
}

char* CTrace::ConvertHexToAscii(char* cad)
{
	CString hexString(cad);
	char* tmp = new char[16];
	strcpy(tmp, "");
	char* resString = new char[strlen(cad)/2];
	strcpy(resString, "");
	CString byte;

	for (int i = 0; i < hexString.GetLength(); i += 2)
	{
		byte = hexString.Mid(i, 2);
		if (i==0)
			tmp[0] = (char)(int)strtol(CStringA(byte), NULL, 16);
		else
			tmp[i-1] = (char)(int)strtol(CStringA(byte), NULL, 16);
	}

	strncpy(resString, tmp, strlen(cad) / 2);

	return resString;
}

char* CTrace::ConvertToAHexWithoutSpaces(char* cad, int size)
{
	int len = size;

	if (len > strlen(cad))
		len = strlen(cad);

	int tam = len * 5 + 1;
	char* hexstr = new char[tam];
	int i = 0;
	_snprintf(hexstr + i * 3, 3, "%02x ", cad[i]);

	for (i = 1; i<len; i++) {
		_snprintf(hexstr + i * 3, 3, "%02x ", cad[i]);
	}
	hexstr[i * 3] = 0;

	CString aux(hexstr);
	aux.Replace(L" ", L"");
	strncpy(hexstr, CStringA(aux), tam);

	return hexstr;
}

char* CTrace::ConvertToAXHexWithoutSpaces(char* cad, int size)
{
	int len = size;

	if (len > strlen(cad))
		len = strlen(cad);

	int tam = len * 5 + 1;
	char* hexstr = new char[tam];
	strcpy(hexstr, "");
	char h[9];
	strcpy(h, "");
	int i = 0;
	
	_snprintf(h, 2, "%02x", cad[i]);
	strcat(hexstr, h);

	for (i = 1; i<len; i++) {
		strcpy(h, "");
		_snprintf(h, 3, ":%02x", cad[i]);
		strcat(hexstr, h);
	}

	CString aux(hexstr);
	aux.Replace(L" ", L"");
	strncpy(hexstr, CStringA(aux), tam);

	return hexstr;
}

int CTrace::IsLevelWriteable(int level)
{
/*	CRITICAL_SECTION m_cs; //Critical section
	InitializeCriticalSection(&m_cs); //Initialize criticaL section

	EnterCriticalSection(&m_cs);
*/
	m_traceLevel = m_cfgProxy->m_traceLevel;

	if (m_traceLevel == NO_TRACE)
		return 0;

	if (level <= m_traceLevel || level == ERR || level == WRNG)
		return 1;
	else
		return 0;

	//END CRITICAL SECTION
/*	if (&m_cs != NULL)
	{
		LeaveCriticalSection(&m_cs);
		DeleteCriticalSection(&m_cs);
	}
*/
}
