/**
 * @file  FileTextEncoding.cpp
 *
 * @brief Implementation of FileTextEncoding structure
 */
#include "StdAfx.h"
#include "unicoder.h"
#include "FileTextEncoding.h"
#include "LanguageSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int FileTextEncoding::m_defaultCodepage = 0;

FileTextEncoding::FileTextEncoding()
{
	Clear();
}

/**
 * @brief Forget any encoding info we have
 */
void FileTextEncoding::Clear()
{
	m_codepage = -1;
	m_unicoding = NONE;
	m_bom = false;
	m_guessed = false;
	m_binary = false;
}

/**
 * @brief Set codepage
 */
void FileTextEncoding::SetCodepage(int codepage)
{
	m_codepage = codepage;
	m_unicoding = codepage == CP_UTF8 ? UTF8 : NONE;
}

void FileTextEncoding::SetUnicoding(UNICODESET unicoding)
{
	ASSERT(unicoding != NONE);
	m_unicoding = unicoding;
	m_codepage = m_unicoding == UTF8 ? CP_UTF8 : CP_ACP;
}

/**
 * @brief Return string representation of encoding, eg "UCS-2LE", or "1252"
 * @todo This resource lookup should be done in GUI code?
 */
String FileTextEncoding::GetName() const
{
	if (m_unicoding == UTF8)
		return m_bom ? _T("UTF-8 (B)") : _T("UTF-8");
	if (m_unicoding == UCS2LE)
		return m_bom ? _T("UCS-2 LE (B)") : _T("UCS-2 LE");
	if (m_unicoding == UCS2BE)
		return m_bom ? _T("UCS-2 BE (B)") : _T("UCS-2 BE");
	if (m_unicoding == UCS4LE)
		return m_bom ? _T("UCS-4 LE (B)") : _T("UCS-4 LE");
	if (m_unicoding == UCS4BE)
		return m_bom ? _T("UCS-4 BE (B)") : _T("UCS-4 BE");
	String str;
	if (m_codepage != -1)
	{
		if (m_codepage == CP_UTF8)
		{
			// We detected codepage to be UTF-8, but unicoding was not set
			str = _T("UTF-8");
		}
		else
		{
			str.append_sprintf(_T("%d"), m_codepage);
		}
	}
	return str;
}

int FileTextEncoding::Collate(FileTextEncoding const &fte1, FileTextEncoding const &fte2)
{
	if (fte1.m_unicoding > fte2.m_unicoding)
		return 1;
	if (fte1.m_unicoding < fte2.m_unicoding)
		return -1;
	if (fte1.m_bom > fte2.m_bom)
		return 1;
	if (fte1.m_bom < fte2.m_bom)
		return -1;
	if (fte1.m_codepage > fte2.m_codepage)
		return 1;
	if (fte1.m_codepage < fte2.m_codepage)
		return -1;
	return 0;
}

/**
 * @brief Update the appropriate default codepage
 */
void FileTextEncoding::UpdateDefaultCodepage(int cpDefaultMode, int customCodepage)
{
	LANGID wLangId;
	switch (cpDefaultMode)
	{
	case 0:
		break;
	case 1:
		TCHAR buff[32];
		wLangId = LanguageSelect.GetLangId();
		if (!GetLocaleInfo(wLangId, LOCALE_IDEFAULTANSICODEPAGE, buff, _countof(buff)))
			break;
		m_defaultCodepage = _ttol(buff);
		return;
	case 2:
		m_defaultCodepage = customCodepage;
		return;
	default:
		// no other valid option
		ASSERT(FALSE);
		break;
	}
	m_defaultCodepage = GetACP();
}

/**
 * @brief Get appropriate default codepage 
 */
int FileTextEncoding::GetDefaultCodepage()
{
	return m_defaultCodepage;
}

int FileTextEncoding::IsCodepageDBCS(int codepage)
{
	CPINFO cpinfo;
	return GetCPInfo(codepage, &cpinfo) && cpinfo.LeadByte[0] != 0 ? codepage : 0;
}
