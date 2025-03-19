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
  & $PSScriptRoot\invoke-buildstep.ps1 -StepAction Ensure -StepSetting ngen
  & $PSScriptRoot\invoke-buildstep.ps1 -StepAction Ensure -StepSetting Cleanup
}

# run sysprep
if (-not $SkipSysprep) {
  if (Test-Path $global:__SETUP_SYSPREP_TAG) {
    "Found sysprep succeeded tag, removing it" | Out-InfoMessage
    rm $global:__SETUP_SYSPREP_TAG -Confirm:$false -ea:Stop
  }

  & $env:SystemRoot\System32\Sysprep\Sysprep.exe /oobe /generalize /quiet /quit /unattend:"$global:__SETUPCOMPLETE_DIR\sysprepunattend.xml"
  while ($true) {
    # several consecutive IMAGE_STATE_COMPLETE at early run
    # could possibly mean 'sysprep errored'
    # but 60s delay mitigate observed false positives
    Start-Sleep -s 5
    $ImageState = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\State" | `
      Select-Object -ExpandProperty ImageState    
    $ImageState | Out-InfoMessage
    if($ImageState -eq 'IMAGE_STATE_GENERALIZE_RESEAL_TO_OOBE') { break }
  }

  while ( -not (Test-Path $global:__SETUP_SYSPREP_TAG) ) {
    "Sysprep succeeded tag not yet exist, hold my beer and wait 5s..." | Out-InfoMessage
    Start-Sleep -s 5
  }

  # sometimes we catch errors @ undone sysprep even everything is correct, give it extra 10s
  "Shutdown in 60s..." | Out-InfoMessage
  Start-Sleep -Seconds 59
  & shutdown /s /t 1
  exit 0
}
