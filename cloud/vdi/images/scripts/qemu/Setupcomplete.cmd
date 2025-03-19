Powershell.exe -ExecutionPolicy Bypass -File "C:\Windows\Setup\Scripts\SetupComplete.ps1"
del "C:\Windows\Setup\Scripts\" /F /S /Q
shutdown /s /t 0
