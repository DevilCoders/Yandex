USE hahn;

DEFINE SUBQUERY $stats($path, $from_prefix) AS
    $ba_clouds = '//home/cloud/billing/exported-billing-tables/clouds_prod';
    --sql
    $raw_stats = (
        SELECT  cloud_id, 
                instance_id,
                sourceip,
                setup_time,
                DateTime::MakeDatetime(
                    DateTime::StartOf(
                        CAST(setup_time as DateTime), Interval('PT1H'))
                ) AS setup_hour,
                dport,
                direction,
                protocol,
                destip,
                bytes/1e6 AS mbytes
        FROM RANGE($path, $from_prefix)
        WITH SCHEMA Struct<cloud_id           : String,
                           instance_id        : String,
                           sourceip           : String,
                           destip             : String,
                           setup_time         : String,
                           protocol           : Uint32,
                           direction          : String, 
                           dport              : Uint32,
                           bytes              : Uint64>
        WHERE dport IN (22, 3389)
              AND protocol = 6 
              AND direction = "egress"
    );
    
    --sql
    $cloud_connections = (
         SELECT cloud_id, 
                instance_id,
                sourceip, 
                setup_hour,
                MIN(CAST(setup_time as DateTime))             AS min_setup_time,
                MAX(CAST(setup_time as DateTime))             AS max_setup_time,
                SUM(IF(dport=22, mbytes, 0))                  AS egress_mbytes_ssh,
                SUM(IF(dport=3389, mbytes, 0))                AS egress_mbytes_rdp,
                COUNT(DISTINCT destip)                        AS n_unique_ssh_rdp_dests
        FROM $raw_stats    
        GROUP BY cloud_id, instance_id, sourceip, setup_hour
    );

    SELECT  cloud_connections.cloud_id AS cloud_id,
            cloud_connections.instance_id AS instance_id,
            cloud_connections.sourceip AS sourceip, 
            cloud_connections.setup_hour AS setup_hour,
            cloud_connections.min_setup_time AS min_setup_time,
            cloud_connections.max_setup_time AS max_setup_time,
            cloud_connections.egress_mbytes_ssh AS egress_mbytes_ssh,
            cloud_connections.egress_mbytes_rdp AS egress_mbytes_rdp,
            cloud_connections.n_unique_ssh_rdp_dests AS n_unique_ssh_rdp_dests,
            MIN_BY(ba_clouds.billing_account_id, 
                    ABS(DateTime::ToSeconds(
                        CAST(ba_clouds.effective_time AS DateTime)
                        - cloud_connections.setup_hour))
            ) AS billing_account_id,
            MIN(ABS(DateTime::ToSeconds(
                        CAST(ba_clouds.effective_time AS DateTime)
                        - cloud_connections.setup_hour))) 
                        AS time_diff
    FROM $cloud_connections AS cloud_connections
    LEFT JOIN $ba_clouds AS ba_clouds
        ON ba_clouds.id = cloud_connections.cloud_id
    GROUP BY cloud_connections.cloud_id, 
             cloud_connections.instance_id,
             cloud_connections.sourceip, 
             cloud_connections.setup_hour,
             cloud_connections.min_setup_time,
             cloud_connections.max_setup_time,
             cloud_connections.egress_mbytes_ssh,
             cloud_connections.egress_mbytes_rdp,
             cloud_connections.n_unique_ssh_rdp_dests


    
END DEFINE;

EXPORT $stats;
