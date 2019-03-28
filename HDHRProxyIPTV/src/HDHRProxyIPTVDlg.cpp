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

// HDHRProxyIPTVDlg.cpp : implementation file
//

#include "stdafx.h"
#include "HDHRProxyIPTV.h"
#include "HDHRProxyIPTVDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
	EnableActiveAccessibility();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CHDHRProxyIPTVDlg dialog

CHDHRProxyIPTVDlg::CHDHRProxyIPTVDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHDHRProxyIPTVDlg::IDD, pParent)
{
	m_trace = new CTrace();

	EnableActiveAccessibility();
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_ServiceWindows = new CServiceW();

	m_CfgProxy = CConfigProxy::GetInstance();
	m_CfgProxy->m_trace = new CTrace();
	
	m_CfgProxy->setDlg(this);

	//Read data of configuration of INI file. If the INI file not exits or it doesn't have somo value, it will get the default value
	m_CfgProxy->ReadFileINI();

	m_GestorProxy = new CGestionProxy();

	if (m_CfgProxy->m_autoCleanLog)
	{
		m_GestorProxy->CleanLog();
		//m_trace->WriteTrace("CLEANING LOG\n", LEVEL_TRZ_1);
		LOGM(TRZ1,0,"CLEANING LOG\n");
	}
}

void CHDHRProxyIPTVDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_IP_CLI_HDHR, m_eIPDHR);
	DDX_Control(pDX, IDC_EDIT_PUERTO_CLI_HDHR, m_ePortCliHDHR);
	DDX_Control(pDX, IDC_BUTTON_INST_SERVICIO, m_btnInstServ);
	DDX_Control(pDX, IDC_BUTTON_DESINST_SERV, m_btnDesInstServ);
	DDX_Control(pDX, IDC_BUTTON_INICIAR_SERV, m_btnStartServ);
	DDX_Control(pDX, IDC_BUTTON_PARAR_SERV, m_btnStopServ);
	DDX_Control(pDX, IDC_INICIAR_PROXY, m_btnStartProxy);
	DDX_Control(pDX, IDC_BUTTON_PARAR_PROXY, m_btnStopProxy);
	DDX_Control(pDX, IDC_COMBO_NIVEL_TRAZA, m_comboLevelTrc);
	DDX_Control(pDX, IDC_EDIT_DEVICE_ID, m_eIdDispositive);
	DDX_Control(pDX, IDC_BUTTON_GUARDAR, m_btnSaveConfigProxy);
	DDX_Control(pDX, IDC_BUTTON_TRZ_LV, m_btnChangeLevelTrc);
	DDX_Control(pDX, IDC_BUTTON_CLEAN_LOG, m_btnCleanLog);
	DDX_Control(pDX, IDC_EDIT_FILE_MAPLIST, m_MapListFile);
	DDX_Control(pDX, IDC_BUTTON_LOAD_MAPLIST, m_btnReloadMapList);
	DDX_Control(pDX, IDC_TAB_TUNERS, m_tabTuners);
	DDX_Control(pDX, IDC_COMBO_TUN_LOCK, m_tunerLock);
	DDX_Control(pDX, IDC_COMBO_AUTOSTART, m_autostart);
	DDX_Control(pDX, IDC_COMBO_NUM_TUNERS, m_numTuners);
	DDX_Control(pDX, IDC_EDIT_TRACE_LOG, m_traceLog);
	DDX_Control(pDX, IDC_EDIT_FILE_CONFIG, m_ConfigFile);
	DDX_Control(pDX, IDC_BUTTON_RESET_TUNER, m_btnResetTuner);
	DDX_Control(pDX, IDC_BUTTON_FORCE_UNLOCK, m_btnForceUnlock);
}

