///////////////////////////////////////////////////////////////////////////
//  File:       vhdl.cpp
//  Version:    1.0
//  Created:    8-Nov-2017
//
//  Copyright:  H.Saido, portions by Tim Gerundt, Stcherbatchenko Andrei
//  E-mail:     saido.nv@gmail.com
//
//  VHDL syntax highlighing definition
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

// VHDL keywords
static BOOL IsVhdlKeyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszVhdlKeywordList[] =
  {
    _T ("abs"),
    _T ("access"),
    _T ("after"),
    _T ("alias"),
    _T ("all"),
    _T ("and"),
    _T ("architecture"),
    _T ("array"),
    _T ("assert"),
    _T ("attribute"),
    _T ("begin"),
    _T ("block"),
    _T ("body"),
    _T ("true"),
    _T ("buffer"),
    _T ("bus"),
    _T ("case"),
    _T ("component"),
    _T ("configuration"),
    _T ("constant"),
    _T ("disconnect"),
    _T ("downto"),
    _T ("else"),
    _T ("elsif"),
    _T ("end"),
    _T ("entity"),
    _T ("exit"),
    _T ("false"),
    _T ("file"),
    _T ("for"),
    _T ("function"),
    _T ("generate"),
    _T ("generic"),
    _T ("guarded"),
    _T ("if"),
    _T ("in"),
    _T ("inout"),
    _T ("is"),
    _T ("label"),
    _T ("library"),
    _T ("linkage"),
    _T ("loop"),
    _T ("map"),
    _T ("mod"),
    _T ("nand"),
    _T ("new"),
    _T ("next"),
    _T ("nor"),
    _T ("not"),
    _T ("null"),
    _T ("of"),
    _T ("on"),
    _T ("open"),
    _T ("or"),
    _T ("others"),
    _T ("out"),
    _T ("pakage"),
    _T ("port"),
    _T ("procedure"),
    _T ("process"),
    _T ("range"),
    _T ("record"),
    _T ("register"),
    _T ("rem"),
    _T ("report"),
    _T ("return"),
    _T ("select"),
    _T ("severity"),
    _T ("signal"),
    _T ("subtype"),
    _T ("then"),
    _T ("to"),
    _T ("transport"),
    _T ("true"),
    _T ("type"),
    _T ("units"),
    _T ("until"),
    _T ("use"),
    _T ("variable"),
    _T ("wait"),
    _T ("when"),
    _T ("while"),
    _T ("with"),
    _T ("xor")
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszVhdlKeywordList);
}

static BOOL IsVhdlAttribute(LPCTSTR pszChars, int nLength, int *nAttributeBegin)
{
  static LPCTSTR const s_apszVhdlAttributeList[] =
  {
    _T ("'active"),
    _T ("'ascending"),
    _T ("'base"),
    _T ("'delayed"),
    _T ("'driving"),
    _T ("'driving_value"),
    _T ("'event"),
    _T ("'high"),
    _T ("'image"),
    _T ("'instance_name"),
    _T ("'last_active"),
    _T ("'last_event"),
    _T ("'last_value"),
    _T ("'left"),
    _T ("'leftof"),
    _T ("'length"),
    _T ("'low"),
    _T ("'path_name"),
    _T ("'pos"),
    _T ("'pred"),
    _T ("'quiet"),
    _T ("'range"),
    _T ("'reverse_range"),
    _T ("'right"),
    _T ("'rightof"),
    _T ("'simple_name"),
    _T ("'stable"),
    _T ("'succ"),
    _T ("'transaction"),
    _T ("'val"),
    _T ("'value")
  };
  for (int I = 0; I < nLength; I++)
    {
      if (pszChars[I] == '\'')
        {
          *nAttributeBegin = I;
          return xiskeyword<_tcsnicmp>(&pszChars[I], nLength - I, s_apszVhdlAttributeList);
        }
   }
  return FALSE;
}

// VHDL Types
static BOOL IsVhdlType(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszVhdlTypeList[] =
  {
    _T ("bit"),
    _T ("bit_vector"),
    _T ("boolean"),
    _T ("character"),
    _T ("fs"),
    _T ("hr"),
    _T ("integer"),
    _T ("line"),
    _T ("min"),
    _T ("ms"),
    _T ("natural"),
    _T ("now"),
    _T ("ns"),
    _T ("positive"),
    _T ("ps"),
    _T ("real"),
    _T ("sec"),
    _T ("side"),
    _T ("signed"),
    _T ("std_logic"),
    _T ("std_logic_vector"),
    _T ("std_ulogic"),
    _T ("std_ulogic_vector"),
    _T ("string"),
    _T ("text"),
    _T ("time"),
    _T ("unsigned"),
    _T ("us"),
    _T ("width")
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszVhdlTypeList);
}

