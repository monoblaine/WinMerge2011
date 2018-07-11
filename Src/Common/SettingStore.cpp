﻿/// <copyright company="JetBrains">
/// Copyright © 2003-2008 JetBrains s.r.o.
/// You may distribute under the terms of the GNU General Public License, as published by the Free Software Foundation, version 2 (see License.txt in the repository root folder).
/// </copyright>

// © JetBrains, Inc, 2005
// Written by (H) Serge Baltic

// Copyright © 2011-2015 WinMerge Team

#include <StdAfx.h>
#include "SettingStore.h"
#include "RegKey.h"
#include "DllProxies.h"
#include <json/json.h>

class StoreName : public String
{
public:
	StoreName(LPCTSTR fmt, LPCTSTR name, bool desktop_specific = true)
	{
		TCHAR desktop[MAX_PATH];
		TCHAR path[MAX_PATH];
		if (name == NULL)
		{
			GetModuleFileName(NULL, path, _countof(path));
			PathRemoveExtension(path);
			name = path;
		}
		if (desktop_specific)
		{
			HDESK handle = GetThreadDesktop(GetCurrentThreadId());
			GetUserObjectInformation(handle, UOI_NAME, desktop, sizeof desktop, NULL);
		}
		else
		{
			fmt += 3;
		}
		append_sprintf(fmt, name, desktop);
	}
};

CSettingStore::CSettingStore(LPCTSTR sCompanyName, LPCTSTR sApplicationName)
{
	ASSERT(ThreadIntegrity);
	m_hHive = HKEY_CURRENT_USER;
	m_regsam = KEY_READ | KEY_WRITE;
	m_bDirty = false;
	m_bUseDesktopSpecificSettings = false;
	// Store
	m_sCompanyName = sCompanyName;
	m_sApplicationName = sApplicationName;
}

CSettingStore::~CSettingStore()
{
	ASSERT(ThreadIntegrity);
	Close();
}

int CSettingStore::GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault) const
{
	ASSERT(ThreadIntegrity);
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);
	if (HKEY hSecKey = GetSectionKey(lpszSection))
	{
		DWORD dwType;
		DWORD dwCount = sizeof(DWORD);
		DWORD dwValue;
		LONG lResult = RegQueryValueEx(hSecKey, lpszEntry, &dwType, (LPBYTE)&dwValue, &dwCount);
		if (lResult == ERROR_SUCCESS && dwType == REG_DWORD && dwCount == sizeof(DWORD))
			nDefault = dwValue;
		RegCloseKey(hSecKey);
	}
	return nDefault;
}

String CSettingStore::GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault) const
{
	ASSERT(ThreadIntegrity);
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);
	if (HKEY hSecKey = GetSectionKey(lpszSection))
	{
		DWORD dwType, dwCount;
		LONG lResult = RegQueryValueEx(hSecKey, lpszEntry, &dwType, NULL, &dwCount);
		if (lResult == ERROR_SUCCESS && dwType == REG_SZ && dwCount >= sizeof(TCHAR))
		{
			LPTSTR lpValue = static_cast<LPTSTR>(_alloca(dwCount));
			lResult = RegQueryValueEx(hSecKey, lpszEntry, &dwType, reinterpret_cast<LPBYTE>(lpValue), &dwCount);
			if (lResult == ERROR_SUCCESS && dwType == REG_SZ && dwCount >= sizeof(TCHAR) &&
				lpValue[dwCount / sizeof(TCHAR) - 1] == _T('\0'))
			{
				lpszDefault = lpValue;
			}
		}
		RegCloseKey(hSecKey);
	}
	return lpszDefault;
}

BOOL CSettingStore::WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue) const
{
	ASSERT(ThreadIntegrity);
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);
	LONG lResult = ERROR_INVALID_PARAMETER;
	if (HKEY hSecKey = GetSectionKey(lpszSection))
	{
		lResult = RegSetValueEx(hSecKey, lpszEntry, REG_DWORD, (LPBYTE)&nValue, sizeof nValue);
		RegCloseKey(hSecKey);
	}
	return lResult == ERROR_SUCCESS;
}

