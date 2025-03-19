PRAGMA Library("datetime.sql");


IMPORT `datetime` SYMBOLS $format_msk_month_cohort_by_timestamp;
IMPORT `datetime` SYMBOLS $get_date_range_inclusive;
IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `datetime` SYMBOLS $format_date;
IMPORT `datetime` SYMBOLS $MSK_TIMEZONE;

-- Utils
$msk_last_dt = AddTimezone(CurrentUtcTimestamp(), $MSK_TIMEZONE);
$msk_last_dt_str = $format_date($msk_last_dt);

-- Input
$base_consumption_table = {{ param["base_consumption_table"] -> quote() }};
$skus_table = {{ param["skus_table"] -> quote() }};
$services_table = {{ param["services_table"] -> quote() }};
$sku_labels_table = {{ param["sku_labels_table"] -> quote() }};
$billing_accounts_table = {{ param["billing_accounts_table"] -> quote() }};
$billing_accounts_history_table = {{ param["billing_accounts_history_table"] -> quote() }};

-- Output
$destination_path = {{ param["destination_path"] -> quote() }};

$billing_accounts = (
  SELECT
    *
  FROM (
    SELECT
      ba.billing_account_id                                AS billing_account_id,
      ba.name                                              AS billing_account_name,
      ba.type                                              AS billing_account_type,
      master_ba.billing_account_id                         AS billing_master_account_id,
      master_ba.name                                       AS billing_master_account_name,
      $format_msk_month_cohort_by_timestamp(ba.created_at) AS billing_account_month_cohort,
      person_type,
      state,
      usage_status,
      is_fraud,
      is_suspended_by_antifraud,
      payment_type,
      is_isv,
      is_var,
      is_subaccount,
      currency,
      country_code,
      $get_date_range_inclusive(
        $format_msk_date_by_timestamp(created_at),
        $msk_last_dt_str
      ) AS `date`
    FROM $billing_accounts_table AS ba
    LEFT JOIN (SELECT name, billing_account_id FROM $billing_accounts_table) AS master_ba
        ON (ba.master_account_id = master_ba.billing_account_id)
  )
  FLATTEN BY `date`
);

$billing_accounts_hist = (
  SELECT
    billing_account_id                            AS billing_account_id,
    `date`                                        AS `date`,
    MAX_BY(type, updated_at)                      AS type,
    MAX_BY(person_type, updated_at)               AS person_type,
    MAX_BY(is_suspended_by_antifraud, updated_at) AS is_suspended_by_antifraud,
    MAX_BY(usage_status, updated_at)              AS usage_status,
    MAX_BY(state, updated_at)                     AS state,
    MAX_BY(is_fraud, updated_at)                  AS is_fraud,
    MAX_BY(payment_type, updated_at)              AS payment_type,
    MAX_BY(is_isv, updated_at)                    AS is_isv,
    MAX_BY(is_var, updated_at)                    AS is_var,
  FROM
    $billing_accounts_history_table AS ba
  GROUP BY ba.billing_account_id AS billing_account_id, $format_msk_date_by_timestamp(updated_at) AS `date`
);

