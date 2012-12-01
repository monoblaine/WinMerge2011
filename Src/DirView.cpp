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
 * @file  DirView.cpp
 *
 * @brief Main implementation file for CDirView
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Constants.h"
#include "Merge.h"
#include "SettingStore.h"
#include "LanguageSelect.h"
#include "DirView.h"
#include "DirFrame.h"  // StatePane
#include "HexMergeFrm.h"
#include "MainFrm.h"
#include "resource.h"
#include "coretools.h"
#include "FileTransform.h"
#include "paths.h"
#include "7zCommon.h"
#include "OptionsDef.h"
#include "DirCmpReport.h"
#include "DirCompProgressDlg.h"
#include "CompareStatisticsDlg.h"
#include "ShellContextMenu.h"
#include "PidlContainer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Limit (in seconds) to signal compare is ready for user.
 * If compare takes longer than this value (in seconds) we inform
 * user about it. Current implementation uses MessageBeep(IDOK).
 */
const int TimeToSignalCompare = 3;

// The resource ID constants/limits for the Shell context menu
static const UINT LeftCmdFirst	= 0x9000;
static const UINT LeftCmdLast	= 0xAFFF;
static const UINT RightCmdFirst	= 0xB000;
static const UINT RightCmdLast	= 0xCFFF;

/////////////////////////////////////////////////////////////////////////////
// CDirView

HFont *CDirView::m_font = NULL;
HImageList *CDirView::m_imageList = NULL;
HImageList *CDirView::m_imageState = NULL;

CDirView::CDirView(CDirFrame *pFrame)
	: m_numcols(-1)
	, m_dispcols(-1)
	, m_pFrame(pFrame)
	, m_nHiddenItems(0)
	, m_nSpecialItems(0)
	, m_pCmpProgressDlg(NULL)
	, m_compareStart(0)
	, m_bTreeMode(false)
	, m_pShellContextMenuLeft(new CShellContextMenu(LeftCmdFirst, LeftCmdLast))
	, m_hShellContextMenuLeft(NULL)
	, m_pShellContextMenuRight(new CShellContextMenu(RightCmdFirst, RightCmdLast))
	, m_hShellContextMenuRight(NULL)
	, m_pCompareAsScriptMenu(NULL)
{
	m_bTreeMode =  COptionsMgr::Get(OPT_TREE_MODE);
}

CDirView::~CDirView()
{
	delete m_pShellContextMenuRight;
	delete m_pShellContextMenuLeft;
	delete m_pCmpProgressDlg;
}

LRESULT CDirView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
			// Update the tool bar's "Undo" icon. It should be enabled when
			// renaming an item and undo is possible.
			m_pFrame->m_pMDIFrame->UpdateCmdUI<ID_EDIT_UNDO>(
				reinterpret_cast<HEdit *>(lParam)->CanUndo() ? MF_ENABLED : MF_GRAYED);
			break;
		case EN_KILLFOCUS:
			m_pFrame->m_pMDIFrame->UpdateCmdUI<ID_EDIT_UNDO>(MF_GRAYED);
			break;
		}
		break;
	case WM_NOTIFY:
		if (LRESULT lResult = OnNotify(lParam))
			return lResult;
		break;
	case WM_WINDOWPOSCHANGED:
		if ((reinterpret_cast<WINDOWPOS *>(lParam)->flags & SWP_NOSIZE) == 0)
			m_pFrame->AlignStatusBar(reinterpret_cast<WINDOWPOS *>(lParam)->cx);
		break;
	case WM_DESTROY:
		OnDestroy();
		break;
	case MSG_UI_UPDATE:
		OnUpdateUIMessage();
		return 0;
	case WM_DRAWITEM:
		if (reinterpret_cast<DRAWITEMSTRUCT *>(lParam)->CtlType == ODT_HEADER)
			m_ctlSortHeader.DrawItem(reinterpret_cast<DRAWITEMSTRUCT *>(lParam));
		return 0;
	}
	return OListView::WindowProc(uMsg, wParam, lParam);
}

void CDirView::AcquireSharedResources()
{
	// Load user-selected font
	m_font = HFont::CreateIndirect(&theApp.m_pMainWnd->m_lfDir);
	// Load the icons used for the list view (to reflect diff status)
	// NOTE: these must be in the exactly the same order than in enum
	// definition in begin of this file!
	const int iconCX = 16;
	const int iconCY = 16;
	m_imageList = HImageList::Create(iconCX, iconCY, ILC_COLOR32 | ILC_MASK, CompareStats::N_DIFFIMG, 0);
	static const WORD rgIDI[CompareStats::N_DIFFIMG] =
	{
		IDI_LFILE,
		IDI_RFILE,
		IDI_NOTEQUALFILE,
		IDI_EQUALFILE,
		IDI_EQUALBINARY,
		IDI_BINARYDIFF,
		IDI_LFOLDER,
		IDI_RFOLDER,
		IDI_FILESKIP,
		IDI_FOLDERSKIP,
		IDI_NOTEQUALFOLDER,
		IDI_EQUALFOLDER,
		IDI_FOLDER,
		IDI_COMPARE_ERROR,
		IDI_FOLDERUP,
		IDI_FOLDERUP_DISABLE,
		IDI_COMPARE_ABORTED,
		IDI_NOTEQUALTEXTFILE,
		IDI_EQUALTEXTFILE
	};
	int i = 0;
	do
	{
		VERIFY(-1 != m_imageList->Add(LanguageSelect.LoadIcon(rgIDI[i])));
	} while (++i < CompareStats::N_DIFFIMG);
	// Load the icons used for the list view (expanded/collapsed state icons)
	m_imageState = LanguageSelect.LoadImageList(IDB_TREE_STATE, 16);
}

void CDirView::ReleaseSharedResources()
{
	m_font->DeleteObject();
	m_font = NULL;
	m_imageList->Destroy();
	m_imageList = NULL;
	m_imageState->Destroy();
	m_imageState = NULL;
}

HFont *CDirView::ReplaceFont(const LOGFONT *lf)
{
	HFont *font = m_font;
	m_font = HFont::CreateIndirect(lf);
	return font;
}

void CDirView::UpdateFont()
{
	if (!OPT_FONT_DIRCMP_LOGFONT.IsDefault())
	{
		SetFont(m_font);
	}
	else
	{
		SetFont(NULL);
	}
}

void CDirView::OnInitialUpdate()
{
	// Load user-selected font
	UpdateFont();
	// Replace standard header with sort header
	if (HHeaderCtrl *pHeaderCtrl = GetHeaderCtrl())
		m_ctlSortHeader.SubclassWindow(pHeaderCtrl);

	SetImageList(m_imageList->m_hImageList, LVSIL_SMALL);

	// Restore column orders as they had them last time they ran
	LoadColumnOrders();

	// Display column headers (in appropriate order)
	ReloadColumns();

	// Show selection across entire row.
	// Also allow user to rearrange columns via drag&drop of headers.
	// Also enable infotips.
	DWORD exstyle = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP;
	SetExtendedStyle(exstyle);
}

/**
 * @brief Called before compare is started.
 * CDirDoc calls this function before new compare is started, so this
 * is good place to setup GUI for compare.
 */
void CDirView::StartCompare()
{
	if (m_pCmpProgressDlg == NULL)
		m_pCmpProgressDlg = new DirCompProgressDlg(m_pFrame);
	m_compareStart = clock();
}

/**
 * @brief Called when folder compare row is double-clicked with mouse.
 * Selected item is opened to folder or file compare.
 */
void CDirView::ReflectItemActivate(NMITEMACTIVATE *pNM)
{
	DoDefaultAction(pNM->iItem);
}

/**
 * @brief Load or reload the columns (headers) of the list view
 */
void CDirView::ReloadColumns()
{
	int i = GetHeaderCtrl()->GetItemCount();
	while (i > m_dispcols)
	{
		DeleteColumn(--i);
	}
	while (i < m_dispcols)
	{
		LVCOLUMN lvc;
		lvc.mask = 0;
		InsertColumn(i++, &lvc);
	}
	UpdateColumns(LVCF_TEXT | LVCF_FMT | LVCF_WIDTH);
}

/**
 * @brief Redisplay items in subfolder
 * @param [in] diffpos First item position in subfolder.
 * @param [in] level Indent level
 * @param [in,out] index Index of the item to be inserted.
 * @param [in,out] alldiffs Number of different items
 */
