/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
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
 * @file  OpenDlg.cpp
 *
 * @brief Implementation of the COpenDlg class
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "paths.h"
#include "Merge.h"
#include "OpenDlg.h"
#include "coretools.h"
#include "environment.h"
#include "OptionsDef.h"
#include "MainFrm.h"
#include "OptionsMgr.h"
#include "SettingStore.h"
#include "LanguageSelect.h"
#include "FileOrFolderSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Timer ID and timeout for delaying path validity check
const UINT IDT_CHECKFILES = 1;
const UINT CHECKFILES_TIMEOUT = 1000; // milliseconds

/** @brief Location for Open-dialog specific help to open. */
static const TCHAR OpenDlgHelpLocation[] = _T("::/htmlhelp/Open_paths.html");

/////////////////////////////////////////////////////////////////////////////
// COpenDlg dialog

static DWORD_PTR GetShellImageList()
{
	SHFILEINFO sfi;
	static DWORD_PTR dwpShellImageList = SHGetFileInfo(
		_T(""), 0,	&sfi, sizeof sfi, SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
	return dwpShellImageList;
}

/**
 * @brief Standard constructor.
 */
COpenDlg::COpenDlg()
	: OResizableDialog(IDD_OPEN)
	, m_idCompareAs(0)
	, m_nAutoComplete(COptionsMgr::Get(OPT_AUTO_COMPLETE_SOURCE))
{
	static const LONG FloatScript[] =
	{
		IDC_LEFT_COMBO,			BY<1000>::X2R,
		IDC_RIGHT_COMBO,		BY<1000>::X2R,
		IDC_EXT_COMBO,			BY<1000>::X2R,
		IDC_COMPARE_AS_COMBO,	BY<1000>::X2R,
		IDC_RECURS_CHECK,		BY<1000>::X2R,
		IDC_FILES_DIRS_GROUP,	BY<1000>::X2R,
		IDC_LEFT_BUTTON,		BY<1000>::X2L | BY<1000>::X2R,
		IDC_RIGHT_BUTTON,		BY<1000>::X2L | BY<1000>::X2R,
		IDC_OPEN_STATUS,		BY<1000>::X2R,
		IDC_SELECT_FILTER,		BY<1000>::X2L | BY<1000>::X2R,
		IDOK,					BY<1000>::X2L | BY<1000>::X2R,
		IDCANCEL,				BY<1000>::X2L | BY<1000>::X2R,
		ID_HELP,				BY<1000>::X2L | BY<1000>::X2R,
		0
	};
	CFloatState::FloatScript = FloatScript;
}

/**
 * @brief Standard destructor.
 */
COpenDlg::~COpenDlg()
{
}

template<ODialog::DDX_Operation op>
bool COpenDlg::UpdateData()
{
	DDX_CBStringExact<op>(IDC_LEFT_COMBO, m_sLeftFile);
	DDX_CBStringExact<op>(IDC_RIGHT_COMBO, m_sRightFile);
	DDX_CBStringExact<op>(IDC_EXT_COMBO, m_sFilter);
	DDX_Check<op>(IDC_RECURS_CHECK, m_nRecursive);
	return true;
}

LRESULT COpenDlg::OnNotify(UNotify *pNM)
{
	switch (pNM->HDR.idFrom)
	{
	case IDC_LEFT_COMBO:
	case IDC_RIGHT_COMBO:
		switch (pNM->HDR.code)
		{
		case CBEN_GETDISPINFO:
			SHFILEINFO sfi;
			DWORD_PTR result = 0;
			if (pNM->COMBOBOXEX.ceItem.iItem == 0 && !pNM->pCB->GetDroppedState())
			{
				pNM->pCB->GetWindowText(pNM->COMBOBOXEX.ceItem.pszText, pNM->COMBOBOXEX.ceItem.cchTextMax);
			}
			if (paths_EndsWithSlash(pNM->COMBOBOXEX.ceItem.pszText))
			{
				TCHAR path[MAX_PATH];
				GetWindowsDirectory(path, MAX_PATH);
				if (LPCTSTR p = PathSkipRoot(pNM->COMBOBOXEX.ceItem.pszText))
				{
					if (*p == _T('\0'))
					{
						*PathSkipRoot(path) = _T('\0');
					}
					result = SHGetFileInfo(path, 0, &sfi, sizeof sfi, SHGFI_SYSICONINDEX);
				}
			}
			else
			{
				LPCTSTR path = PathFindExtension(pNM->COMBOBOXEX.ceItem.pszText);
				sfi.dwAttributes = FILE_ATTRIBUTE_NORMAL;
				result = SHGetFileInfo(path, 0, &sfi, sizeof sfi, SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
			}
			if (result != 0)
			{
				pNM->COMBOBOXEX.ceItem.iImage = sfi.iIcon;
				pNM->COMBOBOXEX.ceItem.iSelectedImage = sfi.iIcon;
			}
			break;
		}
		break;
	}
	return 0;
}

LRESULT COpenDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		OnDestroy();
		break;
	case WM_TIMER:
		OnTimer(wParam);
		break;
	case WM_ACTIVATE:
		OnActivate(LOWORD(wParam), reinterpret_cast<HWND>(lParam), HIWORD(wParam));
		break;
	case WM_DROPFILES:
		OnDropFiles(reinterpret_cast<HDROP>(wParam));
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			OnOK();
			break;
		case IDCANCEL:
			OnCancel();
			break;
		case IDC_RECURS_CHECK:
			Update3StateCheckBoxLabel(IDC_RECURS_CHECK);
			break;
		case MAKEWPARAM(IDC_LEFT_BUTTON, BN_CLICKED):
			OnBrowseButton(IDC_LEFT_COMBO);
			break;
		case MAKEWPARAM(IDC_RIGHT_BUTTON, BN_CLICKED):
			OnBrowseButton(IDC_RIGHT_COMBO);
			break;
		case MAKEWPARAM(IDC_SELECT_FILTER, BN_CLICKED):
			OnSelectFilter();
			break;
		case MAKEWPARAM(IDC_LEFT_COMBO, CBN_SELCHANGE):
		case MAKEWPARAM(IDC_RIGHT_COMBO, CBN_SELCHANGE):
			OnSelchangePathCombo(reinterpret_cast<HSuperComboBox *>(lParam));
			break;
		case MAKEWPARAM(IDC_LEFT_COMBO, CBN_EDITCHANGE):
		case MAKEWPARAM(IDC_RIGHT_COMBO, CBN_EDITCHANGE):
			OnEditchangePathCombo(reinterpret_cast<HSuperComboBox *>(lParam));
			break;
		case MAKEWPARAM(IDC_EXT_COMBO, CBN_EDITCHANGE):
			if (m_pCbExt->GetWindowTextLength() == 0)
			{
				int index = m_pCbExt->FindStringExact(-1, _T("*.*"));
				if (index != CB_ERR)
				{
					m_pCbExt->SetCurSel(index);
				}
				else
				{
					m_pCbExt->SetWindowText(_T("*.*"));
					m_pCbExt->SendDlgItemMessage(1001, EM_SETSEL, 0, 0xFFFF);
				}
			}
			break;
		case MAKEWPARAM(IDC_LEFT_COMBO, CBN_SELENDCANCEL):
		case MAKEWPARAM(IDC_RIGHT_COMBO, CBN_SELENDCANCEL):
			UpdateButtonStates();
			break;
		case MAKEWPARAM(IDC_LEFT_COMBO, CBN_DROPDOWN):
		case MAKEWPARAM(IDC_RIGHT_COMBO, CBN_DROPDOWN):
			// Remove the icon issue placeholder.
			reinterpret_cast<HSuperComboBox *>(lParam)->DeleteString(0);
			// fall through
		case MAKEWPARAM(IDC_EXT_COMBO, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			RegisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE), MOD_SHIFT, VK_DELETE);
			break;
		case MAKEWPARAM(IDC_LEFT_COMBO, CBN_CLOSEUP):
		case MAKEWPARAM(IDC_RIGHT_COMBO, CBN_CLOSEUP):
			// Restore the icon issue placeholder.
			reinterpret_cast<HSuperComboBox *>(lParam)->InsertString(0, LPSTR_TEXTCALLBACK);
			reinterpret_cast<HSuperComboBox *>(lParam)->EnsureSelection();
			// fall through
		case MAKEWPARAM(IDC_EXT_COMBO, CBN_CLOSEUP):
			UnregisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE));
			break;
		}
		break;
	case WM_HOTKEY:
		if (HIWORD(wParam) == VK_DELETE)
		{
			HSuperComboBox *const pCb = static_cast<HSuperComboBox *>(GetDlgItem(LOWORD(wParam)));
			pCb->DeleteString(pCb->GetCurSel());
			pCb->SetWindowText(_T(""));
			SendMessage(WM_COMMAND, MAKEWPARAM(LOWORD(wParam), CBN_EDITCHANGE), reinterpret_cast<LPARAM>(pCb));
		}
		break;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(reinterpret_cast<UNotify *>(lParam)))
			return lResult;
		break;
	case WM_HELP:
		theApp.m_pMainWnd->ShowHelp(OpenDlgHelpLocation);
		return 0;
	}
	return OResizableDialog::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// COpenDlg message handlers

