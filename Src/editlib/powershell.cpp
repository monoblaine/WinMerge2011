///////////////////////////////////////////////////////////////////////////
//  File:       powershell.cpp
//  Version:    1.0
//  Created:    23-Dec-2007
//
//  Copyright:  Stcherbatchenko Andrei, portions by Tim Gerundt
//  E-mail:     windfall@gmx.de
//
//  PowerShell syntax highlighing definition
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ccrystaltextview.h"
#include "SyntaxColors.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

// PowerShell keywords
static BOOL IsPowerShellKeyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszPowerShellKeywordList[] =
  {
    _T ("-and"),
    _T ("-as"),
    _T ("-band"),
    _T ("-bor"),
    _T ("-ccontains"),
    _T ("-ceq"),
    _T ("-cge"),
    _T ("-cgt"),
    _T ("-cle"),
    _T ("-clike"),
    _T ("-clt"),
    _T ("-cmatch"),
    _T ("-cne"),
    _T ("-cnotcontains"),
    _T ("-cnotlike"),
    _T ("-cnotmatch"),
    _T ("-comp"),
    _T ("-contains"),
    _T ("-creplace"),
    _T ("-eq"),
    _T ("-ge"),
    _T ("-gt"),
    _T ("-icontains"),
    _T ("-ieq"),
    _T ("-ige"),
    _T ("-igt"),
    _T ("-ile"),
    _T ("-ilike"),
    _T ("-ilt"),
    _T ("-imatch"),
    _T ("-ine"),
    _T ("-inot"),
    _T ("-inotcontains"),
    _T ("-inotlike"),
    _T ("-inotmatch"),
    _T ("-ireplace"),
    _T ("-is"),
    _T ("-isnot"),
    _T ("-le"),
    _T ("-like"),
    _T ("-lt"),
    _T ("-match"),
    _T ("-ne"),
    _T ("-not"),
    _T ("-notcontains"),
    _T ("-notlike"),
    _T ("-notmatch"),
    _T ("-or"),
    _T ("-replace"),
    _T ("-sl"),
    _T ("-sr"),
    _T ("-xor"),
    _T ("break"),
    _T ("continue"),
    _T ("do"),
    _T ("else"),
    _T ("elseif"),
    _T ("filter"),
    _T ("for"),
    _T ("foreach"),
    _T ("function"),
    _T ("if"),
    _T ("in"),
    _T ("return"),
    _T ("switch"),
    _T ("until"),
    _T ("where"),
    _T ("while"),
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszPowerShellKeywordList);
}

