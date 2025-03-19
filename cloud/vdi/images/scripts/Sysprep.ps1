$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop

# Start sysprep
& $env:SystemRoot\System32\Sysprep\Sysprep.exe /oobe /generalize /quiet /quit /unattend:"C:\Windows\Setup\Scripts\SysprepUnattend.xml"

# Wait for correct system state
do {
    Start-Sleep -s 5

    $SetupState = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\State"
    $ImageState = $SetupState | Select-Object -ExpandProperty ImageState    
    $ImageState | Out-Default
} while ($ImageState -ne 'IMAGE_STATE_GENERALIZE_RESEAL_TO_OOBE')

# Wait for sysprep tag
while (-not (Test-Path 'C:\Windows\System32\Sysprep\Sysprep_succeeded.tag') ) {
    'Sysprep succeeded tag not yet exist...' 

    Start-Sleep -s 5
}

& shutdown /s /t 1
