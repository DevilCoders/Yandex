USE hahn;
IMPORT time SYMBOLS $date_time, $format_datetime ;

IMPORT time SYMBOLS $date_time, $format_datetime ;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;


DEFINE ACTION  $competitors_ip_connections() AS

    $result_table = ('//home/cloud_analytics/smb/targets_from_mass/competitors_ip_connections/' 
                    || $format_datetime(AddTimezone(CurrentUtcDateTime(), "Europe/Moscow")));
    $cloud_orgs_connections = '//home/cloud_analytics/import/network-logs/cloud_orgs_connections_hist';
    
    $competitors = (
        'amazon.com', 
        'hetzner online gmbh', 
        'ooo network of data-centers selectel', 
        'google cloud', 
        'domain names registrar reg.ru, ltd', 
        'dataline ltd', 
        'mail.ru llc', 
        'servers-com', 
        'digital ocean', 
        'microsoft azure', 
        'tencent cloud computing', 
        'triolan', 
        'microsoft bingbot', 
        'dedibox sas'
    );
    
    $cloud_orgs_connections_7d = (
        SELECT *
        FROM $cloud_orgs_connections
        WHERE (connection_date > (CurrentUtcDatetime() - Interval('P20D')))
        AND (org_by_ip IN $competitors)
    );
    
    $threshold_clouds = (
        SELECT cloud_id, 
               org_by_ip, 
               AVG(n_connections_sampled) AS avg_last_connections_sampled
        FROM $cloud_orgs_connections_7d
        GROUP BY cloud_id, org_by_ip
        HAVING AVG(n_connections_sampled) > 500
    );
    
    
    
    $clouds_bas = (
        SELECT billing_account_id, MAX_BY(cloud_id, epoch) AS cloud_id
        FROM `//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict`
        GROUP BY billing_account_id
    );
    
    
    $billing_accounts_info = (
        SELECT billing_account_id, 
               account_name, 
               sales_name_actual,
               sum(real_consumption_vat) AS cons_last_30_days
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE event = 'day_use' AND ba_usage_status != 'service'
                AND ($date_time(event_time) >= (CurrentUtcDate() - Interval('P30D')))
                AND is_fraud != 1
        GROUP BY billing_account_id, 
                 account_name,
                 sales_name_actual
       -- HAVING sum(real_consumption_vat) > 0 
    );
    
    INSERT INTO $result_table WITH TRUNCATE 
    SELECT * 
    FROM $clouds_bas AS clouds_bas
    INNER JOIN $threshold_clouds AS clouds_connections
        ON clouds_bas.cloud_id = clouds_connections.cloud_id
    INNER JOIN $billing_accounts_info AS billing_accounts_info
        ON clouds_bas.billing_account_id = billing_accounts_info.billing_account_id
END DEFINE;


EXPORT $competitors_ip_connections;