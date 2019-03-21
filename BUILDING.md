# Building

1. Download Windows 7.1 SDK: https://www.microsoft.com/en-us/download/details.aspx?id=8279
2. Temporarily change the following values to `4.0.30319`:
   ```
   HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\NET Framework Setup\NDP\v4\Full\Version
   HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\NET Framework Setup\NDP\v4\Client\Version
   ```
3. Install Windows 7.1 SDK
4. Install Visual Studio 2010 C++ Express (https://my.visualstudio.com/Downloads?q=visual%20studio%202010&wt.mc_id=o~msft~vscom~older-downloads)

# Running `setup.hta` on a x64 system

1. Run cmd
2. Execute the following command:
   ```
   START %windir%\system32\mshta.exe "path_to_hta\setup.hta"
   ```