// VHDL functions
static BOOL IsVhdlFunction(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszVhdlFunctionList[] =
  {
    _T ("conv_integer"),
    _T ("conv_signed"),
    _T ("conv_std_logic_vector"),
    _T ("conv_unsigned"),
    _T ("falling_edge"),
    _T ("hread"),
    _T ("hwrite"),
    _T ("oread"),
    _T ("owrite"),
    _T ("read"),
    _T ("readline"),
    _T ("rising_edge"),
    _T ("to_bit"),
    _T ("to_bitvector"),
    _T ("to_signed"),
    _T ("to_stdlogicvector"),
    _T ("to_stdulogic"),
    _T ("to_stdulogicvector"),
    _T ("to_unsigned"),
    _T ("write"),
    _T ("writeline"),
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszVhdlFunctionList);
}

static bool IsVhdlNumber(LPCTSTR pszChars, int nLength)
{
  if (nLength == 3 && pszChars[0] == '\'' && pszChars[2] == '\'')
    {
      if (pszChars[1] == '0' ||
          pszChars[1] == '1' ||
          pszChars[1] == 'u' ||
          pszChars[1] == 'U' ||
          pszChars[1] == 'x' ||
          pszChars[1] == 'X' ||
          pszChars[1] == 'w' ||
          pszChars[1] == 'W' ||
          pszChars[1] == 'l' ||
          pszChars[1] == 'L' ||
          pszChars[1] == 'h' ||
          pszChars[1] == 'H' ||
          pszChars[1] == 'z' ||
          pszChars[1] == 'Z' ||
          pszChars[1] == '-' ||
          pszChars[1] == '_' )
        return true;
    }
  if (nLength >= 4 && pszChars[1] == '"' && pszChars[nLength-1] == '"')
    {
      if (pszChars[0] == 'x' ||
          pszChars[0] == 'X' ||
          pszChars[0] == 'b' ||
          pszChars[0] == 'B' ||
          pszChars[0] == 'o' ||
          pszChars[0] == 'O' )
      {
        for (int I = 2; I < nLength-1; I++)
          {
            if (_istxdigit (pszChars[I]) ||
                  pszChars[I] == 'u' || pszChars[I] == 'U' ||
                  pszChars[I] == 'x' || pszChars[I] == 'X' ||
                  pszChars[I] == 'w' || pszChars[I] == 'W' ||
                  pszChars[I] == 'l' || pszChars[I] == 'L' ||
                  pszChars[I] == 'h' || pszChars[I] == 'H' ||
                  pszChars[I] == 'z' || pszChars[I] == 'Z' ||
                  // FIXME: Could not display "----", yet.
                  /* pszChars[I] == '-' || */ pszChars[I] == '_' )
              continue;
            return false;
          }
        return true;
      }
    }
  if (nLength >= 3 && pszChars[0] == '"' && pszChars[nLength-1] == '"')
    {
      for (int I = 1; I < nLength-1; I++)
        {
          if (_istxdigit (pszChars[I]) ||
                pszChars[I] == 'u' || pszChars[I] == 'U' ||
                pszChars[I] == 'x' || pszChars[I] == 'X' ||
                pszChars[I] == 'w' || pszChars[I] == 'W' ||
                pszChars[I] == 'l' || pszChars[I] == 'L' ||
                pszChars[I] == 'h' || pszChars[I] == 'H' ||
                pszChars[I] == 'z' || pszChars[I] == 'Z' ||
                // FIXME: Could not display "----", yet.
                /* pszChars[I] == '-' || */ pszChars[I] == '_' )
            continue;
          return false;
        }
      return true;
    }
  for (int I = 0; I < nLength; I++)
    {
      if (!_istdigit (pszChars[I]) && pszChars[I] != '+' &&
            pszChars[I] != '-' && pszChars[I] != '.' && pszChars[I] != 'e' &&
            pszChars[I] != 'E' && pszChars[I] != '_')
        return false;
    }
  return true;
}

static bool IsVhdlChar(LPCTSTR pszChars, int nLength)
{
  return nLength == 3 && pszChars[0] == '\'' && pszChars[2] == '\'';
}