BEGIN_MESSAGE_MAP(CHDHRProxyIPTVDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CHDHRProxyIPTVDlg::OnBnClickedOk)
	ON_EN_KILLFOCUS(IDC_EDIT_PUERTO_CLI_HDHR, &CHDHRProxyIPTVDlg::OnEnChangeEditPort)
	ON_BN_CLICKED(IDC_BUTTON_INST_SERVICIO, &CHDHRProxyIPTVDlg::OnBnClickedButtonInstService)
	ON_BN_CLICKED(IDC_BUTTON_DESINST_SERV, &CHDHRProxyIPTVDlg::OnBnClickedButtonDesinstServ)
	ON_BN_CLICKED(IDC_BUTTON_INICIAR_SERV, &CHDHRProxyIPTVDlg::OnBnClickedButtonStartServ)
	ON_BN_CLICKED(IDC_BUTTON_PARAR_SERV, &CHDHRProxyIPTVDlg::OnBnClickedButtonStopServ)
	ON_EN_KILLFOCUS(IDC_EDIT_IP_CLI_HDHR, &CHDHRProxyIPTVDlg::OnEnChangeEditMulticast)
	ON_BN_CLICKED(IDC_BUTTON_GUARDAR, &CHDHRProxyIPTVDlg::OnBnClickedButtonSave)
	ON_BN_CLICKED(IDC_INICIAR_PROXY, &CHDHRProxyIPTVDlg::OnBnClickedStartProxy)
	ON_BN_CLICKED(IDC_BUTTON_PARAR_PROXY, &CHDHRProxyIPTVDlg::OnBnClickedButtonStopProxy)
	ON_BN_CLICKED(IDC_BUTTON_TRZ_LV, &CHDHRProxyIPTVDlg::OnBnClickedButtonChangeLevelTrc)
	ON_BN_CLICKED(IDC_BUTTON_CLEAN_LOG, &CHDHRProxyIPTVDlg::OnBnClickedButtonCleanLog)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_MAPLIST, &CHDHRProxyIPTVDlg::OnBnClickedButtonReloadMappingList)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_TUNERS, &CHDHRProxyIPTVDlg::OnTcnSelchangeTabTuners)
	ON_BN_CLICKED(IDC_BUTTON_RESET_TUNER, &CHDHRProxyIPTVDlg::OnBnClickedButtonResetTuner)
	ON_BN_CLICKED(IDC_BUTTON_FORCE_UNLOCK, &CHDHRProxyIPTVDlg::OnBnClickedButtonForceUnlock)
END_MESSAGE_MAP()

void CHDHRProxyIPTVDlg::OnTcnSelchangeTabTuners(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	m_tabTuners.OnSelchange(pNMHDR, pResult);
	
	UpdateInfoTuner();

	*pResult = 0;
}

void CHDHRProxyIPTVDlg::UpdateInfoTuner()
{
	m_CurrentTuner = m_tabTuners.GetCurSel();
	m_GestorProxy->UpdateInfoCliAtInterface(m_CurrentTuner);

	m_tabTuners.UpdateDataTuner();
}

BOOL CHDHRProxyIPTVDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Data is loaded on screen

	//  HDHR Section
	
	//ID device of HDHR Server: It gets the default device id defined in code but it can be changed
	m_eIdDispositive.SetWindowText(m_CfgProxy->m_device_id);
	
	//IP defined for HDHR Server
	m_eIPDHR.SetWindowText(m_CfgProxy->m_hdhrServerIP);
	
	//Listening port as HDHR Server.In this case the defined by HDHR is shown.
	//It do not get from file, is fixed
	int port = m_GestorProxy->getDiscovery()->ObtainHDHRServPort();
	CString p;
	p.Format(L"%d", port);
	m_ePortCliHDHR.SetWindowText(p);

	m_tunerLock.AddString(L"YES");
	m_tunerLock.AddString(L"NO");
	if (m_CfgProxy->m_Lock)
		m_tunerLock.SetCurSel(1);
	else
		m_tunerLock.SetCurSel(0);

	m_autostart.AddString(L"YES");
	m_autostart.AddString(L"NO");
	if (m_CfgProxy->m_autostart)
		m_autostart.SetCurSel(1);
	else
		m_autostart.SetCurSel(0);

	//Initialize Combo of number of tuners
	CString numTun;
	for (int i = 1; i<=9; i++)
	{
		numTun.Format(L"%d", i);
		m_numTuners.AddString(numTun);
	}
	//Indicates by default the level 4 
	m_numTuners.SetCurSel(m_CfgProxy->m_TunersNumber-1);

	//  TRACE Section
	//Initialize Combo of trace level
	CString level;
	for (int i = 0; i<7; i++)
	{
		level.Format(L"%d",i);
		m_comboLevelTrc.AddString(level);
	}
	//Indicates by default the level 4 
	m_comboLevelTrc.SetCurSel(m_CfgProxy->m_traceLevel);

	TCHAR sDirActual[200];
	GetCurrentDirectory(200, sDirActual);
	CString path;
	path.Format(L"%s\\%s", sDirActual, _T(NAME_FILE_MAPLIST));
	m_MapListFile.SetWindowText(path);

	path.Format(L"%s\\%s", sDirActual, _T(NAME_FILE_INI));
	m_ConfigFile.SetWindowText(path);

	m_traceLog.SetWindowText(CString(TRACE_FILE_NAME));
	
	/************** Tabs Tuner  *************************************/
	m_tabTuners.InitDialogs(m_CfgProxy->getTunersNumber());
	m_tabTuners.ActivateTabDialogs();
	UpdateInfoTuner();
	/***************************************/
	
	m_btnStopProxy.EnableWindow(FALSE);

	int servInstalado = m_ServiceWindows->IsServiceInstalled();
	if (servInstalado == 1)
	{
		m_btnInstServ.EnableWindow(FALSE);
		if (m_ServiceWindows->ObtainStateService() == SERVICE_RUNNING)
			m_btnStartServ.EnableWindow(FALSE);
		else
			m_btnStopServ.EnableWindow(FALSE);
	}
	else if (servInstalado == 0)
	{
		m_btnDesInstServ.EnableWindow(FALSE);
		m_btnStartServ.EnableWindow(FALSE);
		m_btnStopServ.EnableWindow(FALSE);
	}

	/********* The buttons that by moment are not used ara disabled. ***************/
	//Some buttons are disabled directly at the Dialog (file .rc). Option Disabled
	m_btnInstServ.EnableWindow(FALSE);
	m_btnDesInstServ.EnableWindow(FALSE);
	m_btnStartServ.EnableWindow(FALSE);
	m_btnStopServ.EnableWindow(FALSE);
	/***************************************************************************/


	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);

		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	if (m_CfgProxy->m_autostart)
		OnBnClickedStartProxy();

	m_CfgProxy->AddClientToInterface();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHDHRProxyIPTVDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CHDHRProxyIPTVDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CHDHRProxyIPTVDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CHDHRProxyIPTVDlg::OnBnClickedOk()
{
	m_GestorProxy->StopProxy();

	CDialogEx::OnOK();

	CHDHRProxyIPTVDlg::OnDestroy();
}

void CHDHRProxyIPTVDlg::OnEnChangeEditMulticast()
{
	CString ip;
	m_eIPDHR.GetWindowText(ip);
	m_CfgProxy->setMulticast(ip);
}

