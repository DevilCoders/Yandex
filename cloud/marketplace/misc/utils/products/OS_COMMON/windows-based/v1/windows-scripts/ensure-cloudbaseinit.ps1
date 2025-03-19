# importing all libraries from common directory
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose methods

function ensure-cloudbaseinit {
  $isInstalled = Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | ? DisplayName -like "*Cloudbase-init*"
 
  if (-not $isInstalled) {
    # $__BOOTSTRAP_APPS_DIR__ defined @ common
    # test below should be in 'bootstrap'
    if (-not (Test-Path $__BOOTSTRAP_APPS_DIR__)) { 
      Write-Host "Creating $__BOOTSTRAP_APPS_DIR__"
      mkdir $__BOOTSTRAP_APPS_DIR__ | Out-Null
    }

    # Download
    Write-Host "Downloading cloudbase-init.zip"
    Download-S3Object `
      -object "cloudbase-init.zip" `
      -bucket "win-distr" `
      -dir $__BOOTSTRAP_APPS_DIR__

    # Extract
    Write-Host "Unzipping"
    Expand-Archive `
      -Path $__BOOTSTRAP_APPS_DIR__\cloudbase-init.zip `
      -DestinationPath $__BOOTSTRAP_APPS_DIR__\cloudbase-init

    # Install
    Write-Host "Installing cloudbase-init"
    $PATH_TO_MSI = "$__BOOTSTRAP_APPS_DIR__\cloudbase-init\CloudbaseInit.msi"
    Start-Process -FilePath 'msiexec.exe' -ArgumentList "/i $PATH_TO_MSI /qn" -Wait

    # Copy configs
    Write-Host "Copying config"
    $cloudbaseinit_config_dir = "C:\Program Files\Cloudbase Solutions\Cloudbase-Init\conf\"
    $PATH_TO_CONFIG = "$__BOOTSTRAP_APPS_DIR__\cloudbase-init\cloudbase-init-unattend.conf"
    # dicided not to change current behaviour/error coz it changes nothing really
    Copy-Item "$PATH_TO_CONFIG" "$cloudbaseinit_config_dir\cloudbase-init.conf"
    Copy-Item "$PATH_TO_CONFIG" "$cloudbaseinit_config_dir\cloudbase-init-unattend.conf"

    # Clean
    # not needed - sealing will clean it up
    #Write-Host "Removing setup files"
    #Remove-Item -Path $__BOOTSTRAP_APPS_DIR__\cloudbase-init -Recurse
    #Remove-Item -Path $__BOOTSTRAP_APPS_DIR__\cloudbase-init.zip
  }
}

# make it 'in-place' runnable

if (-not (Test-DotSourced)) { ensure-cloudbaseinit }
