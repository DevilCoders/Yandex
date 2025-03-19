PRAGMA yt.Pool = 'cloud_analytics_pool';
USE hahn;

$now = CurrentUtcTimestamp();
$format = DateTime::Format("%Y-%m-%d");
$date_0 = $format($now);
$date_5 = CAST($format( CAST($now AS Date) - Interval("P5D")) as String);

INSERT INTO `//home/cloud_analytics/ml/ba_trust/test` WITH TRUNCATE
SELECT
    x.`billing_account_id` as `billing_account_id`,
    x.`date` as `date`,
    `balance_float`,
    `autopay_failures_count`,
    `paid_amount`,
    `previous_cost`,
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
    `bnpl_score`
FROM `//home/cloud_analytics/ml/ba_trust/all_data` as x
LEFT JOIN `//home/cloud_analytics/ml/ba_trust/frequent_data` as y
ON (x.`billing_account_id` == y.`billing_account_id` AND x.`date` == y.`date`)
WHERE x.`date` >= $date_5
AND x.`date` <= $date_0
AND `state` != 'suspended'
AND `days_from_created` >= 30