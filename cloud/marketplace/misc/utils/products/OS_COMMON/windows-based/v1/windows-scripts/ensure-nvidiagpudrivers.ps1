# importing all libraries from common directory ( gci "$PSScriptRoot\common" -fi *.ps1 | select -exp FullName | % { . $_ }; )
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose
function ensure-nvidiagpudrivers { 
  # add installation detection

  if (-not (Test-Path $__BOOTSTRAP_APPS_DIR__)) { 
    Write-Host "Creating $__BOOTSTRAP_APPS_DIR__"
    mkdir $__BOOTSTRAP_APPS_DIR__ | Out-Null
  }

  $APP_DIR = "$__BOOTSTRAP_APPS_DIR__\nvidia-drivers"
  if (-not (Test-Path $APP_DIR)) { 
    Write-Host "Creating $APP_DIR"
    mkdir "$APP_DIR" | Out-Null
  }

  # Download
  Write-Host "Downloading nvidia gpu drivers"
  $ProgressPreference = 'SilentlyContinue'
  Invoke-RestMethod `
    -Method Get `
    -Uri "https://us.download.nvidia.com/tesla/426.00/426.00-tesla-desktop-winserver-2019-2016-international.exe" `
    -OutFile "$APP_DIR\nvidia-gpu-driver.exe"
  
  # NOT WORKING
  # 'Unzip'
  Write-Host "Unpacking driver"
  Start-Process `
    -FilePath "$APP_DIR\nvidia-gpu-driver.exe" `
    -Args "-y -gm2 -InstallPath=`"$APP_DIR`" -nr" `
    -PassThru | `
      Wait-Process
  
  # ! WRONG PATH !
  # Installing
  Write-Host "Installing "
  Start-Process `
    -FilePath "$APP_DIR\setup.exe" `
    -Args "-clean -noreboot -noeula -passive -s" `
    -PassThru | `
      Wait-Process

  # Cleanup
  #rm $APP_DIR -Recurse -Force
}

# runnable
if (-not (Test-DotSourced)) { ensure-nvidiagpudrivers }