/**
 * @brief Handler for WM_INITDIALOG; conventional location to initialize controls
 * At this point dialog and control windows exist
 */
BOOL COpenDlg::OnInitDialog()
{
	OResizableDialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	m_pPbOk = static_cast<HButton *>(GetDlgItem(IDOK));

	m_pCbLeft = static_cast<HSuperComboBox *>(GetDlgItem(IDC_LEFT_COMBO));
	m_pCbRight = static_cast<HSuperComboBox *>(GetDlgItem(IDC_RIGHT_COMBO));
	m_pCbExt = static_cast<HSuperComboBox *>(GetDlgItem(IDC_EXT_COMBO));

	m_pCbCompareAs = static_cast<HComboBox *>(GetDlgItem(IDC_COMPARE_AS_COMBO));
	m_pCbCompareAs->AddString(_T(""));
	m_pCbCompareAs->SetCurSel(0);

	m_pCbLeft->LoadState(_T("Files\\Left"));
	m_pCbRight->LoadState(_T("Files\\Right"));
	m_pCbExt->LoadState(_T("Files\\Ext"));

	if (HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_DIRVIEW))
	{
		// 1st submenu of IDR_POPUP_DIRVIEW is for item popup
		HMenu *const pPopup = pMenu->GetSubMenu(0)->GetSubMenu(1);
		m_pCompareAsScriptMenu = theApp.m_pMainWnd->SetScriptMenu(pPopup, "CompareAs.Menu");
		int n = pPopup->GetMenuItemCount();
		for (int i = 0 ; i < n ; ++i)
		{
			TCHAR text[INFOTIPSIZE];
			pPopup->GetMenuString(i, text, _countof(text), MF_BYPOSITION);
			SHStripMneumonic(text);
			int j = m_pCbCompareAs->AddString(text);
			UINT id = pPopup->GetMenuItemID(i);
			m_pCbCompareAs->SetItemData(j, id);
		}
		theApp.m_pMainWnd->SetScriptMenu(pPopup, NULL);
		pMenu->DestroyMenu();
	}

	if (DWORD_PTR dwpShellImageList = GetShellImageList())
	{
		m_pCbLeft->SendMessage(CBEM_SETIMAGELIST, 0, dwpShellImageList);
		m_pCbRight->SendMessage(CBEM_SETIMAGELIST, 0, dwpShellImageList);
	}

	if (m_nAutoComplete == AUTO_COMPLETE_FILE_SYSTEM)
	{
		m_pCbLeft->GetEditControl()->SHAutoComplete(SHACF_FILESYSTEM);
		m_pCbRight->GetEditControl()->SHAutoComplete(SHACF_FILESYSTEM);
	}

	String filterString = COptionsMgr::Get(OPT_FILEFILTER_CURRENT);
	if (filterString.empty())
		filterString = _T("*.*");
	int index = m_pCbExt->FindStringExact(-1, filterString.c_str());
	if (index == CB_ERR)
		index = m_pCbExt->InsertString(0, filterString.c_str());
	m_pCbExt->SetCurSel(index);

	if (!m_bOverwriteRecursive)
		m_nRecursive = SettingStore.GetProfileInt(_T("Settings"), _T("Recurse"), 0);

	if (m_sLeftFile.empty())
		m_pCbLeft->GetWindowText(m_sLeftFile);
	else
		m_sLeftFile.erase(0, paths_UndoMagic(&m_sLeftFile.front()) - m_sLeftFile.c_str());

	if (m_sRightFile.empty())
		m_pCbRight->GetWindowText(m_sRightFile);
	else
		m_sRightFile.erase(0, paths_UndoMagic(&m_sRightFile.front()) - m_sRightFile.c_str());

	if (m_sFilter.empty())
		m_pCbExt->GetWindowText(m_sFilter);

	UpdateData<Set>();
	Update3StateCheckBoxLabel(IDC_RECURS_CHECK);

	// Insert placeholders to represent items which are not yet listed as MRU.
	// These placeholders are removed upon dropdown and restored upon closeup.
	// They are invisible to users, but exist merely to help fix icon issues.
	m_pCbLeft->InsertString(0, LPSTR_TEXTCALLBACK);
	m_pCbRight->InsertString(0, LPSTR_TEXTCALLBACK);

	UpdateButtonStates();
	return TRUE;
}

