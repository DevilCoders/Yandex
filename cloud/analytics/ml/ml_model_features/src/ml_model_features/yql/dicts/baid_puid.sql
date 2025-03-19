USE hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.QueryCacheTtl = '1h';

INSERT INTO `//home/cloud_analytics/ml/ml_model_features/dict/baid_puid` WITH TRUNCATE 
    SELECT 
        `billing_account_id`,
        Cast(`owner_passport_uid` AS UInt64) AS `puid`
    FROM `//home/cloud-dwh/data/prod/ods/billing/billing_accounts`
    WHERE `owner_passport_uid` IS NOT NULL
    ORDER BY `billing_account_id`, `puid`
;