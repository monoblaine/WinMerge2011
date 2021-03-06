/**
 * @file  PropCompare.h
 *
 * @brief Declaration of PropCompare propertysheet
 */

/**
 * @brief Property page to set compare options for WinMerge.
 *
 * Whitespace compare:
 *  - Compare all whitespaces, recommended for merging!
 *  - Ignore changes in whitespaces (amount of spaces etc)
 *  - Ignore all whitespace characters
 */
class PropCompare : public OptionsPanel
{
// Construction
public:
	PropCompare();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

// Dialog Data
	BOOL    m_bIgnoreEol;
	BOOL    m_bIgnoreCase;
	BOOL    m_bIgnoreBlankLines;
	BOOL    m_bIgnoreTabExpansion;
	BOOL    m_bIgnoreTrailingSpace;
	int     m_nIgnoreWhite;
	BOOL    m_bMinimal;
	BOOL    m_bSpeedLargeFiles;
	BOOL    m_bApplyHistoricCostLimit;
	BOOL    m_bMovedBlocks;
	BOOL    m_bMatchSimilarLines;
	int     m_nMatchSimilarLinesMax;
	BOOL    m_bFilterCommentsLines;
	int     m_nDiffAlgorithm;
	BOOL    m_bIndentHeuristic;

// Implementation
protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual BOOL OnInitDialog() override;
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnDefaults();
};
