USE hahn;


$all_1_after_trial= "//home/cloud_analytics/tmp/antifraud/all_1h_after_trial";
$features_table = "//home/cloud_analytics/tmp/antifraud/nbs_usage";
$result_table = "//home/cloud_analytics/tmp/antifraud/compute_serv";

$compute_serv = (select `bm.cloud_id` as cloud_id,
                        cast(max(`bm.schema` = "compute.snapshot") as Uint32) as snapshot,
                        cast(max(`bm.schema` = "compute.image") as Uint32) as image
                 from $all_1_after_trial
                 where `bm.schema` = "compute.snapshot" or `bm.schema` = "compute.image" 
                 group by `bm.cloud_id`);
 
INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $compute_serv as su on ft.cloud_id = su.cloud_id

