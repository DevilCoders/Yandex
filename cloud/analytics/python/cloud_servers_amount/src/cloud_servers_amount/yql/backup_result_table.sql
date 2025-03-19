USE hahn;

$today = DateTime::Format('%Y-%m-%d')(CurrentTzDate('Europe/Moscow'));
$result_table = '//home/cloud_analytics/import/cloud_servers_info/resources_info';
$backup_path = '//home/cloud_analytics/import/cloud_servers_info/backups/resources_info_' || $today;

INSERT INTO $backup_path WITH TRUNCATE
    SELECT *
    FROM $result_table
    ORDER BY 
        date_modified,
        resource_type,
        server_dc,
        server_abc,
        type,
        server_rack,
        server_fqdn,
        server_inv
;
