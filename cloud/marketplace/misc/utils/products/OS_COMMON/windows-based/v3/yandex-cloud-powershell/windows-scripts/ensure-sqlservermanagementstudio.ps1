# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


# expose
function Ensure-SQLServerManagementStudio {
  $App = "SSMS-Setup-ENU-18.4"
  "[ENFORCE]::Ensure $App" | Out-InfoMessage

  if (-not (Test-Path $__BOOTSTRAP_APPS_DIR)) { 
    "[ENFORCE]::Creating $__BOOTSTRAP_APPS_DIR" | Out-InfoMessage
    mkdir $__BOOTSTRAP_APPS_DIR | Out-Null
  }

  "[ENFORCE]::Downloading $App.exe" | Out-InfoMessage
  Download-S3Object `
    -object "$($App).exe" `
    -bucket "win-distr" `
    -dir $__BOOTSTRAP_APPS_DIR

  # Install
  "[ENFORCE]::Installing $App" | Out-InfoMessage
  Start-Process $__BOOTSTRAP_APPS_DIR\$($App).exe `
    -ArgumentList @(
      "/install", 
      "/quiet", 
      "/passive", 
      "/norestart") -PassThru | `
        Wait-Process

  return (Test-SQLServerManagementStudio)
}


function Test-SQLServerManagementStudio {
  $r = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* -ea:SilentlyContinue | `
    ? DisplayName -eq "SQL Server Management Studio" | `
    Select-Object -First 1) -ne $null  
  "[TEST]::SQLServerManagementStudio installed? $r" | Out-InfoMessage
  
  return $r
}
