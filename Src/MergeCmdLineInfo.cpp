/////////////////////////////////////////////////////////////////////////////
//    WinMerge: An interactive diff/merge utility
//    Copyright (C) 1997 Dean P. Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  MergeCmdLineInfo.cpp
 *
 * @brief MergeCmdLineInfo class implementation.
 *
 */
#include "StdAfx.h"
#include "Constants.h"
#include "Paths.h"
#include "Environment.h"
#include "Common/SettingStore.h"
#include "MergeCmdLineInfo.h"
#include "OptionsMgr.h"
#include "ProjectFile.h"

/**
 * @brief Eat and digest a command line parameter.
 * @param [in] p Points into the command line.
 * @param [out] param Receives the digested command line parameter.
 * @param [out] flag Tells whether param is the name of a flag.
 * @return Points to the remaining portion of the command line.
 */
LPCTSTR MergeCmdLineInfo::EatParam(LPCTSTR p, String &param, bool *flag) const
{
	if (p && *(p += StrSpn(p, _T(" \t\r\n"))) == _T('\0'))
		p = 0;
	LPCTSTR q = p;
	if (q)
	{
		TCHAR c = *q;
		bool quoted = false;
		do
		{
			if (c == _T('"'))
				quoted = !quoted;
			c = *++q;
		} while (c != _T('\0') && (quoted ||
			c != _T(' ') && c != _T('\t') && c != _T('\r') && c != _T('\n')));
	}
	if (q > p && flag)
	{
		if (m_sOptionChars.find(*p) != String::npos && StrRChr(p, q, _T('/')) <= p)
		{
			*flag = true;
			++p;
			while (LPCTSTR colon = StrRChr(p, q, _T(':')))
				q = colon;
		}
		else
		{
			*flag = false;
			flag = 0;
		}
	}
	param.assign(p ? p : _T(""), static_cast<String::size_type>(q - p));
	if (q > p && flag)
	{
		CharLower(&param.front());
	}
	// Remove any double quotes
	param.erase(std::remove(param.begin(), param.end(), _T('"')), param.end());
	return q;
}

/**
 * @brief Set WinMerge option from command line.
 * @param [in] p Points into the command line.
 * @param [in] key Name of WinMerge option to set.
 * @param [in] value Default value in case none is specified.
 * @return Points to the remaining portion of the command line.
 */
LPCTSTR MergeCmdLineInfo::SetOption(LPCTSTR q, IOptionDef &opt, LPCTSTR value) const
{
	String s;
	if (*q == _T(':'))
	{
		q = EatParam(q, s);
		value = s.c_str() + 1;
	}
	opt.Parse(value);
	return q;
}

/**
 * @brief ClearCaseCmdLineParser's constructor.
 * @param [in] q Points to the beginning of the command line.
 */
MergeCmdLineInfo::MergeCmdLineInfo(LPCTSTR q):
	m_nCmdShow(SW_SHOWNORMAL),
	m_sOptionChars(_T("/-")),
	m_invocationMode(InvocationModeNone),
	m_bExitIfNoDiff(Disabled),
	m_nRecursive(0),
	m_nCompMethod(-1),
	m_bNonInteractive(false),
	m_nSingleInstance(-1),
	m_bShowUsage(false),
	m_nCodepage(0),
	m_dwLeftFlags(FFILEOPEN_DETECT),
	m_dwRightFlags(FFILEOPEN_DETECT)
{
	m_Files.reserve(2);

	// Rational ClearCase has a weird way of executing external
	// tools which replace the build-in ones. It also doesn't allow
	// you to define which parameters to send to the executable.
	// So, in order to run as an external tool, WinMerge should do:
	// if argv[0] is "xcompare" then it "knows" that it was
	// executed from ClearCase. In this case, it should read and
	// parse ClearCase's command line parameters and not the
	// "regular" parameters. More information can be found in
	// C:\Program Files\Rational\ClearCase\lib\mgrs\mgr_info.h file.
	String exeName;
	q = EatParam(q, exeName);
	if (exeName == _T("compare") || exeName == _T("xcompare"))
	{
		m_dwLeftFlags |= FFILEOPEN_NOMRU;
		m_dwRightFlags |= FFILEOPEN_NOMRU;
		ParseClearCaseCmdLine(q, _T("<No Base>"));
	}
	else if (exeName == _T("merge") || exeName == _T("xmerge"))
	{
		ParseClearCaseCmdLine(q, _T(""));
	}
	else
	{
		ParseWinMergeCmdLine(q);
	}
}

/**
 * @brief Parse a command line passed in from ClearCase.
 * @param [in] p Points into the command line.
 */
