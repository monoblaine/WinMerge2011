/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997  Dean P. Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  DirFrame.h
 *
 * @brief Declaration file for CDirFrame
 *
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDirFrame frame

#include "DiffContext.h"

class CDirView;
class CHexMergeFrame;
typedef std::list<CChildFrame *> MergeDocPtrList;
typedef std::list<CHexMergeFrame *> HexMergeDocPtrList;
class CTempPathContext;
struct FileActionItem;

/**
 * @brief Frame window for Directory Compare window
 */
class CDirFrame : public CDocFrame, private CFloatFlags
{
// Attributes
public:
	CDirFrame(CMainFrame *);

	CDirView *const m_pDirView;

// Operations
public:
	virtual FRAMETYPE GetFrameType() const { return FRAME_FOLDER; }
	BOOL PreTranslateMessage(MSG *);
	void SetStatus(LPCTSTR szStatus);
	void SetFilterStatusDisplay(LPCTSTR szFilter);
	BOOL GetLeftReadOnly() const { return m_bROLeft; }
	BOOL GetRightReadOnly() const { return m_bRORight; }
	void SetLeftReadOnly(BOOL bReadOnly);
	void SetRightReadOnly(BOOL bReadOnly);
	bool AddToCollection(FileLocation &, FileLocation &);
	HStatusBar *m_wndStatusBar;
	void UpdateResources();
	void CreateClient();
	void AlignStatusBar(int cx);

	template<UINT>
	void UpdateCmdUI();

protected:

	BOOL m_bROLeft; /**< Is left side read-only */
	BOOL m_bRORight; /**< Is right side read-only */

	virtual ~CDirFrame();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	bool PreventFromClosing();

	template<UINT>
	LRESULT OnWndMsg(WPARAM, LPARAM);

private:
	static const LONG FloatScript[];
// Attributes
public:
	CTempPathContext *m_pTempPathContext;
// Operations
public:
	bool CloseMergeDocs();
	CChildFrame *GetMergeDocForDiff();
	CHexMergeFrame *GetHexMergeDocForDiff();
	bool CanFrameClose();

// Implementation
public:
	bool InitCompare(LPCTSTR pszLeft, LPCTSTR pszRight, int nRecursive, CTempPathContext *);
	void InitMrgmanCompare();
	void Rescan(int nCompareSelected = 0);
	int GetRecursive() const { return m_nRecursive; }
	void CompareReady();
	void UpdateChangedItem(const CChildFrame *);
	DIFFITEM *FindItemFromPaths(LPCTSTR pathLeft, LPCTSTR pathRight);
	void ReloadItemStatus(DIFFITEM *, bool bLeft, bool bRight);
	void Redisplay();
	void AddMergeDoc(CChildFrame *);
	void AddMergeDoc(CHexMergeFrame *);
	void MergeDocClosing(CChildFrame *);
	void MergeDocClosing(CHexMergeFrame *);
	bool UpdateDiffAfterOperation(const FileActionItem &, bool bMakeTargetItemWritable);
	void UpdateHeaderPath(BOOL bLeft);
	void AbortCurrentScan();
	void SetDescriptions(const String &strLeftDesc, const String &strRightDesc);
	void ApplyLeftDisplayRoot(String &);
	void ApplyRightDisplayRoot(String &);

	bool IsShowable(const DIFFITEM *) const;

	CDiffContext *GetDiffContext() { return m_pCtxt; }
	struct AllowUpwardDirectory
	{
		enum ReturnCode
		{
			Never,
			No,
			ParentIsRegularPath,
			ParentIsTempPath
		};
	};
	AllowUpwardDirectory::ReturnCode AllowUpwardDirectory(String &leftParent, String &rightParent);
	void SetItemViewFlag(DIFFITEM *, UINT flag, UINT mask);
	void SetItemViewFlag(UINT flag, UINT mask);
	CompareStats *GetCompareStats() const { return m_pCompareStats; };
	void DeleteTempPathContext();

private:
	bool InitContext(LPCTSTR pszLeft, LPCTSTR pszRight, int nRecursive, DWORD dwContext);
	void DeleteContext();

	// Implementation data
	CDiffContext *m_pCtxt; /**< Pointer to compare results-data */
	ListEntry m_root;
	CompareStats *const m_pCompareStats; /**< Compare statistics */
	MergeDocPtrList m_MergeDocs; /**< List of file compares opened from this compare */
	HexMergeDocPtrList m_HexMergeDocs; /**< List of hex file compares opened from this compare */
	int m_nRecursive; /**< Is current compare recursive? (ternary logic - 2 means flat) */
	String m_strLeftDesc; /**< Left side desription text */
	String m_strRightDesc; /**< Left side desription text */
};