void CDirView::RedisplayChildren(DIFFITEM *di, int level, int &index, int &alldiffs)
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	di = ctxt->GetFirstChildDiff(di);
	while (di != NULL)
	{
		if (!di->isResultSame())
			++alldiffs;

		if (m_pFrame->IsShowable(di))
		{
			if (m_bTreeMode)
			{
				AddNewItem(index, di, I_IMAGECALLBACK, level);
				index++;
				if (di->HasChildren())
				{
					SetItemState(index - 1, INDEXTOSTATEIMAGEMASK((di->customFlags1 & ViewCustomFlags::EXPANDED) ? 2 : 1), LVIS_STATEIMAGEMASK);
					if (di->customFlags1 & ViewCustomFlags::EXPANDED)
						RedisplayChildren(di, level + 1, index, alldiffs);
				}
			}
			else
			{
				if (m_pFrame->GetRecursive() == 0 ||
					!di->isDirectory() ||
					di->isSideLeftOnly() ||
					di->isSideRightOnly())
				{
					AddNewItem(index, di, I_IMAGECALLBACK, 0);
					index++;
				}
				if (di->HasChildren())
				{
					RedisplayChildren(di, level + 1, index, alldiffs);
				}
			}
		}
		di = ctxt->GetNextSiblingDiff(di);
	}
}

/**
 * @brief Redisplay folder compare view.
 * This function clears folder compare view and then adds
 * items from current compare to it.
 */
void CDirView::Redisplay()
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();

	int cnt = 0;
	// Disable redrawing while adding new items
	SetRedraw(FALSE);

	DeleteAllItems();
	m_nSpecialItems = 0;

	SetImageList(m_bTreeMode && m_pFrame->GetRecursive() == 1 ?
		m_imageState->m_hImageList : NULL, LVSIL_STATE);

	// If non-recursive compare, add special item(s)
	String leftParent, rightParent;
	if (m_pFrame->GetRecursive() == 0 ||
		m_pFrame->AllowUpwardDirectory(leftParent, rightParent) == CDirFrame::AllowUpwardDirectory::ParentIsTempPath)
	{
		cnt += m_nSpecialItems = AddSpecialItems();
	}

	int alldiffs = 0;
	RedisplayChildren(NULL, 0, cnt, alldiffs);
	SortColumnsAppropriately();
	SetRedraw(TRUE);
}

/**
 * @brief Format context menu string and disable item if it cannot be applied.
 */
static void NTAPI FormatContextMenu(HMenu *pPopup, UINT uIDItem, int n1, int n2)
{
	TCHAR fmt[INFOTIPSIZE];
	pPopup->GetMenuString(uIDItem, fmt, _countof(fmt));
	String str = fmt;
	String::size_type pct1 = str.find(_T("%1"));
	String::size_type pct2 = str.find(_T("%2"));
	if ((n1 == n2) && (pct1 != String::npos) && (pct2 != String::npos))
	{
		assert(pct2 > pct1); // rethink if that happens...
		str.erase(pct1 + 2, pct2 - pct1);
		pct2 = String::npos;
	}
	if (pct2 != String::npos)
		str.replace(pct2, 2, NumToStr(n2).c_str());
	if (pct1 != String::npos)
		str.replace(pct1, 2, NumToStr(n1).c_str());
	MENUITEMINFO mii;
	mii.cbSize = sizeof mii;
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = const_cast<LPTSTR>(str.c_str());
	pPopup->SetMenuItemInfo(uIDItem, FALSE, &mii);
	if (n1 == 0)
		pPopup->EnableMenuItem(uIDItem, MF_GRAYED);
}

/**
 * @brief User right-clicked in listview rows
 */
void CDirView::ListContextMenu(POINT point)
{
	const BOOL leftRO = m_pFrame->GetLeftReadOnly();
	const BOOL rightRO = m_pFrame->GetRightReadOnly();
	// TODO: It would be more efficient to set
	// all the popup items now with one traverse over selected items
	// instead of using updates, in which we make a traverse for every item
	// Perry, 2002-12-04

	//2003-12-17 Jochen:
	//-	Archive related menu items follow the above suggestion.
	//-	For disabling to work properly, the tracking frame's m_bAutoMenuEnable
	//	member has to temporarily be turned off.
	int nTotal = 0; // total #items (includes files & directories, either side)
	int nCopyableToLeft = 0;
	int nCopyableToRight = 0;
	int nDeletableOnLeft = 0;
	int nDeletableOnRight = 0;
	int nDeletableOnBoth = 0;
	int nOpenableOnLeft = 0;
	int nOpenableOnRight = 0;
	int nOpenableOnBoth = 0;
	int nOpenableOnLeftWith = 0;
	int nOpenableOnRightWith = 0;
	int nFileNames = 0;
	int nDiffItems = 0;
	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (di == NULL) // Invalid value, this must be special item
			break;

		if (!di->isDirectory())
			++nFileNames;

		if (di->isDeletableOnBoth() && !leftRO && !rightRO)
			++nDeletableOnBoth;

		if (di->isSideLeftOrBoth())
		{
			++nOpenableOnLeft;
			if (!di->isDirectory())
				++nOpenableOnLeftWith;
			if (!rightRO)
				++nCopyableToRight;
			if (di->isDeletableOnLeft() && !leftRO)
				++nDeletableOnLeft;
		}

		if (di->isSideRightOrBoth())
		{
			++nOpenableOnRight;
			if (!di->isDirectory())
				++nOpenableOnRightWith;
			if (!leftRO)
				++nCopyableToLeft;
			if (di->isDeletableOnRight() && !rightRO)
				++nDeletableOnRight;
		}

		if (IsItemNavigableDiff(di))
			++nDiffItems;
		if (di->isSideLeftOrBoth() || di->isSideRightOrBoth())
			++nOpenableOnBoth;

		++nTotal;
	}

	if (nTotal == 0)
		return;

	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_DIRVIEW);

	// 1st submenu of IDR_POPUP_DIRVIEW is for item popup
	HMenu *const pPopup = pMenu->GetSubMenu(0);

	FormatContextMenu(pPopup, ID_DIR_COPY_PATHNAMES_LEFT, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_PATHNAMES_RIGHT, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_PATHNAMES_BOTH, nOpenableOnBoth, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_FILENAMES, nFileNames, nTotal);

	FormatContextMenu(pPopup, ID_DIR_ZIP_LEFT, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_ZIP_RIGHT, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_ZIP_BOTH, nOpenableOnBoth, nTotal);
	FormatContextMenu(pPopup, ID_DIR_ZIP_BOTH_DIFFS_ONLY, nDiffItems, nTotal);

	FormatContextMenu(pPopup, ID_DIR_COPY_LEFT_TO_RIGHT, nCopyableToRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_RIGHT_TO_LEFT, nCopyableToLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_LEFT_TO_BROWSE, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_COPY_RIGHT_TO_BROWSE, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_MOVE_LEFT_TO_BROWSE, nDeletableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_MOVE_RIGHT_TO_BROWSE, nDeletableOnRight, nTotal);

	FormatContextMenu(pPopup, ID_DIR_DEL_LEFT, nDeletableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_DEL_RIGHT, nDeletableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_DEL_BOTH, nDeletableOnBoth, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_LEFT, nOpenableOnLeft, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_LEFT_WITH, nOpenableOnLeftWith, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_RIGHT, nOpenableOnRight, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_RIGHT_WITH, nOpenableOnRightWith, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_LEFT_WITHEDITOR, nOpenableOnLeftWith, nTotal);
	FormatContextMenu(pPopup, ID_DIR_OPEN_RIGHT_WITHEDITOR, nOpenableOnRightWith, nTotal);
	
	pPopup->EnableMenuItem(ID_DIR_ITEM_RENAME, nTotal == 1 ? MF_ENABLED : MF_GRAYED);

	bool bEnableShellContextMenu = COptionsMgr::Get(OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU);
	if (bEnableShellContextMenu)
	{
		if (nOpenableOnLeft || nOpenableOnRight)
		{
			pPopup->AppendMenu(MF_SEPARATOR, 0, NULL);
			if (nOpenableOnLeft)
			{
				String s = LanguageSelect.LoadString(IDS_SHELL_CONTEXT_MENU_LEFT);
				m_hShellContextMenuLeft = ::CreatePopupMenu();
				pPopup->AppendMenu(MF_POPUP, (UINT_PTR)m_hShellContextMenuLeft, s.c_str());
			}
			if (nOpenableOnRight)
			{
				String s = LanguageSelect.LoadString(IDS_SHELL_CONTEXT_MENU_RIGHT);
				m_hShellContextMenuRight = ::CreatePopupMenu();
				pPopup->AppendMenu(MF_POPUP, (UINT_PTR)m_hShellContextMenuRight, s.c_str());
			}
		}
	}

	m_pCompareAsScriptMenu = pPopup->GetSubMenu(1);
	// Disable "Compare As" if multiple items are selected, or if the selected
	// item is unique or represents a folder.
	if (nTotal != 1 || nFileNames != 1 || nOpenableOnLeft != 1 || nOpenableOnRight != 1)
		pPopup->EnableMenuItem(1, MF_GRAYED | MF_BYPOSITION);

	CMainFrame *pMDIFrame = m_pFrame->m_pMDIFrame;
	// invoke context menu
	// this will invoke all the OnUpdate methods to enable/disable the individual items
	int nCmd = pPopup->TrackPopupMenu(
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		point.x, point.y, pMDIFrame->m_pWnd);

	HMenu *pScriptMenu = pMDIFrame->SetScriptMenu(m_pCompareAsScriptMenu, NULL);

	if (nCmd >= IDC_SCRIPT_FIRST && nCmd <= IDC_SCRIPT_LAST)
	{
		if (pScriptMenu)
		{
			TCHAR szCompareAs[MAX_PATH];
			pScriptMenu->GetMenuString(nCmd, szCompareAs, _countof(szCompareAs));
			OpenSelection(szCompareAs, 0);
		}
	}
	else if (nCmd)
	{
		(m_pShellContextMenuLeft->InvokeCommand(nCmd, pMDIFrame->m_hWnd))
		|| (m_pShellContextMenuRight->InvokeCommand(nCmd, pMDIFrame->m_hWnd))
		// we have called TrackPopupMenu with TPM_RETURNCMD flag so we have to post message ourselves
		|| m_pFrame->PostMessage(WM_COMMAND, nCmd);
	}
	m_hShellContextMenuLeft = NULL;
	m_hShellContextMenuRight = NULL;

	if (pScriptMenu)
		pScriptMenu->DestroyMenu();

	m_pCompareAsScriptMenu = NULL;
	pMenu->DestroyMenu();
}

