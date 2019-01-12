#include "StdAfx.h"
#include "Merge.h"
#include "Common/DllProxies.h"
#include "Common/SettingStore.h"
#include "Common/WindowPlacement.h"

static HWND NTAPI AllocConsoleHidden(LPCTSTR lpTitle)
{
	HWND hWnd = NULL;
	if (struct KERNEL32V51 *KERNEL32V51 = ::KERNEL32V51)
	{
		// Silently launch a ping.exe 127.0.0.1 and attach to its console.
		// This gives us control over the console window's initial visibility.
		STARTUPINFO si;
		ZeroMemory(&si, sizeof si);
		PROCESS_INFORMATION pi;
		si.cb = sizeof si;
		si.lpTitle = const_cast<LPTSTR>(lpTitle);
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.wShowWindow = SW_HIDE;
		TCHAR path[MAX_PATH];
		GetSystemDirectory(path, MAX_PATH);
		PathAppend(path, _T("ping.exe -n 2 127.0.0.1"));
		if (CreateProcess(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			DWORD dwExitCode;
			do
			{
				Sleep(100);
				GetExitCodeProcess(pi.hProcess, &dwExitCode);
			} while (!KERNEL32V51->AttachConsole(pi.dwProcessId) && dwExitCode == STILL_ACTIVE);
			hWnd = GetConsoleWindow();
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
	}
	return hWnd;
}

void NTAPI ShowConsoleWindow(int showCmd)
{
	if (HWND hWnd = GetConsoleWindow())
	{
		ShowWindow(hWnd, showCmd);
	}
	else if (HWND hWnd = AllocConsoleHidden(_T("WinMerge")))
	{
		SetConsoleOutputCP(GetACP());
		SetConsoleCtrlHandler(CMergeApp::ConsoleCtrlHandler, TRUE);
		if (HMENU hMenu = GetSystemMenu(hWnd, FALSE))
		{
			DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
		}
		CWindowPlacement wp;
		CRegKeyEx rk = SettingStore.GetSectionKey(_T("Settings"));
		if (wp.RegQuery(rk, _T("ConsoleWindowPlacement")))
		{
			wp.showCmd = showCmd;
			SetWindowPlacement(hWnd, &wp);
		}
		else
		{
			ShowWindow(hWnd, showCmd);
		}
	}
}

void NTAPI KillConsoleWindow()
{
	if (HWND hWnd = GetConsoleWindow())
	{
		CWindowPlacement wp;
		if (GetWindowPlacement(hWnd, &wp))
		{
			CRegKeyEx rk = SettingStore.GetSectionKey(_T("Settings"));
			wp.RegWrite(rk, _T("ConsoleWindowPlacement"));
		}
		FreeConsole();
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	}
}
