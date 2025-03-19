USE hahn;


$all_1_after_trial= "//home/cloud_analytics/tmp/antifraud/all_1h_after_trial";
$features_table = "//home/cloud_analytics/tmp/antifraud/service_usage";
$result_table = "//home/cloud_analytics/tmp/antifraud/nbs_usage";

$structs = (select  AsStruct(`bm.cloud_id` AS cloud_id),
                    Yson::ConvertTo(`bm.tags`, Struct<size:Uint64,
                                                      type:String>)
            from $all_1_after_trial
            where `bm.schema` = 'nbs.volume.allocated.v2');
            
$flatten = (SELECT * 
            from $structs
            FLATTEN COLUMNS);
            
$nbs_usage = (select cloud_id,
                     max(cast(type="network-hdd" as Uint32)) as hdd,
                     max(cast(type="network-nvme" as Uint32)) as nvme,
                     max(cast(type="network-ssd" as Uint32)) as ssd,
                     min(size) as nbs_min_size,
                     max(size) as nbs_max_size,
                     median(size) as nbs_median_size,
                     stddev(size) as nbs_std_size
              from $flatten 
              group by cloud_id);
    
INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $nbs_usage as su on ft.cloud_id = su.cloud_id
