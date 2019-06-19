/**
 * @file  PropBackups.h
 *
 * @brief Declaration file for PropBackups propertyheet
 */

/**
 * @brief A class for Backup file options page.
 */
class PropBackups : public OptionsPanel
{
public:
	PropBackups();

// Implement IOptionsPanel
	virtual void ReadOptions() override;
	virtual void WriteOptions() override;
	virtual void UpdateScreen() override;

// Dialog Data

	/** @brief Backup file locations. */
	enum BACKUP_FOLDER
	{
		FOLDER_ORIGINAL,
		FOLDER_GLOBAL,
	};

	BOOL m_bCreateForFolderCmp;
	BOOL m_bCreateForFileCmp;
	String m_sGlobalFolder;
	BOOL m_bAppendBak;
	BOOL m_bAppendTime;
	int m_nBackupFolder;

protected:
	template<DDX_Operation>
			bool UpdateData();
	virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);
	void OnBnClickedBackupBrowse();
};
