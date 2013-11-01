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
 * @file  codepage.cpp
 */
// RCS ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "LanguageSelect.h" // for CLanguageSelect::GetLangId()
#include "FileLocation.h"
#include "DiffFileInfo.h"
#include "codepage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int f_nDefaultCodepage; // store the default codepage as specified by user in options

// map number to bit code
enum { CP_SUPPORTED_FLAG = 0x01, CP_INSTALLED_FLAG = 0x10 };
static stl::map<int, int> f_codepage_status;

/**
 * @brief Update the appropriate default codepage
 */
void updateDefaultCodepage(int cpDefaultMode, int customCodepage)
{
	int wLangId;
	switch (cpDefaultMode)
	{
	case 0:
		f_nDefaultCodepage = GetACP();
		break;
	case 1:
		TCHAR buff[32];
		wLangId = LanguageSelect.GetLangId();
		if (GetLocaleInfo(wLangId, LOCALE_IDEFAULTANSICODEPAGE, buff, _countof(buff)))
			f_nDefaultCodepage = _ttol(buff);
		else
			f_nDefaultCodepage = GetACP();
		break;
	case 2:
		f_nDefaultCodepage = customCodepage;
		break;
	default:
		// no other valid option
		ASSERT(FALSE);
		f_nDefaultCodepage = GetACP();
	}
}

/**
 * @brief Get appropriate default codepage 
 */
int getDefaultCodepage()
{
	return f_nDefaultCodepage;
}

/**
 * @brief Callback used by initializeCodepages
 */
static BOOL CALLBACK EnumInstalledCodePagesProc(LPTSTR lpCodePageString)
{
	int codepage = _ttol(lpCodePageString);
	if (codepage)
	{
		f_codepage_status[codepage] |= CP_INSTALLED_FLAG;
	}
	return TRUE; // continue enumeration
}

/**
 * @brief Callback used by initializeCodepages
 */
static BOOL CALLBACK EnumSupportedCodePagesProc(LPTSTR lpCodePageString)
{
	int codepage = _ttol(lpCodePageString);
	if (codepage)
	{
		f_codepage_status[codepage] |= CP_SUPPORTED_FLAG;
	}
	return TRUE; // continue enumeration
}

/**
 * @brief Load information about codepages into local cache
 */
static void initialize()
{
	static int f_bInitialized = false;
	if (f_bInitialized)
		return;
	EnumSystemCodePages(EnumInstalledCodePagesProc, CP_INSTALLED);
	EnumSystemCodePages(EnumSupportedCodePagesProc, CP_SUPPORTED);
	f_bInitialized = true;
}

/**
 * @brief Return true if specified codepage is installed on this system (according to EnumSystemCodePages)
 */
bool isCodepageInstalled(int codepage)
{
	initialize();
    // the following line will insert an extra element in the map if not already present
    // but its value will be 0 which means not installed nor supported
	return (f_codepage_status[codepage] & CP_INSTALLED_FLAG) == CP_INSTALLED_FLAG;
}

int isCodepageDBCS(int codepage)
{
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd318667
	switch (codepage)
	{
	case 932: case 936: case 949: case 950: case 1361:
		return codepage;
	}
	return 0;
}
