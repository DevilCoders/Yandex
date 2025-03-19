USE hahn;

DEFINE ACTION $interfax_spark_cloud_users() AS
    $result_table = '//home/cloud_analytics/import/interfax/cloud_users';
    
    $acq = (
        SELECT CAST(puid AS Int64) as passport_id, billing_account_id, account_name, sales_name
        FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
        WHERE event = 'ba_created' and ba_usage_status != 'service'
    );
    
    
    $inn = (
        SELECT DISTINCT  inn,  passport_id
        FROM `//home/cloud_analytics/import/balance/balance_persons`
        WHERE (inn IS NOT NULL ) AND (passport_id IS NOT NULL)
    );
    
    
    $spark = (
        SELECT * 
        FROM `//home/cloud_analytics/import/interfax/all`
    );
    
    
    INSERT INTO $result_table WITH TRUNCATE 
    SELECT acq.*, spark.*
    FROM $acq AS acq
    INNER JOIN $inn AS inn
        ON acq.passport_id = inn.passport_id
    INNER JOIN $spark AS spark
        ON spark.inn = inn.inn
END DEFINE;


EXPORT $interfax_spark_cloud_users;