/**
 * @brief User right-clicked on specified logical column
 */
void CDirView::HeaderContextMenu(POINT point)
{
	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_DIRVIEW);
	// 2nd submenu of IDR_POPUP_DIRVIEW is for header popup
	HMenu *const pSubMenu = pMenu->GetSubMenu(1);
	// invoke context menu
	// this will invoke all the OnUpdate methods to enable/disable the individual items
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, m_pFrame->m_pMDIFrame->m_pWnd);
	pMenu->DestroyMenu();
}

/**
 * @brief Gets Explorer's context menu for a group of selected files.
 *
 * @param [in] Side whether to get context menu for the files from the left or
 *   right side.
 * @retval The HMENU.
 */
HMENU CDirView::ListShellContextMenu(SIDE_TYPE side)
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();

	CMyComPtr<IShellFolder> pDesktop;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);
	if (FAILED(hr))
		return NULL;

	String parentDir; // use it to track that all selected files are in the same parent directory
	CMyComPtr<IShellFolder> pCurrFolder;
	CPidlContainer pidls;

	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (di == NULL) // Invalid value, this must be special item
			continue;
		String currentDir = (side == SIDE_LEFT) ?
			ctxt->GetLeftFilepath(di) :
			ctxt->GetRightFilepath(di);
		String filename = (side == SIDE_LEFT) ? di->left.filename : di->right.filename;
		if (parentDir.empty()) // first iteration, initialize parentDir and pCurrFolder
		{
			parentDir = currentDir;
			LPITEMIDLIST dirPidl;
			LPCTSTR displayName = paths_UndoMagic(&currentDir.front());
			hr = pDesktop->ParseDisplayName(NULL, NULL, const_cast<LPOLESTR>(displayName), NULL, &dirPidl, NULL);
			if (FAILED(hr))
				return NULL;
			hr = pDesktop->BindToObject(dirPidl, NULL, IID_IShellFolder, reinterpret_cast<void**>(&pCurrFolder));
			pidls.Free(dirPidl);
			if (FAILED(hr))
				return NULL;
		}
		else if (currentDir != parentDir) // check whether file belongs to the same parentDir, break otherwise
		{
			return NULL;
		}
		LPITEMIDLIST pidl;
		hr = pCurrFolder->ParseDisplayName(NULL, NULL, const_cast<LPOLESTR>(filename.c_str()), NULL, &pidl, NULL);
		if (FAILED(hr))
			return NULL;
		pidls.Add(pidl);
	}

	if (pCurrFolder == NULL)
		return NULL;

	CMyComPtr<IContextMenu> pContextMenu;
	hr = pCurrFolder->GetUIObjectOf(NULL,
		pidls.Size(), pidls.GetList(), IID_IContextMenu, 0,
		reinterpret_cast<void**>(&pContextMenu));
	if (FAILED(hr))
		return NULL;

	CShellContextMenu *pscm = (side == SIDE_LEFT) ?
		m_pShellContextMenuLeft : m_pShellContextMenuRight;
	return pscm->QueryShellContextMenu(pContextMenu);
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CDirView::UpdateResources()
{
	UpdateColumns(LVCF_TEXT);
}

/**
 * @brief User just clicked a column, so perform sort
 */
void CDirView::ReflectColumnClick(NM_LISTVIEW *pNMListView)
{
	// set sort parameters and handle ascending/descending
	int oldSortColumn = COptionsMgr::Get(OPT_DIRVIEW_SORT_COLUMN);
	int sortcol = m_invcolorder[pNMListView->iSubItem];
	if (sortcol == oldSortColumn)
	{
		// Swap direction
		bool bSortAscending = COptionsMgr::Get(OPT_DIRVIEW_SORT_ASCENDING);
		COptionsMgr::SaveOption(OPT_DIRVIEW_SORT_ASCENDING, !bSortAscending);
	}
	else
	{
		COptionsMgr::SaveOption(OPT_DIRVIEW_SORT_COLUMN, sortcol);
		// most columns start off ascending, but not dates
		COptionsMgr::SaveOption(OPT_DIRVIEW_SORT_ASCENDING, f_cols[sortcol].defSortUp);
	}

	SortColumnsAppropriately();
}

void CDirView::SortColumnsAppropriately()
{
	int sortCol = COptionsMgr::Get(OPT_DIRVIEW_SORT_COLUMN);
	if (sortCol == -1)
		return;

	bool bSortAscending = COptionsMgr::Get(OPT_DIRVIEW_SORT_ASCENDING);
	m_ctlSortHeader.SetSortImage(ColLogToPhys(sortCol), bSortAscending);
	//sort using static CompareFunc comparison function
	CompareState cs(this, sortCol, bSortAscending);
	SortItems(cs.CompareFunc, reinterpret_cast<DWORD_PTR>(&cs));
}

/// Do any last minute work as view closes
void CDirView::OnDestroy()
{
	ValidateColumnOrdering();
	SaveColumnOrders();
	SaveColumnWidths();
}

/**
 * @brief Open selected item when user presses ENTER key.
 */
void CDirView::DoDefaultAction(int sel)
{
	if (sel >= 0)
	{
		const DIFFITEM *di = GetDiffItem(sel);
		if (m_bTreeMode && m_pFrame->GetRecursive() == 1 && di->isDirectory())
		{
			if (di->customFlags1 & ViewCustomFlags::EXPANDED)
				CollapseSubdir(sel);
			else
				ExpandSubdir(sel);
		}
		else
		{
			OpenSelection(NULL, ID_MERGE_COMPARE);
		}
	}
}

/**
 * @brief Expand/collapse subfolder when "+/-" icon is clicked.
 */
void CDirView::ReflectClick(NMITEMACTIVATE *pNM)
{
	LVHITTESTINFO lvhti;
	lvhti.pt = pNM->ptAction;
	SubItemHitTest(&lvhti);
	if (lvhti.flags == LVHT_ONITEMSTATEICON)
		ReflectItemActivate(pNM);
}

/**
 * @brief Delete children
 * @param [in] dpi Ancestor item.
 * @param [in] i First child item index in listview.
 */
void CDirView::DeleteChildren(const DIFFITEM *dip, int i)
{
	int count = GetItemCount();
	while (count > i)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (!di->IsAncestor(dip))
			break;
		DeleteItem(i);
		--count;
	}
}

/**
 * @brief Collapse subfolder
 * @param [in] i Folder item index in listview.
 */