#define DEFINE_BLOCK(pos, colorindex)   \
ASSERT((pos) >= 0 && (pos) <= nLength);\
if (pBuf != NULL)\
  {\
    if (nActualItems == 0 || pBuf[nActualItems - 1].m_nCharPos <= (pos)){\
        pBuf[nActualItems].m_nCharPos = (pos);\
        pBuf[nActualItems].m_nColorIndex = (colorindex);\
        pBuf[nActualItems].m_nBgColorIndex = COLORINDEX_BKGND;\
        nActualItems ++;}\
  }

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::ParseLineVhdl(DWORD dwCookie, int nLineIndex, TEXTBLOCK * pBuf, int &nActualItems)
{
  const int nLength = GetLineLength(nLineIndex);
  if (nLength == 0)
    return dwCookie & COOKIE_EXT_COMMENT;

  const LPCTSTR pszChars = GetLineChars(nLineIndex);
  bool bRedefineBlock = true;
  bool bDecIndex = false;
  bool bNum = false;
  int nIdentBegin = -1;
  int nAttributeBegin = 0;
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
          else if (dwCookie & (COOKIE_STRING))
            {
              DEFINE_BLOCK(nPos, COLORINDEX_STRING);
            }
          else if (dwCookie & COOKIE_PREPROCESSOR)
            {
              DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
            }
          else
            {
              if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.' && nPos > 0 && (!xisalpha(pszChars[nPos - 1]) && !xisalpha(pszChars[nPos + 1])))
                {
                  DEFINE_BLOCK(nPos, COLORINDEX_NORMALTEXT);
                }
              else if (pszChars[nPos] == '"' || pszChars[nPos] == '\'')
                {
                  DEFINE_BLOCK(nPos, COLORINDEX_NORMALTEXT);
                }
              else
                {
                  DEFINE_BLOCK(nPos, COLORINDEX_OPERATOR);
                  bRedefineBlock = true;
                  bDecIndex = true;
                  goto out;
                }
            }
          bRedefineBlock = false;
          bDecIndex = false;
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

      // Line comment --...
      if (I > 0 && pszChars[I] == '-' && pszChars[nPrevI] == '-')
        {
          DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_COMMENT;
          break;
        }

      //  String constant "...."
      if (dwCookie & COOKIE_STRING)
        {
          if (pszChars[I] == '"' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
            {
              dwCookie &= ~COOKIE_STRING;
              bRedefineBlock = true;
            }
          continue;
        }

      // //  Normal text
      // if (pszChars[I] == '"')
      //   {
      //     DEFINE_BLOCK(I, COLORINDEX_STRING);
      //     dwCookie |= COOKIE_STRING;
      //     continue;
      //   }

      if (pBuf == NULL)
        continue;               //  We don't need to extract keywords,
      //  for faster parsing skip the rest of loop

      if (xisalnum(pszChars[I]) || pszChars[I] == '.' && I > 0 && (!xisalpha(pszChars[nPrevI]) && !xisalpha(pszChars[I + 1])))
        {
          if (nIdentBegin == -1)
            nIdentBegin = I;
        }
      else if (pszChars[I] == '"' || pszChars[I] == '\'')
        {
          if (nIdentBegin == -1)
            nIdentBegin = I;
        }
      else
        {
          if (nIdentBegin >= 0)
            {
              if (IsVhdlNumber(pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
                }
              else if (pszChars[nIdentBegin] == '"')
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_STRING);
                  dwCookie |= COOKIE_STRING;
                  continue;
                }
              else if (IsVhdlKeyword(pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
                }
              else if (IsVhdlAttribute (pszChars + nIdentBegin, I - nIdentBegin, &nAttributeBegin))
                {
                  DEFINE_BLOCK(nIdentBegin + nAttributeBegin, COLORINDEX_FUNCNAME);
                }
              else if (IsVhdlType (pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_PREPROCESSOR);
                }
              else if (IsVhdlFunction (pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
                }
              else if (IsVhdlChar (pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_STRING);
                }
              bRedefineBlock = true;
              bDecIndex = true;
              nIdentBegin = -1;
            }
        }
    }

  if (nIdentBegin >= 0)
    {
      if (IsVhdlNumber(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
        }
      else if (pszChars[nIdentBegin] == '"')
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_STRING);
          dwCookie |= COOKIE_STRING;
        }
      else if (IsVhdlKeyword(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
        }
      else if (IsVhdlAttribute(pszChars + nIdentBegin, I - nIdentBegin, &nAttributeBegin))
        {
          DEFINE_BLOCK(nIdentBegin + nAttributeBegin, COLORINDEX_FUNCNAME);
        }
      else if (IsVhdlType(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_PREPROCESSOR);
        }
      else if (IsVhdlFunction(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
        }
      else if (IsVhdlChar(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_STRING);
        }
    }

  dwCookie &= COOKIE_EXT_COMMENT;
  return dwCookie;
}
