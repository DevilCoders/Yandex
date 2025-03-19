USE hahn;


PRAGMA File('libcrypta_identifier_udf.so', 'yt://hahn/home/crypta/public/udfs/libcrypta_identifier_udf.so');
PRAGMA Udf('libcrypta_identifier_udf.so');


$result_table = '{result_table}';

DEFINE SUBQUERY $get_yc_attribute($column) AS
    SELECT DISTINCT  
        CAST(puid AS Int64) AS passport_id, 
        $column AS criterion,
        nvl(CAST(TableRow().$column AS String), '') AS value,
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE  ba_usage_status != 'service'
            AND (puid IS NOT NULL) AND puid != ''
END DEFINE;


$all_attributes = (
    SELECT *
    FROM $get_yc_attribute('phone')
    
    UNION ALL
    
    SELECT *
    FROM $get_yc_attribute('email')
    
    UNION ALL
    
    SELECT *
    FROM $get_yc_attribute('account_name')
    
    UNION ALL

    SELECT *
    FROM $get_yc_attribute('cloud_id')


    UNION ALL

    SELECT DISTINCT
        CAST(passport_uid AS Int64) AS passport_id,
        'cloud_id' AS criterion,
        cloud_id AS value
    FROM `//home/cloud_analytics/import/iam/cloud_owners_history`
    WHERE cloud_status = 'CREATING'

    UNION ALL

    SELECT *
    FROM $get_yc_attribute('billing_account_id')

    UNION ALL

    SELECT DISTINCT  
        CAST(puid AS Int64) AS passport_id, 
        'billing_account_id' AS criterion,
        nvl(master_account_id, '') AS value,
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE  ba_usage_status != 'service'
            AND (puid IS NOT NULL) AND puid != ''
    
    
    UNION ALL
    SELECT DISTINCT 
            CAST(passport_id AS Int64) AS passport_id, 
            'inn' AS criterion,
            inn AS value
    FROM `//home/cloud_analytics/import/balance/balance_persons`
    WHERE (inn IS NOT NULL ) AND (passport_id IS NOT NULL)
);

INSERT INTO $result_table WITH TRUNCATE 
SELECT passport_id,
       IF(criterion = 'account_name', 
          'name', 
          criterion) AS criterion,
       CASE criterion 
         WHEN 'phone' THEN Identifiers::Phone('+' || value).Normalize
         WHEN 'email' THEN Identifiers::Email(value).Normalize
         ELSE value 
       END AS value
FROM $all_attributes
WHERE (value IS NOT NULL) AND (value != '') AND (String::ToLower(value) NOT LIKE '%fake%') 