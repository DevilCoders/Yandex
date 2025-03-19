param ($crit_limit, $warn_limit, $disk_name) 

$disk_us = Get-PSDrive $disk_name | Select Used, Free
$usage = 100 * [math]::round($disk_us.Used / ($disk_us.Free + $disk_us.Used), 3)

if ($usage -gt $crit_limit) { 
        echo "2;Disk usage is $usage"
        exit 0
} elseif ($usage -gt $warn_limit) {
        echo "1;Disk usage is $usage"
        exit 0
}

echo "0;OK"

