////////////////////////////////////////////////////////////////////////////
//  File:       ceditreplacedlg.cpp
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CEditReplaceDlg dialog, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: regular expressions, go to line and things ...
//  +   FEATURE: some other things I've forgotten ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////
/**
 *  @file ceditreplacedlg.cpp
 *
 *  @brief Implementation of Replace-dialog.
 */
#include "StdAfx.h"
#include "LanguageSelect.h"
#include "Locality.h"
#include "resource.h"
#include "ccrystaleditview.h"
#include "ceditreplacedlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditReplaceDlg dialog


CEditReplaceDlg::CEditReplaceDlg(CCrystalEditView * pBuddy)
	: ODialog(IDD_EDIT_REPLACE)
	, m_pBuddy(pBuddy)
	, m_nScope(-1)
	, m_nCaptures(-1)
	, m_bEnableScopeSelection(true)
	, m_bConfirmed(false)
{
}

void CEditReplaceDlg::UpdateRegExp()
{
	if (m_bRegExp)
	{
		GetDlgItem(IDC_EDIT_WHOLE_WORD)->EnableWindow(FALSE);
		m_bWholeWord = FALSE;
	}
	else
	{
		GetDlgItem(IDC_EDIT_WHOLE_WORD)->EnableWindow(TRUE);
	}
}

template<ODialog::DDX_Operation op>
bool CEditReplaceDlg::UpdateData()
{
	DDX_Check<op>(IDC_EDIT_MATCH_CASE, m_bMatchCase);
	DDX_Check<op>(IDC_EDIT_WHOLE_WORD, m_bWholeWord);
	DDX_Check<op>(IDC_EDIT_REGEXP, m_bRegExp);
	DDX_CBStringExact<op>(IDC_EDIT_FINDTEXT, m_sText);
	DDX_CBStringExact<op>(IDC_EDIT_REPLACE_WITH, m_sNewText);
	DDX_Check<op>(IDC_EDIT_SCOPE_SELECTION, m_nScope, 0);
	DDX_Check<op>(IDC_EDIT_SCOPE_WHOLE_FILE, m_nScope, 1);
	DDX_Check<op>(IDC_EDIT_SCOPE_DONT_WRAP, m_bNoWrap);
	return true;
}

LRESULT CEditReplaceDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDCANCEL:
			OnCancel();
			break;
		case MAKEWPARAM(IDC_EDIT_REPLACE, BN_CLICKED):
			OnEditReplace();
			break;
		case MAKEWPARAM(IDC_EDIT_REPLACE_ALL, BN_CLICKED):
			OnEditReplaceAll();
			break;
		case MAKEWPARAM(IDC_EDIT_SKIP, BN_CLICKED):
			OnEditSkip();
			break;
		case MAKEWPARAM(IDC_EDIT_REGEXP, BN_CLICKED):
			OnRegExp();
			break;
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_SELCHANGE):
			OnChangeSelected();
			// fall through
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_EDITCHANGE):
			OnChangeEditText();
			break;
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_DROPDOWN):
		case MAKEWPARAM(IDC_EDIT_REPLACE_WITH, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			RegisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE), MOD_SHIFT, VK_DELETE);
			break;
		case MAKEWPARAM(IDC_EDIT_FINDTEXT, CBN_CLOSEUP):
		case MAKEWPARAM(IDC_EDIT_REPLACE_WITH, CBN_CLOSEUP):
			UnregisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE));
			break;
		}
		break;
	case WM_HOTKEY:
		if (HIWORD(wParam) == VK_DELETE)
		{
			HComboBox *const pCb = static_cast<HComboBox *>(GetDlgItem(LOWORD(wParam)));
			pCb->DeleteString(pCb->GetCurSel());
			OnChangeEditText();
		}
		break;
	case WM_SHOWWINDOW:
		if (wParam != FALSE)
			CenterWindow(m_pWnd, m_pBuddy->m_pWnd);
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CEditReplaceDlg message handlers

