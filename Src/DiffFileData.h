/** 
 * @file DiffFileData.h
 *
 * @brief Declaration for DiffFileData class.
 *
 * @date  Created: 2003-08-22
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _DIFFFILEDATA_H_
#define _DIFFFILEDATA_H_

// forward declarations needed by DiffFileData
struct file_data;
class PrediffingInfo;
class CDiffContext;

/**
 * @brief C++ container for the structure (file_data) used by diffutils' diff_2_files(...)
 */
struct DiffFileData
{
// class interface

	static void SetDefaultCodepage(int defcp); // set codepage to assume for all unknown files


// instance interface

	DiffFileData();
	~DiffFileData();

	bool OpenFiles(LPCTSTR szFilepath1, LPCTSTR szFilepath2);
	HANDLE GetFileHandle(int i);
	void *AllocBuffer(int i, size_t len);
	void Reset();
	void Close() { Reset(); }
	void SetDisplayFilepaths(LPCTSTR szTrueFilepath1, LPCTSTR szTrueFilepath2);

// Data (public)
	file_data *const m_inf;
	bool m_used; // whether m_inf has real data
	stl::vector<FileLocation> m_FileLocation;
	stl::vector<FileTextStats> m_textStats;

	stl::vector<String> m_sDisplayFilepath;

private:
	bool DoOpenFiles();
};

#endif // _DIFFFILEDATA_H_