BOOL CSettingStore::WriteProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue) const
{
	ASSERT(ThreadIntegrity);
	ASSERT(lpszSection != NULL);
	LONG lResult = ERROR_INVALID_PARAMETER;
	if (lpszEntry == NULL) //delete whole section
	{
		if (HKEY hAppKey = GetAppRegistryKey())
		{
			lResult = RegDeleteKey(hAppKey, lpszSection);
			RegCloseKey(hAppKey);
		}
	}
	else if (HKEY hSecKey = GetSectionKey(lpszSection))
	{
		if (lpszValue == NULL)
		{
			lResult = RegDeleteValue(hSecKey, lpszEntry);
		}
		else
		{
			lResult = RegSetValueEx(hSecKey, lpszEntry, REG_SZ,
				reinterpret_cast<const BYTE *>(lpszValue),
				(lstrlen(lpszValue) + 1) * sizeof(TCHAR));
		}
		RegCloseKey(hSecKey);
	}
	return lResult == ERROR_SUCCESS;
}

void CSettingStore::Close()
{
	ASSERT(ThreadIntegrity);
	if (m_regsam == 0)
	{
		Json::Value *const root = reinterpret_cast<Json::Value *>(m_hHive);
		if (m_bDirty)
		{
			if (FILE *tf = _tfopen(m_sFileName.c_str(), _T("w")))
			{
				try
				{
					Json::Writer writer(tf);
					writer.write(*root);
				}
				catch (OException *e)
				{
					e->ReportError(NULL, MB_TASKMODAL | MB_ICONSTOP);
					delete e;
				}
				fclose(tf);
			}
		}
		delete root;
	}
	m_bDirty = false;
	m_hHive = NULL;
}

bool CSettingStore::SetFileName(String sFileName)
{
	ASSERT(ThreadIntegrity);
	if (sFileName.empty())
	{
		StoreName(_T("%s@%s.json"), NULL, m_bUseDesktopSpecificSettings).swap(sFileName);
		if (m_sFileName != sFileName)
		{
			StoreName sAlternateFileName(_T("%s@%s.json"), NULL);
			DWORD dwFileAttributes = GetFileAttributes(sFileName.c_str());
			DWORD dwAlternateFileAttributes = m_bUseDesktopSpecificSettings ?
				dwFileAttributes : GetFileAttributes(sAlternateFileName.c_str());
			if (dwFileAttributes & dwAlternateFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				sFileName.clear();
				CSettingStore clone(m_sCompanyName, m_sApplicationName);
				clone.m_bUseDesktopSpecificSettings = true;
				if (CRegKeyEx key = clone.GetAppRegistryKey(true))
					if (key.ReadDword(NULL, 0) != 0)
						m_bUseDesktopSpecificSettings = true;
			}
			else
			{
				if ((dwAlternateFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY)) == 0)
				{
					sAlternateFileName.swap(sFileName);
					m_bUseDesktopSpecificSettings = true;
				}
				PathStripToRoot(&sAlternateFileName.front());
				SetEnvironmentVariable(_T("PortableRoot"), sAlternateFileName.c_str());
			}
		}
	}
	if (m_sFileName == sFileName)
		return false;
	Close();
	m_sFileName.swap(sFileName);
	if (m_sFileName.empty())
	{
		m_regsam = KEY_READ | KEY_WRITE;
		m_hHive = HKEY_CURRENT_USER;
	}
	else
	{
		m_regsam = 0;
		Json::Value *const root = new Json::Value;
		if (FILE *tf = _tfopen(m_sFileName.c_str(), _T("r")))
		{
			try
			{
				Json::Reader reader(tf);
				reader.parse(*root);
				if (ftell(tf) != 0 && !reader.good())
				{
					// create copy of broken file for reference
					CopyFile(m_sFileName.c_str(), (m_sFileName + _T(".bad")).c_str(), FALSE);
					Json::ThrowJsonException(reader.getFormattedErrorMessages().c_str());
				}
			}
			catch (OException *e)
			{
				e->ReportError(NULL, MB_TASKMODAL | MB_ICONSTOP);
				delete e;
			}
			fclose(tf);
		}
		m_hHive = reinterpret_cast<HKEY>(root);
	}
	return true;
}

