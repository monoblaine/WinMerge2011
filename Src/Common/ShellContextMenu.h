/////////////////////////////////////////////////////////////////////////////
//	  WinMerge:  an interactive diff/merge utility
//	  Copyright (C) 1997  Dean P. Grimm
//
//	  This program is free software; you can redistribute it and/or modify
//	  it under the terms of the GNU General Public License as published by
//	  the Free Software Foundation; either version 3 of the License, or
//	  (at your option) any later version.
//
//	  This program is distributed in the hope that it will be useful,
//	  but WITHOUT ANY WARRANTY; without even the implied warranty of
//	  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//	  GNU General Public License for more details.
//
//	  You should have received a copy of the GNU General Public License
//	  along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 *	@file ShellContextMenu.h
 *
 *	@brief Declaration of class CShellContextMenu
 */
#pragma once

#include <MyCom.h>

struct __declspec(uuid("000214e4-0000-0000-c000-000000000046")) IContextMenu;
struct __declspec(uuid("000214f4-0000-0000-c000-000000000046")) IContextMenu2;
struct __declspec(uuid("bcfce0a0-ec17-11d0-8d10-00a0c90f2719")) IContextMenu3;

/**
 * @brief Explorer's context menu
 *
 * Allows to query shell context menu for a group of files
 *
 * Usage:
 * <ol>
 * <li>Show popup menu via TrackPopupMenu[Ex]() with TPM_RETURNCMD flag using handle that is returned by GetHMENU().
 *	   Handle WM_INITMENUPOPUP, WM_DRAWITEM, WM_MEASUREITEM and WM_MENUCHAR in window procedure of the menu owner
 *	   and pass them to HandleMenuMessage().</li>
 * <li>Call InvokeCommand() with nCmd returned by TrackPopupMenu[Ex]().</li>
 * </ol>
 */
class CShellContextMenu
{
public:
	/**
	 * @brief Constructor
	 *
	 * @param[in]	cmdFirst	minimum value for a menu item identifier
	 * @param[in]	cmdLast		maximum value for a menu item identifier
	 *
	 * @pre		cmdFirst < cmdLast <= 0xffff
	 * @pre		[cmdFirst, cmdLast] range should not intersect with available command IDs
	 */
	CShellContextMenu(UINT cmdFirst, UINT cmdLast);

	/**
	 * @brief Destructor
	 */
	~CShellContextMenu();

	/**
	 * @brief	Forwards certain messages to context menu so it works properly
	 *
	 * Handles WM_INITMENUPOPUP, WM_DRAWITEM, WM_MEASUREITEM and WM_MENUCHAR messages
	 *
	 * @param[in]		message		Message to handle
	 * @param[in]		wParam		Additional message-specific information
	 * @param[in]		lParam		Additional message-specific information
	 * @param[out]		retval		Value returned by message handler
	 *
	 * @retval	true	message was handled
	 * @retval	false	message was not handled
	 */
	bool HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& retval);

	/**
	 * @brief	Handles selected menu command
	 *
	 * @param[in]	nCmd	Menu item identifier returned by TrackPopupMenu[Ex]()
	 * @param[in]	hWnd	Handle to the window that owns popup menu ( window handle passed to TrackPopupMenu[Ex]() )
	 *
	 * @retval	true	Everything is OK
	 * @retval	false	Something failed
	 */
	bool InvokeCommand(UINT nCmd, HWND hWnd);

	/**
	 * @brief	Queries context menu from the shell
	 *
	 * Initializes \ref m_pPreferredMenu, \ref m_pShellContextMenu2, \ref m_pShellContextMenu3 and \ref m_hShellContextMenu
	 * that are used later for showing menu and handling commands
	 *
	 * @retval	true	Menu is queried successfully
	 * @retval	false	Failed to query context menu
	 */
	HMENU QueryShellContextMenu(IContextMenu *);

private:
	CMyComPtr<IContextMenu> m_pPreferredMenu;  /**< IContextMenu interface of current context menu */
	CMyComPtr<IContextMenu2> m_pShellContextMenu2; /**< IContextMenu2 interface of current context menu */
	CMyComPtr<IContextMenu3> m_pShellContextMenu3; /**< IContextMenu3 interface of current context menu */
	const UINT m_cmdFirst; /**< minimum value for a menu item identifier */
	const UINT m_cmdLast; /**< maximum value for a menu item identifier */
};
