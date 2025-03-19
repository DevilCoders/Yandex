Param (
[string]$action = ''
)

if ($action -match 'disk_resize') {
    # Variable specifying the drive you want to extend
    $drive_letter = "D"

    # Script to get the partition sizes and then resize the volume
    $size = (Get-PartitionSupportedSize -DriveLetter $drive_letter)
    Resize-Partition -DriveLetter $drive_letter -Size $size.SizeMax
}

Start-ClusterNode

$limit = (Get-Date).AddMinutes(5)
while ((Get-Date) -le $limit) {
    try {
        Get-Cluster
        break
    }
    catch {
        Start-Sleep -Seconds 1
    }
}
