# ToDo:
#       * discover how can we detect sysprep
#       * detect dependency and copy only subset of required scripts?
#       * take out scripts needed for setupcomplete from here


param(
  [Switch]$PreserveSetupcompleteFolder,
  [Switch]$PreserveSetupcompleteSupplementaryFolder,
  [Switch]$SkipUserdataRemoval,
  [Switch]$SkipCleanup,
  [Switch]$SkipSysprep
)

. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"

# for setupcomplete, if there leftovers - better remove'em
if (Test-Path $global:__SETUPCOMPLETE_DIR) {
  if (-not $PreserveSetupcompleteFolder) {
    ls $global:__SETUPCOMPLETE_DIR -Recurse | rm -Confirm:$false -ea:SilentlyContinue
  }    
} else {
  mkdir $global:__SETUPCOMPLETE_DIR | out-null
}

# copy additional setupcomplete scripts if there any
if ( Test-Path $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR ) {
  cp $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR\* $global:__SETUPCOMPLETE_DIR\ -Recurse
  if (-not $PreserveSetupcompleteSupplementaryFolder) {
    rm $global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR -Confirm:$false -Recurse
  }
}

# copy setupcomplete scripts & sysprep
cp $global:__BOOTSTRAP_LIBS_COMMON_DIR                                         $global:__SETUPCOMPLETE_DIR\ -Recurse
cp "$global:__BOOTSTRAP_LIBS_DIR\invoke-buildstep.ps1"                         $global:__SETUPCOMPLETE_DIR\
cp "$global:__BOOTSTRAP_LIBS_DIR\ensure-winrm.ps1"                             $global:__SETUPCOMPLETE_DIR\
cp "$global:__BOOTSTRAP_LIBS_DIR\ensure-activaded.ps1"                         $global:__SETUPCOMPLETE_DIR\
cp "$global:__BOOTSTRAP_LIBS_DIR\ensure-adminpwdneverexp.ps1"                  $global:__SETUPCOMPLETE_DIR\
cp "$global:__BOOTSTRAP_LIBS_DIR\ensure-eth.ps1"                               $global:__SETUPCOMPLETE_DIR\
cp "$global:__BOOTSTRAP_LIBS_DIR\ensure-packertasksremoved.ps1"                $global:__SETUPCOMPLETE_DIR\
cp "$global:__BOOTSTRAP_LIBS_UNATTEND_DIR\sysprepunattend-cloudbase-init.xml" "$global:__SETUPCOMPLETE_DIR\sysprepunattend.xml"

# create setupcomplete.cmd and .ps1
cp "$global:__BOOTSTRAP_LIBS_DIR\unattend\Setupcomplete.cmd" $global:__SETUPCOMPLETE_DIR\
cp "$global:__BOOTSTRAP_LIBS_DIR\unattend\Setupcomplete.ps1" $global:__SETUPCOMPLETE_DIR\

# remove bootstrap & userdata invoker
if (-not $SkipUserdataRemoval) {
  Get-ScheduledTask -TaskName "userdata" -ea:SilentlyContinue | Unregister-ScheduledTask -Confirm:$false
}

if (-not $SkipCleanup) {
  & $PSScriptRoot\invoke-buildstep.ps1 -StepAction Ensure -StepSetting Cleanup
}

# run sysprep
if (-not $SkipSysprep) {
  & $env:SystemRoot\System32\Sysprep\Sysprep.exe /oobe /generalize /quiet /quit /unattend:"$global:__SETUPCOMPLETE_DIR\sysprepunattend.xml"
  while ($true) {
    # several consecutive IMAGE_STATE_COMPLETE at early run
    # could possibly mean 'sysprep errored'
    # but 60s delay mitigate observed false positives
    Start-Sleep -s 60
    $ImageState = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\State" | `
      Select-Object -ExpandProperty ImageState    
    Write-Output $ImageState
    if($ImageState -eq 'IMAGE_STATE_GENERALIZE_RESEAL_TO_OOBE') { break }
  }

  Stop-Computer -Force
}