void COpenDlg::OnDestroy()
{
	if (m_pCompareAsScriptMenu)
	{
		m_pCompareAsScriptMenu->DestroyMenu();
		m_pCompareAsScriptMenu = NULL;
	}
}

/** 
 * @brief Called when "Browse..." button is selected for left or right path.
 */
void COpenDlg::OnBrowseButton(UINT id) 
{
	String sfolder;
	GetDlgItemText(id, sfolder);
	if (paths_DoesPathExist(sfolder.c_str()) == IS_EXISTING_FILE)
		sfolder = GetPathOnly(sfolder.c_str());
	String s;
	if (SelectFileOrFolder(m_hWnd, s, sfolder.c_str()))
	{
		SetDlgItemText(id, s.c_str());
		UpdateButtonStates();
	}	
}

/** 
 * @brief Called when dialog is closed with "OK".
 *
 * Checks that paths are valid and sets filters.
 */
void COpenDlg::OnOK() 
{
	UpdateData<Get>();
	TrimPaths();

	// If left path is a project-file, load it
	if (m_sRightFile.empty() && ProjectFile::IsProjectFile(m_sLeftFile.c_str()))
		if (!ProjectFile::Read(m_sLeftFile.c_str()))
			return;

	m_attrLeft = GetFileAttributes(m_sLeftFile.c_str());
	m_attrRight = GetFileAttributes(m_sRightFile.c_str());
	if ((m_attrLeft == INVALID_FILE_ATTRIBUTES) || (m_attrRight == INVALID_FILE_ATTRIBUTES))
	{
		LanguageSelect.MsgBox(IDS_ERROR_INCOMPARABLE, MB_ICONSTOP);
		return;
	}

	m_sLeftFile = paths_GetLongPath(m_sLeftFile.c_str());
	m_sRightFile = paths_GetLongPath(m_sRightFile.c_str());

	// Add trailing '\' for directories if its missing
	if ((m_attrLeft & FILE_ATTRIBUTE_DIRECTORY) && !paths_EndsWithSlash(m_sLeftFile.c_str()))
		m_sLeftFile += _T('\\');
	if ((m_attrRight & FILE_ATTRIBUTE_DIRECTORY) && !paths_EndsWithSlash(m_sRightFile.c_str()))
		m_sRightFile += _T('\\');

	UpdateData<Set>();
	// Hide magic prefix from users
	m_pCbLeft->SetWindowText(paths_UndoMagic(&String(m_sLeftFile).front()));
	m_pCbRight->SetWindowText(paths_UndoMagic(&String(m_sRightFile).front()));

	KillTimer(IDT_CHECKFILES);

	globalFileFilter.SetFilter(m_sFilter);
	m_sFilter = globalFileFilter.GetFilterNameOrMask();
	COptionsMgr::SaveOption(OPT_FILEFILTER_CURRENT, m_sFilter);

	SaveComboboxStates();
	SettingStore.WriteProfileInt(_T("Settings"), _T("Recurse"), m_nRecursive);

	if ((m_attrLeft ^ m_attrRight) & FILE_ATTRIBUTE_DIRECTORY)
	{
		m_idCompareAs = 0;
	}
	else
	{
		int i = m_pCbCompareAs->GetCurSel();
		m_idCompareAs = m_pCbCompareAs->GetItemData(i);
		m_pCompareAsScriptMenu->GetMenuString(m_idCompareAs, m_szCompareAs, _countof(m_szCompareAs));
	}

	EndDialog(IDOK);
}

