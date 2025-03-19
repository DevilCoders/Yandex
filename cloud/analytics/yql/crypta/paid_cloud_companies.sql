USE hahn;

DEFINE ACTION $paid_cloud_companies() AS
    $result_table = '//home/cloud_analytics/export/crypta/paid_cloud_companies';
    INSERT INTO $result_table WITH TRUNCATE 
    SELECT
        DISTINCT owner_passport_uid AS puids
    FROM `//home/cloud-dwh/data/prod/ods/billing/billing_accounts`
    WHERE person_type LIKE '%company%'
        AND is_verified
        AND NOT is_fraud 
        AND usage_status = 'paid'
END DEFINE;

EXPORT $paid_cloud_companies;