void CEditReplaceDlg::OnChangeSelected()
{
	int sel = m_pCbFindText->GetCurSel();
	if (sel != CB_ERR)
	{
		String sText;
		m_pCbFindText->GetLBText(sel, sText);
		m_pCbFindText->SetWindowText(sText.c_str());
	}
}

void CEditReplaceDlg::OnChangeEditText()
{
	UpdateData<Get>();
	UpdateControls();
}

void CEditReplaceDlg::OnCancel()
{
	UpdateData<Get>();
	EndDialog(IDCANCEL);
}

BOOL CEditReplaceDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	m_pCbFindText = static_cast<HSuperComboBox *>(GetDlgItem(IDC_EDIT_FINDTEXT));
	m_pCbReplText = static_cast<HSuperComboBox *>(GetDlgItem(IDC_EDIT_REPLACE_WITH));

	m_pCbFindText->LoadState(_T("Files\\ReplaceInFile"));
	m_pCbReplText->LoadState(_T("Files\\ReplaceWithInFile"));
	GetDlgItemText(IDC_EDIT_REPLACE_WITH, m_sNewText);
	UpdateData<Set>();

	UpdateControls();
	GetDlgItem(IDC_EDIT_SCOPE_SELECTION)->EnableWindow(m_bEnableScopeSelection);
	m_nCaptures = -1;

	return TRUE;
}

int CEditReplaceDlg::DoHighlightText(BOOL bNotifyIfNotFound)
{
	DWORD dwSearchFlags = 0;
	if (m_bMatchCase)
		dwSearchFlags |= FIND_MATCH_CASE;
	if (m_bWholeWord)
		dwSearchFlags |= FIND_WHOLE_WORD;
	if (m_bRegExp)
	{
		dwSearchFlags |= FIND_REGEXP;
		if (m_sText[0] == _T('^'))
			m_ptFoundAt.x = 0;
	}

	int nCaptures;
	if (m_nScope == 0)
	{
		// Searching selection only
		nCaptures = m_pBuddy->FindTextInBlock(m_sText.c_str(), m_ptFoundAt,
			m_ptBlockBegin, m_ptBlockEnd, dwSearchFlags, FALSE, m_ptFoundAt, m_captures);
	}
	else
	{
		// Searching whole text, (no) wrap
		nCaptures = m_pBuddy->FindText(m_sText.c_str(), m_ptFoundAt, dwSearchFlags, !m_bNoWrap, m_ptFoundAt, m_captures);
	}

	if (nCaptures < 0)
	{
		if (bNotifyIfNotFound) 
			LanguageSelect.Format(IDS_EDIT_TEXT_NOT_FOUND, m_sText.c_str()).MsgBox(MB_ICONINFORMATION);
		if (m_nScope == 0)
			m_ptCurrentPos = m_ptBlockBegin;
		return -1;
	}

	m_pBuddy->HighlightText(m_ptFoundAt, m_pBuddy->m_nLastFindWhatLen);
	return nCaptures;
}

void CEditReplaceDlg::OnEditSkip()
{
	UpdateData<Get>();
	m_pCbFindText->SaveState(_T("Files\\ReplaceInFile"));
	m_pCbReplText->SaveState(_T("Files\\ReplaceWithInFile"));
	m_bConfirmed = true;
	if (m_nCaptures < 0)
	{
		m_ptFoundAt = m_ptCurrentPos;
		m_nCaptures = DoHighlightText(TRUE);
		SetDefID(m_nCaptures >= 0 ? IDC_EDIT_REPLACE : IDC_EDIT_SKIP);
		return;
	}
	if (m_pBuddy->m_nLastFindWhatLen)
	{
		m_ptFoundAt.x += 1;
	}
	else if (m_ptFoundAt.y + 1 < m_pBuddy->GetLineCount())
	{
		m_ptFoundAt.x = 0;
		m_ptFoundAt.y++;
	}
	else
	{
		m_nCaptures = -1;
		return;
	}
	m_nCaptures = DoHighlightText(TRUE);
	SetDefID(m_nCaptures >= 0 ? IDC_EDIT_REPLACE : IDC_EDIT_SKIP);
}