void MergeCmdLineInfo::ParseClearCaseCmdLine(LPCTSTR q, LPCTSTR basedesc)
{
	String sBaseFile;  /**< Base file path. */
	String sBaseDesc = basedesc;  /**< Base file description. */
	String sOutFile;   /**< Out file path. */
	m_invocationMode = InvocationModeCompareTool;
	String param;
	bool flag;
	while ((q = EatParam(q, param, &flag)) != 0)
	{
		if (!flag)
		{
			// Not a flag
			param = paths_GetLongPath(param.c_str());
			const size_t ord = m_Files.size();
			m_Files.push_back(param);
			String folder = paths_GetParentPath(param.c_str());
			DWORD attr = GetFileAttributes(folder.c_str());
			paths_UndoMagic(param);
			if (ord == 0)
			{
				if ((!m_sLeftDesc.empty() && m_sLeftDesc != param) || (attr & FILE_ATTRIBUTE_READONLY))
					m_dwLeftFlags |= FFILEOPEN_READONLY;
			}
			else if (ord == 1)
			{
				if ((!m_sRightDesc.empty() && m_sRightDesc != param) || (attr & FILE_ATTRIBUTE_READONLY))
					m_dwRightFlags |= FFILEOPEN_READONLY;
			}
		}
		else if (param == _T("base"))
		{
			// -base is followed by common ancestor file description.
			q = EatParam(q, sBaseFile);
		}
		else if (param == _T("out"))
		{
			// -out is followed by merge's output file name.
			q = EatParam(q, sOutFile);
		}
		else if (param == _T("fname"))
		{
			// -fname is followed by file description.
			if (sBaseDesc.empty())
				q = EatParam(q, sBaseDesc);
			else if (m_sLeftDesc.empty())
				q = EatParam(q, m_sLeftDesc);
			else if (m_sRightDesc.empty())
				q = EatParam(q, m_sRightDesc);
			else
				q = EatParam(q, param); // ignore excess arguments
		}
	}
	if (!sOutFile.empty())
	{
		m_invocationMode = InvocationModeMergeTool;
		m_nSingleInstance = 0;
		String path = paths_GetLongPath(sOutFile.c_str());
		m_Files.push_back(path);
	}
}

/**
 * @brief Add path to list of paths.
 * This method adds given string as a path to the list of paths. Path
 * are converted if needed, shortcuts expanded etc.
 * @param [in] path Path string to add.
 */
void MergeCmdLineInfo::AddPath(const String &path)
{
	String param(path);
	// Convert paths given in Linux-style ('/' as separator) given from
	// Cygwin to Windows style ('\' as separator)
	string_replace(param, _T('/'), _T('\\'));

	// If shortcut, expand it first
	if (paths_IsShortcut(param.c_str()))
		param = ExpandShortcut(param.c_str());
	param = paths_GetLongPath(param.c_str());

	// Set flag indicating path is from command line
	const size_t ord = m_Files.size();
	if (ord == 0)
		m_dwLeftFlags |= FFILEOPEN_CMDLINE;
	else if (ord == 1)
		m_dwRightFlags |= FFILEOPEN_CMDLINE;

	m_Files.push_back(param);
}

static const TCHAR WordListYesNo[]
(
	_T("0;no;false\0")
	_T("1;yes;true\0")
);

static LPCTSTR FindInWordList(LPCTSTR p, LPCTSTR q)
{
	while (int n = lstrlen(q))
	{
		if (PathMatchSpec(p, q))
			return q;
		q += n + 1;
	}
	return NULL;
}

/**
 * @brief Parse native WinMerge command line.
 * @param [in] q Points into the command line.
 */
