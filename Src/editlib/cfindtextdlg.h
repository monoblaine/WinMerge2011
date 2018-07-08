////////////////////////////////////////////////////////////////////////////
//  File:       cfindtextdlg.h
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Declaration of the CFindTextDlg dialog, a part of Crystal Edit -
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
//  +   FEATURE: see cpps ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////
/**
 *  @file cfindtextdlg.h
 *
 *  @brief Declaration Find-dialog.
 */
#pragma once

#include "SuperComboBox.h"

class CCrystalTextView;

/////////////////////////////////////////////////////////////////////////////
// CFindTextDlg dialog

class CFindTextDlg
	: ZeroInit<CFindTextDlg>
	, public ODialog
{
private:
	void UpdateControls();
	CCrystalTextView *const m_pBuddy;

	// Construction
public:
	explicit CFindTextDlg(CCrystalTextView *);

	POINT m_ptCurrentPos;
	bool m_bConfirmed;

	// Dialog Data
	HSuperComboBox *m_pCbFindText;
	int m_nDirection;
	BOOL m_bMatchCase;
	String m_sText;
	BOOL m_bWholeWord;
	BOOL m_bRegExp;
	BOOL m_bNoWrap;

	// Overrides
	// ClassWizard generated virtual function overrides
protected :
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

	void UpdateRegExp();

	// Generated message map functions
	void OnOK();
	void OnChangeEditText();
	void OnChangeSelected();
	void OnCancel();
	void OnRegExp();
};
