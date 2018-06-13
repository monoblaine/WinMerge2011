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
 * @file  FileFilter.h
 *
 * @brief Declaration file for FileFilter
 */
#include "pcre.h"
#include "DirItem.h"
#include "RegExpItem.h"
#include "Common/MyCom.h"

/**
 * @brief One actual filter.
 *
 * For example, this might be a GNU C filter, excluding *.o files and CVS
 * directories. That is to say, a filter is a set of file masks and
 * directory masks. Usually FileFilter contains rules from one filter
 * definition file. So it can be thought as filter file contents.
 * fileinfo contains time of filter file's last modification time. By
 * comparing fileinfo to filter file in disk we can determine if file
 * is changed since we last time loaded it.
 * @sa FileFilterList
 */
struct FileFilter
	: ZeroInit<FileFilter>
{
	String name;			/**< Filter name (shown in UI) */
	String description;		/**< Filter description text */
	String fullpath;		/**< Full path to filter file */
	String sql;				/**< SQL query template for LogParser */
	BYTE sqlopt[2];			/**< Which sides to apply it to */
	std::vector<std::map<String, String> > params;	/**< SQL query parameters */
	std::vector<String> rawsql;					/**< Raw SQL clause as provided by user */
	std::vector<regexp_item> filefilters;		/**< List of inclusion rules for files */
	std::vector<regexp_item> dirfilters;		/**< List of inclusion rules for directories */
	std::vector<regexp_item> xfilefilters;		/**< List of exclusion rules for files */
	std::vector<regexp_item> xdirfilters;		/**< List of exclusion rules for directories */
	std::vector<regexp_item> fileprefilters;	/**< List of prefilter rules for files */
	std::vector<regexp_item> dirprefilters;		/**< List of prefilter rules for directories */
	FileFilter();
	~FileFilter();
	BSTR getSql(int side);
	BSTR composeSql(int side);
	// methods to actually use filter
	bool TestFileNameAgainstFilter(LPCTSTR szPath, LPCTSTR szFileName) const;
	bool TestDirNameAgainstFilter(LPCTSTR szPath, LPCTSTR szDirName) const;
	static stl_size_t ApplyPrefilterRegExps(const std::vector<regexp_item> &, char *dst, const char *src, stl_size_t len);
	bool Load();

protected:
	void Clear();
	virtual bool CreateFromMask() { return false; }

private:
	static int TestAgainstRegList(const std::vector<regexp_item> &, LPCTSTR, LPCTSTR);
	static void EmptyFilterList(std::vector<regexp_item> &);
};