$billing_accounts_enriched_hist = (
  SELECT
    ba.`date`                                                                                                                       AS `date`,
    ba.billing_account_id                                                                                                           AS billing_account_id,
    ba.billing_account_name                                                                                                         AS billing_account_name,
    ba.billing_account_month_cohort                                                                                                 AS billing_account_month_cohort,
    ba.currency                                                                                                                     AS currency,
    ba.is_subaccount                                                                                                                AS is_subaccount,
    ba.country_code                                                                                                                 AS country_code,
    ba.billing_master_account_id                                                                                                    AS billing_master_account_id,
    ba.billing_master_account_name                                                                                                  AS billing_master_account_name,
    ba.billing_account_type                                                                                                         AS billing_account_type_current,
    LAST_VALUE(ba_hist.type) IGNORE NULLS OVER w_to_current_row                                                                     AS billing_account_type,
    ba.person_type                                                                                                                  AS person_type_current,
    LAST_VALUE(ba_hist.person_type) IGNORE NULLS OVER w_to_current_row                                                              AS person_type,
    ba.is_suspended_by_antifraud                                                                                                    AS is_suspended_by_antifraud_current,
    LAST_VALUE(ba_hist.is_suspended_by_antifraud) IGNORE NULLS OVER w_to_current_row                                                AS is_suspended_by_antifraud,
    ba.usage_status                                                                                                                 AS usage_status_current,
    LAST_VALUE(ba_hist.usage_status) IGNORE NULLS OVER w_to_current_row                                                             AS usage_status,
    ba.state                                                                                                                        AS state_current,
    LAST_VALUE(ba_hist.state) IGNORE NULLS OVER w_to_current_row                                                                    AS state,
    ba.is_fraud                                                                                                                     AS is_fraud_current,
    LAST_VALUE(ba_hist.is_fraud) IGNORE NULLS OVER w_to_current_row                                                                 AS is_fraud,
    ba.payment_type                                                                                                                 AS payment_type_current,
    LAST_VALUE(ba_hist.payment_type) IGNORE NULLS OVER w_to_current_row                                                             AS payment_type,
    ba.is_isv                                                                                                                       AS is_isv_current,
    LAST_VALUE(ba_hist.is_isv) IGNORE NULLS OVER w_to_current_row                                                                   AS is_isv,
    ba.is_var                                                                                                                       AS is_var_current,
    LAST_VALUE(ba_hist.is_var) IGNORE NULLS OVER w_to_current_row                                                                   AS is_var,
    FROM $billing_accounts AS ba
  LEFT JOIN $billing_accounts_hist AS ba_hist ON (ba.billing_account_id = ba_hist.billing_account_id AND ba.`date` = ba_hist.`date`)
  WINDOW
    w_to_current_row AS (PARTITION BY ba.billing_account_id ORDER BY ba.`date` ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
);

$enriched_skus = (
  SELECT
    skus.sku_id             as sku_id,
    skus.name               as sku_name,
    skus.pricing_unit       as sku_pricing_unit,
    skus.translation_en     as sku_name_eng,
    skus.translation_ru     as sku_name_rus,
    services.name           as sku_service_name,
    services.description    as sku_service_name_eng,
    services.group          as sku_service_group,
    labels.subservice       as sku_subservice_name,
  FROM $skus_table as skus
  LEFT JOIN $sku_labels_table as labels ON (skus.sku_id = labels.sku_id)
  LEFT JOIN $services_table as services ON (labels.real_service_id = services.service_id)
);

$consumption_enriched_skus_info = (
  SELECT
    base.billing_account_id                                 as billing_account_id,
    base.billing_record_origin_service                      as billing_record_origin_service,
    base.sku_id                                             as sku_id,
    base.billing_record_labels_hash                         as billing_record_labels_hash,
    base.billing_record_msk_date                            as billing_record_msk_date,
    base.billing_record_msk_month                           as billing_record_msk_month,
    base.billing_record_msk_quarter                         as billing_record_msk_quarter,
    base.billing_record_msk_half_year                       as billing_record_msk_half_year,
    base.billing_record_msk_year                            as billing_record_msk_year,
    base.billing_account_currency                           as billing_account_currency,
    SUM(base.billing_record_currency_exchange_rate)         as billing_record_currency_exchange_rate,
    SUM(base.billing_account_vat)                           as billing_account_vat,
    SUM(base.billing_record_pricing_quantity)               as billing_record_pricing_quantity,
    SUM(base.billing_record_cost)                           as billing_record_cost,
    SUM(base.billing_record_cost_rub)                       as billing_record_cost_rub,
    SUM(base.billing_record_cost_rub_vat)                   as billing_record_cost_rub_vat,
    SUM(base.billing_record_credit)                         as billing_record_credit,
    SUM(base.billing_record_credit_rub)                     as billing_record_credit_rub,
    SUM(base.billing_record_credit_rub_vat)                 as billing_record_credit_rub_vat,
    SUM(base.billing_record_credit_monetary_grant)          as billing_record_credit_monetary_grant,
    SUM(base.billing_record_credit_monetary_grant_rub)      as billing_record_credit_monetary_grant_rub,
    SUM(base.billing_record_credit_monetary_grant_rub_vat)  as billing_record_credit_monetary_grant_rub_vat,
    SUM(base.billing_record_credit_cud)                     as billing_record_credit_cud,
    SUM(base.billing_record_credit_cud_rub)                 as billing_record_credit_cud_rub,
    SUM(base.billing_record_credit_cud_rub_vat)             as billing_record_credit_cud_rub_vat,
    SUM(base.billing_record_credit_volume_incentive)        as billing_record_credit_volume_incentive,
    SUM(base.billing_record_credit_volume_incentive_rub)    as billing_record_credit_volume_incentive_rub,
    SUM(base.billing_record_credit_volume_incentive_rub_vat) as billing_record_credit_volume_incentive_rub_vat,
    SUM(base.billing_record_credit_trial)                   as billing_record_credit_trial,
    SUM(base.billing_record_credit_trial_rub)               as billing_record_credit_trial_rub,
    SUM(base.billing_record_credit_trial_rub_vat)           as billing_record_credit_trial_rub_vat,
    SUM(base.billing_record_credit_disabled)                as billing_record_credit_disabled,
    SUM(base.billing_record_credit_disabled_rub)            as billing_record_credit_disabled_rub,
    SUM(base.billing_record_credit_disabled_rub_vat)        as billing_record_credit_disabled_rub_vat,
    SUM(base.billing_record_credit_service)                 as billing_record_credit_service,
    SUM(base.billing_record_credit_service_rub)             as billing_record_credit_service_rub,
    SUM(base.billing_record_credit_service_rub_vat)         as billing_record_credit_service_rub_vat,
    SUM(base.billing_record_total)                          as billing_record_total,
    SUM(base.billing_record_total_rub)                      as billing_record_total_rub,
    SUM(base.billing_record_total_rub_vat)                  as billing_record_total_rub_vat,
    SUM(base.billing_record_expense)                        as billing_record_expense,
    SUM(base.billing_record_expense_rub)                    as billing_record_expense_rub,
    SUM(base.billing_record_expense_rub_vat)                as billing_record_expense_rub_vat,
    SUM(base.billing_record_var_reward)                     as billing_record_var_reward,
    SUM(base.billing_record_var_reward_rub)                 as billing_record_var_reward_rub,
    SUM(base.billing_record_var_reward_rub_vat)             as billing_record_var_reward_rub_vat,

    SUM(base.billing_record_vat)                            as billing_record_vat,
    SUM(base.billing_record_real_consumption)               as billing_record_real_consumption,
    SUM(base.billing_record_real_consumption_rub)           as billing_record_real_consumption_rub,
    SUM(base.billing_record_real_consumption_rub_vat)       as billing_record_real_consumption_rub_vat,
    skus.sku_name                                           as sku_name,
    skus.sku_pricing_unit                                   as sku_pricing_unit,
    skus.sku_name_eng                                       as sku_name_eng,
    skus.sku_name_rus                                       as sku_name_rus,
    skus.sku_service_name                                   as sku_service_name,
    skus.sku_service_name_eng                               as sku_service_name_eng,
    skus.sku_service_group                                  as sku_service_group,
    skus.sku_subservice_name                                as sku_subservice_name
  FROM $base_consumption_table as base
  JOIN $enriched_skus as skus USING(sku_id)
  GROUP BY
    base.billing_account_id,
    base.billing_record_origin_service,
    base.sku_id,
    base.billing_record_labels_hash,
    skus.sku_name,
    skus.sku_pricing_unit,
    skus.sku_name_eng,
    skus.sku_name_rus,
    skus.sku_service_name,
    skus.sku_service_name_eng,
    skus.sku_service_group,
    skus.sku_subservice_name,
    base.billing_record_msk_date,
    base.billing_record_msk_month,
    base.billing_record_msk_quarter,
    base.billing_record_msk_half_year,
    base.billing_record_msk_year,
    base.billing_account_currency
);

$base_consumption_enriched_billing_accounts = (
  SELECT
    base.billing_account_id                             as billing_account_id,
    base.billing_record_origin_service                  as billing_record_origin_service,
    base.billing_record_labels_hash                     as billing_record_labels_hash,
    base.sku_id                                         as sku_id,
    base.sku_name                                       as sku_name,
    base.sku_pricing_unit                               as sku_pricing_unit,
    base.sku_name_eng                                   as sku_name_eng,
    base.sku_name_rus                                   as sku_name_rus,
    base.sku_service_name                               as sku_service_name,
    base.sku_service_name_eng                           as sku_service_name_eng,
    base.sku_service_group                              as sku_service_group,
    base.sku_subservice_name                            as sku_subservice_name,
    base.billing_record_msk_date                        as billing_record_msk_date,
    base.billing_record_msk_month                       as billing_record_msk_month,
    base.billing_record_msk_quarter                     as billing_record_msk_quarter,
    base.billing_record_msk_half_year                   as billing_record_msk_half_year,
    base.billing_record_msk_year                        as billing_record_msk_year,
    base.billing_account_currency                       as billing_account_currency,
    base.billing_record_currency_exchange_rate          as billing_record_currency_exchange_rate,
    base.billing_account_vat                            as billing_account_vat,
    base.billing_record_pricing_quantity                as billing_record_pricing_quantity,
    base.billing_record_cost                            as billing_record_cost,
    base.billing_record_cost_rub                        as billing_record_cost_rub,
    base.billing_record_cost_rub_vat                    as billing_record_cost_rub_vat,
    base.billing_record_credit                          as billing_record_credit,
    base.billing_record_credit_rub                      as billing_record_credit_rub,
    base.billing_record_credit_rub_vat                  as billing_record_credit_rub_vat,
    base.billing_record_credit_monetary_grant           as billing_record_credit_monetary_grant,
    base.billing_record_credit_monetary_grant_rub       as billing_record_credit_monetary_grant_rub,
    base.billing_record_credit_monetary_grant_rub_vat   as billing_record_credit_monetary_grant_rub_vat,
    base.billing_record_credit_cud                      as billing_record_credit_cud,
    base.billing_record_credit_cud_rub                  as billing_record_credit_cud_rub,
    base.billing_record_credit_cud_rub_vat              as billing_record_credit_cud_rub_vat,
    base.billing_record_credit_volume_incentive         as billing_record_credit_volume_incentive,
    base.billing_record_credit_volume_incentive_rub     as billing_record_credit_volume_incentive_rub,
    base.billing_record_credit_volume_incentive_rub_vat as billing_record_credit_volume_incentive_rub_vat,
    base.billing_record_credit_trial                    as billing_record_credit_trial,
    base.billing_record_credit_trial_rub                as billing_record_credit_trial_rub,
    base.billing_record_credit_trial_rub_vat            as billing_record_credit_trial_rub_vat,
    base.billing_record_credit_disabled                 as billing_record_credit_disabled,
    base.billing_record_credit_disabled_rub             as billing_record_credit_disabled_rub,
    base.billing_record_credit_disabled_rub_vat         as billing_record_credit_disabled_rub_vat,
    base.billing_record_credit_service                  as billing_record_credit_service,
    base.billing_record_credit_service_rub              as billing_record_credit_service_rub,
    base.billing_record_credit_service_rub_vat          as billing_record_credit_service_rub_vat,
    base.billing_record_total                           as billing_record_total,
    base.billing_record_total_rub                       as billing_record_total_rub,
    base.billing_record_total_rub_vat                   as billing_record_total_rub_vat,
    base.billing_record_expense                         as billing_record_expense,
    base.billing_record_expense_rub                     as billing_record_expense_rub,
    base.billing_record_expense_rub_vat                 as billing_record_expense_rub_vat,
    base.billing_record_var_reward                      as billing_record_var_reward,
    base.billing_record_var_reward_rub                  as billing_record_var_reward_rub,
    base.billing_record_var_reward_rub_vat              as billing_record_var_reward_rub_vat,

    base.billing_record_vat                             as billing_record_vat,
    base.billing_record_real_consumption                as billing_record_real_consumption,
    base.billing_record_real_consumption_rub            as billing_record_real_consumption_rub,
    base.billing_record_real_consumption_rub_vat        as billing_record_real_consumption_rub_vat,

    ba.billing_account_name                             as billing_account_name,
    ba.billing_account_type                             as billing_account_type,
    ba.billing_account_type_current                     as billing_account_type_current,
    ba.billing_master_account_id                        as billing_master_account_id,
    ba.billing_master_account_name                      as billing_master_account_name,
    ba.billing_account_month_cohort                     as billing_account_month_cohort,
    ba.is_subaccount                                    as billing_account_is_subaccount,
    ba.country_code                                     as billing_account_country_code,
    ba.person_type_current                              as billing_account_person_type_current,
    ba.person_type                                      as billing_account_person_type,
    ba.usage_status_current                             as billing_account_usage_status_current,
    ba.usage_status                                     as billing_account_usage_status,
    ba.state_current                                    as billing_account_state_current,
    ba.`state`                                          as billing_account_state,
    ba.is_isv_current                                   as billing_account_is_isv_current,
    ba.is_isv                                           as billing_account_is_isv,
    ba.is_var_current                                   as billing_account_is_var_current,
    ba.is_var                                           as billing_account_is_var,
    ba.is_fraud_current                                 as billing_account_is_fraud_current,
    ba.is_fraud                                         as billing_account_is_fraud,
    ba.is_suspended_by_antifraud                        as billing_account_is_suspended_by_antifraud,
    ba.is_suspended_by_antifraud_current                as billing_account_is_suspended_by_antifraud_current,
    ba.payment_type_current                             as billing_account_payment_type_current,
    ba.payment_type                                     as billing_account_payment_type
  FROM $consumption_enriched_skus_info AS base
    INNER JOIN $billing_accounts_enriched_hist AS ba
      ON base.billing_record_msk_date = ba.`date` AND base.billing_account_id = ba.billing_account_id
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  *
FROM $base_consumption_enriched_billing_accounts
ORDER BY billing_record_msk_date, billing_account_id;
