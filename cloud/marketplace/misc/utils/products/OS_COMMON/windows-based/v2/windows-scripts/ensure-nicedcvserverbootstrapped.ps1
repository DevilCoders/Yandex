# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


function Ensure-NICEDCVServerBootstrapped {
  param(
    $name = "nice-dcv-server-x64-Release-2020.1-9012.msi",
    $installer_path = "msiexec.exe",
    $installer_args = "/i nice-dcv-server-x64-Release-2020.1-9012.msi /quiet /norestart"
  )

  "[ENFORCE]::Ensure $name" | Out-InfoMessage
  if (-not (Test-Path $__BOOTSTRAP_APPS_DIR)) { mkdir $__BOOTSTRAP_APPS_DIR | Out-Null }

  $APP_DIR = "$__BOOTSTRAP_APPS_DIR\$name"
  if (-not (Test-Path $APP_DIR)) { mkdir "$APP_DIR" | Out-Null }

  "[ENFORCE]::Downloading" | Out-InfoMessage
  $d = @{
    object = "$($name).zip"
    bucket = "win-distr"
    dir = $__BOOTSTRAP_APPS_DIR
  }
  Download-S3Object @d
  
  "[ENFORCE]::Extracting" | Out-InfoMessage
  $e = @{
    Path = "$__BOOTSTRAP_APPS_DIR\$($name).zip"
    DestinationPath = $APP_DIR
  }
  Expand-Archive @e
  
  # Copy distr
  "[ENFORCE]::Copying distr to c:\distr" | Out-InfoMessage
  mkdir C:\distr\nicedcvserver\ | Out-Null
  cp $APP_DIR\$name c:\distr\nicedcvserver\$name -Force

  # copy bootstrap
  "[ENFORCE]::Copying install script to $($global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR)" | Out-InfoMessage
  if (-not (Test-Path $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR\) ) {
    mkdir $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR | Out-Null
  }
  cp $PSScriptRoot\Ensure-nicedcvserver.ps1 $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR\
  
  return (Test-NICEDCVServerBootstrapped)
}


function Test-NICEDCVServerBootstrapped {
  param(
    $display_name = "NICE Desktop Cloud Visualization Server*"
  )

  $s = Test-Path $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR\Ensure-nicedcvserver.ps1
  "[TEST]::Script in correct place? $s" | Out-InfoMessage

  $d = (ls c:\distr\nicedcvserver\ -filter *.msi -ea:SilentlyContinue | select -exp fullName | select -f 1) -ne $null  
  "[TEST]::Distr folder contain *.msi? $d" | Out-InfoMessage

  return ($s -and $d)
}
