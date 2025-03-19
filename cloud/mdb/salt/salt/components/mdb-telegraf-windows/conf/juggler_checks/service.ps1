param ($service_name)  

$status = Get-Service -ErrorAction SilentlyContinue $service_name | Select -Expand Status

if ($status -ne "Running") {
        echo "2;service is dead"
        exit 0
}

echo "0;OK"