void MergeCmdLineInfo::ParseWinMergeCmdLine(LPCTSTR q)
{
	String param;
	bool flag;

	while ((q = EatParam(q, param, &flag)) != 0)
	{
		if (!flag)
		{
			// Its not a flag so it is a path
			AddPath(param);
		}
		else if (param == _T("?"))
		{
			// -? to show common command line arguments.
			m_bShowUsage = true;
		}
		else if (param == _T("dl"))
		{
			// -dl "desc" - description for left file
			q = EatParam(q, m_sLeftDesc);
		}
		else if (param == _T("dr"))
		{
			// -dr "desc" - description for right file
			q = EatParam(q, m_sRightDesc);
		}
		else if (param == _T("e"))
		{
			// -e to allow closing with single esc press
			m_invocationMode = InvocationModeEscShutdown;
		}
		else if (param == _T("f"))
		{
			// -f "mask" - file filter mask ("*.h *.cpp")
			q = EatParam(q, m_sFileFilter);
		}
		else if (param == _T("m"))
		{
			// -m "method" - compare method
			q = EatParam(q, param);
			if (param == _T("Full"))
				m_nCompMethod = CMP_CONTENT;
			else if (param == _T("Quick"))
				m_nCompMethod = CMP_QUICK_CONTENT;
			else if (param == _T("Date"))
				m_nCompMethod = CMP_DATE;
			else if (param == _T("SizeDate"))
				m_nCompMethod = CMP_DATE_SIZE;
			else if (param == _T("Size"))
				m_nCompMethod = CMP_SIZE;
		}
		else if (param == _T("t"))
		{
			// -t "type" - builtin type specifier or file transform moniker
			q = EatParam(q, m_sContentType);
		}
		else if (param == _T("run"))
		{
			// -run is followed by .wsf name
			q = EatParam(q, m_sRunScript);
			ProjectFile project;
			if (project.Read(m_sRunScript.c_str()))
			{
				m_sRunScript.clear();
				AddPath(project.m_sLeftFile);
				AddPath(project.m_sRightFile);
				m_sFileFilter = project.m_sFilter;
				m_sContentType = project.m_sCompareAs;
				m_sConfigFileName = project.m_sConfig;
				m_nRecursive = project.m_nRecursive;
				if (project.m_bLeftPathReadOnly)
					m_dwLeftFlags |= FFILEOPEN_READONLY;
				if (project.m_bRightPathReadOnly)
					m_dwRightFlags |= FFILEOPEN_READONLY;
			}
			else
			{
				ParseWinMergeCmdLine(project.m_sOptions.c_str());
			}
		}
		else if (param == _T("r"))
		{
			// -r to compare recursively
			m_nRecursive = 1;
		}
		else if (param == _T("r2"))
		{
			// -r2 to compare recursively but ignore folder structure
			m_nRecursive = 2;
		}
		else if (param == _T("rp"))
		{
			// -rp to follow options as previously adjusted in Open dialog
			m_nRecursive = SettingStore.GetProfileInt(_T("Settings"), _T("Recurse"), 0);
		}
		else if (param == _T("s"))
		{
			// -s to allow only one instance
			if (*q == ':')
			{
				q = EatParam(q, param);
				if (LPCTSTR match = FindInWordList(param.c_str() + 1, WordListYesNo))
				{
					m_nSingleInstance = _ttol(match);
				}
			}
			else
			{
				m_nSingleInstance = 1;
			}
		}
		else if (param == _T("option"))
		{
			// -option to assign a custom set of option indicators
			q = EatParam(q, m_sOptionChars);
		}
		else if (param == _T("noninteractive"))
		{
			// -noninteractive to suppress message boxes & close with result code
			m_bNonInteractive = true;
		}
		else if (param == _T("noprefs"))
		{
			// -noprefs means do not load or remember options (preferences)
			// Load all default settings, but don't save them in registry.
			IOptionDef::InitOptions(NULL, NULL);
		}
		else if (param == _T("config"))
		{
			q = EatParam(q, m_sConfigFileName);
		}
		else if (param == _T("minimize"))
		{
			// -minimize means minimize the main window.
			m_nCmdShow = SW_SHOWMINNOACTIVE;
		}
		else if (param == _T("maximize"))
		{
			// -maximize means maximize the main window.
			m_nCmdShow = SW_MAXIMIZE;
		}
		else if (param == _T("winebugs"))
		{
			// -winebugs means don't work around wine bugs.
			const_cast<const char *&>(wine_version) = NULL;
		}
		else if (param == _T("wl"))
		{
			// -wl to open left path as read-only
			m_dwLeftFlags |= FFILEOPEN_READONLY;
		}
		else if (param == _T("wr"))
		{
			// -wr to open right path as read-only
			m_dwRightFlags |= FFILEOPEN_READONLY;
		}
		else if (param == _T("wp"))
		{
			// -wp to respect user's previous read-only setup when merging into 3rd file
			switch (COptionsMgr::Get(OPT_READ_ONLY))
			{
			case 0:
				m_dwLeftFlags |= FFILEOPEN_READONLY;
				break;
			case 1:
				m_dwRightFlags |= FFILEOPEN_READONLY;
				break;
			}
		}
		else if (param == _T("ul"))
		{
			// -ul to not add left path to MRU
			m_dwLeftFlags |= FFILEOPEN_NOMRU;
		}
		else if (param == _T("ur"))
		{
			// -ur to not add right path to MRU
			m_dwRightFlags |= FFILEOPEN_NOMRU;
		}
		else if (param == _T("u") || param == _T("ub"))
		{
			// -u or -ub (deprecated) to add neither right nor left path to MRU
			m_dwLeftFlags |= FFILEOPEN_NOMRU;
			m_dwRightFlags |= FFILEOPEN_NOMRU;
		}
		else if (param == _T("x"))
		{
			// -x to close application if files are identical.
			m_bExitIfNoDiff = Exit;
		}
		else if (param == _T("xq"))
		{
			// -xn to close application if files are identical without showing
			// any messages
			m_bExitIfNoDiff = ExitQuiet;
		}
		else if (param == _T("cp"))
		{
			q = EatParam(q, param);
			m_nCodepage = _ttol(param.c_str());
		}
		else if (param == _T("ignorews"))
		{
			q = SetOption(q, OPT_CMP_IGNORE_WHITESPACE);
		}
		else if (param == _T("ignoreblanklines"))
		{
			q = SetOption(q, OPT_CMP_IGNORE_BLANKLINES);
		}
		else if (param == _T("ignorecase"))
		{
			q = SetOption(q, OPT_CMP_IGNORE_CASE);
		}
		else if (param == _T("ignoreeol"))
		{
			q = SetOption(q, OPT_CMP_IGNORE_EOL);
		}
		else if (!m_sRunScript.empty() && *q == _T(':'))
		{
			String s;
			q = EatParam(q + 1, s);
			string_replace(s, _T(':'), _T('?'));
			m_sRunScript += string_format(_T("?%s=\"%s\""), param.c_str(), s.c_str());
		}
	}
}
