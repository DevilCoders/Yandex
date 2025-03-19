Powershell.exe -ExecutionPolicy Bypass -File "C:\Windows\Setup\Scripts\Setupcomplete.ps1"
rmdir "C:\Windows\Setup\Scripts\COMMON\" /Q /S
del "C:\Windows\Setup\Scripts\" /F /S /Q
