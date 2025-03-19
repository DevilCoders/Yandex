USE hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';

INSERT INTO `//home/cloud_analytics/ml/ba_trust/mlp` WITH TRUNCATE
SELECT *
FROM
(
    SELECT 
    TableName() as `date`,
    CAST(`puid` as String) as `puid`,
    `score`
    FROM RANGE(`//home/mlp/export/saturn/bnpl/puid`)
    WHERE `is_prod`
) AS x
JOIN 
(
    SELECT billing_account_id, owner_passport_uid AS puid
    FROM `//home/cloud-dwh/data/prod/ods/billing/billing_accounts`
) AS y
ON x.puid = y.puid
WHERE LEN(`date`) == 10