/** 
 * @brief Called when dialog is closed via Cancel.
 *
 * Open-dialog is canceled when 'Cancel' button is selected or
 * Esc-key is pressed. Save combobox states, since user may have
 * removed items from them and don't want them to re-appear.
 */
void COpenDlg::OnCancel()
{
	if (m_pCbLeft->GetWindowTextLength() == 0)
		m_pCbLeft->SaveState(_T("Files\\Left"));
	if (m_pCbRight->GetWindowTextLength() == 0)
		m_pCbRight->SaveState(_T("Files\\Right"));
	if (m_pCbExt->GetWindowTextLength() == 0)
		m_pCbExt->SaveState(_T("Files\\Ext"));
	EndDialog(IDCANCEL);
}

/** 
 * @brief Save File- and filter-combobox states.
 */
void COpenDlg::SaveComboboxStates()
{
	m_pCbLeft->SaveState(_T("Files\\Left"));
	m_pCbRight->SaveState(_T("Files\\Right"));
	m_pCbExt->SaveState(_T("Files\\Ext"));
}

/** 
 * @brief Enable/disable components based on validity of paths.
 */
void COpenDlg::UpdateButtonStates()
{
	UpdateData<Get>(); // load member variables from screen
	KillTimer(IDT_CHECKFILES);
	TrimPaths();

	int nShowFilter = SW_HIDE;
	if (paths_EndsWithSlash(m_sLeftFile.c_str()) ||
		paths_EndsWithSlash(m_sRightFile.c_str()))
	{
		nShowFilter = SW_SHOW;
	}
	m_pCbExt->ShowWindow(nShowFilter);
	m_pCbExt->GetWindow(GW_HWNDPREV)->ShowWindow(nShowFilter); // asociated label
	GetDlgItem(IDC_SELECT_FILTER)->ShowWindow(nShowFilter);
	nShowFilter ^= SW_SHOW;
	m_pCbCompareAs->ShowWindow(nShowFilter);
	m_pCbCompareAs->GetWindow(GW_HWNDPREV)->ShowWindow(nShowFilter); // asociated label

	// Enable buttons as appropriate
	if (COptionsMgr::Get(OPT_VERIFY_OPEN_PATHS))
	{
		const DWORD attrLeft = GetFileAttributes(m_sLeftFile.c_str());
		const DWORD attrRight = GetFileAttributes(m_sRightFile.c_str());
		// If "both paths are valid", or
		// if "left path refers to a project file and right path is empty"
		if (attrLeft != INVALID_FILE_ATTRIBUTES && attrRight != INVALID_FILE_ATTRIBUTES ||
			m_sRightFile.empty() && ProjectFile::IsProjectFile(m_sLeftFile.c_str()))
		{
			m_pPbOk->EnableWindow(TRUE);
			SetStatus(IDS_OPEN_FILESDIRS);
		}
		else
		{
			m_pPbOk->EnableWindow(FALSE);
			SetStatus(
				attrLeft != INVALID_FILE_ATTRIBUTES ? IDS_OPEN_RIGHTINVALID :
				attrRight != INVALID_FILE_ATTRIBUTES ? IDS_OPEN_LEFTINVALID :
				IDS_OPEN_BOTHINVALID);
		}
	}
	else
	{
		m_pPbOk->EnableWindow(TRUE);
		SetStatus(IDS_OPEN_FILESDIRS);
	}
}

