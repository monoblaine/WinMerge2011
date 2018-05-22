/**
 * @file  coretools.cpp
 *
 * @brief Common routines
 *
 */
#include "StdAfx.h"
#include "coretools.h"
#include "paths.h"

int linelen(const char *string)
{
	int stringlen = 0;
	while (char c = string[stringlen])
	{
		if (c == '\r' || c == '\n')
			break;
		++stringlen;
	}
	return stringlen;
}

/**
 * @brief Convert C style \\nnn, \\r, \\n, \\t etc into their indicated characters.
 * @param [in] codepage Codepage to use in conversion.
 * @param [in,out] s String to convert.
 */
unsigned Unslash(unsigned codepage, char *string)
{
	char *p = string;
	char *q = p;
	char c;
	do
	{
		char *r = q + 1;
		switch (c = *q)
		{
		case '\\':
			switch (c = *r++)
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
			case 'x':
				*p = (char)strtol(r, &q, 16);
				break;
			default:
				*p = (char)strtol(r - 1, &q, 8);
				break;
			}
			if (q >= r)
				break;
			// fall through
		default:
			*p = c;
			if ((*p & 0x80) && IsDBCSLeadByteEx(codepage, *p))
				*++p = *r++;
			q = r;
		}
		++p;
	} while (c != '\0');
	return static_cast<unsigned>(p - 1 - string);
}

unsigned RemoveLeadingZeros(char *string)
{
	char *p = string;
	char *q = p;
	char c = '?';
	char d = '\0';
	do
	{
		if (*q == '0' && (c < '0' || c > '9'))
		{
			d = *q++;
			continue;
		}
		c = *q++;
		if (d != '\0' && (c < '0' || c > '9'))
		{
			*p++ = d;
		}
		*p++ = c;
		d = '\0';
	} while (c != '\0');
	return static_cast<unsigned>(p - 1 - string);
}

/**
 * @brief Remove prefix from the text.
 * @param [in] text Text to process.
 * @param [in] prefix Prefix to remove.
 * @return Text without the prefix.
 */
LPSTR NTAPI EatPrefix(LPCSTR text, LPCSTR prefix)
{
	if (size_t const len = strlen(prefix))
		if (_memicmp(text, prefix, len) == 0)
			return const_cast<LPSTR>(text + len);
	return NULL;
}

/**
 * @brief Eat prefix and return pointer to remaining text
 */
LPWSTR NTAPI EatPrefix(LPCWSTR text, LPCWSTR prefix)
{
	if (int const len = lstrlenW(prefix))
		if (StrIsIntlEqualW(FALSE, text, prefix, len))
			return const_cast<LPWSTR>(text + len);
	return NULL;
}

/**
 * @brief Eat prefix and whitespace and return pointer to remaining text
 */
LPWSTR NTAPI EatPrefixTrim(LPCWSTR text, LPCWSTR prefix)
{
	LPWSTR const trim = EatPrefix(text, prefix);
	return trim ? trim + StrSpn(trim, _T(" \t\r\n")) : NULL;
}

HANDLE NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir, HANDLE *phReadPipe, WORD wShowWindow)
{
	STARTUPINFO si;
	ZeroMemory(&si, sizeof si);
	si.cb = sizeof si;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = wShowWindow;
	BOOL bInheritHandles = FALSE;
	if (phReadPipe != NULL)
	{
		SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
		if (!CreatePipe(phReadPipe, &si.hStdOutput, &sa, 0))
			return NULL;
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.hStdError = si.hStdOutput;
		bInheritHandles = TRUE;
	}
	PROCESS_INFORMATION pi;
	if (CreateProcess(szExeFile, const_cast<LPTSTR>(szArgs), NULL, NULL,
			bInheritHandles, CREATE_NEW_CONSOLE, NULL, szDir, &si, &pi))
	{
		CloseHandle(pi.hThread);
	}
	else
	{
		pi.hProcess = NULL;
		if (phReadPipe != NULL)
			CloseHandle(*phReadPipe);
	}
	if (phReadPipe != NULL)
		CloseHandle(si.hStdOutput);
	return pi.hProcess;
}

DWORD NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir, WORD wShowWindow)
{
	DWORD code = STILL_ACTIVE;
	if (HANDLE hProcess = RunIt(szExeFile, szArgs, szDir, NULL, wShowWindow))
	{
		WaitForSingleObject(hProcess, INFINITE);
		GetExitCodeProcess(hProcess, &code);
		CloseHandle(hProcess);
	}
	return code;
}

/**
 * @brief Return module's path component (without filename).
 * @param [in] hModule Module's handle.
 * @return Module's path.
 */
String GetModulePath(HMODULE hModule /* = NULL*/)
{
	TCHAR temp[MAX_PATH];
	temp[0] = _T('\0');
	GetModuleFileName(hModule, temp, MAX_PATH);
	return paths_GetParentPath(temp);
}

DWORD_PTR GetShellImageList()
{
	SHFILEINFO sfi;
	static DWORD_PTR dwpShellImageList = SHGetFileInfo(
		_T(""), 0, &sfi, sizeof sfi, SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
	return dwpShellImageList;
}

/**
 * @brief Decorates commandline for giving to CreateProcess() or
 * ShellExecute().
 *
 * Adds quotation marks around executable path if needed, but not
 * around commandline switches. For example (C:\p ath\ex.exe -p -o)
 * becomes ("C:\p ath\ex.exe" -p -o)
 * @param [bi] sCmdLine commandline to decorate
 * @param [out] sExecutable Executable for validating file extension etc
 */
void DecorateCmdLine(String &sCmdLine, String &sExecutable)
{
	// Remove whitespaces from begin and and
	// (Earlier code was cutting off last non-ws character)
	string_trim_ws(sCmdLine);
	String::size_type pos = 0;
	do
	{
		pos = sCmdLine.find_first_of(_T("\"/-"), pos + 1);
	} while (pos != String::npos && sCmdLine[pos - 1] != _T(' '));
	if (pos != String::npos)
	{
		sExecutable = sCmdLine.substr(0U, pos);
		string_trim_ws(sExecutable);
		if (sExecutable.front() != _T('"'))
		{
			sCmdLine.insert(sExecutable.length(), 1U, _T('"'));
			sCmdLine.insert(0U, 1U, _T('"'));
		}
	}
	else
	{
		sExecutable = sCmdLine;
	}
	sExecutable.erase(std::remove(sExecutable.begin(), sExecutable.end(), _T('"')), sExecutable.end());
}

/**
 * @brief Compute the Euclidean square distance between two colors
 */
long SquareColorDistance(COLORREF a, COLORREF b)
{
	long const dr = static_cast<long>(GetRValue(a)) - static_cast<long>(GetRValue(b));
	long const dg = static_cast<long>(GetGValue(a)) - static_cast<long>(GetGValue(b));
	long const db = static_cast<long>(GetBValue(a)) - static_cast<long>(GetBValue(b));
	return dr * dr + dg * dg + db * db;
}
