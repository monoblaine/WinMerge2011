/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  ConfigLog.h
 *
 * @brief Declaration file ConfigLog class
 */
#pragma once

class CfgSettings;
class UniStdioFile;

/**
 * @brief Compare-related settings.
 */
struct COMPARESETTINGS
{
	int nCompareMethod;
	bool bStopAfterFirst;
};

/**
 * @brief View settings for directory compare
 */
struct VIEWSETTINGS
{
	bool bShowIdent;
	bool bShowDiff;
	bool bShowUniqueLeft;
	bool bShowUniqueRight;
	bool bShowBinaries;
	bool bShowSkipped;
	bool bTreeView;
};

/**
 * @brief Misc WinMerge settings
 */
struct MISCSETTINGS
{
	bool bAutomaticRescan;
	bool bAllowMixedEol;
	bool bScrollToFirst;
	bool bBackup;
	bool bViewWhitespace;
	bool bMovedBlocks;
	bool bDetectCodepage;
	bool bShowLinenumbers;
	bool bWrapLines;
	bool bMergeMode;
	bool bSyntaxHighlight;
	int  nInsertTabs;
	int  nTabSize;
	bool bPreserveFiletimes;
	bool bMatchSimilarLines;
	int  nMatchSimilarLinesMax;
	bool bMerge7zEnable;
	bool bMerge7zProbeSignature;
};

/**
 * @brief Codepage WinMerge settings
 */
struct CPSETTINGS
{
	int nDefaultMode;
	int nDefaultCustomValue;
	bool bDetectCodepage;
};

/**
 * @brief Major Font WinMerge settings
 */
struct FONTSETTINGS
{
	BYTE nCharset;
	String sFacename;
};

/**
 * @brief Class for saving configuration log file
 */
class CConfigLog
{
public:
	CConfigLog();
	~CConfigLog();

	DIFFOPTIONS m_diffOptions;
	COMPARESETTINGS m_compareSettings;
	VIEWSETTINGS m_viewSettings;
	MISCSETTINGS m_miscSettings;
	CPSETTINGS m_cpSettings;
	FONTSETTINGS m_fontSettings;

	String GetFileName() const;
	bool WriteLogFile(String &sError);
	void ReadLogFile(LPCTSTR Filepath);

	// Implementation methods
protected:
	void WriteItem(int indent, LPCTSTR key, LPCTSTR value = 0);
	void WriteItem(int indent, LPCTSTR key, long value);
	void WriteVersionOf1(int indent, LPTSTR path, bool bDllGetVersion = true);
	void WriteVersionOf(int indent, LPTSTR path);
	void WriteLocaleSettings(LCID locid, LPCTSTR title);

private:
	bool DoFile(bool writing, String &sError);
	void WritePluginsInLogFile(LPCWSTR transformationEvent);
	string_format GetWindowsVer() const;
	void FileWriteString(LPCTSTR lpsz);
	template<class any>
	void FileWriteString(any fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		String s;
		s.append_sprintf_va_list(fmt, args);
		FileWriteString(s.c_str());
		va_end(args);
	}
	void CloseFile();
    void WriteItemYesNo(int indent, LPCTSTR key, bool *pvalue);
	void WriteItemYesNoInverted(int indent, LPCTSTR key, bool *pvalue);
    void WriteItemYesNoInverted(int indent, LPCTSTR key, int *pvalue);
	void WriteItemWhitespace(int indent, LPCTSTR key, int *pvalue);
	bool ParseSettings(LPCTSTR Filepath);

	CConfigLog(const CConfigLog &); // disallow copy construction
	void operator=(const CConfigLog &); // disallow assignment

	// Implementation data
private:
	String m_sFileName;
	UniStdioFile *const m_pfile;
	bool m_writing;
	// Collection of configuration settings found in config log (name/value map)
	std::map<String, String> m_settings;
};