/**
 * @brief Called when user changes selection in left or right path's combo box.
 */
void COpenDlg::OnSelchangePathCombo(HSuperComboBox *pCb)
{
	if (!pCb->GetDroppedState() && pCb->GetCurSel() == 0)
		pCb->SetCurSel(1);
	UpdateButtonStates();
}

/** 
 * @brief Called every time paths are edited.
 */
void COpenDlg::OnEditchangePathCombo(HSuperComboBox *pCb)
{
	if (m_nAutoComplete == AUTO_COMPLETE_RECENTLY_USED)
		pCb->AutoCompleteFromLB(0);
	if (!pCb->GetDroppedState())
		if (DWORD_PTR dwpShellImageList = GetShellImageList())
			pCb->SendMessage(CBEM_SETIMAGELIST, 0, dwpShellImageList);
	// (Re)start timer to path validity check delay
	// If timer starting fails, update buttonstates immediately
	if (!SetTimer(IDT_CHECKFILES, CHECKFILES_TIMEOUT, NULL))
		UpdateButtonStates();
}

/**
 * @brief Handle timer events.
 * Checks if paths are valid and sets control states accordingly.
 * @param [in] nIDEvent Timer ID that fired.
 */
void COpenDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_CHECKFILES)
		UpdateButtonStates();
}

/**
 * @brief Sets the path status text.
 * The open dialog shows a status text of selected paths. This function
 * is used to set that status text.
 * @param [in] msgID Resource ID of status text to set.
 */
