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
 * @file  FileFiltersDlg.h
 *
 * @brief Declaration file for File Filters dialog
 */
#pragma once

/**
 * @brief Class for dialog allowing user to select
 * and edit used file filters
 */
class FileFiltersDlg : public ODialog
{
// Construction
public:
	FileFiltersDlg();   // standard constructor
	String m_strCaption;
	String m_sFileFilterPath;
// Implementation data
private:
	const std::vector<FileFilter *> &m_Filters;

	HListView *m_listFilters;

// Implementation methods
protected:
	void InitList();
	void SelectFilterByIndex(int index);
	void AddToGrid(int filterIndex);
	bool IsFilterItemNone(int item) const;
	void UpdateFiltersList();
	void EditFileFilter(LPCTSTR path);

	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	LRESULT OnNotify(NMHDR *);
	void OnApply();
	void OnFiltersEditbtn();
	void OnLvnItemchangedFilterfileList(NMHDR *);
	void OnBnClickedFilterfileTestButton();
	void OnBnClickedFilterfileNewbutton();
	void OnBnClickedFilterfileDelete();
	void OnBnClickedFilterfileInstall();
};
