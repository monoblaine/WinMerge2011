/**
 * @file  RegKey.cpp
 *
 * @brief Declaration of CRegKeyEx C++ wrapper class for reading Windows registry
 */

class CSettingStore;

/**
 * @brief Class for reading/writing registry.
 */
class CRegKeyEx
{
// Construction
public:
	CRegKeyEx();
	CRegKeyEx(HKEY);
	~CRegKeyEx();

// Operations
public:
	operator HKEY() const { return m_hKey; }
	void Close();
	LONG Open(HKEY hKeyRoot, LPCTSTR pszPath);
	LONG OpenWithAccess(HKEY hKeyRoot, LPCTSTR pszPath, REGSAM regsam);
	LONG OpenNoCreateWithAccess(HKEY hKeyRoot, LPCTSTR pszPath, REGSAM regsam);
	LONG QueryRegMachine(LPCTSTR key);
	LONG QueryRegUser(LPCTSTR key);

	LONG WriteDword(LPCTSTR pszKey, DWORD dwVal) const;
	LONG WriteString(LPCTSTR pszKey, LPCTSTR pszVal) const;

	DWORD ReadDword(LPCTSTR pszKey, DWORD defval) const;
	template<size_t cb>
	struct Blob : BLOB
	{
		BYTE buf[cb];
		Blob()
		{
			cbSize = cb;
			pBlobData = buf;
		}
		operator BLOB *()
		{
			return this;
		}
	};
	typedef Blob<MAX_PATH * sizeof(TCHAR)> MaxPath;
	struct StringRef : BLOB
	{
		StringRef(LPTSTR val, UINT len)
		{
			cbSize = len * sizeof *val;
			pBlobData = reinterpret_cast<BYTE *>(val);
		}
		operator BLOB *()
		{
			return this;
		}
	};
	LPCTSTR ReadString(LPCTSTR pszKey, LPCTSTR defval, BLOB * = MaxPath()) const;

	LONG DeleteValue(LPCTSTR pszKey) const;

protected:
	const CSettingStore &m_store;
	HKEY m_hKey; /**< Open key (HKLM, HKCU, etc). */
};
