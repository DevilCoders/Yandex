# importing all libraries from common directory ( gci "$PSScriptRoot\common" -fi *.ps1 | select -exp FullName | % { . $_ }; )
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose
function ensure-sqlserverbootstrapped { 
  Start-Service MSSQLSERVER
  Start-Sleep -Seconds 10
  & SQLCMD -Q "DECLARE @InternalInstanceName sysname;DECLARE @MachineInstanceName sysname;SELECT @InternalInstanceName = @@SERVERNAME, @MachineInstanceName = CAST(SERVERPROPERTY('MACHINENAME') AS VARCHAR(128))  + COALESCE('\' + CAST(SERVERPROPERTY('INSTANCENAME') AS VARCHAR(128)), '');IF @InternalInstanceName <> @MachineInstanceName BEGIN EXEC sp_dropserver @InternalInstanceName; EXEC sp_addserver @MachineInstanceName, 'LOCAL'; END"
  Restart-Service MSSQLSERVER -Force
  Restart-Service SQLSERVERAGENT
}

# runnable
if (-not (Test-DotSourced)) { ensure-sqlserverbootstrapped }
