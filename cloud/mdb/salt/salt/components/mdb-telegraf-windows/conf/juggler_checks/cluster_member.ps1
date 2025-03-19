param (
        $format
)

$result = Get-ClusterNode $ENV:COMPUTERNAME -ErrorAction SilentlyContinue|Where {$_.State -eq 'Up'}|Measure-Object|Select -exp Count
if ($result -ne 1) {
        switch ($format) {
                'influx' { echo 'cluster_membership,service_name=witness is_alive=false,is_master=false,can_read=false,can_write=false,instance_userfault_broken=false,replica_type="Quorum"'
}
                default { echo "2;node is not an active member of a cluster" }
        }
        exit 0
}
else {
        switch ($format) {
                'influx' { echo 'cluster_membership,service_name=witness is_alive=true,is_master=false,can_read=false,can_write=false,instance_userfault_broken=false,replica_type="Quorum"'
}
                default { echo "0;OK" }
        }
}