void CEditReplaceDlg::ReplaceSelection()
{
	LPCTSTR p = m_sNewText.c_str();
	int n = m_sNewText.size();
	String s;
	if (m_bRegExp)
	{
		String t;
		while (LPCTSTR q = _tcschr(p, '\\'))
		{
			s.append(p, q);
			switch (TCHAR c = *++q)
			{
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				int i;
				i = c - '0';
				if (i < m_nCaptures)
				{
					if (t.empty())
					{
						POINT ptStart, ptEnd;
						m_pBuddy->GetSelection(ptStart, ptEnd);
						m_pBuddy->GetText(ptStart, ptEnd, t);
					}
					i *= 2; 
					s.append(t.c_str() + m_captures[i] - m_captures[0],
						t.c_str() + m_captures[i + 1] - m_captures[0]);
				}
				++q;
				break;

				for (;;)
				{
				case 'a':
					c = '\a';
					break;
				case 'b':
					c = '\b';
					break;
				case 'f':
					c = '\f';
					break;
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				case 'v':
					c = '\v';
					break;
				}
				// fall through
			default:
				s.push_back(c);
				++q;
				break;
			}
			p = q;
		}
		s.append(p);
		p = s.c_str();
		n = s.length();
	}
	m_pBuddy->ReplaceSelection(p, n);
}

void CEditReplaceDlg::OnEditReplace()
{
	UpdateData<Get>();
	m_pCbFindText->SaveState(_T("Files\\ReplaceInFile"));
	m_pCbReplText->SaveState(_T("Files\\ReplaceWithInFile"));
	m_bConfirmed = true;
	if (m_nCaptures < 0)
	{
		m_ptFoundAt = m_ptCurrentPos;
		m_nCaptures = DoHighlightText(TRUE);
		SetDefID(m_nCaptures >= 0 ? IDC_EDIT_REPLACE : IDC_EDIT_SKIP);
		return;
	}

	//  We have highlighted text
	ReplaceSelection();

	//  Manually recalculate points
	if (m_bEnableScopeSelection)
	{
		if (m_ptBlockBegin.y == m_ptFoundAt.y && m_ptBlockBegin.x > m_ptFoundAt.x)
		{
			m_ptBlockBegin.x -= m_pBuddy->m_nLastFindWhatLen;
			m_ptBlockBegin.x += m_pBuddy->m_nLastReplaceLen;
		}
		if (m_ptBlockEnd.y == m_ptFoundAt.y && m_ptBlockEnd.x > m_ptFoundAt.x)
		{
			m_ptBlockEnd.x -= m_pBuddy->m_nLastFindWhatLen;
			m_ptBlockEnd.x += m_pBuddy->m_nLastReplaceLen;
		}
	}
	if (m_pBuddy->m_nLastFindWhatLen)
	{
		m_ptFoundAt.x += m_pBuddy->m_nLastReplaceLen;
		m_ptFoundAt = m_pBuddy->GetCursorPos();
	}
	else if (m_ptFoundAt.y + 1 < m_pBuddy->GetLineCount())
	{
		m_ptFoundAt.x = 0;
		m_ptFoundAt.y++;
	}
	else
	{
		m_nCaptures = -1;
		return;
	}
	m_nCaptures = DoHighlightText(TRUE);
}

