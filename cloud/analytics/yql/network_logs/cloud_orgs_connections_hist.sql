USE hahn;


DEFINE ACTION $cloud_orgs_connections() AS
    $result_table = '//home/cloud_analytics/import/network-logs/cloud_orgs_connections_hist';
    
    $last_update_date = (
        SELECT MAX(connection_date) AS last_update_date
        FROM $result_table
    );
    
    $top_orgs_by_cloud = (
        SELECT
            cloud_id,
            org_by_ip,
            connection_date,
            COUNT(DISTINCT unixtime) AS n_connections_sampled
        FROM `home/cloud_analytics/import/network-logs/sflows/cloud_ips_sflow_last_days`
        WHERE connection_date > $last_update_date
        GROUP BY 
            cloud_id, 
            DateTime::MakeDatetime(DateTime::StartOfDay(
                DateTime::FromSeconds(Unwrap(CAST(unixtime AS Uint32)))
                )) AS connection_date,
            Geo::GetOrgNameByIp(dst_ip) AS org_by_ip
    );
    
    
    INSERT INTO  $result_table 
    SELECT * 
    FROM $top_orgs_by_cloud
    WHERE org_by_ip != ''
    ORDER BY n_connections_sampled DESC, org_by_ip;
END DEFINE;


EXPORT $cloud_orgs_connections;