bool CSettingStore::UseDesktopSpecificSettings(bool bUseDesktopSpecificSettings)
{
	if (m_bUseDesktopSpecificSettings == bUseDesktopSpecificSettings)
		return false;
	String sFileName;
	if (m_regsam != 0)
	{
		if (m_bUseDesktopSpecificSettings)
			if (CRegKeyEx key = GetAppRegistryKey())
				key.WriteDword(NULL, 0);
		m_bUseDesktopSpecificSettings = bUseDesktopSpecificSettings;
		if (m_bUseDesktopSpecificSettings)
			if (CRegKeyEx key = GetAppRegistryKey())
				key.WriteDword(NULL, 1);
	}
	else
	{
		m_bUseDesktopSpecificSettings = bUseDesktopSpecificSettings;
		StoreName(_T("%s@%s.json"), NULL, m_bUseDesktopSpecificSettings).swap(sFileName);
		if (m_bUseDesktopSpecificSettings)
		{
			DWORD attr = GetFileAttributes(sFileName.c_str());
			if (attr != INVALID_FILE_ATTRIBUTES)
				SetFileAttributes(sFileName.c_str(), attr & ~FILE_ATTRIBUTE_HIDDEN);
		}
	}
	if (SetFileName(sFileName) && m_regsam == 0 && !m_bUseDesktopSpecificSettings)
	{
		StoreName(_T("%s@%s.json"), NULL).swap(sFileName);
		DWORD attr = GetFileAttributes(sFileName.c_str());
		if (attr != INVALID_FILE_ATTRIBUTES)
			SetFileAttributes(sFileName.c_str(), attr | FILE_ATTRIBUTE_HIDDEN);
	}
	return true;
}

// returns key for HKEY_CURRENT_USER\"Software"\RegistryKey\ProfileName
// creating it if it doesn't exist
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CSettingStore::GetAppRegistryKey(bool bNoCreate) const
{
	ASSERT(ThreadIntegrity);
	ASSERT(m_sCompanyName != NULL);
	ASSERT(m_sApplicationName != NULL);

	HKEY hAppKey = NULL;
	if (m_regsam != 0)
	{
		HKEY hSoftKey = NULL;
		if (::RegCreateKeyEx(m_hHive, _T("Software"), 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, m_regsam, NULL,
			&hSoftKey, NULL) == ERROR_SUCCESS)
		{
			HKEY hCompanyKey = NULL;
			if (::RegCreateKeyEx(hSoftKey, m_sCompanyName, 0, REG_NONE,
				REG_OPTION_NON_VOLATILE, m_regsam, NULL,
				&hCompanyKey, NULL) == ERROR_SUCCESS)
			{
				StoreName sStoreName(_T("%s@%s"), m_sApplicationName, m_bUseDesktopSpecificSettings);
				if (bNoCreate)
				{
					::RegOpenKeyEx(hCompanyKey, sStoreName.c_str(), 0, m_regsam, &hAppKey);
				}
				else
				{
					::RegCreateKeyEx(hCompanyKey, sStoreName.c_str(), 0, REG_NONE,
						REG_OPTION_NON_VOLATILE, m_regsam, NULL,
						&hAppKey, NULL);
				}
				::RegCloseKey(hCompanyKey);
			}
			::RegCloseKey(hSoftKey);
		}
	}
	else
	{
		hAppKey = m_hHive;
	}
	return hAppKey;
}

// returns key for:
//      HKEY_CURRENT_USER\"Software"\RegistryKey\AppName\lpszSection
// creating it if it doesn't exist.
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CSettingStore::GetSectionKey(LPCTSTR lpszSection, DWORD dwCount) const
{
	ASSERT(ThreadIntegrity);
	ASSERT(lpszSection != NULL);
	HKEY hSectionKey = NULL;
	if (HKEY hAppKey = GetAppRegistryKey())
	{
		while (RegCreateKeyEx(hAppKey, lpszSection, REG_OPTION_NON_VOLATILE,
			m_regsam, &hSectionKey) == ERROR_SUCCESS && dwCount != 0xFFFFFFFF)
		{
			DWORD dwFound = 0;
			RegQueryInfoKey(hSectionKey, &dwFound);
			if (dwCount == dwFound)
				break;
			dwCount = 0xFFFFFFFF; // no further iteration
			RegCloseKey(hSectionKey);
			hSectionKey = NULL;
			SHDeleteKey(hAppKey, lpszSection);
		}
		RegCloseKey(hAppKey);
	}
	return hSectionKey;
}

