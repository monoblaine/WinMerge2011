/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  OpenDlg.h
 *
 * @brief Declaration file for COpenDlg dialog
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#pragma once

/////////////////////////////////////////////////////////////////////////////
// COpenDlg dialog
#include "SuperComboBox.h"

class ProjectFile;

/**
 * @brief The Open-dialog class.
 * The Open-dialog allows user to select paths to compare. In addition to
 * the two paths, there are controls for selecting filter.
 * If one of the paths is a project file, that projec file is loaded,
 * overwriting possible other values in other dialog controls.
 * The dialog shows also a status of the selected paths (found/not found),
 * if enabled in the options (enabled by default).
 */
class COpenDlg
	: ZeroInit<COpenDlg>
	, public OResizableDialog
{
// Construction
public:

	COpenDlg();   // standard constructor
	~COpenDlg();

// Dialog Data
	HButton			*m_pPbOk;
	HSuperComboBox	*m_pCbLeft;
	HSuperComboBox	*m_pCbRight;
	HSuperComboBox	*m_pCbExt;
	HComboBox		*m_pCbCompareAs;
	HMenu			*m_pCompareAsScriptMenu;
	String	m_strLeft;
	String	m_strRight;
	String	m_strExt;
	BOOL	m_bRecurse;
	bool m_bLeftReadOnly; /**< Is left path opened as read-only */
	bool m_bRightReadOnly; /**< Is right path opened as read-only */
	BOOL m_bOverwriteRecursive;  /**< If TRUE overwrite last used value of recursive */
	UINT m_idCompareAs;
	TCHAR m_szCompareAs[MAX_PATH];

// Implementation data
private:
	enum
	{
		AUTO_COMPLETE_DISABLED,
		AUTO_COMPLETE_FILE_SYSTEM,
		AUTO_COMPLETE_RECENTLY_USED
	};
	const int m_nAutoComplete;

// Implementation
protected:
	void UpdateButtonStates();
	void SetStatus(UINT msgID);
	BOOL LoadProjectFile(LPCTSTR path);
	void TrimPaths();

	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	LRESULT OnNotify(UNotify *);

	void OnDestroy();
	void OnBrowseButton(UINT id);
	void OnOK();
	void OnCancel();
	void SaveComboboxStates();
	void OnSelchangePathCombo(HSuperComboBox *);
	void OnEditchangePathCombo(HSuperComboBox *);
	void OnEditEvent();
	void OnTimer(UINT_PTR);
	void OnSelectFilter();
	void OnActivate(UINT nState, HWND hWndOther, BOOL bMinimized);
	void OnHelp();
	void OnDropFiles(HDROP dropInfo);
};