static BOOL IsCmdletKeyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszCmdletKeywordList[] =
  {
    _T ("ac"),
    _T ("add-content"),
    _T ("add-history"),
    _T ("add-member"),
    _T ("add-pssnapin"),
    _T ("asnp"),
    _T ("cat"),
    _T ("cd"),
    _T ("chdir"),
    _T ("clc"),
    _T ("clear"),
    _T ("clear-content"),
    _T ("clear-item"),
    _T ("clear-itemproperty"),
    _T ("clear-variable"),
    _T ("cli"),
    _T ("clp"),
    _T ("cls"),
    _T ("clv"),
    _T ("compare-object"),
    _T ("convert-path"),
    _T ("convertfrom-securestring"),
    _T ("convertto-html"),
    _T ("convertto-securestring"),
    _T ("copy"),
    _T ("copy-item"),
    _T ("copy-itemproperty"),
    _T ("cp"),
    _T ("cpi"),
    _T ("cpp"),
    _T ("cvpa"),
    _T ("del"),
    _T ("diff"),
    _T ("dir"),
    _T ("echo"),
    _T ("epal"),
    _T ("epcsv"),
    _T ("erase"),
    _T ("export-alias"),
    _T ("export-clixml"),
    _T ("export-console"),
    _T ("export-csv"),
    _T ("fc"),
    _T ("fl"),
    _T ("foreach"),
    _T ("foreach-object"),
    _T ("format-custom"),
    _T ("format-list"),
    _T ("format-table"),
    _T ("format-wide"),
    _T ("ft"),
    _T ("fw"),
    _T ("gal"),
    _T ("gc"),
    _T ("gci"),
    _T ("gcm"),
    _T ("gdr"),
    _T ("get-acl"),
    _T ("get-alias"),
    _T ("get-authenticodesignature"),
    _T ("get-childitem"),
    _T ("get-command"),
    _T ("get-content"),
    _T ("get-credential"),
    _T ("get-culture"),
    _T ("get-date"),
    _T ("get-eventlog"),
    _T ("get-executionpolicy"),
    _T ("get-help"),
    _T ("get-history"),
    _T ("get-host"),
    _T ("get-item"),
    _T ("get-itemproperty"),
    _T ("get-location"),
    _T ("get-member"),
    _T ("get-pfxcertificate"),
    _T ("get-process"),
    _T ("get-psdrive"),
    _T ("get-psprovider"),
    _T ("get-pssnapin"),
    _T ("get-service"),
    _T ("get-tracesource"),
    _T ("get-uiculture"),
    _T ("get-unique"),
    _T ("get-variable"),
    _T ("get-wmiobject"),
    _T ("ghy"),
    _T ("gi"),
    _T ("gl"),
    _T ("gm"),
    _T ("gp"),
    _T ("gps"),
    _T ("group"),
    _T ("group-object"),
    _T ("gsnp"),
    _T ("gsv"),
    _T ("gu"),
    _T ("gv"),
    _T ("gwmi"),
    _T ("h"),
    _T ("history"),
    _T ("iex"),
    _T ("ihy"),
    _T ("ii"),
    _T ("import-alias"),
    _T ("import-clixml"),
    _T ("import-csv"),
    _T ("invoke-expression"),
    _T ("invoke-history"),
    _T ("invoke-item"),
    _T ("ipal"),
    _T ("ipcsv"),
    _T ("join-path"),
    _T ("kill"),
    _T ("lp"),
    _T ("ls"),
    _T ("measure-command"),
    _T ("measure-object"),
    _T ("mi"),
    _T ("mount"),
    _T ("move"),
    _T ("move-item"),
    _T ("move-itemproperty"),
    _T ("mp"),
    _T ("mv"),
    _T ("nal"),
    _T ("ndr"),
    _T ("new-alias"),
    _T ("new-item"),
    _T ("new-itemproperty"),
    _T ("new-object"),
    _T ("new-psdrive"),
    _T ("new-service"),
    _T ("new-timespan"),
    _T ("new-variable"),
    _T ("ni"),
    _T ("nv"),
    _T ("oh"),
    _T ("out-default"),
    _T ("out-file"),
    _T ("out-host"),
    _T ("out-null"),
    _T ("out-printer"),
    _T ("out-string"),
    _T ("pop-location"),
    _T ("popd"),
    _T ("ps"),
    _T ("push-location"),
    _T ("pushd"),
    _T ("pwd"),
    _T ("r"),
    _T ("rd"),
    _T ("rdr"),
    _T ("read-host"),
    _T ("remove-item"),
    _T ("remove-itemproperty"),
    _T ("remove-psdrive"),
    _T ("remove-pssnapin"),
    _T ("remove-variable"),
    _T ("ren"),
    _T ("rename-item"),
    _T ("rename-itemproperty"),
    _T ("resolve-path"),
    _T ("restart-service"),
    _T ("resume-service"),
    _T ("ri"),
    _T ("rm"),
    _T ("rmdir"),
    _T ("rni"),
    _T ("rnp"),
    _T ("rp"),
    _T ("rsnp"),
    _T ("rv"),
    _T ("rvpa"),
    _T ("sal"),
    _T ("sasv"),
    _T ("sc"),
    _T ("select"),
    _T ("select-object"),
    _T ("select-string"),
    _T ("set"),
    _T ("set-acl"),
    _T ("set-alias"),
    _T ("set-authenticodesignature"),
    _T ("set-content"),
    _T ("set-date"),
    _T ("set-executionpolicy"),
    _T ("set-item"),
    _T ("set-itemproperty"),
    _T ("set-location"),
    _T ("set-psdebug"),
    _T ("set-service"),
    _T ("set-tracesource"),
    _T ("set-variable"),
    _T ("si"),
    _T ("sl"),
    _T ("sleep"),
    _T ("sort"),
    _T ("sort-object"),
    _T ("sp"),
    _T ("split-path"),
    _T ("spps"),
    _T ("spsv"),
    _T ("start-service"),
    _T ("start-sleep"),
    _T ("start-transcript"),
    _T ("stop-process"),
    _T ("stop-service"),
    _T ("stop-transcript"),
    _T ("suspend-service"),
    _T ("sv"),
    _T ("tee"),
    _T ("tee-object"),
    _T ("test-path"),
    _T ("trace-command"),
    _T ("type"),
    _T ("update-formatdata"),
    _T ("update-typedata"),
    _T ("where"),
    _T ("where-object"),
    _T ("write"),
    _T ("write-debug"),
    _T ("write-error"),
    _T ("write-host"),
    _T ("write-output"),
    _T ("write-progress"),
    _T ("write-verbose"),
    _T ("write-warning"),
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszCmdletKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010
#define COOKIE_VARIABLE         0x0020

DWORD CCrystalTextView::ParseLinePowerShell(DWORD dwCookie, int nLineIndex, TextBlock::Array &pBuf)
{
  const int nLength = GetLineLength(nLineIndex);
  if (nLength == 0)
    return dwCookie & COOKIE_EXT_COMMENT;

  const LPCTSTR pszChars = GetLineChars(nLineIndex);
  BOOL bFirstChar = (dwCookie & ~COOKIE_EXT_COMMENT) == 0;
  BOOL bRedefineBlock = TRUE;
  BOOL bDecIndex = FALSE;
  int nIdentBegin = -1;
  int nPrevI = -1;
  int I;
  for (I = 0; I <= nLength; nPrevI = I++)
    {
      if (bRedefineBlock)
        {
          int nPos = I;
          if (bDecIndex)
            nPos = nPrevI;
          if (dwCookie & (COOKIE_COMMENT | COOKIE_EXT_COMMENT))
            {
              DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
            }
          else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
            {
              DEFINE_BLOCK(nPos, COLORINDEX_STRING);
            }
          else if (dwCookie & COOKIE_VARIABLE)
            {
              DEFINE_BLOCK(nPos, COLORINDEX_USER1);
            }
          else
            {
              //if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.' || pszChars[nPos] == '-')
              if (xisalnum(pszChars[nPos]) || (pszChars[nPos] == '-' && nPos > 0 && (xisalpha(pszChars[nPos + 1]))))
                {
                  DEFINE_BLOCK(nPos, COLORINDEX_NORMALTEXT);
                }
              else
                {
                  DEFINE_BLOCK(nPos, COLORINDEX_OPERATOR);
                  bRedefineBlock = TRUE;
                  bDecIndex = TRUE;
                  goto out;
                }
            }
          bRedefineBlock = FALSE;
          bDecIndex = FALSE;
        }
out:

      // Can be bigger than length if there is binary data
      // See bug #1474782 Crash when comparing SQL with with binary data
      if (I >= nLength)
        break;

      if (dwCookie & COOKIE_COMMENT)
        {
          DEFINE_BLOCK(I, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_COMMENT;
          break;
        }

      //  String constant "...."
      if (dwCookie & COOKIE_STRING)
        {
          if (pszChars[I] == '"' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
            {
              dwCookie &= ~COOKIE_STRING;
              bRedefineBlock = TRUE;
            }
          continue;
        }

      //  Char constant '..'
      if (dwCookie & COOKIE_CHAR)
        {
          if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
            {
              dwCookie &= ~COOKIE_CHAR;
              bRedefineBlock = TRUE;
            }
          continue;
        }

      //  Variable
      if (dwCookie & COOKIE_VARIABLE)
        {
          if (!xisalnum(pszChars[I]))
            {
              dwCookie &= ~COOKIE_VARIABLE;
              bRedefineBlock = TRUE;
              bDecIndex = TRUE;
            }
          continue;
        }

      // Comment #
      if (pszChars[I] == '#')
        {
          DEFINE_BLOCK(I, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_COMMENT;
          break;
        }

      //  Normal text
      if (pszChars[I] == '"')
        {
          DEFINE_BLOCK(I, COLORINDEX_STRING);
          dwCookie |= COOKIE_STRING;
          continue;
        }
      if (pszChars[I] == '\'')
        {
          // if (I + 1 < nLength && pszChars[I + 1] == '\'' || I + 2 < nLength && pszChars[I + 1] != '\\' && pszChars[I + 2] == '\'' || I + 3 < nLength && pszChars[I + 1] == '\\' && pszChars[I + 3] == '\'')
          if (!I || !xisalnum(pszChars[nPrevI]))
            {
              DEFINE_BLOCK(I, COLORINDEX_STRING);
              dwCookie |= COOKIE_CHAR;
              continue;
            }
        }

      // Variable
      if (pszChars[I] == '$')
        {
          DEFINE_BLOCK(I, COLORINDEX_USER1);
          dwCookie |= COOKIE_VARIABLE;
          continue;
        }

      if (bFirstChar)
        {
          if (!xisspace(pszChars[I]))
            bFirstChar = FALSE;
        }

      if (pBuf == NULL)
        continue;               //  We don't need to extract keywords,
      //  for faster parsing skip the rest of loop

      if (xisalnum(pszChars[I]) || pszChars[I] == '-')
        {
          if (nIdentBegin == -1)
            nIdentBegin = I;
        }
      else
        {
          if (nIdentBegin >= 0)
            {
              if (IsPowerShellKeyword(pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
                }
              else if (IsCmdletKeyword(pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
                }
              else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
                }
              bRedefineBlock = TRUE;
              bDecIndex = TRUE;
              nIdentBegin = -1;
            }
        }
    }

  if (nIdentBegin >= 0)
    {
      if (IsPowerShellKeyword(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
        }
      else if (IsCmdletKeyword(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
        }
      else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
        }
    }

  dwCookie &= COOKIE_EXT_COMMENT;
  return dwCookie;
}

TESTCASE
{
	int count = 0;
	BOOL (*pfnIsKeyword)(LPCTSTR, int) = NULL;
	FILE *file = fopen(__FILE__, "r");
	assert(file);
	TCHAR text[1024];
	while (_fgetts(text, _countof(text), file))
	{
		TCHAR c, *p, *q;
		if (pfnIsKeyword && (p = _tcschr(text, '"')) != NULL && (q = _tcschr(++p, '"')) != NULL)
			assert(pfnIsKeyword(p, static_cast<int>(q - p)));
		else if (_stscanf(text, _T(" static BOOL IsPowerShellKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsPowerShellKeyword;
		else if (_stscanf(text, _T(" static BOOL IsCmdletKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsCmdletKeyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			pfnIsKeyword = NULL;
	}
	fclose(file);
	assert(count == 2);
	return count;
}
