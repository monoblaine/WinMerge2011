////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaltextview.h
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Interface of the CCrystalTextView class, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: see cpps ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////
/** 
 * @file  ccrystaltextview.h
 *
 * @brief Declaration file for CCrystalTextView
 */
#pragma once

#include "SyntaxColors.h"

#define COMMON_LEXIS 001
#define NATIVE_LEXIS 002
#define SCRIPT_LEXIS 004
#define NATIVE_LEXIS_ONLY _T(_CRT_STRINGIZE(_CRT_APPEND(\, NATIVE_LEXIS)))
#define SCRIPT_LEXIS_ONLY _T(_CRT_STRINGIZE(_CRT_APPEND(\, SCRIPT_LEXIS)))

#define COOKIE_PARSER           0x0F000000UL
#define COOKIE_PARSER_BASIC     0x01000000UL
#define COOKIE_PARSER_CSHARP    0x02000000UL
#define COOKIE_PARSER_JAVA      0x03000000UL
#define COOKIE_PARSER_PERL      0x04000000UL
#define COOKIE_PARSER_PHP       0x05000000UL
#define COOKIE_PARSER_JSCRIPT   0x06000000UL
#define COOKIE_PARSER_VBSCRIPT  0x07000000UL
#define COOKIE_PARSER_CSS       0x08000000UL
#define COOKIE_PARSER_MWSL      0x09000000UL
#define COOKIE_PARSER_GLOBAL    0xF0000000UL

#define SRCOPT_COOKIE(X)        ((X) << 4)
C_ASSERT(COOKIE_PARSER_GLOBAL == COOKIE_PARSER << 4);

// Cookie bits which are shared across parsers
#define COOKIE_TEMPLATE_STRING  0x80

////////////////////////////////////////////////////////////////////////////
// Forward class declarations

class CCrystalTextBuffer;
class CUpdateContext;

////////////////////////////////////////////////////////////////////////////
// CCrystalTextView class declaration

//  CCrystalTextView::FindText() flags
enum
{
	FIND_MATCH_CASE = 0x0001,
	FIND_WHOLE_WORD = 0x0002,
	FIND_REGEXP = 0x0004,
	FIND_DIRECTION_UP = 0x0010,
	REPLACE_SELECTION = 0x0100, 
	FIND_NO_WRAP = 0x0200
};

//  CCrystalTextView::UpdateView() flags
enum
{
	UPDATE_HORZRANGE = 0x0001,  //  update horz scrollbar
	UPDATE_VERTRANGE = 0x0002, //  update vert scrollbar
	UPDATE_SINGLELINE = 0x0100,    //  single line has changed
	UPDATE_FLAGSONLY = 0x0200, //  only line-flags were changed

	UPDATE_RESET = 0x1000       //  document was reloaded, update all!
};

/**
 * @brief Class for text view.
 * This class implements class for text viewing. Class implements all
 * the routines we need for showing, selecting text etc. BUT it does
 * not implement text editing. There are classes inherited from this
 * class which implement text editing.
 */