int CDirView::CollapseSubdir(int i)
{
	if (DIFFITEM *dip = GetDiffItem(i))
	{
		if (!(dip->customFlags1 & ViewCustomFlags::EXPANDED))
		{
			return GetItemIndex(dip->parent);
		}
		SetRedraw(FALSE);	// Turn off updating (better performance)
		dip->customFlags1 &= ~ViewCustomFlags::EXPANDED;
		SetItemState(i, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		DeleteChildren(dip, ++i);
		SetRedraw(TRUE);	// Turn updating back on
	}
	return -1;
}

/**
 * @brief Expand subfolder
 * @param [in] i Folder item index in listview.
 */
int CDirView::ExpandSubdir(int i)
{
	if (DIFFITEM *dip = GetDiffItem(i))
	{
		if (dip->customFlags1 & ViewCustomFlags::EXPANDED)
		{
			DIFFITEM *di = GetDiffItem(++i);
			return di->parent == dip ? i : -1;
		}
		const CDiffContext *const ctxt = m_pFrame->GetDiffContext();
		if (DIFFITEM *di = ctxt->GetFirstChildDiff(dip))
		{
			SetRedraw(FALSE);	// Turn off updating (better performance)
			dip->customFlags1 |= ViewCustomFlags::EXPANDED;
			SetItemState(i, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			++i;
			int alldiffs;
			RedisplayChildren(dip, dip->GetDepth() + 1, i, alldiffs);
			SortColumnsAppropriately();
			SetRedraw(TRUE);	// Turn updating back on
		}
	}
	return -1;
}

/**
 * @brief Expand subfolder
 * @param [in] i Folder item index in listview.
 */
void CDirView::DeepExpandSubdir(int i)
{
	const CDiffContext *const ctxt = m_pFrame->GetDiffContext();
	if (DIFFITEM *dip = GetDiffItem(i))
	{
		if (DIFFITEM *di = ctxt->GetFirstChildDiff(dip))
		{
			SetRedraw(FALSE);	// Turn off updating (better performance)
			dip->customFlags1 |= ViewCustomFlags::EXPANDED;
			SetItemState(i, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			while (di)
			{
				if (!di->IsAncestor(dip))
					break;
				if (di->isDirectory())
					di->customFlags1 |= ViewCustomFlags::EXPANDED;
				di = ctxt->GetNextDiff(di);
			}
			DeleteChildren(dip, ++i);
			int alldiffs;
			RedisplayChildren(dip, dip->GetDepth() + 1, i, alldiffs);
			SortColumnsAppropriately();
			SetRedraw(TRUE);	// Turn updating back on
		}
	}
}

/**
 * @brief Open parent folder if possible.
 */
void CDirView::OpenParentDirectory()
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	FileLocation filelocLeft, filelocRight;
	switch (m_pFrame->AllowUpwardDirectory(filelocLeft.filepath, filelocRight.filepath))
	{
		for (;;)
		{
		case CDirFrame::AllowUpwardDirectory::ParentIsTempPath:
			m_lastLeftPath = m_pFrame->m_pTempPathContext->m_strLeftDisplayRoot;
			m_lastRightPath = m_pFrame->m_pTempPathContext->m_strRightDisplayRoot;
			m_pFrame->m_pTempPathContext = m_pFrame->m_pTempPathContext->DeleteHead();
			break;
		case CDirFrame::AllowUpwardDirectory::ParentIsRegularPath:
			// Below assignments exclude any trailing backslashes
			const String &leftPath = ctxt->GetLeftPath();
			m_lastLeftPath.assign(leftPath.c_str(),
				leftPath.find_last_not_of(_T('\\')) + 1);
			const String &rightPath = ctxt->GetRightPath();
			m_lastRightPath.assign(rightPath.c_str(),
				rightPath.find_last_not_of(_T('\\')) + 1);
			break;
		}
		theApp.m_pMainWnd->DoFileOpen(filelocLeft, filelocRight,
			FFILEOPEN_NOMRU, FFILEOPEN_NOMRU, m_pFrame->GetRecursive(), m_pFrame);
		break;
	case CDirFrame::AllowUpwardDirectory::No:
		break;
	default:
		LanguageSelect.MsgBox(IDS_INVALID_DIRECTORY, MB_ICONSTOP);
		break;
	}
}

/**
 * @brief Get one or two selected items
 *
 * Returns false if 0 or more than 2 items selecte
 */
int CDirView::GetSelectedItems(DIFFITEM **rgdi)
{
	int i = -1;
	int j = 0;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		DIFFITEM *di = GetDiffItem(i);
		if (di == NULL)
			return -i; // minus index of special item
		if (j < 2)
			rgdi[j] = di;
		++j;
	}
	return j; // number of regular items
}

/**
 * @brief Creates a pairing folder for unique folder item.
 * This function creates a pairing folder for unique folder item in
 * folder compare. This way user can browse into unique folder's
 * contents and don't necessarily need to copy whole folder structure.
 * @param [in] newFolder New created folder (full folder path).
 * @return true if user agreed and folder was created.
 */
bool CDirView::CreatePairFolder(LPCTSTR newFolder)
{
	int response = LanguageSelect.FormatMessage(
		IDS_CREATE_PAIR_FOLDER, paths_UndoMagic(&String(newFolder).front())
	).MsgBox(MB_YESNO | MB_ICONWARNING);
	return response == IDYES && paths_CreateIfNeeded(newFolder);
}

/**
 * @brief Open one selected item.
 * @param [in] pos1 Item position.
 * @param [in,out] di1 Pointer to first diffitem.
 * @param [in,out] di2 Pointer to second diffitem.
 * @param [out] path1 First path.
 * @param [out] path2 Second path.
 * @param [out] sel1 Item's selection index in listview.
 * @param [in,out] isDir Is item folder?
 * return false if there was error or item was completely processed.
 */
bool CDirView::OpenOneItem(DIFFITEM *di, String &path1, String &path2)
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	GetItemFileNames(di, path1, path2);
	const bool isDir = di->isDirectory();
	if (isDir && di->isSideBoth())
	{
		// Check both folders exist. If either folder is missing that means
		// folder has been changed behind our back, so we just tell user to
		// refresh the compare.
		PATH_EXISTENCE path1Exists = paths_DoesPathExist(path1.c_str());
		PATH_EXISTENCE path2Exists = paths_DoesPathExist(path2.c_str());
		if (path1Exists != IS_EXISTING_DIR || path2Exists != IS_EXISTING_DIR)
		{
			String invalid = path1Exists == IS_EXISTING_DIR ? path1 : path2;
			LanguageSelect.MsgBox(IDS_DIRCMP_NOTSYNC, invalid.c_str(), MB_ICONSTOP);
			return false;
		}
	}
	else if (di->isSideLeftOnly())
	{
		// Open left-only item to editor if its not a folder or binary
		if (isDir)
		{
			path2 = paths_ConcatPath(di->GetLeftFilepath(ctxt->GetRightPath()), di->left.filename);
			if (CreatePairFolder(path2.c_str()))
			{
				return true;
			}
		}
		else if (di->isBin())
			LanguageSelect.MsgBox(IDS_CANNOT_OPEN_BINARYFILE, MB_ICONSTOP);
		else
			DoOpenWithEditor(SIDE_LEFT);
		return false;
	}
	else if (di->isSideRightOnly())
	{
		// Open right-only item to editor if its not a folder or binary
		if (isDir)
		{
			path1 = paths_ConcatPath(di->GetRightFilepath(ctxt->GetLeftPath()), di->right.filename);
			if (CreatePairFolder(path1.c_str()))
			{
				return true;
			}
		}
		else if (di->isBin())
			LanguageSelect.MsgBox(IDS_CANNOT_OPEN_BINARYFILE, MB_ICONSTOP);
		else
			DoOpenWithEditor(SIDE_RIGHT);
		return false;
	}
	// Fall through and compare files (which may be archives)

	return true;
}

/**
 * @brief Open two selected items.
 * @param [in] pos1 First item position.
 * @param [in] pos2 Second item position.
 * @param [in,out] di1 Pointer to first diffitem.
 * @param [in,out] di2 Pointer to second diffitem.
 * @param [out] path1 First path.
 * @param [out] path2 Second path.
 * @param [in,out] isDir Is item folder?
 * return false if there was error or item was completely processed.
 */
