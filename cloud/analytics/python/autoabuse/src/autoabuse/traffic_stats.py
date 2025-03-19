
from clan_tools.data_adapters.YQLAdapter import YQLAdapter


def get_traffic_stats(traffic_stats_path, current_tickets_path, labels_history_path):
    ip_clouds = YQLAdapter().execute_query(f'''
        
        PRAGMA AnsiInForEmptyOrNullableItemsCollections;
        PRAGMA yt.Pool='cloud_analytics_pool';

        --sql
        $ba_info = (
            SELECT   
                billing_account_id AS billing_account_id, 
                MAX(event_time) AS last_event_time,
                MAX_BY(segment, event_time) AS segment,
                MAX_BY(ba_usage_status, event_time) AS ba_usage_status,
                MAX_BY(ba_state, event_time) AS ba_state,
                MAX_BY(sales_name, event_time) AS sales_name,
                MAX_BY(email, event_time) AS email,
                MAX_BY(phone, event_time) AS phone
            FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
            WHERE  ba_usage_status != 'service'
                    AND (event  IN ('day_use', 'cloud_created', 'ba_created'))
            GROUP BY billing_account_id
        );

        --sql
        $cloud_ips = (
            SELECT *
            FROM `{traffic_stats_path}`
            WHERE setup_hour > (CurrentUtcDate() - Interval('P2D'))
        );

        --sql
        $current_tickets_flat = (
            SELECT *
            FROM `{current_tickets_path}`
            WHERE key NOT IN (SELECT key FROM `{labels_history_path}`)  
        );

        --sql
        $current_tickets = (
            SELECT ip, AGGREGATE_LIST(key) AS keys
            FROM $current_tickets_flat
            GROUP BY ip
        );


        --sql
        $cloud_keys_all = (
            SELECT current_tickets.*, 
                cloud_ips.*,
                (egress_mbytes_ssh
                        + egress_mbytes_rdp
                        + n_unique_ssh_rdp_dests) AS traffic
            FROM $current_tickets AS current_tickets
            INNER JOIN $cloud_ips AS cloud_ips
            ON current_tickets.ip = cloud_ips.sourceip
        );



        --sql
        $traffic_stats = (
            SELECT  sourceip,
                    keys,
                    billing_account_id,
                    cloud_id                                  AS cloud_id,
                    MAX_BY(instance_id, traffic)              AS instance_id,
                    MAX_BY(egress_mbytes_rdp, traffic)        AS egress_mbytes_rdp,
                    MAX_BY(egress_mbytes_ssh, traffic)        AS egress_mbytes_ssh,
                    MAX_BY(max_setup_time, traffic)           AS max_setup_time,
                    MAX_BY(min_setup_time, traffic)           AS min_setup_time,
                    MAX_BY(n_unique_ssh_rdp_dests, traffic)   AS n_unique_ssh_rdp_dests,
                    MAX(traffic)                              AS traffic
            FROM $cloud_keys_all AS cloud_keys_all
            GROUP BY sourceip, keys, cloud_id, billing_account_id
        );

        --sql
        SELECT *
        FROM $traffic_stats AS traffic_stats
        LEFT JOIN $ba_info AS ba_info
            ON traffic_stats.billing_account_id = ba_info.billing_account_id

    ''', to_pandas=True)
    return ip_clouds