# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


function Ensure-NvidiaGRIDDrivers {
  param(
    $name = "grid-v442.06",
    $installer_path = "\setup.exe",
    $installer_args = "-s -noreboot -clean -noeula"
  )

  "[ENFORCE]::Ensure $name" | Out-InfoMessage
  if (-not (Test-Path $__BOOTSTRAP_APPS_DIR)) {
    mkdir $__BOOTSTRAP_APPS_DIR | Out-Null
  }

  $APP_DIR = "$__BOOTSTRAP_APPS_DIR\$name"
  if (-not (Test-Path $APP_DIR)) {
    mkdir "$APP_DIR" | Out-Null
  }

  # Download
  "[ENFORCE]::Downloading" | Out-InfoMessage
  Download-S3Object `
    -object "$($name).zip" `
    -bucket "win-distr" `
    -dir $__BOOTSTRAP_APPS_DIR
  
  "[ENFORCE]::Extracting" | Out-InfoMessage
  Expand-Archive `
    -Path "$__BOOTSTRAP_APPS_DIR\$($name).zip" `
    -DestinationPath $APP_DIR
  
  "[ENFORCE]::Installing" | Out-InfoMessage
  Start-Process `
    -FilePath (Join-Path $APP_DIR $installer_path) `
    -Args $installer_args `
    -PassThru | `
      Wait-Process
  
  return (Test-NvidiaGRIDDrivers)
}


function Test-NvidiaGRIDDrivers {
  param(
    $display_name = "NVIDIA Graphics Driver"
  )

  $r = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | `
    ? DisplayName -like "*$display_name*") -ne $null

  "[TEST]::$display_name installed? $r" | Out-InfoMessage

  return $r
}