class CCrystalTextView
	: ZeroInit<CCrystalTextView>
	, public OWindow
	, public IDropSource
	, public IDataObject
{
public:
	typedef int Captures[30];

private:
	LOGFONT m_lfBaseFont;
	HFont *m_apFonts[4];

	//  Line/character dimensions
	int m_nLineHeight, m_nCharWidth;
	void CalcLineCharDim();

	//  Text attributes
	bool m_bViewTabs;
	bool m_bViewEols;
	bool m_bDistinguishEols;
	bool m_bSelMargin;
	bool m_bViewLineNumbers;
	bool m_bSeparateCombinedChars;
	DWORD m_dwFlags;

	CSyntaxColors *m_pColors;

	//BEGIN SW
	/**
	Contains for each line the number of sublines. If the line is not
	wrapped, the value for this line is 1. The value of a line is invalid,
	if it is -1.

	Must create pointer, because contructor uses AFX_ZERO_INIT_OBJECT to
	initialize the member objects. This would destroy a CArray object.
	*/
	std::vector<int> m_panSubLines;
	std::vector<int> m_panSubLineIndexCache;
	int m_nLastLineIndexCalculatedSubLineIndex;
	//END SW

	int m_nMaxLineLength;
	int m_nIdealCharPos;

	static int FindStringHelper(
		LPCTSTR pchFindWhere, UINT cchFindWhere,
		LPCTSTR pchFindWhat, DWORD dwFlags,
		Captures &ovector);

protected:

	bool m_bFocused;
	bool m_bCursorHidden;
	bool m_bOvrMode;
	bool m_bShowInactiveSelection;

	//  [JRT]
	bool m_bDisableDragAndDrop;

	//BEGIN SW
	bool m_bWordWrap;
	//END SW

	//  Search parameters
	bool m_bLastSearch;
	DWORD m_dwLastSearchFlags;
	String m_strLastFindWhat;

	//  Parsing stuff
	class TextBlock
	{
	private:
		~TextBlock() { } // prevents deletion from outside TextBlock::Array
	public:
		int m_nCharPos;
		int m_nColorIndex;
		int m_nBgColorIndex;
		class Array
		{
		private:
			TextBlock *m_pBuf;
		public:
			void const *m_bRecording;
			int m_nActualItems;
			Array(TextBlock *pBuf)
				: m_pBuf(pBuf), m_bRecording(m_pBuf), m_nActualItems(0)
			{ }
			~Array() { delete[] m_pBuf; }
			operator TextBlock *() { return m_pBuf; }
			void swap(Array &other)
			{
				std::swap(m_pBuf, other.m_pBuf);
				std::swap(m_nActualItems, other.m_nActualItems);
			}
			__forceinline void DefineBlock(int pos, int colorindex)
			{
				if (m_bRecording)
				{
					if (m_nActualItems == 0 || m_pBuf[m_nActualItems - 1].m_nCharPos <= pos)
					{
						m_pBuf[m_nActualItems].m_nCharPos = pos;
						m_pBuf[m_nActualItems].m_nColorIndex = colorindex;
						m_pBuf[m_nActualItems].m_nBgColorIndex = COLORINDEX_BKGND;
						++m_nActualItems;
					}
				}
			}
		};
		class Cookie
		{
		public:
			Cookie(DWORD dwCookie = -1)
				: m_dwCookie(dwCookie), m_dwNesting(0)
			{ }
			void Clear()
			{
				m_dwCookie = -1;
				m_dwNesting = 0;
			}
			BOOL Empty() const
			{
				return m_dwCookie == -1;
			}
			DWORD m_dwCookie;
			DWORD m_dwNesting;
		private:
			void operator=(int);
		};
		typedef void (CCrystalTextView::*ParseProc)(Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, Array &pBuf);
	};
	/**  
	This array must be initialized to (DWORD) - 1, code for invalid values (not yet computed).
	We prefer to limit the recomputing delay to the moment when we need to read
	a parseCookie value for drawing.
	GetParseCookie must always be used to read the m_ParseCookies value of a line.
	If the actual value is invalid code, GetParseCookie computes the value, 
	stores it in m_ParseCookies, and returns the new valid value.
	When we edit the text, the parse cookies value may change for the modified line
	and all the lines below (As m_ParseCookies[line i] depends on m_ParseCookies[line (i-1)])
	It would be a loss of time to recompute all these values after each action.
	So we just set all these values to invalid code (DWORD) - 1.
	*/
	std::vector<TextBlock::Cookie> m_ParseCookies;
	TextBlock::Cookie GetParseCookie(int nLineIndex);

	/**
	Pre-calculated line lengths (in characters)
	This array works as the parse cookie Array
	and must be initialized to - 1, code for invalid values (not yet computed).
	for the same reason.
	*/
	std::vector<int> m_pnActualLineLength;

	bool m_bPreparingToDrag;
	bool m_bDraggingText;
	bool m_bDragSelection, m_bWordSelection, m_bLineSelection;
	UINT_PTR m_nDragSelTimer;

	POINT m_ptDrawSelStart, m_ptDrawSelEnd;
	POINT m_ptAnchor;
	POINT m_ptCursorPos, m_ptCursorLast;
	POINT m_ptSelStart, m_ptSelEnd;
	void PrepareSelBounds();

	//  Helper functions
	int ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, String &line, int nActualOffset);

	int ApproxActualOffset(int nLineIndex, int nOffset);
	void AdjustTextPoint(POINT & point);
	void DrawLineHelperImpl(HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
		int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd,
		LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset,
		int nBgColorIndexZeorWidthBlock, int cxZeorWidthBlock);
	bool IsInsideSelBlock(POINT ptTextPos);

	bool m_bBookmarkExist;        // More bookmarks
	void ToggleBookmark(int nLine);

