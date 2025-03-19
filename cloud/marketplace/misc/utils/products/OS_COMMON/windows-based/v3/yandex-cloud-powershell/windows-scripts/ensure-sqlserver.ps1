# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


function Ensure-SQLServer {
  param(
    [Parameter(Mandatory=$true)]
    [Alias("Version")]
    [ValidateSet(
      "sql-2019-enterprise",
      "sql-2019-standard",
      "sql-2019-web",
      "sql-2017-ent",
      "sql-2017-web",
      "sql-2016-enterprise",
      "sql-2016-standard",
      "sql-2016-web")]
    [String]$sql_version
  )

  "[ENFORCE]::Ensure $sql_version" | Out-InfoMessage

  if (-not (Test-Path $__BOOTSTRAP_APPS_DIR)) { 
    "[ENFORCE]::Creating $__BOOTSTRAP_APPS_DIR" | Out-InfoMessage
    mkdir $__BOOTSTRAP_APPS_DIR | Out-Null
  }

  "[ENFORCE]::Downloading $sql_version.zip" | Out-InfoMessage
  
  Download-S3Object `
    -object "$($sql_version).zip" `
    -bucket "win-distr" `
    -dir $__BOOTSTRAP_APPS_DIR

  # Extract
  "[ENFORCE]::Unzipping" | Out-InfoMessage
  Expand-Archive `
    -Path "$__BOOTSTRAP_APPS_DIR\$($sql_version).zip" `
    -DestinationPath $__BOOTSTRAP_APPS_DIR\sqlserver

  # Install
  "[ENFORCE]::Installing $sql_version" | Out-InfoMessage
  $l = & $__BOOTSTRAP_APPS_DIR\sqlserver\$sql_version\setup.exe `
    /QUIET `
    /INDICATEPROGRESS `
    /IACCEPTSQLSERVERLICENSETERMS `
    /ACTION=INSTALL `
    /FEATURES=SQLENGINE `
    /INSTANCENAME=MSSQLSERVER `
    /UpdateEnabled=TRUE `
    /SQLSYSADMINACCOUNTS="BUILTIN\Administrators"
  $r = -not [string]::IsNullOrEmpty( ($l -match 'Setup result: 0') )
  "[ENFORCE]::Setup result code 0? $r" | Out-InfoMessage

  if (-not $r) {
    throw "Setup result code not equal zero"
  }

  # Copy distr
  "[ENFORCE]::Deleting license file and copying distr to c:\distr" | Out-InfoMessage
  rm $__BOOTSTRAP_APPS_DIR\sqlserver\$sql_version\x64\DefaultSetup.ini
  cp $__BOOTSTRAP_APPS_DIR\sqlserver\$sql_version c:\distr\sqlserver -Recurse -Force

  # copy bootstrap
  "[ENFORCE]::Copying bootstrap script to $($global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR)" | Out-InfoMessage
  if (-not (Test-Path $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR\) ) {
    mkdir $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR | Out-Null
  }
  cp $PSScriptRoot\Ensure-SQLServerbootstrapped.ps1 $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR\

  # firewall
  $Rule = @{
    DisplayName = "Allow inbound TCP Port 1433 (SQLServer)"
    Direction = "inbound"
    LocalPort = 1433
    Protocol = "TCP"
    Action = "Allow"
  }
  New-NetFirewallRule @Rule | Out-Null

  return (Test-SQLServer)
}


function Test-SQLServer {
  $r = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* -ea:SilentlyContinue | `
    ? DisplayName -Like "SQL Server * Database Engine Services" | `
    Select-Object -First 1) -ne $null  
  "[TEST]::SQLServer installed? $r" | Out-InfoMessage

  # disabled until find a right way to pass desired tests
  # coz'em vary before and after sysprep
  #$p = Test-Path $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR\Ensure-SQLServerbootstrapped.ps1
  #"[TEST]::SQLServer bootstrap script copied? $p" | Out-InfoMessage

  $l = -not (Test-Path c:\distr\sqlserver\x64\DefaultSetup.ini)
  "[TEST]::License is absent at c:\distr\sqlserver...? $l" | Out-InfoMessage

  $f = (Get-NetFirewallRule -DisplayName "Allow inbound TCP Port 1433 (SQLServer)" -ea:SilentlyContinue).Enabled -eq "True"
  "[TEST]::Port 1433 open? $f" | Out-InfoMessage

  return ($r -and $f -and $l)
}
