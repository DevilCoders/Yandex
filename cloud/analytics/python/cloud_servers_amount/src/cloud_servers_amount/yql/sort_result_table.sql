USE hahn;

$result_table = '//home/cloud_analytics/import/cloud_servers_info/resources_info';

INSERT INTO $result_table WITH TRUNCATE
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
