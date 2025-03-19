# ToDo: 
#       * check config somehow
#         (at time being only after s3 keys and secret could be passed in test)


# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


function Ensure-Cloudbaseinit {
  "[ENFORCE]::Ensure Cloudbase-init" | Out-InfoMessage

  if (-not (Test-Path $__BOOTSTRAP_APPS_DIR)) {
    "[ENFORCE]::Creating $__BOOTSTRAP_APPS_DIR" | Out-InfoMessage
    mkdir $__BOOTSTRAP_APPS_DIR__ | Out-Null
  }

  # Download
  "[ENFORCE]::Downloading" | Out-InfoMessage
  Download-S3Object `
    -object "cloudbase-init.zip" `
    -bucket "win-distr" `
    -dir $__BOOTSTRAP_APPS_DIR

  # Extract
  "[ENFORCE]::Extracting" | Out-InfoMessage
  Expand-Archive `
    -Path $__BOOTSTRAP_APPS_DIR\cloudbase-init.zip `
    -DestinationPath $__BOOTSTRAP_APPS_DIR\cloudbase-init

  # Install
  "[ENFORCE]::Installing" | Out-InfoMessage
  $PATH_TO_MSI = "$__BOOTSTRAP_APPS_DIR\cloudbase-init\CloudbaseInit.msi"
  Start-Process -FilePath 'msiexec.exe' -ArgumentList "/i $PATH_TO_MSI /qn" -Wait

  # Copy configs
  "[ENFORCE]::Copying config" | Out-InfoMessage
  $cloudbaseinit_config_dir = "C:\Program Files\Cloudbase Solutions\Cloudbase-Init\conf\"
  $PATH_TO_CONFIG = "$__BOOTSTRAP_APPS_DIR\cloudbase-init\cloudbase-init-unattend.conf"
  # decided not to change current behaviour/error coz it changes nothing really
  # and 'soon' we'll move to Yandex.Agent
  Copy-Item "$PATH_TO_CONFIG" "$cloudbaseinit_config_dir\cloudbase-init.conf"
  Copy-Item "$PATH_TO_CONFIG" "$cloudbaseinit_config_dir\cloudbase-init-unattend.conf"

  return (Test-Cloudbaseinit)
}


function Test-Cloudbaseinit {
  $r = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | `
    ? DisplayName -like "*Cloudbase-init*") -ne $null

  "[TEST]::Cloudbase-init installed? $r" | Out-InfoMessage

  return $r
}
