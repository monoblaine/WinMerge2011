/** 
 * @file DiffFileData.h
 *
 * @brief Declaration for DiffFileData class.
 *
 * @date  Created: 2003-08-22
 */
#pragma once

#include "Diff.h"

class CDiffWrapper;

/**
 * @brief C++ container for the structure (file_data) used by diffutils' diff_2_files(...)
 */
struct DiffFileData : comparison
{
// instance interface

	explicit DiffFileData(int codepage);
	~DiffFileData();

	bool OpenFiles(LPCTSTR szFilepath1, LPCTSTR szFilepath2);
	HANDLE GetFileHandle(int i);
	void *AllocBuffer(int i, size_t len, size_t alloc_extra);
	void Reset();
	void Close() { Reset(); }
	void SetDisplayFilepaths(LPCTSTR szTrueFilepath1, LPCTSTR szTrueFilepath2);

// Data (public)
	CDiffWrapper *m_pDiffWrapper;
	bool m_used; // whether m_inf has real data
	std::vector<FileLocation> m_FileLocation;
	std::vector<FileTextStats> m_textStats;

	std::vector<String> m_sDisplayFilepath;

private:
	bool DoOpenFiles();
	DiffFileData(const DiffFileData &); // disallow copy construction
	void operator=(const DiffFileData &); // disallow assignment
};