public:
	virtual void ResetView();

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	// IDropSource
	STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
	STDMETHOD(GiveFeedback)(DWORD dwEffect);
	// IDataObject
	STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(QueryGetData)(LPFORMATETC);
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC);
	STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL);
	STDMETHOD(EnumFormatEtc)(DWORD, LPENUMFORMATETC *);
	STDMETHOD(DAdvise)(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD);
	STDMETHOD(DUnadvise)(DWORD);
	STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *);
	
	int GetLineCount();
	virtual int ComputeRealLine(int nApparentLine) const;
	virtual void OnUpdateCaret(bool bShowHide);
	bool IsTextBufferInitialized() const;
	LPCTSTR GetTextBufferEol(int nLine) const;

	CSyntaxColors *GetSyntaxColors() { return m_pColors; }
	void SetColorContext(CSyntaxColors *pColors) { m_pColors = pColors; }
	virtual void SetSelection(const POINT &ptStart, const POINT &ptEnd);

	bool IsSelection();
	bool HasFocus() const { return m_bFocused; }
	virtual bool QueryEditable();

	bool HasBookmarks() const { return m_bBookmarkExist; }

	void SelectAll();
	void UpdateCaret(bool bShowHide = false);
	void SetAnchor(const POINT &);

	static void InitSharedResources();
	static void FreeSharedResources();

