$CurrentMachineGUID = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Cryptography\' -Name MachineGuid | Select-Object -ExpandProperty MachineGuid

$PrivateKeys = Get-ChildItem -Path 'C:\ProgramData\Microsoft\Crypto\Keys'
foreach ($PrivateKey in $PrivateKeys) {
    $NameParts = $PrivateKey.Name -split "_"

    # naming conventions is [uniqueID]_[MachineGUID]
    $ID = $NameParts[0]
    $MachineGUID = $NameParts[1]

    if ($MachineGUID -ne $CurrentMachineGUID) {
        $NewName = "{0}_{1}" -f $ID, $CurrentMachineGUID

        "Rename: {0} into {1}" -f $PrivateKey.FullName, $NewName

        Rename-Item -Path $PrivateKey.FullName -NewName $NewName
    }
}
