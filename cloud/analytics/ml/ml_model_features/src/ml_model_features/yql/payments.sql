Use hahn;
PRAGMA yt.Pool = 'cloud_analytics_pool';
PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA OrderedColumns;
DECLARE $dates AS List<String>;

$dm_crm_tags_path = "//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags";
$balance_history = "//home/cloud_analytics/ml/ba_trust/balance_history";
$currency_rates = "//home/cloud_analytics/ml/ba_trust/currency_rates";
$billing_accounts = "//home/cloud-dwh/data/prod/ods/billing/billing_accounts";


DEFINE ACTION $get_data_for_one_date($date) as
    $output = "//home/cloud_analytics/ml/ml_model_features/by_baid/payments" || "/" || $date;
    $pattern = $date || "-%";

    $crm_tags = (
    SELECT
        x.`billing_account_id` as `billing_account_id`,
        `date`,
        MIN_OF(COALESCE(DateTime::ToDays(CAST(`date` as Date) - `created_at`), 0), 365) as `days_from_created`,
        `usage_status`,
        `person_type`,
        `currency`,
        `state`,
        `is_fraud`,
        `is_subaccount`,
        `is_suspended_by_antifraud`,
        `is_isv`,
        `is_var`,
        `crm_account_id`,
        `partner_manager`,
        `segment`,
        `payment_type`
    FROM $dm_crm_tags_path as x
    LEFT JOIN (SELECT `billing_account_id`, `created_at` FROM $billing_accounts) as y
    ON x.`billing_account_id` == y.`billing_account_id`
    WHERE `date` LIKE $pattern
    );

    $valid_transactions = (
    SELECT
        x.`billing_account_id` as `billing_account_id`,
        `date`,
        SUM(`amount` * COALESCE(`quote`, 1)) as `paid_amount`
    FROM $balance_history as x
    LEFT JOIN $currency_rates as y
    ON x.currency == y.currency AND SUBSTRING(CAST(x.`created_at` as String), 0, 10) == y.msk_date
    WHERE x.`status` == 'ok' AND x.`transaction_type` == 'payments' AND NOT `is_aborted` AND `date` LIKE $pattern
    GROUP BY SUBSTRING(CAST(x.`created_at` as String), 0, 10) as `date`, x.`billing_account_id`
    );

    $last_payment = (
    SELECT 
        `billing_account_id`,
        `date`,
        DateTime::ToDays(CAST(MAX(y.`date`) as Date) - CAST(`date` as Date)) as `days_since_last_payment`
    FROM $crm_tags as x
    LEFT JOIN $valid_transactions as y
    ON x.`billing_account_id` == y.`billing_account_id`
    WHERE y.`date` <= x.`date`
    GROUP BY x.`billing_account_id` as `billing_account_id`,  x.`date` as `date`
    );

    INSERT INTO $output WITH TRUNCATE
    SELECT * 
    FROM $crm_tags as x
    LEFT JOIN $valid_transactions as y
    ON x.`billing_account_id` == y.`billing_account_id` AND x.`date` == y.`date`
    LEFT JOIN $last_payment as z
    ON x.`billing_account_id` == z.`billing_account_id` AND x.`date` == z.`date`
END DEFINE;

EVALUATE FOR $date IN $dates
    DO $get_data_for_one_date($date)