protected:
	POINT WordToRight(POINT pt);
	POINT WordToLeft(POINT pt);

	static HImageList *m_pIcons;
	static HBrush *m_pHatchBrush;
	CCrystalTextBuffer *m_pTextBuffer;
	POINT m_ptDraggedTextBegin, m_ptDraggedTextEnd;
	int GetMarginWidth();
	bool IsValidTextPos(const POINT &);
	bool IsValidTextPosX(const POINT &);
	bool IsValidTextPosY(int);

	POINT ClientToText(const POINT &);
	POINT TextToClient(const POINT &);
	void InvalidateLines(int nLine1, int nLine2);
	int CalculateActualOffset(int nLineIndex, int nCharIndex, BOOL bAccumulate = FALSE);

	bool IsInsideSelection(const POINT &ptTextPos);
	void GetFullySelectedLines(int &firstLine, int &lastLine);

	//  Amount of lines/characters that completely fits the client area
	int m_nScreenLines, m_nScreenChars;

	int m_nTopLine, m_nOffsetChar;
	//BEGIN SW
	/**
	The index of the subline that is the first visible line on the screen.
	*/
	int m_nTopSubLine;
	//END SW

	int GetLineHeight();
	//BEGIN SW
	/**
	Returns the number of sublines the given line contains of.
	Allway "1", if word wrapping is disabled.

	@param nLineIndex Index of the line to get the subline count of.

	@return Number of sublines the given line contains of
	*/
	int GetSubLines(int nLineIndex);

	virtual int GetEmptySubLines(int nLineIndex);
	bool IsEmptySubLineIndex(int nSubLineIndex);

	/**
	Converts the given character position for the given line into a point.

	After the call the x-member of the returned point contains the
	character position relative to the beginning of the subline. The y-member
	contains the zero based index of the subline relative to the line, the
	character position was given for.

	@param nLineIndex Zero based index of the line, nCharPos refers to.
	@param nCharPos The character position, the point shoult be calculated for.
	@param charPoint Reference to a point, which should receive the result.

	@return The character position of the beginning of the subline charPoint.y.
	*/
	int CharPosToPoint(int nLineIndex, int nCharPos, POINT &charPoint);

	/**
	Converts the given cursor point for the given line to the character position
	for the given line.

	The y-member of the cursor position specifies the subline inside the given
	line, the cursor is placed on and the x-member specifies the cursor position
	(in character widths) relative to the beginning of that subline.

	@param nLineIndex Zero based index of the line the cursor position refers to.
	@param curPoint Position of the cursor relative to the line in sub lines and
		char widths.

	@return The character position the best matches the cursor position.
	*/
	int CursorPointToCharPos(int nLineIndex, const POINT &curPoint);

	/**
	Converts the given cursor position to a text position.

	The x-member of the subLinePos parameter describes the cursor position in
	char widths relative to the beginning of the subline described by the
	y-member. The subline is the zero based number of the subline relative to
	the beginning of the text buffer.

	<p>
	The returned point contains a valid text position, where the y-member is
	the zero based index of the textline and the x-member is the character
	position inside this line.

	@param subLinePos The sublinebased cursor position
		(see text above for detailed description).
	@param textPos The calculated line and character position that best matches
		the cursor position (see text above for detailed descritpion).
	*/
	void SubLineCursorPosToTextPos(int x, int y, POINT &textPos);

	/**
	Returns the character position relative to the given line, that matches
	the end of the given sub line.

	@param nLineIndex Zero based index of the line to work on.
	@param nSubLineOffset Zero based index of the subline inside the given line.

	@return Character position that matches the end of the given subline, relative
		to the given line.
	*/
	int SubLineEndToCharPos(int nLineIndex, int nSubLineOffset);

	/**
	Returns the character position relative to the given line, that matches
	the start of the given sub line.

	@param nLineIndex Zero based index of the line to work on.
	@param nSubLineOffset Zero based index of the subline inside the given line.

	@return Character position that matches the start of the given subline, relative
		to the given line.
	*/
	int SubLineHomeToCharPos(int nLineIndex, int nSubLineOffset);
	//END SW
	int GetCharWidth();
	int GetMaxLineLength();
	int GetScreenLines();
	int GetScreenChars();
	HFont *GetFont(int nColorIndex = 0);

	virtual int RecalcVertScrollBar(bool bPositionOnly = false);
	virtual int RecalcHorzScrollBar(bool bPositionOnly = false);

	//  Scrolling helpers
	void ScrollToChar(int nNewOffsetChar);
	void ScrollToLine(int nNewTopLine);

	//BEGIN SW
	/**
	Scrolls to the given sub line, so that the sub line is the first visible
	line on the screen.

	@param nNewTopSubLine Index of the sub line to scroll to.
	@param bNoSmoothScroll TRUE to disable smooth scrolling, else FALSE.
	@param bTrackScrollBar TRUE to recalculate the scroll bar after scrolling,
		else FALSE.
	*/
	virtual void ScrollToSubLine(int nNewTopSubLine);
	//END SW

	//  Splitter support
	void UpdateSiblingScrollPos();

	void OnUpdateSibling(const CCrystalTextView *pUpdateSource);

	//BEGIN SW
	/**
	Returns the number of sublines in the whole text buffer.

	The number of sublines is the sum of all sublines of all lines.

	@return Number of sublines in the whole text buffer.
	*/
	int GetSubLineCount();

	/**
	Returns the zero-based subline index of the given line.

	@param nLineIndex Index of the line to calculate the subline index of

	@return The zero-based subline index of the given line.
	*/
	int GetSubLineIndex(int nLineIndex);

	/**
	 * @brief Splits the given subline index into line and sub line of this line.
	 * @param [in] nSubLineIndex The zero based index of the subline to get info about
	 * @param [out] nLine Gets the line number the give subline is included in
	 * @param [out] nSubLine Get the subline of the given subline relative to nLine
	 */
	void GetLineBySubLine(int nSubLineIndex, int &nLine, int &nSubLine);