bool CDirView::OpenTwoItems(DIFFITEM *di1, DIFFITEM *di2, String &path1, String &path2)
{
	// Check for binary & side compatibility & file/dir compatibility
	if (!AreItemsOpenable(di1, di2))
	{
		return false;
	}
	// Ensure that di1 is on left (swap if needed)
	if (di1->isSideRightOnly() || (di1->isSideBoth() && di2->isSideLeftOnly()))
	{
		stl::swap(di1, di2);
	}
	// Fill in pathLeft & pathRight
	String temp;
	GetItemFileNames(di1, path1, temp);
	GetItemFileNames(di2, temp, path2);

	if (di1->isDirectory())
	{
		if (GetPairComparability(path1.c_str(), path2.c_str()) != IS_EXISTING_DIR)
		{
			LanguageSelect.MsgBox(IDS_INVALID_DIRECTORY, MB_ICONSTOP);
			return false;
		}
	}
	return true;
}

/**
 * @brief Open selected files or directories.
 *
 * Opens selected files to file compare. If comparing
 * directories non-recursively, then subfolders and parent
 * folder are opened too.
 *
 * This handles the case that one item is selected
 * and the case that two items are selected (one on each side)
 */
void CDirView::OpenSelection(LPCTSTR szCompareAs, UINT idCompareAs)
{
	WaitStatusCursor waitstatus(IDS_STATUS_OPENING_SELECTION);
	// First, figure out what was selected (store into di[])
	DIFFITEM *di[] = { NULL, NULL };
	FileLocation filelocLeft, filelocRight;
	switch (int selected = GetSelectedItems(di))
	{
	case 0:
		OpenParentDirectory();
		break;

		// Now handle the various cases of what was selected
		for (;;)
		{
		case 1:
			// Only one item selected, so perform diff on its sides
			if (!OpenOneItem(di[1] = di[0], filelocLeft.filepath, filelocRight.filepath))
				return;
			break;
		case 2:
			if (!OpenTwoItems(di[0], di[1], filelocLeft.filepath, filelocRight.filepath))
				return;
			break;
		}

		// Now pathLeft, pathRight, di1, di2, and isdir are all set
		// We have two items to compare, no matter whether same or different underlying DirView item

		PackingInfo packingInfo;

		if (szCompareAs)
			packingInfo.SetPlugin(szCompareAs);

		// Open identical and different files
		BOOL bLeftRO = m_pFrame->GetLeftReadOnly();
		BOOL bRightRO = m_pFrame->GetRightReadOnly();

		filelocLeft.encoding = di[0]->left.encoding;
		filelocRight.encoding = di[1]->right.encoding;

		DWORD leftFlags = bLeftRO ? FFILEOPEN_READONLY : 0;
		DWORD rightFlags = bRightRO ? FFILEOPEN_READONLY : 0;

		theApp.m_pMainWnd->DoFileOpen(
			packingInfo, idCompareAs,
			filelocLeft, filelocRight,
			leftFlags | FFILEOPEN_NOMRU | FFILEOPEN_DETECT,
			rightFlags | FFILEOPEN_NOMRU | FFILEOPEN_DETECT,
			m_pFrame->GetRecursive(), m_pFrame);
	}
}

/**
 * Given index in list control, get modifiable reference to its DIFFITEM data
 */
DIFFITEM *CDirView::GetDiffItem(int sel)
{
	return reinterpret_cast<DIFFITEM *>(GetItemData(sel));
}

/**
 * @brief Given key, get index of item which has it stored.
 * This function searches from list in UI.
 */
int CDirView::GetItemIndex(DIFFITEM *di)
{
	LVFINDINFO findInfo;
	findInfo.flags = LVFI_PARAM;  // Search for itemdata
	findInfo.lParam = reinterpret_cast<LPARAM>(di);
	return FindItem(&findInfo);
}

// Go to first diff
// If none or one item selected select found item
// This is used for scrolling to first diff too
void CDirView::OnFirstdiff()
{
	const int count = GetItemCount();
	int i = 0;
	int currentInd = GetNextItem(-1, LVNI_SELECTED);
	while (i < count)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i);
			break;
		}
		i++;
	}
}

// Go to last diff
// If none or one item selected select found item
void CDirView::OnLastdiff()
{
	int i = GetItemCount();
	int currentInd = GetNextItem(-1, LVNI_SELECTED);
	while (i > 0)
	{
		--i;
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i);
			break;
		}
	}
}

// Go to next diff
// If none or one item selected select found item
void CDirView::OnNextdiff()
{
	const int count = GetItemCount();
	int i = GetFocusedItem();
	int currentInd = i;
	while (++i < count)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i);
			break;
		}
	}
}

// Go to prev diff
// If none or one item selected select found item
void CDirView::OnPrevdiff()
{
	int i = GetFocusedItem();
	int currentInd = i;
	while (i > 0)
	{
		--i;
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			MoveFocus(currentInd, i);
			break;
		}
	}
}

void CDirView::OnCurdiff()
{
	EnsureVisible(GetFocusedItem(), FALSE);
}

int CDirView::GetFocusedItem()
{
	return GetNextItem(-1, LVNI_FOCUSED);
}

int CDirView::GetFirstDifferentItem()
{
	const int count = GetItemCount();
	BOOL found = FALSE;
	int i = 0;
	int foundInd = -1;

	while (i < count && found == FALSE)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			foundInd = i;
			found = TRUE;
		}
		i++;
	}
	return foundInd;
}

int CDirView::GetLastDifferentItem()
{
	const int count = GetItemCount();
	BOOL found = FALSE;
	int i = count - 1;
	int foundInd = -1;

	while (i > 0 && found == FALSE)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			foundInd = i;
			found = TRUE;
		}
		i--;
	}
	return foundInd;
}

// When navigating differences, do we stop at this one ?
bool CDirView::IsItemNavigableDiff(const DIFFITEM *di) const
{
	// Not a valid diffitem, one of special items (e.g "..")
	if (di == NULL)
		return false;
	if (di->isResultFiltered() || di->isResultError())
		return false;
	if (!di->isResultDiff() && !di->isSideLeftOnly() &&
			!di->isSideRightOnly())
		return false;
	return true;
}

/**
 * @brief Move focus to specified item (and selection if multiple items not selected)
 *
 * Moves the focus from item [currentInd] to item [i]
 * Additionally, if there are not multiple items selected,
 *  deselects item [currentInd] and selects item [i]
 */
void CDirView::MoveFocus(int currentInd, int i)
{
	int selCount = GetSelectedCount();
	if (selCount <= 1)
	{
		// Not multiple items selected, so bring selection with us
		SetItemState(currentInd, 0, LVIS_SELECTED);
		//SetItemState(currentInd, 0, LVIS_FOCUSED);
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}

	// Move focus to specified item
	// (this automatically defocuses old item)
	SetItemState(i, LVIS_FOCUSED, LVIS_FOCUSED);
	EnsureVisible(i, FALSE);
}

LRESULT CDirView::ReflectKeydown(NMLVKEYDOWN *pParam)
{
	// Check if we got 'ESC pressed' -message
	switch (pParam->wVKey)
	{
	case VK_DELETE:
		DoDelAll();
		return 1;
	case VK_BACK:
		if (m_nSpecialItems > 0 && GetDiffItem(0) == NULL)
		{
			WaitStatusCursor waitstatus(IDS_STATUS_OPENING_SELECTION);
			OpenParentDirectory();
		}
		return 1;
	case VK_RETURN:
		DoDefaultAction(GetFocusedItem());
		return 1;
	case VK_LEFT:
		if (m_bTreeMode)
		{
			int currentInd = GetFocusedItem();
			int i = CollapseSubdir(currentInd);
			if (i != -1)
			{
				MoveFocus(currentInd, i);
			}
			return 1;
		}
		break;
	case VK_RIGHT:
		if (m_bTreeMode)
		{
			int currentInd = GetFocusedItem();
			int i = ExpandSubdir(currentInd);
			if (i != -1)
			{
				MoveFocus(currentInd, i);
			}
			return 1;
		}
		break;
	case VK_SUBTRACT:
		if (m_bTreeMode)
		{
			int currentInd = GetFocusedItem();
			CollapseSubdir(currentInd);
			return 1;
		}
		break;
	case VK_ADD:
		if (m_bTreeMode)
		{
			int currentInd = GetFocusedItem();
			ExpandSubdir(currentInd);
			return 1;
		}
		break;
	case VK_MULTIPLY:
		if (m_bTreeMode)
		{
			int currentInd = GetFocusedItem();
			DeepExpandSubdir(currentInd);
			return 1;
		}
		break;
	}
	return 0;
}

/**
 * @brief Called when compare thread asks UI update.
 * @note Currently thread asks update after compare is ready
 * or aborted.
 */
