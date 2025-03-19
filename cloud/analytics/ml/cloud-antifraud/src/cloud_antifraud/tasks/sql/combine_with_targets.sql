USE hahn;

$result_table = "//home/cloud_analytics/tmp/antifraud/combine_with_targets";
$compute_features = "//home/cloud_analytics/tmp/antifraud/compute_features";

$acq="//home/cloud_analytics/cubes/acquisition_cube/cube";
$cloud_is_fraud = (SELECT 
                        cloud_id, 
                        CAST((sum(is_fraud) > 0) AS Uint32) as is_fraud
                FROM $acq
                WHERE event='day_use' AND billing_account_id IS NOT NULL
                    AND ba_usage_status!='service'
                GROUP BY cloud_id);

insert into  $result_table  with truncate          
select * 
from $compute_features AS cf
left join $cloud_is_fraud  AS isf
on cf.cloud_id = isf.cloud_id;