LONG CSettingStore::RegCreateKeyEx(HKEY hKey, LPCTSTR lpSubKey, DWORD dwOptions, REGSAM samDesired, PHKEY phkResult) const
{
	ASSERT(ThreadIntegrity);
	LONG lResult = 0;
	if (m_regsam != 0)
		lResult = ::RegCreateKeyEx(hKey, lpSubKey, 0, NULL, dwOptions, samDesired, NULL, phkResult, NULL);
	else if (Json::Value *node = reinterpret_cast<Json::Value *>(hKey))
	{
		OString key = HString::Uni(lpSubKey)->Oct(CP_UTF8);
		if (phkResult != NULL)
			*phkResult = reinterpret_cast<HKEY>(&(*node)[key.A]);
		else
			lResult = ERROR_INVALID_PARAMETER;
	}
	else
	{
		lResult = ERROR_INVALID_PARAMETER;
	}
	return lResult;
}

LONG CSettingStore::RegCloseKey(HKEY hKey) const
{
	ASSERT(ThreadIntegrity);
	LONG lResult = 0;
	if (m_regsam != 0)
		lResult = ::RegCloseKey(hKey);
	else if (hKey == NULL)
		lResult = ERROR_INVALID_PARAMETER;
	return lResult;
}

LONG CSettingStore::RegQueryValueEx(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) const
{
	ASSERT(ThreadIntegrity);
	LONG lResult = 0;
	if (m_regsam != 0)
		lResult = ::RegQueryValueEx(hKey, lpValueName, NULL, lpType, lpData, lpcbData);
	else if (const Json::Value *const node = reinterpret_cast<const Json::Value *>(hKey))
	{
		OString key = HString::Uni(lpValueName)->Oct(CP_UTF8);
		const Json::Value &value = (*node)[key.A];
		if (value.isIntegral())
		{
			if (lpType != NULL)
				*lpType = REG_DWORD;
			if (lpcbData != NULL)
			{
				if (lpData != NULL)
				{
					if (*lpcbData < sizeof(DWORD))
						lResult = ERROR_MORE_DATA;
					else
						*reinterpret_cast<DWORD *>(lpData) = value.asUInt();
				}
				*lpcbData = sizeof(DWORD);
			}
			else if (lpData != NULL)
			{
				lResult = ERROR_INVALID_PARAMETER;
			}
		}
		else if (value.isString())
		{
			if (lpType != NULL)
				*lpType = REG_SZ;
			if (lpcbData != NULL)
			{
				const char *str = value.asCString();
				if (int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0))
				{
					DWORD cbData = len * sizeof(WCHAR);
					if (lpData != NULL)
					{
						if (*lpcbData < cbData)
							lResult = ERROR_MORE_DATA;
						else
							MultiByteToWideChar(CP_UTF8, 0, str, -1, reinterpret_cast<LPWSTR>(lpData), len);
					}
					*lpcbData = cbData;
				}
				else
				{
					lResult = GetLastError();
				}
			}
			else if (lpData != NULL)
			{
				lResult = ERROR_INVALID_PARAMETER;
			}
		}
		else
		{
			lResult = ERROR_FILE_NOT_FOUND;
		}
	}
	else
	{
		lResult = ERROR_INVALID_PARAMETER;
	}
	return lResult;
}