public:
	int GetLineLength(int nLineIndex) const;
	int GetFullLineLength(int nLineIndex) const;
	int GetViewableLineLength(int nLineIndex) const;
	int GetLineActualLength(int nLineIndex);
	LPCTSTR GetLineChars(int nLineIndex) const;
	DWORD GetLineFlags(int nLineIndex) const;
	void GetSelection(POINT &ptStart, POINT &ptEnd);
	void GetText(int nStartLine, int nStartChar, int nEndLine, int nEndChar, String &text);
	void GetText(const POINT &ptStart, const POINT &ptEnd, String &text)
	{
		GetText(ptStart.y, ptStart.x, ptEnd.y, ptEnd.x, text);
	}
protected:

	// Clipboard
	void PutToClipboard(HGLOBAL);

	// Drag-n-drop
	HGLOBAL PrepareDragData();
	virtual DWORD GetDropEffect();
	virtual void OnDropSource(DWORD de);

	virtual COLORREF GetColor(int nColorIndex);
	virtual void GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText) = 0;
	virtual bool GetItalic(int nColorIndex);
	virtual bool GetBold(int nColorIndex);

	void DrawLineHelper(HSurface *, POINT &ptOrigin, const RECT &rcClip,
		int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd,
		LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset, POINT ptTextPos,
		int nBgColorIndexZeorWidthBlock, int cxZeorWidthBlock);
	virtual void DrawSingleLine(HSurface *, const RECT &, int nLineIndex);
	virtual void DrawMargin(HSurface *, const RECT &, int nLineIndex, int nLineNumber);

	int GetCharWidthFromChar(LPCTSTR) const;
	int GetCharWidthFromDisplayableChar(LPCTSTR) const;

	void AdjustCursorAfterMoveLeft();
	void AdjustCursorAfterMoveRight();

	//BEGIN SW
	// word wrapping

	/**
	Called to wrap the line with the given index into sublines.

	The standard implementation wraps the line at the first non-whitespace after
	an whitespace that exceeds the visible line width (nMaxLineWidth). Override
	this function to provide your own word wrapping.

	<b>Attention:</b> Never call this function directly,
	call WrapLineCached() instead, which calls this method.

	@param nLineIndex The index of the line to wrap

	@param nMaxLineWidth The number of characters a subline of this line should
	not exceed (except whitespaces)

	@param anBreaks An array of integers. Put the positions where to wrap the line
	in that array (its allready allocated). If this pointer is NULL, the function
	has only to compute the number of breaks (the parameter nBreaks).

	@param nBreaks The number of breaks this line has (number of sublines - 1). When
	the function is called, this variable is 0. If the line is not wrapped, this value
	should be 0 after the call.

	@see WrapLineCached()
	*/
	void WrapLine(int nLineIndex, int *anBreaks, int &nBreaks);

	/**
	Called to wrap the line with the given index into sublines.

	Call this method instead of WrapLine() (which is called internal by this
	method). This function uses an internal cache which contains the number
	of sublines for each line, so it has only to call WrapLine(), if the
	cache for the given line is invalid or if the caller wants to get the
	wrap postions (anBreaks != NULL).

	This functions also tests m_bWordWrap -- you can call it even if
	word wrapping is disabled and you will retrieve a valid value.

	@param nLineIndex The index of the line to wrap

	@param nMaxLineWidth The number of characters a subline of this line should
	not exceed (except whitespaces)

	@param anBreaks An array of integers. Put the positions where to wrap the line
	in that array (its allready allocated). If this pointer is NULL, the function
	has only to compute the number of breaks (the parameter nBreaks).

	@param nBreaks The number of breaks this line has (number of sublines - 1). When
	the function is called, this variable is 0. If the line is not wrapped, this value
	should be 0 after the call.

	@see WrapLine()
	@see m_anSubLines
	*/
	void WrapLineCached(int nLineIndex, int *anBreaks, int &nBreaks);

	/**
	Invalidates the cached data for the given lines.

	<b>Remarks:</b> Override this method, if your derived class caches other
	view specific line info, which becomes invalid, when this line changes.
	Call this standard implementation in your overriding.

	@param nLineIndex1 The index of the first line to invalidate.

	@param nLineIndex2 The index of the last line to invalidate. If this value is
	-1 (default) all lines from nLineIndex1 to the end are invalidated.
	*/
	virtual void InvalidateLineCache(int nLineIndex1, int nLineIndex2);
	virtual void InvalidateSubLineIndexCache(int nLineIndex1);
	void InvalidateScreenRect();
	//END SW

	//BEGIN SW
	// function to draw a single screen line
	// (a wrapped line can consist of many screen lines
	void DrawScreenLine(HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
		TextBlock::Array &pBuf, int &nActualItem,
		COLORREF crText, COLORREF crBkgnd,
		LPCTSTR pszChars,
		int nOffset, int nCount, int &nActualOffset, int nLineIndex);
	//END SW

	void MergeTextBlocks(TextBlock::Array &pBuf1, TextBlock::Array &pBuf2);
	virtual void GetAdditionalTextBlocks(int nLineIndex, TextBlock::Array &pBuf) = 0;

