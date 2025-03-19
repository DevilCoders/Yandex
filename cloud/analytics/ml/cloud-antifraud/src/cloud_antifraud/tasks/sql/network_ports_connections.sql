PRAGMA Library("time.sql");
IMPORT time SYMBOLS $date_time;
PRAGMA yt.JoinMergeForce = 'true';
-- PRAGMA yt.Pool = 'cloud_analytics_pool';


$network_rules = "//logs/yc-antifraud-overlay-flows-stats/1d";
$result_table = "//home/cloud_analytics/tmp/antifraud/network_ports_connections";
$features_table = "//home/cloud_analytics/tmp/antifraud/passport_rules";

$partitions_count = 200;

$network_brute = 
    (SELECT
        cloud_id,
        CAST(setup_time as DateTime) as setup_time,
        destip,
        bytes,
        dport,
        CAST(Random(1)*$partitions_count AS Uint32) as salt, 
    FROM
        RANGE($network_rules)
    WHERE
        protocol = 6 AND
        dport IN (3389, 22) AND
        direction = "egress" AND
        (cloud_id is not null) AND
        (CAST(setup_time as DateTime) is not null)
        );


$features_salt_list = (SELECT cloud_id, 
                         trial_start, 
                         ListFromRange(0, $partitions_count) as salt
                  FROM $features_table
                 );
                 


$features_salt = (select * 
                  from $features_salt_list
                  FLATTEN LIST BY salt
                 );
                 

$features_network_raw = (SELECT DISTINCT network.*
              FROM $features_salt as ft
              LEFT JOIN $network_brute as network 
              ON (ft.cloud_id = network.cloud_id) 
                AND (ft.salt = network.salt) 
              WHERE Math::Abs(DateTime::ToSeconds(network.setup_time) 
                             - DateTime::ToSeconds(ft.trial_start)) / 3600 <=1
              );


$network_agg = (SELECT  cloud_id,
                        COUNT(DISTINCT if(dport=22, destip)) AS unique_targets_ssh,
                        COUNT(DISTINCT if(dport=3389, destip)) AS unique_targets_rdp,
                        SUM(if(dport=22, bytes, 0)) as egress_bytes_ssh,
                        SUM(if(dport=3389, bytes, 0)) as egress_bytes_rdp
                  FROM $features_network_raw
                  GROUP BY cloud_id
        
        );

INSERT INTO $result_table WITH TRUNCATE  
select * 
from $features_table  as ft
left join $network_agg  as network_agg 
    on ft.cloud_id = network_agg.cloud_id
