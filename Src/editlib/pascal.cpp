///////////////////////////////////////////////////////////////////////////
//  File:    pascal.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  Pascal syntax highlighing definition
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

// Pascal keywords
static BOOL IsPascalKeyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszPascalKeywordList[] =
  {
    _T ("Abstract"),
    _T ("and"),
    _T ("array"),
    _T ("As"),
    _T ("asm"),
    _T ("assembler"),
    _T ("begin"),
    _T ("case"),
    _T ("Class"),
    _T ("const"),
    _T ("constructor"),
    _T ("Default"),
    _T ("destructor"),
    _T ("div"),
    _T ("do"),
    _T ("downto"),
    _T ("Dynamic"),
    _T ("else"),
    _T ("end"),
    _T ("Except"),
    _T ("exit"),
    _T ("Export"),
    _T ("external"),
    _T ("far"),
    _T ("file"),
    _T ("Finally"),
    _T ("for"),
    _T ("function"),
    _T ("goto"),
    _T ("if"),
    _T ("implementation"),
    _T ("In"),
    _T ("Index"),
    _T ("inherited"),
    _T ("inline"),
    _T ("interface"),
    _T ("Is"),
    _T ("label"),
    _T ("mod"),
    _T ("near"),
    _T ("nil"),
    _T ("not"),
    _T ("object"),
    _T ("of"),
    _T ("On"),
    _T ("or"),
    _T ("Out"),
    _T ("Overload"),
    _T ("Override"),
    _T ("Packed"),
    _T ("Private"),
    _T ("procedure"),
    _T ("program"),
    _T ("Property"),
    _T ("Protected"),
    _T ("Public"),
    _T ("Published"),
    _T ("Raise"),
    _T ("record"),
    _T ("repeat"),
    _T ("set"),
    _T ("Shl"),
    _T ("Shr"),
    _T ("string"),
    _T ("then"),
    _T ("ThreadVar"),
    _T ("to"),
    _T ("Try"),
    _T ("type"),
    _T ("unit"),
    _T ("until"),
    _T ("uses"),
    _T ("var"),
    _T ("virtual"),
    _T ("Virtual"),
    _T ("while"),
    _T ("with"),
    _T ("xor"),
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszPascalKeywordList);
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
#define COOKIE_EXT_COMMENT2     0x0020

DWORD CCrystalTextView::ParseLinePascal(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems)
{
  const int nLength = GetLineLength(nLineIndex);
  if (nLength == 0)
    return dwCookie & (COOKIE_EXT_COMMENT | COOKIE_EXT_COMMENT2);

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
          if (dwCookie & (COOKIE_COMMENT | COOKIE_EXT_COMMENT | COOKIE_EXT_COMMENT2))
            {
              DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
            }
          else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
            {
              DEFINE_BLOCK(nPos, COLORINDEX_STRING);
            }
          else
            {
              if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.' && nPos > 0 && (!xisalpha(pszChars[nPos - 1]) && !xisalpha(pszChars[nPos + 1])))
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

      //  Extended comment /*....*/
      if (dwCookie & COOKIE_EXT_COMMENT)
        {
          // if (I > 0 && pszChars[I] == ')' && pszChars[nPrevI] == '*')
          if ((I > 1 && pszChars[I] == ')' && pszChars[nPrevI] == '*' && pszChars[nPrevI - 1] != '(') || (I == 1 && pszChars[I] == ')' && pszChars[nPrevI] == '*'))
            {
              dwCookie &= ~COOKIE_EXT_COMMENT;
              bRedefineBlock = TRUE;
            }
          continue;
        }

      //  Extended comment {....}
      if (dwCookie & COOKIE_EXT_COMMENT2)
        {
          if (pszChars[I] == '}')
            {
              dwCookie &= ~COOKIE_EXT_COMMENT2;
              bRedefineBlock = TRUE;
            }
          continue;
        }

      if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '/')
        {
          DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
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
      if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '(')
        {
          DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_EXT_COMMENT;
          continue;
        }

      if (pszChars[I] == '{')
        {
          DEFINE_BLOCK(I, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_EXT_COMMENT2;
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

      if (xisalnum(pszChars[I]) || pszChars[I] == '.' && I > 0 && (!xisalpha(pszChars[nPrevI]) && !xisalpha(pszChars[I + 1])))
        {
          if (nIdentBegin == -1)
            nIdentBegin = I;
        }
      else
        {
          if (nIdentBegin >= 0)
            {
              if (IsPascalKeyword(pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
                }
              else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
                {
                  DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
                }
              else
                {
                  bool bFunction = FALSE;

                  for (int j = I; j < nLength; j++)
                    {
                      if (!xisspace(pszChars[j]))
                        {
                          if (pszChars[j] == '(')
                            {
                              bFunction = TRUE;
                            }
                          break;
                        }
                    }
                  if (bFunction)
                    {
                      DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
                    }
                }
              bRedefineBlock = TRUE;
              bDecIndex = TRUE;
              nIdentBegin = -1;
            }
        }
    }

  if (nIdentBegin >= 0)
    {
      if (IsPascalKeyword(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
        }
      else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
        {
          DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
        }
      else
        {
          bool bFunction = FALSE;

          for (int j = I; j < nLength; j++)
            {
              if (!xisspace(pszChars[j]))
                {
                  if (pszChars[j] == '(')
                    {
                      bFunction = TRUE;
                    }
                  break;
                }
            }
          if (bFunction)
            {
              DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
            }
        }
    }

  if (pszChars[nLength - 1] != '\\')
    dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_EXT_COMMENT2);
  return dwCookie;
}
