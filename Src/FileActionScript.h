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
 * @file  FileActionScript.h
 *
 * @brief Declaration file for FileActionScript and related classes
 */
#pragma once

/**
 * @brief FileAction presents one filesystem action we want to do.
 *
 * Action consists of source, destination and action type. This
 * allows us to combine lots of different kind of actions into
 * one list.
 **/
struct FileAction
{
	/**
	 * @brief the type of the action.
	 * These action types are low level actions for filesystem, not
	 * higher level actions user is doing (e.g. synchronizing).
	 */
	enum ACT_TYPE
	{
		ACT_COPY,     /**< Copy the item(s). */
		ACT_MOVE,     /**< Move the item(s). */
		ACT_DEL,      /**< Delete the item(s). */
		ACT_TYPE
	} atype; /**< Action's type */
	bool dirflag; /**< Is it directory? (TRUE means directory) */
	String src; /**< Source for action */
	String dest; /**< Destination action */
};

/**
 * @brief FileActionItem presents one filesystem action from GUI perspective.
 */
struct FileActionItem : public FileAction
{
	/**
	 * @brief UI result of the action done.
	 * These values present the change in the UI happening, due to lower
	 * level actions. E.g. delete operation may cause left item to be removed
	 * from the list.
	 */
	enum UI_RESULT
	{
		UI_SYNC = 1,   /**< Make items identical (synchronized). */
		UI_DESYNC,     /**< Make items different. */
		UI_DEL_LEFT,   /**< Remove left item. */
		UI_DEL_RIGHT,  /**< Remove right item. */
		UI_DEL_BOTH,   /**< Remove both items (removes the row). */
		UI_DONT_CARE,  /**< Ignore the GUI change. */
	};

	/**
	 * @brief Side of the action.
	 * This lists possible values for origin and destination sides.
	 */
	enum UI_SIDE
	{
		UI_LEFT,
		UI_RIGHT
	};

	/**
	 * Optional context value for the item.
	 * This is an arbitrary value that can be used to associate the item with
	 * other items. This can be e.g. indext of the item in the GUI.
	 */
	int context;
	UI_RESULT UIResult; /**< Resulting UI action */
	UI_SIDE UIOrigin; /**< Original UI-side */
	UI_SIDE UIDestination; /**< Destination UI-side */
};

/**
 * @brief FileActionScript holds list of fileactions and runs those actions.
 *
 * This class holds list of actions we want to make with filesystem. After
 * whole list of actions (script) is composed we can run this sript with
 * one command.
 */
class FileActionScript
{
public:
	FileActionScript(LPCTSTR destBase = NULL);
	~FileActionScript();

	bool ConfirmCopy(
		int origin, int destination,
		LPCTSTR src, LPCTSTR dest);

	bool ConfirmMove(
		int origin, int destination,
		LPCTSTR src, LPCTSTR dest);

	bool ConfirmDialog(
		UINT caption, UINT question,
		int origin, int destination,
		LPCTSTR src, LPCTSTR dest);

	bool Run(HListView *, DWORD flags);

	// Manipulate the FileActionList
	int GetActionItemCount() const { return m_actions.size(); }

	/**
	 * Add new item to the action list.
	 * @param [in] item Item to add to the list.
	 */
	void AddActionItem(const FileActionItem &item) { m_actions.push_back(item); }

	FileActionItem RemoveTailActionItem();

	/**
	 * Get first action item in the list.
	 * @return First item in the list.
	 */
	FileActionItem GetHeadActionItem() const { return m_actions[0]; }

	String const m_destBase; /**< Base destination path for some operations */
	bool m_bMakeTargetItemWritable; /**< Whether to make copy/move target items writable */
	bool m_bIgnoreFolderStructure; /**< Whether to ignore the folder structure */

private:
	std::vector<FileActionItem> m_actions; /**< List of all actions for this script. */
};
