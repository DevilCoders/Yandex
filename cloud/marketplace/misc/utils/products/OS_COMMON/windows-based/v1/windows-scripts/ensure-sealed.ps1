# importing all libraries from common directory ( gci "$PSScriptRoot\common" -fi *.ps1 | select -exp FullName | % { . $_ }; )
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose
function ensure-sealed {
  $SETUPCOMPLETE_FOLDER   = "C:\Windows\Setup\Scripts"
  $BOOTSTRAP_FOLDER       = "C:\bootstrap"
  $WINDOWS_SCRIPTS_FOLDER = "$BOOTSTRAP_FOLDER\windows-scripts"
  $UNATTEND_FOLDER        = "$WINDOWS_SCRIPTS_FOLDER\unattend"

  # for setupcomplete
  if (Test-Path $SETUPCOMPLETE_FOLDER) { rm $SETUPCOMPLETE_FOLDER -Confirm:$false -Recurse }
  mkdir $SETUPCOMPLETE_FOLDER | out-null

  # copy setupcomplete scripts & sysprep
  cp "$WINDOWS_SCRIPTS_FOLDER\common\"                       "$SETUPCOMPLETE_FOLDER\" -Recurse
  cp "$WINDOWS_SCRIPTS_FOLDER\ensure-winrm.ps1"              "$SETUPCOMPLETE_FOLDER\ensure-winrm.ps1"
  cp "$WINDOWS_SCRIPTS_FOLDER\ensure-activaded.ps1"          "$SETUPCOMPLETE_FOLDER\ensure-activaded.ps1"
  cp "$WINDOWS_SCRIPTS_FOLDER\ensure-adminpwdneverexp.ps1"   "$SETUPCOMPLETE_FOLDER\ensure-adminpwdneverexp.ps1"
  cp "$WINDOWS_SCRIPTS_FOLDER\ensure-eth.ps1"                "$SETUPCOMPLETE_FOLDER\ensure-eth.ps1"
  cp "$WINDOWS_SCRIPTS_FOLDER\ensure-packertasksremoved.ps1" "$SETUPCOMPLETE_FOLDER\ensure-packertasksremoved.ps1"
  cp "$UNATTEND_FOLDER\sysprepunattend-cloudbase-init.xml"   "$SETUPCOMPLETE_FOLDER\sysprepunattend.xml"

  # create setupcomplete.cmd and .ps1
  cp "$WINDOWS_SCRIPTS_FOLDER\unattend\Setupcomplete.cmd" "$SETUPCOMPLETE_FOLDER\Setupcomplete.cmd"
  cp "$WINDOWS_SCRIPTS_FOLDER\unattend\Setupcomplete.ps1" "$SETUPCOMPLETE_FOLDER\Setupcomplete.ps1"

  # remove bootstrap & userdata invoker
  #if (Test-Path $BOOTSTRAP_FOLDER) { rm $BOOTSTRAP_FOLDER -Confirm:$false -Recurse }
  Get-ScheduledTask -TaskName "userdata" | Unregister-ScheduledTask -Confirm:$false
  & "$PSScriptRoot\ensure-cleanup.ps1"

  # run sysprep
  & $env:SystemRoot\System32\Sysprep\Sysprep.exe /oobe /generalize /quiet /quit /unattend:"$SETUPCOMPLETE_FOLDER\sysprepunattend.xml"
  while ($true) {
    # ToDo: several consecutive IMAGE_STATE_COMPLETE at early run could possibly mean 'sysprep errored'
    Start-Sleep -s 60
    $ImageState = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\State" | `
      Select-Object -ExpandProperty ImageState    
    Write-Output $ImageState
    if($ImageState -eq 'IMAGE_STATE_GENERALIZE_RESEAL_TO_OOBE') { break }
  }

  Stop-Computer -Force
}

# runnable
if (-not (Test-DotSourced)) { ensure-sealed }