void CHDHRProxyIPTVDlg::OnEnChangeEditPort()
{
	CString p;
	m_ePortCliHDHR.GetWindowTextW(p);
	m_CfgProxy->setPort(p);
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonInstService()
{
	if (!m_ServiceWindows->InstallService())
		AfxMessageBox(_T("Error when installation the Service"), MB_OK);
	else
	{
		m_btnInstServ.EnableWindow(FALSE);
		m_btnStartServ.EnableWindow(FALSE);
		m_btnStopServ.EnableWindow(TRUE);
		m_btnDesInstServ.EnableWindow(TRUE);
	}
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonDesinstServ()
{
	if (!m_ServiceWindows->StopService())
		AfxMessageBox(_T("Error when stop the service"), MB_OK);

	if (!m_ServiceWindows->DesInstallService())
	{
		AfxMessageBox(_T("Error when uninstall the service"), MB_OK);
		char textErr[1024];
		memset(textErr, 0, 1024);
		CString sErr;
		int err = GetLastError();
		_snprintf(textErr, sizeof(textErr) - 2, "Err %d", err);
		sErr.Format(L"Error %d", err);
		AfxMessageBox(sErr, MB_OK);
		m_ServiceWindows->ObtainError(err);
	}
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonStartServ()
{
	if (!m_ServiceWindows->StartServiceWin())
		AfxMessageBox(_T("Error when start the service"), MB_OK);
	else
	{
		m_btnStartServ.EnableWindow(FALSE);
		m_btnDesInstServ.EnableWindow(TRUE);
		m_btnStartServ.EnableWindow(FALSE);
		m_btnStopServ.EnableWindow(TRUE);
	}
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonStopServ()
{
	if (!m_ServiceWindows->StopService())
		AfxMessageBox(_T("Error when stop the service"), MB_OK);
	else
	{
		m_btnStartServ.EnableWindow(TRUE);
		m_btnStopServ.EnableWindow(FALSE);
	}
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonSave()
{
	m_eIdDispositive.GetWindowText(m_CfgProxy->m_device_id);
	m_eIPDHR.GetWindowText(m_CfgProxy->m_hdhrServerIP);
	m_ePortCliHDHR.GetWindowText(m_CfgProxy->m_hdhrServerPuerto);

	//Number of Tuners is saved to m_CfgProxy->m_TunersNumberToSaveINI and not in m_TunersNumber
	//because if this value change it will apply when the application restart.
	m_CfgProxy->m_TunersNumberToSaveINI = m_numTuners.GetCurSel()+1;

	int tunLock = m_tunerLock.GetCurSel();
	if (tunLock == 1)
		m_CfgProxy->m_Lock = 1;
	else
		m_CfgProxy->m_Lock = 0;

	if (m_autostart.GetCurSel() == 1)
		m_CfgProxy->m_autostart = 1;
	else
		m_CfgProxy->m_autostart = 0;

	m_CfgProxy->m_traceLevel = m_comboLevelTrc.GetCurSel();

	UpdateData(TRUE);
		
	if (m_CfgProxy->SaveItems() == 2)
	{
		int res = AfxMessageBox(_T("Changes of \"AutoStart\" and \"Number of Tuners\" need restart the application to be applied.\n Restart application now? "), MB_YESNO);

		if (res == IDYES)
		{
			int a = 0;
			OnBnClickedOk();

			HANDLE hmutex = theApp.getMutex();

			//Close mutex to open de application
			if (hmutex)
			{
				CloseHandle(hmutex);
			}

			PROCESS_INFORMATION ProcessInfo; //out parameter
			STARTUPINFO StartupInfo; //in parameter

			ZeroMemory(&StartupInfo, sizeof(StartupInfo));
			StartupInfo.cb = sizeof StartupInfo;


			if (CreateProcess(L"HDHRProxyIPTV.exe", NULL,
				NULL, NULL, FALSE, 0, NULL,
				NULL, &StartupInfo, &ProcessInfo))
			{
				WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
				CloseHandle(ProcessInfo.hThread);
				CloseHandle(ProcessInfo.hProcess);
			}
			else
			{
				AfxMessageBox(L"Application could not be started", MB_OK);
			}
		}
	}

	//m_trace->WriteTrace("Configuration saved in Config INI file\n",LEVEL_TRZ_1);
	LOGM(TRZ1,0,"SAVE CONFIG IN INI FILE\n");
}

void CHDHRProxyIPTVDlg::OnBnClickedStartProxy()
{
	//Update states of buttons
	m_btnStartProxy.EnableWindow(FALSE);
	m_btnStopProxy.EnableWindow(TRUE);
	m_btnSaveConfigProxy.EnableWindow(FALSE);

	//Save the data
	OnBnClickedButtonSave();

	//Load data from Mapping List file
	m_CfgProxy->ReadFileMappingList();

	//Obtain the level of trace that must apply
	int nivelTraza = m_comboLevelTrc.GetCurSel();

	//Obtain id device of HDHR Server for the Proxy, for if it has changed
	CString idDisp, ip;
	m_eIdDispositive.GetWindowText(idDisp);
	m_eIPDHR.GetWindowText(ip);

	//Initilize Proxy
	if (!m_GestorProxy->InitializeProxy(nivelTraza, idDisp, ip))
	{
		m_btnStartProxy.EnableWindow(TRUE);
		m_btnStopProxy.EnableWindow(FALSE);
		m_btnSaveConfigProxy.EnableWindow(TRUE);
		AfxMessageBox(_T("Proxy failed to start. Retry."), MB_OK);
	}
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonStopProxy()
{
	m_GestorProxy->ForceUnlockTuner(m_CurrentTuner);
	m_GestorProxy->ResetTuner(m_CurrentTuner);
	m_GestorProxy->StopProxy();

	m_btnStartProxy.EnableWindow(TRUE);
	m_btnStopProxy.EnableWindow(FALSE);
	m_btnSaveConfigProxy.EnableWindow(TRUE);
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonChangeLevelTrc()
{
	int new_value = m_comboLevelTrc.GetCurSel();
	int old_value = m_CfgProxy->m_traceLevel;

	//if (m_trace->IsLevelWriteable(LEVEL_TRZ_1))
	//{
	//	char log_output[500];
	//	memset(log_output, 0, 500);
	//	_snprintf(log_output, sizeof(log_output) - 2, "CHANGING LOG LEVEL FROM %d TO %d\n", old_value, new_value);
	//	m_trace->WriteTrace(log_output, LEVEL_TRZ_1);
	//}
	LOGM(TRZ1,1,"CHANGING LOG LEVEL FROM %d TO %d\n", old_value, new_value);

	m_CfgProxy->m_traceLevel = new_value;
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonCleanLog()
{
	m_GestorProxy->CleanLog();

	//m_trace->WriteTrace("CLEANING LOG\n", LEVEL_TRZ_1);
	LOGM(TRZ1,0,"CLEANING LOG\n");
}

void CHDHRProxyIPTVDlg::OnDestroy()
{
	//m_trace->WriteTrace("******************* Kill Proxy SATIP2HDHR *******************\n", LEVEL_TRZ_1);
	LOGM(TRZ1,0,"******************* KILL APPLICATION *******************\n");

	m_GestorProxy->StopProxy();

	CDialogEx::OnDestroy();
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonReloadMappingList()
{
	CString chans, ch;

	//Load data from Mapping List file
	//m_trace->WriteTrace("Reading Mapping List\n", LEVEL_TRZ_1);
	LOGM(TRZ1,0,"READING MAPPING LIST\n");
	m_CfgProxy->ReadFileMappingList();

	chans.Format(L"[");
	for (int i = 0; i < m_CfgProxy->m_numChannels; i++)
	{
		ch.Format(L"%d", m_CfgProxy->m_infoChannels[i].channel);
		chans.Append(ch);
		if (i<m_CfgProxy->m_numChannels - 1)
			chans.Append(L";");
	}
	chans.Append(L"]");
	
	//if (m_trace->IsLevelWriteable(LEVEL_TRZ_1))
	//{
	//	char log_output[2048];
	//	memset(log_output, 0, 2048);
	//	_snprintf(log_output, sizeof(log_output) - 2, "Reload of MappingList: Number of channels in Mapping List File: %d %s\n", m_CfgProxy->m_numChannels, CStringA(chans));
	//	m_trace->WriteTrace(log_output, LEVEL_TRZ_1);
	//}
	LOGM(TRZ1,0,"Reload of MappingList: Number of channels in Mapping List File: %d %s\n", m_CfgProxy->m_numChannels, CStringA(chans));
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonResetTuner()
{
	m_GestorProxy->ForceUnlockTuner(m_CurrentTuner);
	m_GestorProxy->ResetTuner(m_CurrentTuner);
}

void CHDHRProxyIPTVDlg::OnBnClickedButtonForceUnlock()
{
	m_GestorProxy->ForceUnlockTuner(m_CurrentTuner);
}
