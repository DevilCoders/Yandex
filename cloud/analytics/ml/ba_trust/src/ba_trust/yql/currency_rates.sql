USE hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';

INSERT INTO hahn.`home/cloud_analytics/ml/ba_trust/currency_rates` WITH TRUNCATE
SELECT
    `date` as `msk_date`,
    `currency`,
    CAST(`quote` as float) as `quote`
FROM hahn.`home/cloud-dwh/data/prod/ods/statbox/currency_rates`