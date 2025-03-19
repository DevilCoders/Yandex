# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


function Ensure-NvidiaCUDAPackage {
  param(
    $name = "nvidia_cuda_11.0.3_451.82",
    $installer_path = "\nvidia_cuda_11.0.3_451.82\setup.exe",
    $installer_args = "-s"
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
  
  "[ENFORCE]::Installing" | Out-InfoMessage
  $i = @{
    FilePath = (Join-Path $APP_DIR $installer_path)
    Args = $installer_args
  }
  Start-Process -PassThru @i | Wait-Process
  
  return (Test-NvidiaCUDAPackage -inflight)
}


function Test-NvidiaCUDAPackage {
  param(
    $display_name = "NVIDIA CUDA Runtime",
    [switch]$inflight
  )

  $cuda = 'NVIDIA CUDA Runtime'
  $r = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | 
    ? DisplayName -like "*$cuda*" | Select -First 1) -ne $null
  "[TEST]::$display_name installed? $r" | Out-InfoMessage

  $driver = 'NVIDIA Graphics Driver'
  $d = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | 
  ? DisplayName -like "*$driver*" | Select -First 1) -ne $null
  "[TEST]::$driver installed? $d" | Out-InfoMessage
  
  # coz we test on fabrique without GPU code below is useless
  # but we still test it during build process
  if ($inflight) {
    Start-Sleep -Seconds 60

    # if server got 2 cards attached, one pass another fails...
    $__PASSED = 'Result = PASS'
    $__FAILED = 'Result = FAILED'

    $deviceQuery = ls -Filter deviceQuery.exe -Path c:\ -Recurse -ea:SilentlyContinue | select -exp fullname | ? {$_ -like "*extras*"} | select -first 1
    if (-not $deviceQuery ) {
      "[TEST]::deviceQuery.exe not found!" | Out-InfoMessage
      exit 1
    }
    $dqTestResult = (& "$deviceQuery")
    $dq = ( $dqTestResult -contains $__PASSED ) -and ( $dqTestResult -notcontains $__FAILED )
    "[TEST]::Device query test passed? $dq" | Out-InfoMessage

    $bandwidthTest = ls -Filter bandwidthTest.exe -Path c:\ -Recurse -ea:SilentlyContinue | select -exp fullname | ? {$_ -like "*extras*"} | select -first 1
    if (-not $bandwidthTest ) {
      "[TEST]::bandwidthTest.exe not found!" | Out-InfoMessage
      exit 1
    }
    $bwTestResult = & "$bandwidthTest"
    $bw = ( $bwTestResult -contains $__PASSED ) -and ( $bwTestResult -notcontains $__FAILED )
    "[TEST]::Bandwidth test passed? $bw" | Out-InfoMessage

    # if it failed - we must fail whole build
    # l8r we'll refactor another script that will 'exit 1' for every ruined test during build
    if ( -not ($dq -and $bw) ) { exit 1 }

    return ($r -and $d -and $dq -and $bw)
  }

  return ($r -and $d)
}