void COpenDlg::SetStatus(UINT msgID)
{
	String msg = LanguageSelect.LoadString(msgID);
	SetDlgItemText(IDC_OPEN_STATUS, msg.c_str());
}

/** 
 * @brief Called when "Select..." button for filters is selected.
 */
void COpenDlg::OnSelectFilter()
{
	theApp.m_pMainWnd->SelectFilter();
	String filterNameOrMask = globalFileFilter.GetFilterNameOrMask();
	SetDlgItemText(IDC_EXT_COMBO, filterNameOrMask.c_str());
}

/** 
 * @brief Removes whitespaces from left and right paths
 * @note Assumes UpdateData(TRUE) is called before this function.
 */
void COpenDlg::TrimPaths()
{
	m_sLeftFile = string_trim_ws(m_sLeftFile);
	m_sRightFile = string_trim_ws(m_sRightFile);
	// If user has edited path by hand, expand environment variables
	if (m_pCbLeft->GetEditControl()->GetModify())
		m_sLeftFile = env_ExpandVariables(m_sLeftFile.c_str());
	if (m_pCbRight->GetEditControl()->GetModify())
		m_sRightFile = env_ExpandVariables(m_sRightFile.c_str());
}

/** 
 * @brief Update control states when dialog is activated.
 *
 * Update control states when user re-activates dialog. User might have
 * switched for other program to e.g. update files/folders and then
 * swiches back to WinMerge. Its nice to see WinMerge detects updated
 * files/folders.
 */
void COpenDlg::OnActivate(UINT nState, HWND hWndOther, BOOL bMinimized)
{
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE)
		UpdateButtonStates();
}

/**
 * @brief Open help from mainframe when user presses F1.
 */
void COpenDlg::OnHelp()
{
	theApp.m_pMainWnd->ShowHelp(OpenDlgHelpLocation);
}

/////////////////////////////////////////////////////////////////////////////
//
//	OnDropFiles code from CDropEdit
//	Copyright 1997 Chris Losinger

/**
 * @brief Drop paths(s) to the dialog.
 * One or two paths can be dropped to the dialog. The behaviour is:
 *   If 1 file:
 *     - drop to empty path edit box (check left first)
 *     - if both boxes have a path, drop to left path
 *   If two files:
 *    - overwrite both paths, empty or not
 * @param [in] dropInfo Dropped data, including paths.
 */
void COpenDlg::OnDropFiles(HDROP dropInfo)
{
	// Get the number of pathnames that have been dropped
	UINT fileCount = DragQueryFile(dropInfo, 0xFFFFFFFF, NULL, 0);
	if (fileCount > 2)
		fileCount = 2;
	stl::vector<String> files(fileCount);
	UINT i;
	// get all file names. but we'll only need the first one.
	for (i = 0; i < fileCount; i++)
	{
		// Get the number of bytes required by the file's full pathname
		if (UINT len = DragQueryFile(dropInfo, i, NULL, 0))
		{
			files[i].resize(len);
			DragQueryFile(dropInfo, i, files[i].begin(), len + 1);
		}
	}

	for (i = 0; i < fileCount; i++)
	{
		if (paths_IsShortcut(files[i].c_str()))
		{
			// if this was a shortcut, we need to expand it to the target path
			String expandedFile = ExpandShortcut(files[i].c_str());
			// if that worked, we should have a real file name
			if (!expandedFile.empty())
				files[i] = expandedFile;
		}
	}

	// Add dropped paths to the dialog
	UpdateData<Get>();
	if (fileCount == 2)
	{
		m_sLeftFile = files[0];
		m_sRightFile = files[1];
		UpdateData<Set>();
		UpdateButtonStates();
	}
	else if (fileCount == 1)
	{
		String *pTarget = !m_sLeftFile.empty() && m_sRightFile.empty() ? &m_sRightFile : &m_sLeftFile;
		POINT pt;
		if (DragQueryPoint(dropInfo, &pt))
		{
			if (HWindow *pHit = ChildWindowFromPoint(pt,
				CWP_SKIPINVISIBLE | CWP_SKIPDISABLED | CWP_SKIPTRANSPARENT))
			{
				if (pHit == m_pCbLeft)
					pTarget = &m_sLeftFile;
				else if (pHit == m_pCbRight)
					pTarget = &m_sRightFile;
			}
		}
		*pTarget = files[0];
		UpdateData<Set>();
		UpdateButtonStates();
	}
	// Free the memory block containing the dropped-file information
	DragFinish(dropInfo);
}