LONG CSettingStore::RegSetValueEx(HKEY hKey, LPCTSTR lpValueName, DWORD dwType, CONST BYTE* lpData, DWORD cbData) const
{
	ASSERT(ThreadIntegrity);
	LONG lResult = 0;
	if (m_regsam != 0)
		lResult = ::RegSetValueEx(hKey, lpValueName, 0, dwType, lpData, cbData);
	else if (Json::Value *const node = reinterpret_cast<Json::Value *>(hKey))
	{
		OString key = HString::Uni(lpValueName)->Oct(CP_UTF8);
		Json::Value &value = (*node)[key.A];
		Json::Value newValue;
		if (lpData != NULL)
		{
			switch (dwType)
			{
			case REG_DWORD:
				// Prevent compare() from unintentionally indicating a difference
				if (value.isUInt())
					value = value.asUInt();
				newValue = static_cast<Json::Value::UInt>(*reinterpret_cast<const DWORD *>(lpData));
				break;
			case REG_SZ:
				if (HString *pStr = HString::Oct(reinterpret_cast<LPCSTR>(lpData), cbData)->Oct(CP_UTF8))
					newValue = OString(pStr).A;
				break;
			default:
				lResult = ERROR_INVALID_PARAMETER;
				break;
			}
			if (value.compare(newValue) != 0)
			{
				value.swapPayload(newValue);
				m_bDirty = true;
			}
		}
		else if (cbData != 0)
		{
			lResult = ERROR_INVALID_PARAMETER;
		}
	}
	else
	{
		lResult = ERROR_INVALID_PARAMETER;
	}
	return lResult;
}

LONG CSettingStore::SHDeleteKey(HKEY hKey, LPCTSTR pszSubKey) const
{
	ASSERT(ThreadIntegrity);
	LONG lResult = 0;
	if (m_regsam != 0)
		lResult = ::SHDeleteKey(hKey, pszSubKey);
	else if (Json::Value *const node = reinterpret_cast<Json::Value *>(hKey))
	{
		OString key = HString::Uni(pszSubKey)->Oct(CP_UTF8);
		if (node->removeMember(key.A, 0))
			m_bDirty = true;
	}
	else
	{
		lResult = ERROR_INVALID_PARAMETER;
	}
	return lResult;
}

LONG CSettingStore::RegDeleteValue(HKEY hKey, LPCTSTR lpValueName) const
{
	ASSERT(ThreadIntegrity);
	LONG lResult = 0;
	if (m_regsam != 0)
		lResult = ::RegDeleteValue(hKey, lpValueName);
	else if (Json::Value *const node = reinterpret_cast<Json::Value *>(hKey))
	{
		OString key = HString::Uni(lpValueName)->Oct(CP_UTF8);
		if (node->removeMember(key.A, 0))
			m_bDirty = true;
	}
	else
	{
		lResult = ERROR_INVALID_PARAMETER;
	}
	return lResult;
}

LONG CSettingStore::RegQueryInfoKey(HKEY hKey, LPDWORD lpcValues, LPDWORD lpcbMaxValueLen) const
{
	ASSERT(ThreadIntegrity);
	LONG lResult = 0;
	if (m_regsam != 0)
		lResult = ::RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, lpcValues, NULL, lpcbMaxValueLen, NULL, NULL);
	else if (const Json::Value *const node = reinterpret_cast<const Json::Value *>(hKey))
	{
		DWORD cValues = node->size();
		DWORD cbMaxValueLen = 0;
		if (const Json::Value::ObjectValues *const values = node->getObjectValues())
		{
			Json::Value::ObjectValues::const_iterator cur = values->begin();
			Json::Value::ObjectValues::const_iterator end = values->end();
			while (cur != end)
			{
				const Json::Value &value = cur->second;
				DWORD cbData = 0;
				if (value.isIntegral())
				{
					cbData = sizeof(DWORD);
				}
				else if (value.isString())
				{
					const char *str = value.asCString();
					if (int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0))
					{
						cbData = len * sizeof(WCHAR);
					}
				}
				if (cbMaxValueLen < cbData)
					cbMaxValueLen = cbData;
				++cur;
			}
		}
		if (lpcValues != NULL)
			*lpcValues = cValues;
		if (lpcbMaxValueLen != NULL)
			*lpcbMaxValueLen = cbMaxValueLen;
	}
	else
	{
		lResult = ERROR_INVALID_PARAMETER;
	}
	return lResult;
}
