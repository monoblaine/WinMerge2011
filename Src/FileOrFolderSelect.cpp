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
 * @file  FileOrFolderSelect.cpp
 *
 * @brief Implementation of the file and folder selection routines.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "Environment.h"
#include "FileOrFolderSelect.h"
#include "coretools.h"
#include "paths.h"

static int CALLBACK BrowseCallbackProc(HWND, UINT, LPARAM, LPARAM);

/**
 * @brief Helper function for selecting folder or file.
 * This function shows standard Windows file selection dialog for selecting
 * file or folder to open or file to save. The last parameter @p is_open selects
 * between open or save modes. Biggest difference is that in save-mode Windows
 * asks if user wants to override existing file.
 * @param [in] parent Handle to parent window
 * @param [out] path Selected path is returned in this string
 * @param [in] titleid Resource string ID for dialog title.
 * @param [in] filterid 0 or STRING ID for filter string
 *     - 0 means "All files (*.*)". Note the string formatting!
 * @param [in] is_open Selects Open/Save -dialog (mode).
 * @note Be careful when setting @p parent to NULL as there are potential
 * modality problems with this. Dialog can be lost behind other windows!
 * @param [in] defaultExtension Extension to append if user doesn't provide one
 */
BOOL SelectFile(HWND parent, String &path,
	UINT titleid, UINT filterid, BOOL is_open, LPCTSTR defaultExtension)
{
	ASSERT(parent != NULL);

	// This will tell common file dialog what to show
	// and also this will hold its return value
	TCHAR sSelectedFile[MAX_PATH];

	String title = LanguageSelect.LoadString(titleid);
	String filters = LanguageSelect.LoadString(filterid);
	// Convert extension mask from MFC style separators ('|')
	//  to Win32 style separators ('\0')
	stl::replace(filters.begin(), filters.end(), _T('|'), _T('\0'));
	paths_UndoMagic(path);

	OPENFILENAME ofn;
	memset(&ofn, 0, OPENFILENAME_SIZE_VERSION_400);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = parent;
	ofn.lpstrFilter = filters.c_str();
	ofn.nFilterIndex = 1;
	if (paths_EndsWithSlash(path.c_str()))
	{
		ofn.lpstrInitialDir = path.c_str();
		sSelectedFile[0] = _T('\0');
	}
	else
	{
		lstrcpyn(sSelectedFile, path.c_str(), MAX_PATH);
	}
	ofn.lpstrFile = sSelectedFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title.c_str();
	ofn.lpstrDefExt = defaultExtension;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

	const BOOL bRetVal = (is_open ? GetOpenFileName : GetSaveFileName)(&ofn);
	// common file dialog populated sSelectedFile variable's buffer
	SetCurrentDirectory(env_GetWindowsDirectory().c_str()); // Free handle held by GetOpenFileName

	if (bRetVal)
		path = sSelectedFile;
	
	return bRetVal;
}

/** 
 * @brief Helper function for selecting directory
 * @param [in] parent Handle to parent window
 * @param [out] path Selected path is returned in this string
 * @param [in] titleid Resource string ID for dialog title.
 * @return TRUE if valid folder selected (not cancelled)
 */
BOOL SelectFolder(HWND parent, String &path, UINT titleid)
{
	ASSERT(parent != NULL);

	BOOL bRet = FALSE;
	String title = LanguageSelect.LoadString(titleid);

	TCHAR szPath[MAX_PATH];

	BROWSEINFO bi;
	bi.hwndOwner = parent;
	bi.pidlRoot = NULL;  // Start from desktop folder
	bi.pszDisplayName = szPath;
	bi.lpszTitle = title.c_str();
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_VALIDATE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = reinterpret_cast<LPARAM>(path.c_str());

	if (LPITEMIDLIST pidl = SHBrowseForFolder(&bi))
	{
		if (SHGetPathFromIDList(pidl, szPath))
		{
			path = szPath;
			bRet = TRUE;
		}
		LPMALLOC pMalloc;
		SHGetMalloc(&pMalloc);
		pMalloc->Free(pidl);
		pMalloc->Release();
	}
	return bRet;
}

/**
 * @brief Callback function for setting selected folder for folder dialog.
 */
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}


/** 
 * @brief Shows file/folder selection dialog.
 *
 * We need this custom function so we can select files and folders with the
 * same dialog.
 * - If existing filename is selected return it
 * - If filename in (CFileDialog) editbox and current folder doesn't form
 * a valid path to file, return current folder.
 * @param [in] parent Handle to parent window. Can be a NULL, but then
 *     CMainFrame is used which can cause modality problems.
 * @param [out] path Selected folder/filename
 * @return TRUE if user choosed a file/folder, FALSE if user canceled dialog.
 */
BOOL SelectFileOrFolder(HWND parent, String &path, const String &filter)
{
	ASSERT(parent != NULL);

	String title = LanguageSelect.LoadString(IDS_OPEN_TITLE);
	String filters = LanguageSelect.LoadString(IDS_ALLFILES);
	string_replace(filters, _T("||"), _T("|"));
	if (String::size_type len = filter.size())
	{
		filters.append(filter.c_str(), len + 1);
		filters.append(filter.c_str(), len + 1);
	}
	// Convert extension mask from MFC style separators ('|')
	//  to Win32 style separators ('\0')
	stl::replace(filters.begin(), filters.end(), _T('|'), _T('\0'));

	String sSelectedFile = LanguageSelect.LoadString(IDS_DIRSEL_TAG);
	// Set initial filename to folder selection tag
	sSelectedFile.push_back(_T('.')); // Treat it as filename
	sSelectedFile.resize(MAX_PATH);

	OPENFILENAME ofn;
	memset(&ofn, 0, OPENFILENAME_SIZE_VERSION_400);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = parent;
	ofn.lpstrFilter = filters.c_str();
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = &sSelectedFile.front();
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrInitialDir = path.c_str();
	ofn.lpstrTitle = title.c_str();
	ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOTESTFILECREATE;

	BOOL bRetVal = GetOpenFileName(&ofn);
	// common file dialog populated sSelectedFile variable's buffer
	SetCurrentDirectory(env_GetWindowsDirectory().c_str()); // Free handle held by GetOpenFileName

	if (bRetVal)
	{
		path = ofn.lpstrFile;
		if (!PathFileExists(path.c_str()))
		{
			// We have a valid folder name, but propably garbage as a filename.
			// Return folder name
			path.resize(path.rfind(_T('\\')) + 1);
			if (ofn.nFilterIndex == 2)
			{
				// Filename from other side was selected from filter combobox.
				path += filter;
			}
		}
	}
	return bRetVal;
}
