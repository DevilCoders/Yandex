# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"

# fix after sysprep
$drinkMe = @"
DECLARE @InternalInstanceName sysname;

DECLARE @MachineInstanceName sysname;

SELECT 
  @InternalInstanceName = @@SERVERNAME, 
  @MachineInstanceName = CAST(SERVERPROPERTY('MACHINENAME') AS VARCHAR(128)) 
                         + COALESCE('\' + CAST(SERVERPROPERTY('INSTANCENAME') AS VARCHAR(128)), '');

IF @InternalInstanceName <> @MachineInstanceName
  BEGIN
    EXEC sp_dropserver @InternalInstanceName;
    EXEC sp_addserver @MachineInstanceName, 'LOCAL';
  END
"@

$eatMe = @"
DECLARE @sqlstmt varchar(200);

DECLARE @UserName varchar(200);

SET @UserName = CAST(SERVERPROPERTY('MACHINENAME') AS VARCHAR(128)) + '\Administrator';

SET @sqlstmt ='CREATE LOGIN [' + @UserName + '] FROM WINDOWS;';

EXEC (@sqlstmt);

EXEC sp_addsrvrolemember @loginame=@UserName, @rolename='sysadmin'
"@

# for test, silly but still ok at minimal
$testMe = @"
CREATE DATABASE MyDatabase;
GO

CREATE TABLE MyDatabase.dbo.test (
  test_id INT IDENTITY(1,1) PRIMARY KEY,
  test_name VARCHAR(30) NOT NULL
);
GO

INSERT INTO MyDatabase.dbo.test (test_name) 
VALUES ('one');
GO

SELECT * FROM MyDatabase.dbo.test;
GO
"@

# expose
function Ensure-SQLServerBootstrapped { 
  Start-Service MSSQLSERVER
  & SQLCMD -Q $drinkMe
  # might have depended services
  Restart-Service MSSQLSERVER -Force
  
  # Since we add BUILTIN\Administrators as 'sa' and it is well-known group
  # i think 'eatMe' is not needed anymore
  # but i'll leave it commented here, just in case...
  # & SQLCMD -Q $
  # Restart-Service MSSQLSERVER

  return (Test-SQLServerBootstrapped)
}

function Test-SQLServerBootstrapped {
  param(
    [switch]$IncludeFunctional
  )

  $n = ((& SQLCMD -Q "SELECT @@SERVERNAME;")[-3].Trim() -eq $ENV:COMPUTERNAME)
  "[TEST]::Name is correct? $n" | Out-InfoMessage

  if ($IncludeFunctional) {
    $r = (& SQLCMD -Q $testMe)[-3].Trim() -eq "1 one"
    "[TEST]::Created db and inserted a row? $r" | Out-InfoMessage

    return ($r -and $n)
  }

  return $n
}
