# importing all libraries from common directory ( gci "$PSScriptRoot\common" -fi *.ps1 | select -exp FullName | % { . $_ }; )
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose
function ensure-sqlserverstd ($sql_version) {
  # Download
  mkdir 'C:\BOOTSTRAP\APPS' | out-null

  Write-Host "Downloading $sql_version.zip"
  Download-S3Object `
    -object "$($sql_version).zip" `
    -bucket "win-distr" `
    -dir $__BOOTSTRAP_APPS_DIR__

  # Extract
  Write-Host "Unzipping"
  Expand-Archive `
    -Path "$__BOOTSTRAP_APPS_DIR__\$($sql_version).zip" `
    -DestinationPath $__BOOTSTRAP_APPS_DIR__\sqlserver

  # Install
  Write-Host "Installing sqlserver"
  & $__BOOTSTRAP_APPS_DIR__\sqlserver\$sql_version\setup.exe `
    /QUIET `
    /INDICATEPROGRESS `
    /IACCEPTSQLSERVERLICENSETERMS `
    /ACTION=INSTALL `
    /FEATURES=SQLENGINE `
    /INSTANCENAME=MSSQLSERVER `
    /UpdateEnabled=TRUE `
    /SQLSYSADMINACCOUNTS="BUILTIN\Administrators"

  # Copy distr
  Write-Host "Deleting license file and copying distr to c:\distr"
  rm $__BOOTSTRAP_APPS_DIR__\sqlserver\$sql_version\x64\DefaultSetup.ini
  cp $__BOOTSTRAP_APPS_DIR__\sqlserver\$sql_version c:\distr\sqlserver -Recurse -Force

  # Clean
  rm $__BOOTSTRAP_APPS_DIR__\sqlserver -Recurse -Force
}

# runnable
if (-not (Test-DotSourced)) { ensure-sqlserverstd $args[0] }
