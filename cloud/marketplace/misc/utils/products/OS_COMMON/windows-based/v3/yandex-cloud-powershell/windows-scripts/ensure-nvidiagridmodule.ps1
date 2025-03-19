# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


function Ensure-NvidiaGRIDModule {
  param(
    $name = "GridLicensing"
  )

  "[ENFORCE]::Ensure $name" | Out-InfoMessage
  if (-not (Test-Path $__BOOTSTRAP_APPS_DIR)) {
    mkdir $__BOOTSTRAP_APPS_DIR | Out-Null
  }

  # Download
  "[ENFORCE]::Downloading" | Out-InfoMessage
  Download-S3Object `
    -object "$($name).zip" `
    -bucket "win-distr" `
    -dir $__BOOTSTRAP_APPS_DIR
  
  "[ENFORCE]::Extracting to modules directory" | Out-InfoMessage
  Expand-Archive `
    -Path "$__BOOTSTRAP_APPS_DIR\$($name).zip" `
    -DestinationPath (Join-Path $ENV:ProgramFiles "WindowsPowerShell\Modules\")
  
  return (Test-NvidiaGRIDModule)
}


function Test-NvidiaGRIDModule {
  # this string is a test itself :)
  # packer will fall if import wont succeed
  Import-Module GridLicensing -ea:SilentlyContinue

  $r = (gcm "Set-GridLicensing" -ea:SilentlyContinue) -ne $null
  "[TEST]::GridLicensing module installed? $r" | Out-InfoMessage

  return $r
}
