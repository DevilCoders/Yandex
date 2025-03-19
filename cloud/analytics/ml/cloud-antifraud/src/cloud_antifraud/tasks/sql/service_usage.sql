USE hahn;


$all_1_after_trial= "//home/cloud_analytics/tmp/antifraud/all_1h_after_trial";
$features_table = "//home/cloud_analytics/tmp/antifraud/sdn_features";
$result_table = "//home/cloud_analytics/tmp/antifraud/service_usage";

$cloud_services = (select distinct 
                          `bm.cloud_id` as cloud_id,  
                          String::SplitToList(`bm.schema`, '.')[0] as service
                   from $all_1_after_trial);
                   
$service_usage = (select cloud_id, cast(max(service='nbs') as Uint32) as nbs,
                                   cast(max(service='nlb') as Uint32) as nlb, 
                                   cast(max(service='sdn') as Uint32) as sdn, 
                                   cast(max(service='vpc') as Uint32) as vpc
                  from $cloud_services
                  group by cloud_id);
                  
                  
INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $service_usage as su on ft.cloud_id = su.cloud_id