public:
	void GetHTMLLine(int nLineIndex, String &);
	void GetHTMLStyles(String &);
	void GetHTMLAttribute(int nColorIndex, int nBgColorIndex, COLORREF crText, COLORREF crBkgnd, String &);

	void GoToLine(int nLine, bool bRelative);
	void ParseLine(TextBlock::Cookie &cookie, int nLineIndex, TextBlock::Array &pBuf);
	static DWORD ScriptCookie(LPCTSTR lang, DWORD defval = COOKIE_PARSER);
	static TextBlock::ParseProc ScriptParseProc(DWORD dwCookie);
	void ParseLinePlain(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineUnknown(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineAsp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineBasic(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineBatch(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineC(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineCSharp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRazor(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineCss(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineDcl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineFortran(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineGo(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineIni(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineInnoSetup(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineIS(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineJava(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineLisp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineLua(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineNsis(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePascal(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePerl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePhp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePo(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePowerShell(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLinePython(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRexx(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRsrc(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRuby(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineRust(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSgml(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSh(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSiod(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineSql(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineTcl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineTex(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineVerilog(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineVhdl(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	void ParseLineXml(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf);
	// Attributes
public:
	bool GetViewTabs() const;
	void SetViewTabs(bool);
	void SetViewEols(bool bViewEols, bool bDistinguishEols);
	int GetTabSize() const;
	bool GetSeparateCombinedChars() const;
	void SetTabSize(int nTabSize, bool bSeparateCombinedChars);
	bool GetSelectionMargin() const;
	void SetSelectionMargin(bool);
	bool GetViewLineNumbers() const;
	void SetViewLineNumbers(bool);
	void GetFont(LOGFONT &) const;
	void SetFont(const LOGFONT &);
	DWORD GetFlags() const;
	void SetFlags(DWORD dwFlags);
	//  [JRT]:
	bool GetDisableDragAndDrop() const;
	void SetDisableDragAndDrop(bool);

	//BEGIN SW
	bool GetWordWrapping() const;
	void SetWordWrapping(bool);

	//END SW

	int m_nLastFindWhatLen;

	enum TextType
	{
		SRC_PLAIN,
		SRC_ASP,
		SRC_BASIC,
		SRC_VBSCRIPT,
		SRC_BATCH,
		SRC_C,
		SRC_CSHARP,
		SRC_CSHTML,
		SRC_CSS,
		SRC_DCL,
		SRC_FORTRAN,
		SRC_GO,
		SRC_HTML,
		SRC_INI,
		SRC_INNOSETUP,
		SRC_INSTALLSHIELD,
		SRC_JAVA,
		SRC_JSCRIPT,
		SRC_JSP,
		SRC_LISP,
		SRC_LUA,
		SRC_MWSL,
		SRC_NSIS,
		SRC_PASCAL,
		SRC_PERL,
		SRC_PHP,
		SRC_PO,
		SRC_POWERSHELL,
		SRC_PYTHON,
		SRC_REXX,
		SRC_RSRC,
		SRC_RUBY,
		SRC_RUST,
		SRC_SGML,
		SRC_SH,
		SRC_SIOD,
		SRC_SQL,
		SRC_TCL,
		SRC_TEX,
		SRC_VERILOG,
		SRC_VHDL,
		SRC_XML
	};

// Tabsize is commented out since we have only GUI setting for it now.
// Not removed because we may later want to have per-filetype settings again.
// See ccrystaltextview.cpp for per filetype table initialization.
	typedef struct tagTextDefinition
	{
		TextType type;
		LPCTSTR name;
		LPCTSTR exts;
		TextBlock::ParseProc ParseLineX;
		DWORD flags;
		LPCTSTR opencomment;
		LPCTSTR closecomment;
		LPCTSTR commentline;
	} const TextDefinition;

	static DWORD const SRCOPT_INSERTTABS	= 0x0001;
	static DWORD const SRCOPT_SHOWTABS		= 0x0002;
//	static DWORD const SRCOPT_BSATBOL		= 0x0004;
	static DWORD const SRCOPT_SELMARGIN		= 0x0008;
	static DWORD const SRCOPT_AUTOINDENT	= 0x0010;
	static DWORD const SRCOPT_BRACEANSI		= 0x0020;
	static DWORD const SRCOPT_BRACEGNU		= 0x0040;
	static DWORD const SRCOPT_EOLNDOS		= 0x0080;
	static DWORD const SRCOPT_EOLNUNIX		= 0x0100;
	static DWORD const SRCOPT_EOLNMAC		= 0x0200;
	static DWORD const SRCOPT_FNBRACE		= 0x0400;
//	static DWORD const SRCOPT_WORDWRAP		= 0x0800;
	static DWORD const SRCOPT_HTML4_LEXIS	= 0x1000;
	static DWORD const SRCOPT_HTML5_LEXIS	= 0x2000;

	//  Source type
	TextDefinition *m_CurSourceDef;
	static TextDefinition m_StaticSourceDefs[];
	static TextDefinition *m_SourceDefs;
	virtual TextDefinition *DoSetTextType(TextDefinition *);
	static TextDefinition *GetTextType(LPCTSTR pszExt);
	static TextDefinition *GetTextType(int index);
	static BOOL ScanParserAssociations(LPTSTR);
	static void DumpParserAssociations(LPTSTR);
	static void FreeParserAssociations();
	TextDefinition *SetTextType(LPCTSTR pszExt);
	TextDefinition *SetTextType(TextType enuType);
	TextDefinition *SetTextType(TextDefinition *);
	TextDefinition *SetTextTypeByContent(LPCTSTR pszContent);

	// Operations
public:
	void ReAttachToBuffer(CCrystalTextBuffer *);
	void AttachToBuffer(CCrystalTextBuffer *);
	void DetachFromBuffer();

	//  Buffer-view interaction, multiple views
	CCrystalTextBuffer *GetTextBuffer() { return m_pTextBuffer; }
	virtual void UpdateView(CCrystalTextView *pSource, CUpdateContext *pContext, DWORD dwFlags, int nLineIndex = -1);

	//  Attributes
	const POINT &GetCursorPos() const;
	void SetCursorPos(const POINT &ptCursorPos);
	void ShowCursor();
	void HideCursor();

	//  Operations
	void EnsureCursorVisible();
	void EnsureSelectionVisible();

	//  Text search helpers
	int FindText(LPCTSTR pszText, const POINT &ptStartPos,
		DWORD dwFlags, BOOL bWrapSearch, POINT &ptFoundPos,
		Captures &captures);
	int FindTextInBlock(LPCTSTR pszText, const POINT &ptStartPos,
		const POINT &ptBlockBegin, const POINT &ptBlockEnd,
		DWORD dwFlags, BOOL bWrapSearch, POINT &ptFoundPos,
		Captures &captures);
	BOOL HighlightText(const POINT & ptStartPos, int nLength, BOOL bCursorToLeft = FALSE);

	// IME (input method editor)
	void UpdateCompositionWindowPos();
	void UpdateCompositionWindowFont();

	//  Overridable: an opportunity for Auto-Indent, Smart-Indent etc.
	virtual void OnEditOperation(int nAction, LPCTSTR pszText, int cchText) = 0;

public:
	CCrystalTextView(size_t);
	~CCrystalTextView();
#ifdef _DEBUG
	void AssertValidTextPos(const POINT & pt);
#endif
	int GetScrollPos(UINT nBar, UINT nSBCode);

	void OnDraw(HSurface *);

	//  Keyboard handlers
	void MoveLeft(BOOL bSelect);
	void MoveRight(BOOL bSelect);
	void MoveWordLeft(BOOL bSelect);
	void MoveWordRight(BOOL bSelect);
	void MoveUp(BOOL bSelect);
	void MoveDown(BOOL bSelect);
	void MoveHome(BOOL bSelect);
	void MoveEnd(BOOL bSelect);
	void MovePgUp(BOOL bSelect);
	void MovePgDn(BOOL bSelect);
	void MoveCtrlHome(BOOL bSelect);
	void MoveCtrlEnd(BOOL bSelect);
	void OnSize();
	void OnVScroll(UINT nSBCode);
	BOOL OnSetCursor(UINT nHitTest);
	void OnLButtonDown(LPARAM);
	void OnSetFocus();
	void OnHScroll(UINT nSBCode);
	void OnLButtonUp(LPARAM);
	void OnMouseMove(LPARAM);
	void OnTimer(UINT_PTR nIDEvent);
	void OnKillFocus();
	void OnLButtonDblClk(LPARAM);
	void OnRButtonDown(LPARAM);
	void OnEditCopy();
	void OnEditFind();
	void OnEditRepeat();
	BOOL OnMouseWheel(WPARAM, LPARAM);
	void OnPageUp();
	void OnExtPageUp();
	void OnPageDown();
	void OnExtPageDown();
	void OnToggleBookmark(UINT nCmdID);
	void OnGoBookmark(UINT nCmdID);
	void OnClearBookmarks();

	void OnToggleBookmark(); // More bookmarks

	void OnClearAllBookmarks();
	void OnNextBookmark();
	void OnPrevBookmark();

	void ScrollUp();
	void ScrollDown();
	void ScrollLeft();
	void ScrollRight();

	void OnMatchBrace(BOOL bSelect);
	BOOL CanMatchBrace();

	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDestroy();
};

#ifdef _DEBUG
#define ASSERT_VALIDTEXTPOS(pt) AssertValidTextPos(pt);
#else
#define ASSERT_VALIDTEXTPOS(pt)
#endif
