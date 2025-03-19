Param (
[string]$disk_type_id
)

if ($disk_type_id -eq 'local-ssd') {
    if (Get-StoragePool -FriendlyName 'NVME_storagepool') {
        $Disk = Get-Disk -FriendlyName Data
        $Disk | set-disk -IsOffline 0
        $Disk | Set-Disk -IsReadOnly 0
        Get-Partition -DiskNumber $disk.Number|where Type -eq Basic|set-partition -NewDriveLetter D
    }
    else{
        $nvmedisks = Get-PhysicalDisk | Where-Object{$_.SerialNumber -like "*nvme-disk*" -and $_.CanPool -eq $True}
        if ($null -ne $nvmedisks) {
            $StoragePool = New-StoragePool -PhysicalDisks $nvmedisks -StorageSubSystemFriendlyName "Windows*" -FriendlyName "NVME_storagepool"
            $StoragePool|Get-PhysicalDisk|Set-PhysicalDisk -MediaType SSD
            $VirtualDisk = New-VirtualDisk -StoragePoolFriendlyName "NVME_storagepool" -FriendlyName "DATA" -ResiliencySettingName Simple -ProvisioningType Fixed -UseMaximumSize
            $Disk = $VirtualDisk|Get-Disk
            $Disk | Set-Disk -IsReadOnly 0
            $Disk | Set-Disk -IsOffline 0
            $Disk | Initialize-Disk -PartitionStyle GPT
            $Disk | New-Partition -DriveLetter "D" -UseMaximumSize
            Initialize-Volume -DriveLetter "D" -FileSystem NTFS -Confirm:$false -NewFileSystemLabel "DATA" -AllocationUnitSize 64KB
        }
        else {
            throw "No suitable NVMe disks found"
        }
    }
}
else {
    $systemDisk = Get-Partition | Where-Object { $_.DriveLetter -eq "C" } 
    $dataDisk = Get-Disk | Where-Object { $_.Number -ne $systemDisk.DiskNumber }
    if ($null -eq $dataDisk) {
        throw "DataDisk not found"
    }
    if ($dataDisk.IsOffline) {
        Set-Disk -IsOffline $false -Number $dataDisk.Number
        Set-Disk -IsReadOnly $false -Number $dataDisk.Number
    }
    $dataVolume = Get-Volume | Where-Object { $_.FileSystemLabel -eq "DATA" -and $_.DriveLetter -eq "D" }
    if ($null -eq $dataVolume) {
        $dataVolume = New-Volume -DiskNumber $dataDisk.Number -FileSystem NTFS -DriveLetter "D" -FriendlyName "DATA" -AllocationUnitSize 64KB
    }
}