void CDirView::OnUpdateUIMessage()
{
	// Close and destroy the dialog after compare
	delete m_pCmpProgressDlg;
	m_pCmpProgressDlg = NULL;

	m_pFrame->CompareReady();
	// Don't Redisplay() if triggered by OnMarkedRescan()
	if (GetItemCount() == 0)
	{
		Redisplay();
		if (DIFFITEM *di = m_pFrame->FindItemFromPaths(m_lastLeftPath.c_str(), m_lastRightPath.c_str()))
		{
			int i = GetItemIndex(di);
			if (i != -1)
			{
				MoveFocus(i, i);
			}
		}
		else if (COptionsMgr::Get(OPT_SCROLL_TO_FIRST))
		{
			OnFirstdiff();
		}
		else
		{
			MoveFocus(0, 0);
		}
		m_lastLeftPath.clear();
		m_lastRightPath.clear();
	}
	// If compare took more than TimeToSignalCompare seconds, notify user
	clock_t elapsed = clock() - m_compareStart;
	theApp.m_pMainWnd->SetStatus(LanguageSelect.Format(IDS_ELAPSED_TIME, elapsed));
	if (elapsed > TimeToSignalCompare * CLOCKS_PER_SEC)
		MessageBeep(IDOK);
	theApp.m_pMainWnd->StartFlashing();
}

LRESULT CDirView::OnNotify(LPARAM lParam)
{
	union
	{
		LPARAM lp;
		NMHDR *hdr;
		NMHEADER *header;
	} const nm = { lParam };
	if (nm.hdr->code == HDN_ENDDRAG)
	{
		// User just finished dragging a column header
		int src = nm.header->iItem;
		int dest = nm.header->pitem->iOrder;
		if (src != dest && dest != -1)
			MoveColumn(src, dest);
		return 1;
	}
	if (nm.hdr->code == HDN_BEGINDRAG)
	{
		// User is starting to drag a column header
		// save column widths before user reorders them
		// so we can reload them on the end drag
		SaveColumnWidths();
	}
	return 0;
}

LRESULT CDirView::ReflectNotify(LPARAM lParam)
{
	union
	{
		LPARAM lp;
		NMHDR *hdr;
		NMMOUSE *mouse;
		NMITEMACTIVATE *itemactivate;
		NMLISTVIEW *listview;
		NMLVCUSTOMDRAW *lvcustomdraw;
		NMLVDISPINFO *lvdispinfo;
		NMLVKEYDOWN *lvkeydown;
	} const nm = { lParam };
	if (nm.hdr->hwndFrom == m_pFrame->m_wndStatusBar->m_hWnd)
	{
		if (nm.hdr->code == NM_CLICK)
		{
			switch (nm.mouse->dwItemSpec)
			{
			case 1:
				m_pFrame->m_pMDIFrame->PostMessage(WM_COMMAND, ID_TOOLS_FILTERS);
				break;
			}
		}
		return 0;
	}
	switch (nm.hdr->code)
	{
	case LVN_GETDISPINFO:
		ReflectGetdispinfo(nm.lvdispinfo);
		break;
	case LVN_KEYDOWN:
		return ReflectKeydown(nm.lvkeydown);
	case LVN_BEGINLABELEDIT:
		return ReflectBeginLabelEdit(nm.lvdispinfo);
	case LVN_ENDLABELEDIT:
		return ReflectEndLabelEdit(nm.lvdispinfo);
	case LVN_COLUMNCLICK:
		ReflectColumnClick(nm.listview);
		break;
	case NM_CLICK:
		ReflectClick(nm.itemactivate);
		break;
	case LVN_ITEMACTIVATE:
		ReflectItemActivate(nm.itemactivate);
		break;
	case LVN_BEGINDRAG:
		ReflectBeginDrag();
		break;
	case NM_CUSTOMDRAW:
		return ReflectCustomDraw(nm.lvcustomdraw);
	}
	return 0;
}

/** @brief store current column widths into registry */
void CDirView::SaveColumnWidths()
{
	for (int i = 0; i < m_numcols; i++)
	{
		int phy = ColLogToPhys(i);
		if (phy >= 0)
		{
			string_format sWidthKey(_T("WDirHdr_%s_Width"), f_cols[i].regName);
			int w = GetColumnWidth(phy);
			SettingStore.WriteProfileInt(_T("DirView"), sWidthKey.c_str(), w);
		}
	}
}

/**
 * @brief Open dialog to customize dirview columns
 */
void CDirView::OnCustomizeColumns()
{
	// Located in DirViewColHandler.cpp
	OnEditColumns();
	SaveColumnOrders();
}

/**
 * @brief Generate report from dir compare results.
 */
void CDirView::OnToolsGenerateReport()
{
	// Make list of registry keys for columns
	// (needed for XML reports)
	DirCmpReport report(this);

	String errStr;
	if (report.GenerateReport(errStr))
	{
		if (errStr.empty())
			LanguageSelect.MsgBox(IDS_REPORT_SUCCESS, MB_ICONINFORMATION);
		else
			LanguageSelect.MsgBox(IDS_REPORT_ERROR, errStr.c_str(), MB_ICONSTOP);
	}
}

/**
 * @brief Add special items for non-recursive compare
 * to directory view.
 *
 * Currently only special item is ".." for browsing to
 * parent folders.
 * @return number of items added to view
 */
int CDirView::AddSpecialItems()
{
	int retVal = 0;
	int iImgDirUp = CompareStats::DIFFIMG_DIRUP;
	String leftParent, rightParent;
	switch (m_pFrame->AllowUpwardDirectory(leftParent, rightParent))
	{
	case CDirFrame::AllowUpwardDirectory::No:
		iImgDirUp = CompareStats::DIFFIMG_DIRUP_DISABLE;
		// fall through
	default:
		// Add "Parent folder" ("..") item to directory view
		AddNewItem(0, NULL, iImgDirUp, 0);
		retVal = 1;
		// fall through
	case CDirFrame::AllowUpwardDirectory::Never:
		break;
	}
	return retVal;
}

/**
 * @brief Zip selected files from left side.
 */
void CDirView::OnCtxtDirZipLeft()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Left
	).CompressArchive();
}

/**
 * @brief Zip selected files from right side.
 */
void CDirView::OnCtxtDirZipRight()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Right
	).CompressArchive();
}

/**
 * @brief Zip selected files from both sides, using original/altered format.
 */
void CDirView::OnCtxtDirZipBoth()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Original
		|	DirItemEnumerator::Altered
		|	DirItemEnumerator::BalanceFolders
	).CompressArchive();
}

/**
 * @brief Zip selected diffs from both sides, using original/altered format.
 */
void CDirView::OnCtxtDirZipBothDiffsOnly()
{
	if (!HasZipSupport())
	{
		LanguageSelect.MsgBox(IDS_NO_ZIP_SUPPORT, MB_ICONINFORMATION);
		return;
	}

	DirItemEnumerator
	(
		this, LVNI_SELECTED
		|	DirItemEnumerator::Original
		|	DirItemEnumerator::Altered
		|	DirItemEnumerator::BalanceFolders
		|	DirItemEnumerator::DiffsOnly
	).CompressArchive();
}


/**
 * @brief Select all visible items in dir compare
 */