void CEditReplaceDlg::OnEditReplaceAll()
{
	UpdateData<Get>();
	m_pCbFindText->SaveState(_T("Files\\ReplaceInFile"));
	m_pCbReplText->SaveState(_T("Files\\ReplaceWithInFile"));
	m_bConfirmed = true;

	int nNumReplaced = 0;
	BOOL bWrapped = FALSE;
	WaitStatusCursor waitCursor;

	if (m_nCaptures < 0)
	{
		m_ptFoundAt = m_ptCurrentPos;
		m_nCaptures = DoHighlightText(FALSE);
	}

	POINT m_ptFirstFound = m_ptFoundAt;

	while (m_nCaptures >= 0)
	{
		//  We have highlighted text
		m_pBuddy->m_nLastReplaceLen = 0;
		ReplaceSelection();
		nNumReplaced++;

		//  Manually recalculate points
		if (m_bEnableScopeSelection)
		{
			if (m_ptBlockBegin.y == m_ptFoundAt.y && m_ptBlockBegin.x > m_ptFoundAt.x)
			{
				m_ptBlockBegin.x -= m_pBuddy->m_nLastFindWhatLen;
				m_ptBlockBegin.x += m_pBuddy->m_nLastReplaceLen;
			}
			if (m_ptBlockEnd.y == m_ptFoundAt.y && m_ptBlockEnd.x > m_ptFoundAt.x)
			{
				m_ptBlockEnd.x -= m_pBuddy->m_nLastFindWhatLen;
				m_ptBlockEnd.x += m_pBuddy->m_nLastReplaceLen;
			}
		}
		// recalculate m_ptFirstFound
		if (m_ptFirstFound.y == m_ptFoundAt.y && m_ptFirstFound.x > m_ptFoundAt.x)
		{
			m_ptFirstFound.x -= m_pBuddy->m_nLastFindWhatLen;
			m_ptFirstFound.x += m_pBuddy->m_nLastReplaceLen;
		}

		// calculate the end of the current replacement
		POINT m_ptCurrentReplacedEnd;
		m_ptCurrentReplacedEnd.y = m_ptFoundAt.y;
		m_ptCurrentReplacedEnd.x = m_ptFoundAt.x + m_pBuddy->m_nLastReplaceLen;

		// m_ptFoundAt.x has two meanings:
		// (1) One is the position of the word that was found.
		// (2) The other is next position to search.
		// The code below calculates the latter.
		if (m_pBuddy->m_nLastFindWhatLen || m_pBuddy->m_nLastReplaceLen)
		{
			m_ptFoundAt.x += m_pBuddy->m_nLastReplaceLen;
			if (m_pBuddy->m_nLastFindWhatLen)
			{
				m_ptFoundAt = m_pBuddy->GetCursorPos();
			}
			else
			{
				m_ptFoundAt.x = 0;
				m_ptFoundAt.y++;
			}
		}
		else if (m_ptFoundAt.y + 1 < m_pBuddy->GetLineCount())
		{
			m_ptFoundAt.x = 0;
			m_ptFoundAt.y++;
		}
		else
		{
			m_nCaptures = -1;
			break;
		}

		// find the next instance
		m_nCaptures = DoHighlightText(FALSE);

		// detect if we just wrapped at end of file
		if (m_ptFoundAt.y < m_ptCurrentReplacedEnd.y ||
			(m_ptFoundAt.y == m_ptCurrentReplacedEnd.y && m_ptFoundAt.x < m_ptCurrentReplacedEnd.x))
		{
			bWrapped = TRUE;
		}

		// after wrapping, stop at m_ptFirstFound
		// so we don't replace twice when replacement string includes replaced string 
		// (like replace "here" with "there")
		if (bWrapped)
		{
			if (m_ptFoundAt.y > m_ptFirstFound.y ||
				(m_ptFoundAt.y == m_ptFirstFound.y && m_ptFoundAt.x >= m_ptFirstFound.x))
			{
				break;
			}
		}
	}
	// Let user know how many strings were replaced
	LanguageSelect.FormatStrings(
		IDS_NUM_REPLACED, NumToStr(nNumReplaced).c_str()
	).MsgBox(MB_ICONINFORMATION|MB_DONT_DISPLAY_AGAIN);
}

void CEditReplaceDlg::OnRegExp()
{
	UpdateData<Get>();
	UpdateRegExp();
	UpdateData<Set>();
}

void CEditReplaceDlg::UpdateControls()
{
	bool enable = !m_sText.empty();
	GetDlgItem(IDC_EDIT_SKIP)->EnableWindow(enable);
	GetDlgItem(IDC_EDIT_REPLACE)->EnableWindow(enable);
	GetDlgItem(IDC_EDIT_REPLACE_ALL)->EnableWindow(enable);
	UpdateRegExp();
}

void CEditReplaceDlg::SetScope(bool bWithSelection)
{
	m_nScope = bWithSelection ? 0 : 1;
}
