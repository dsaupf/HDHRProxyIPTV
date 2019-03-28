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


// HDHRProxyIPTVDlg.h : header file
//

#pragma once

#include "ServiceW.h"
#include "ConfigProxy.h"
#include "GestionProxy.h"
#include "PanelInfoTunerDlg.h"
#include "TabTunersCtrl.h"


class CHDHRProxyIPTVDlg : public CDialogEx
{
// Construction
public:

	CConfigProxy *m_CfgProxy;
	CGestionProxy *m_GestorProxy;
	CServiceW *m_ServiceWindows;
	CTrace *m_trace;
	int m_CurrentCli;
	int m_CurrentTuner;

	CPanelInfoTunerDlg* m_panelInfoTuner;

	CHDHRProxyIPTVDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_HDHRProxyIPTV_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeEditPort();
	afx_msg void OnBnClickedButtonInstService();
	void ObtainHostIP(CString *ip);
	void UpdateInfoTuner();

//private:
public:
	CEdit m_eIPDHR;
	CEdit m_ePortCliHDHR;
	CEdit m_eIdDispositive;
	CComboBox m_tunerLock;
	CComboBox m_autostart;
	CComboBox m_numTuners;
	CEdit m_traceLog;
	CEdit m_ConfigFile;
	CEdit m_MapListFile;
	CButton m_btnSaveConfigProxy;
	CComboBox m_comboLevelTrc;
	CButton m_btnChangeLevelTrc;
	CButton m_btnCleanLog;
	CButton m_btnReloadMapList;
	CButton m_btnResetTuner;
	CButton m_btnForceUnlock;

	CTabTunersCtrl m_tabTuners;

	CButton m_btnInstServ;
	CButton m_btnDesInstServ;
	CButton m_btnStartServ;
	CButton m_btnStopServ;
	CButton m_btnStartProxy;
	CButton m_btnStopProxy;

public:
	afx_msg void OnBnClickedButtonDesinstServ();
	afx_msg void OnBnClickedButtonStartServ();
	afx_msg void OnBnClickedButtonStopServ();
	afx_msg void OnEnChangeEditMulticast();
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnEnChangeEditIpCliHdhr();
	afx_msg void OnBnClickedStartProxy();
	afx_msg void OnBnClickedButtonStopProxy();
	afx_msg void OnBnClickedButtonChangeLevelTrc();
	afx_msg void OnBnClickedButtonCleanLog();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonReloadMappingList();
	afx_msg void OnTcnSelchangeTabTuners(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonResetTuner();
	afx_msg void OnBnClickedButtonForceUnlock();

#define LOGM(level, only, text, ...) LOGG(m_trace, "#############", level, only, text, ##__VA_ARGS__)    
};