void CDirView::OnSelectAll()
{
	// While the user is renaming an item, select all the edited text.
	if (HEdit *pEdit = GetEditControl())
	{
		pEdit->SetSel(pEdit->GetWindowTextLength());
	}
	else
	{
		int n = GetItemCount();
		for (int i = 0; i < n; i++)
		{
			// Don't select special items (SPECIAL_ITEM_POS)
			if (GetDiffItem(i) != NULL)
				SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
}

/**
 * @brief Resets column widths to defaults.
 */
void CDirView::ResetColumnWidths()
{
	for (int i = 0; i < m_numcols; i++)
	{
		int phy = ColLogToPhys(i);
		if (phy >= 0)
		{
			string_format sWidthKey(_T("WDirHdr_%s_Width"), f_cols[i].regName);
			SettingStore.WriteProfileInt(_T("DirView"), sWidthKey.c_str(), DefColumnWidth);
		}
	}
}

/**
 * @brief Copy selected item left side paths (containing filenames) to clipboard.
 */
void CDirView::OnCopyLeftPathnames()
{
	if (!OpenClipboard())
		return;
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM *di = GetDiffItem(sel);
			if (!di->isSideRightOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String spath = ctxt->GetLeftFilepathAndName(di);
				LPCTSTR path = paths_UndoMagic(&spath.front());
				file.WriteString(path, spath.c_str() + spath.length() - path);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Copy selected item right side paths (containing filenames) to clipboard.
 */
void CDirView::OnCopyRightPathnames()
{
	if (!OpenClipboard())
		return;
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM *di = GetDiffItem(sel);
			if (!di->isSideLeftOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String spath = ctxt->GetRightFilepathAndName(di);
				LPCTSTR path = paths_UndoMagic(&spath.front());
				file.WriteString(path, spath.c_str() + spath.length() - path);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Copy selected item both side paths (containing filenames) to clipboard.
 */
void CDirView::OnCopyBothPathnames()
{
	if (!OpenClipboard())
		return;
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM *di = GetDiffItem(sel);
			if (!di->isSideRightOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String spath = ctxt->GetLeftFilepathAndName(di);
				LPCTSTR path = paths_UndoMagic(&spath.front());
				file.WriteString(path, spath.c_str() + spath.length() - path);
			}
			if (!di->isSideLeftOnly())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				String spath = ctxt->GetRightFilepathAndName(di);
				LPCTSTR path = paths_UndoMagic(&spath.front());
				file.WriteString(path, spath.c_str() + spath.length() - path);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Copy selected item filenames to clipboard.
 */
void CDirView::OnCopyFilenames()
{
	if (!OpenClipboard())
		return;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		int sel = -1;
		while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
		{
			const DIFFITEM *di = GetDiffItem(sel);
			if (!di->isDirectory())
			{
				// Append space between paths. Space is better than
				// EOL since it allows copying to console/command line.
				if (GlobalSize(hMem) != 0)
					file.WriteString(_T(" "), 1);
				file.WriteString(di->left.filename);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

/**
 * @brief Rename a selected item on both sides.
 *
 */
void CDirView::OnItemRename()
{
	EditLabel(GetNextItem(-1, LVNI_SELECTED));
}

/**
 * @brief hide selected item filenames (removes them from the ListView)
 */
void CDirView::OnHideFilenames()
{
	int sel = -1;
	SetRedraw(FALSE);	// Turn off updating (better performance)
	while ((sel = GetNextItem(sel, LVNI_SELECTED)) != -1)
	{
		DIFFITEM *di = GetDiffItem(sel);
		if (di == NULL)
			continue;
		(di->customFlags1 &= ~ViewCustomFlags::VISIBILITY) |= ViewCustomFlags::HIDDEN;
		if (m_bTreeMode && di->isDirectory())
		{
			int count = GetItemCount();
			for (int i = sel + 1; i < count; i++)
			{
				const DIFFITEM *dic = GetDiffItem(i);
				if (!dic->IsAncestor(di))
					break;
				DeleteItem(i--);
				count--;
			}
		}
		DeleteItem(sel--);
		m_nHiddenItems++;
	}
	SetRedraw(TRUE);	// Turn updating back on
}

LRESULT CDirView::ReflectCustomDraw(NMLVCUSTOMDRAW *pNM)
{
	if (pNM->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		OnUpdateStatusNum();
	}
	return 0;
}

/**
 * @brief Called before user start to item label edit.
 *
 * Disable label edit if initiated from a user double-click.
 */
LRESULT CDirView::ReflectBeginLabelEdit(NMLVDISPINFO *pdi)
{
	if ((GetSelectedCount() != 1) || (pdi->item.lParam == NULL))
		return 1;
	// If label edit is allowed.
	// Locate the edit box on the right column in case the user changed the
	// column order.
	const int nColPos = ColLogToPhys(0);

	// Get text from the "File Name" column.
	String sText = GetItemText(pdi->item.iItem, nColPos);
	ASSERT(!sText.empty());
	HEdit *pEdit = GetEditControl();
	pEdit->SetWindowText(sText.c_str());

	String::size_type i = sText.find('|');
	if (i != String::npos)
	{
		// Names differ in case. Pre-select right name so user can easily delete
		// it to make both names uniform.
		pEdit->SetSel(i, -1);
	}
	return 0;
}

/**
 * @brief Called when user done with item label edit.
 */
LRESULT CDirView::ReflectEndLabelEdit(NMLVDISPINFO *pdi)
{
	LRESULT lResult = 0;
	if (pdi->item.pszText && pdi->item.pszText[0])
	{
		// Prevent DocFrame from attempting to return focus to in-place edit.
		SetFocus();
		lResult = DoItemRename(pdi->item.iItem, pdi->item.pszText);
	}
	return lResult;
}

/**
 * @brief Called to update the item count in the status bar
 */
void CDirView::OnUpdateStatusNum()
{
	TCHAR num[20], idx[20], cnt[20];

	int items = GetSelectedCount();
	_itot(items, num, 10);
	m_pFrame->SetStatus(LanguageSelect.FormatMessage(
		items == 1 ? IDS_STATUS_SELITEM1 : IDS_STATUS_SELITEMS, num));

	CMainFrame *const pMDIFrame = m_pFrame->m_pMDIFrame;
	CDocFrame *const pDocFrame = pMDIFrame->GetActiveDocFrame();
	if (pDocFrame == NULL || !pDocFrame->IsChild(m_pWnd))
		return;

	m_pFrame->UpdateCmdUI<ID_L2R>();
	m_pFrame->UpdateCmdUI<ID_R2L>();
	m_pFrame->UpdateCmdUI<ID_MERGE_DELETE>();

	pMDIFrame->UpdateCmdUI<ID_FILE_ENCODING>(items != 0 ? 0 : MF_GRAYED);

	DIFFITEM *di[] = { NULL, NULL };
	items = GetSelectedItems(di);
	pMDIFrame->UpdateCmdUI<ID_MERGE_COMPARE>
	(
		items == 1 && IsItemOpenable(di[0]) ||
		items == 2 && AreItemsOpenable(di[0], di[1]) ?
		MF_ENABLED : MF_GRAYED
	);

	items = GetItemCount();
	int focusItem = GetFocusedItem();

	pMDIFrame->UpdateCmdUI<ID_CURDIFF>( 
		GetItemState(focusItem, LVIS_SELECTED) ? MF_ENABLED : MF_GRAYED);

	BYTE enable = MF_GRAYED;
	int i = focusItem;
	while (--i >= 0)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			enable = MF_ENABLED;
			break;
		}
	}
	pMDIFrame->UpdateCmdUI<ID_PREVDIFF>(enable);

	enable = MF_GRAYED;
	i = focusItem;
	while (++i < items)
	{
		const DIFFITEM *di = GetDiffItem(i);
		if (IsItemNavigableDiff(di))
		{
			enable = MF_ENABLED;
			break;
		}
	}
	pMDIFrame->UpdateCmdUI<ID_NEXTDIFF>(enable);

	String s;
	if (focusItem == -1)
	{
		// No item has focus
		_itot(items, cnt, 10);
		// "Items: %1"
		s = LanguageSelect.FormatMessage(IDS_DIRVIEW_STATUS_FMT_NOFOCUS, cnt);
	}
	else
	{
		// An item has focus
		// Don't show number to special items
		if (GetDiffItem(focusItem) != NULL)
		{
			// Reduce by special items count
			focusItem -= m_nSpecialItems;
			items -= m_nSpecialItems;
			_itot(focusItem + 1, idx, 10);
			_itot(items, cnt, 10);
			// "Item %1 of %2"
			s = LanguageSelect.FormatMessage(IDS_DIRVIEW_STATUS_FMT_FOCUS, idx, cnt);
		}
	}
	pMDIFrame->GetStatusBar()->SetPartText(2, s.c_str());
}

/**
 * @brief Show all hidden items.
 */
void CDirView::OnViewShowHiddenItems()
{
	m_pFrame->SetItemViewFlag(ViewCustomFlags::VISIBLE, ViewCustomFlags::VISIBILITY);
	m_nHiddenItems = 0;
	Redisplay();
}

/**
 * @brief Toggle Tree Mode
 */
void CDirView::OnViewTreeMode()
{
	m_bTreeMode = !m_bTreeMode;
	COptionsMgr::SaveOption(OPT_TREE_MODE, m_bTreeMode); // reverse
	Redisplay();
}

/**
 * @brief Expand all subfolders
 */
void CDirView::OnViewExpandAllSubdirs()
{
	CDiffContext *ctxt = m_pFrame->GetDiffContext();
	DIFFITEM *di = ctxt->GetFirstChildDiff(NULL);
	while (di)
	{
		if (di->isDirectory())
			di->customFlags1 |= ViewCustomFlags::EXPANDED;
		di = ctxt->GetNextDiff(di);
	}
	Redisplay();
}

/**
 * @brief Collapse all subfolders
 */
void CDirView::OnViewCollapseAllSubdirs()
{
	CDiffContext *ctxt = m_pFrame->GetDiffContext();
	DIFFITEM *di = ctxt->GetFirstChildDiff(NULL);
	while (di)
	{
		di->customFlags1 &= ~ViewCustomFlags::EXPANDED;
		di = ctxt->GetNextDiff(di);
	}
	Redisplay();
}

void CDirView::OnViewCompareStatistics()
{
	CompareStatisticsDlg dlg(m_pFrame->GetCompareStats());
	LanguageSelect.DoModal(dlg);
}

/**
 * @brief Allow edit "Paste" when renaming an item.
 */
void CDirView::OnEditCopy()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Copy();
}

/**
 * @brief Allow edit "Cut" when renaming an item.
 */
void CDirView::OnEditCut()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Cut();
}

/**
* @brief Allow edit "Paste" when renaming an item.
 */
void CDirView::OnEditPaste()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Paste();
}

/**
 * @brief Allow edit "Undo" when renaming an item.
 */
void CDirView::OnEditUndo()
{
	if (HEdit *pEdit = GetEditControl())
		pEdit->Undo();
}

LPCTSTR CDirView::SuggestArchiveExtensions()
{
	// 7z can only write 7z, zip, and tar(.gz|.bz2) archives, so don't suggest
	// other formats here.
	static const TCHAR extensions[]
	(
		_T("7z\0*.7z\0")
		//_T("z\0*.z\0")
		_T("zip\0*.zip\0")
		_T("jar (zip)\0*.jar\0")
		_T("ear (zip)\0*.ear\0")
		_T("war (zip)\0*.war\0")
		_T("xpi (zip)\0*.xpi\0")
		//_T("rar\0*.rar\0")
		_T("tar\0*.tar\0")
		_T("tar.z\0*.tar.z\0")
		_T("tar.gz\0*.tar.gz\0")
		_T("tar.bz2\0*.tar.bz2\0")
		//_T("tz\0*.tz\0")
		_T("tgz\0*.tgz\0")
		_T("tbz2\0*.tbz2\0")
		//_T("lzh\0*.lzh\0")
		//_T("cab\0*.cab\0")
		//_T("arj\0*.arj\0")
		//_T("deb\0*.deb\0")
		//_T("rpm\0*.rpm\0")
		//_T("cpio\0*.cpio\0")
		//_T("\0")
	);
	return extensions;
}

bool CDirView::IsShellMenuCmdID(UINT id)
{
	return (id >= LeftCmdFirst) && (id <= RightCmdLast);
}

void CDirView::HandleMenuSelect(WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam) & MF_POPUP)
	{
		HMENU hMenu = reinterpret_cast<HMENU>(lParam);
		MENUITEMINFO mii;
		mii.cbSize = sizeof mii;
		mii.fMask = MIIM_SUBMENU;
		::GetMenuItemInfo(hMenu, LOWORD(wParam), TRUE, &mii);
		HMENU hSubMenu = mii.hSubMenu;
		if (hSubMenu == m_hShellContextMenuLeft)
		{
			mii.hSubMenu = ListShellContextMenu(SIDE_LEFT);
			m_hShellContextMenuLeft = NULL;
		}
		else if (hSubMenu == m_hShellContextMenuRight)
		{
			mii.hSubMenu = ListShellContextMenu(SIDE_RIGHT);
			m_hShellContextMenuRight = NULL;
		}
		if (hSubMenu != mii.hSubMenu)
		{
			::SetMenuItemInfo(hMenu, LOWORD(wParam), TRUE, &mii);
			::DestroyMenu(hSubMenu);
		}
	}
}

/**
 * @brief Handle messages related to correct menu working.
 *
 * We need to requery shell context menu each time we switch from context menu
 * for one side to context menu for other side. Here we check whether we need to
 * requery and call ShellContextMenuHandleMenuMessage.
 */
LRESULT CDirView::HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;
	if (message == WM_INITMENUPOPUP)
	{
		HMenu *const pMenu = reinterpret_cast<HMenu *>(wParam);
		if (pMenu == m_pCompareAsScriptMenu)
			m_pCompareAsScriptMenu = theApp.m_pMainWnd->SetScriptMenu(pMenu, "CompareAs.Menu");
	}
	m_pShellContextMenuLeft->HandleMenuMessage(message, wParam, lParam, res);
	m_pShellContextMenuRight->HandleMenuMessage(message, wParam, lParam, res);
	return res;
}

/** 
 * @brief Retrieves file list of all selected files, and store them like 
 * file_path1\nfile_path2\n...file_pathN\n
 *
 * @param [out] pstm Stream where file list will be stored
 */
void CDirView::PrepareDragData(UniStdioFile &file)
{
	const CDiffContext *ctxt = m_pFrame->GetDiffContext();
	int i = -1;
	while ((i = GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		const DIFFITEM *diffitem = GetDiffItem(i);
		// check for special items (e.g not "..")
		if (diffitem->diffcode == 0)
		{
			continue;
		}

		if (diffitem->isSideLeftOnly())
		{
			String spath = ctxt->GetLeftFilepathAndName(diffitem);
			LPCTSTR path = paths_UndoMagic(&spath.front());
			file.WriteString(path, spath.c_str() + spath.length() - path);
		}
		else if (diffitem->isSideRightOnly())
		{
			String spath = ctxt->GetRightFilepathAndName(diffitem);
			LPCTSTR path = paths_UndoMagic(&spath.front());
			file.WriteString(path, spath.c_str() + spath.length() - path);
		}
		else if (diffitem->isSideBoth())
		{
			// when both files equal, there is no difference between what file to drag 
			// so we put file from the left panel
			String spath = ctxt->GetLeftFilepathAndName(diffitem);
			LPCTSTR path = paths_UndoMagic(&spath.front());
			file.WriteString(path, spath.c_str() + spath.length() - path);

			// if both files are different then we also put file from the right panel 
			if (diffitem->isResultDiff())
			{
				file.WriteString(_T("\n"), 1); // end of left file path
				String spath = ctxt->GetRightFilepathAndName(diffitem);
				LPCTSTR path = paths_UndoMagic(&spath.front());
				file.WriteString(path, spath.c_str() + spath.length() - path);
			}
		}
		file.WriteString(_T("\n"), 1); // end of file path
	}
	file.WriteString(_T(""), 1); // include terminating zero
}

/** 
 * @brief Drag files/directories from folder compare listing view. 
 */ 
void CDirView::ReflectBeginDrag()
{
	DWORD de;
	DoDragDrop(this, this, DROPEFFECT_COPY, &de);
}

HRESULT CDirView::QueryInterface(REFIID iid, void **ppv)
{
    static const QITAB rgqit[] = 
    {   
        QITABENT(CDirView, IDropSource),
        QITABENT(CDirView, IDataObject),
        { 0 }
    };
    return QISearch(this, rgqit, iid, ppv);
}

ULONG CDirView::AddRef()
{
	return 1;
}

ULONG CDirView::Release()
{
	return 1;
}

HRESULT CDirView::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed)
		return DRAGDROP_S_CANCEL;
	if (!(grfKeyState & MK_LBUTTON))
		return DRAGDROP_S_DROP;
	return S_OK;
}

HRESULT CDirView::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT CDirView::GetData(LPFORMATETC etc, LPSTGMEDIUM stg)
{
	if (etc->cfFormat != CF_UNICODETEXT)
		return DV_E_FORMATETC;
	stg->tymed = TYMED_HGLOBAL;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	stg->hGlobal = file.CreateStreamOnHGlobal();
	PrepareDragData(file);
	stg->pUnkForRelease = NULL;
	return S_OK;
}

HRESULT CDirView::GetDataHere(LPFORMATETC, LPSTGMEDIUM)
{
	return E_NOTIMPL;
}

HRESULT CDirView::QueryGetData(LPFORMATETC etc)
{
	if (etc->cfFormat != CF_UNICODETEXT)
		return DV_E_FORMATETC;
	return S_OK;
}

HRESULT CDirView::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC)
{
	return E_NOTIMPL;
}

HRESULT CDirView::SetData(LPFORMATETC, LPSTGMEDIUM, BOOL)
{
	return E_NOTIMPL;
}

HRESULT CDirView::EnumFormatEtc(DWORD, LPENUMFORMATETC*)
{
	return E_NOTIMPL;
}

HRESULT CDirView::DAdvise(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD)
{
	return E_NOTIMPL;
}

HRESULT CDirView::DUnadvise(DWORD)
{
	return E_NOTIMPL;
}

HRESULT CDirView::EnumDAdvise(LPENUMSTATDATA*)
{
	return E_NOTIMPL;
}
