PRAGMA yt.Pool = 'cloud_analytics_pool';
USE hahn;
$now = CurrentUtcTimestamp();
$format = DateTime::Format("%Y-%m-%d");

$date_60 = CAST($format( CAST($now AS Date) - Interval("P60D")) as String);
$date_120 = CAST($format( CAST($now AS Date) - Interval("P120D")) as String);

INSERT INTO `//home/cloud_analytics/ml/ba_trust/train` WITH TRUNCATE
SELECT
    x.`billing_account_id` as `billing_account_id`,
    x.`date` as `date`,
    `balance_float`,
    `autopay_failures_count`,
    `paid_amount`,
    `previous_cost`,
    `unchanging_debt_in_30d`,
    `wont_pay_anymore_in_30d`,
    `is_isv`,
    `is_subaccount`,
    `is_suspended_by_antifraud`,
    `is_var`,
    `payment_type`,
    `person_type`,
    `segment`,
    `state`,
    `usage_status`,
    `days_from_created`,
    `days_after_last_payment`,
    `days_after_became_paid`,
    `bnpl_score`,
    `wont_pay_anymore_in_30d` AND (`unchanging_debt_in_30d` > 1000) as `target_dbt`
FROM `//home/cloud_analytics/ml/ba_trust/all_data` as x
LEFT JOIN `//home/cloud_analytics/ml/ba_trust/frequent_data` as y
ON (x.`billing_account_id` == y.`billing_account_id` AND x.`date` == y.`date`)
WHERE x.`date` >= $date_120
AND x.`date` <= $date_60
AND `state` != 'suspended'