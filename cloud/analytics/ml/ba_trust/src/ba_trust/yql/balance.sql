USE hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ba_trust/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';

INSERT INTO `//home/cloud_analytics/ml/ba_trust/billing_accounts_balance` WITH TRUNCATE 
SELECT
    `billing_account_id`,
    LAST_VALUE(CAST(`balance` as float)) over w as `balance`,
    CAST(CAST(`updated_at` as Date) as String) as `date`
FROM `//home/cloud-dwh/data/prod/ods/billing/billing_accounts_history`
WINDOW w AS (
    PARTITION BY (`billing_account_id`, CAST(`updated_at` as Date))
    ORDER BY `updated_at`
);
