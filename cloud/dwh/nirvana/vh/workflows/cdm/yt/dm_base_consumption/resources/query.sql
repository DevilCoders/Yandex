PRAGMA Library("datetime.sql");
PRAGMA Library("currency.sql");


IMPORT `datetime` SYMBOLS $format_msk_quarter_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_month_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_half_year_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_year_by_timestamp;
IMPORT `currency` SYMBOLS $get_vat_decimal_35_15;

-- Constants
$ZERO = Decimal('0', 35, 15);
$ONE = Decimal('1', 35, 15);

-- Utils
$convert_amount_to_rub = ($amount, $rate) -> (IF($rate != $ONE, $amount * $rate, $amount));
$to_double = ($value) -> (CAST($value as Double));
$is_mk8s = ($labels) -> (
    DictContains(
        Yson::LookupDict(Yson::ParseJson($labels, Yson::Options()), "user_labels"),
        "managed-kubernetes-cluster-id"
    )
);
$to_origin_service = ($labels) -> (if( $is_mk8s($labels),'mk8s', 'undefined'));

-- Input
$billing_records_folder = {{ param["billing_records_folder"] -> quote() }};
$billing_accounts_table = {{ param["billing_accounts_table"] -> quote() }};
$currency_rates_table = {{ param["currency_rates_table"] -> quote() }};

-- Output
$destination_path = {{ param["destination_path"] -> quote() }};

$billing_accounts = (
    SELECT
      billing_account_id AS billing_account_id,
      currency           AS currency,
    FROM $billing_accounts_table
);

$billing_records = (
  SELECT
    billing_account_id                                    as billing_account_id,
    folder_id                                             as folder_id,
    billing_record_origin_service                         as billing_record_origin_service,
    sku_id                                                as sku_id,
    labels_hash                                           as billing_record_labels_hash,
    resource_id                                           as resource_id,
    cloud_id                                              as cloud_id,
    SUM(cost)                                             as billing_record_cost,
    SUM(credit)                                           as billing_record_credit,
    SUM(expense)                                          as billing_record_expense,
    SUM(expense - NVL(reward, $ZERO))                     as billing_record_total,
    SUM(NVL(monetary_grant_credit, $ZERO))                as billing_record_credit_monetary_grant,
    SUM(NVL(cud_credit, $ZERO))                           as billing_record_credit_cud,
    SUM(NVL(volume_incentive_credit, $ZERO))              as billing_record_credit_volume_incentive,
    SUM(NVL(trial_credit, $ZERO))                         as billing_record_credit_trial,
    SUM(NVL(disabled_credit, $ZERO))                      as billing_record_credit_disabled,
    SUM(NVL(service_credit, $ZERO))                       as billing_record_credit_service,
    SUM(expense - NVL(reward, $ZERO))                     as billing_record_real_consumption,
    SUM(pricing_quantity)                                 as billing_record_pricing_quantity,
    NVL(SUM(reward), $ZERO)                               as billing_record_var_reward,
    `date`                                                as billing_record_msk_date,
    $get_vat_decimal_35_15($ONE, `date`, currency)        as billing_record_vat,
    $format_msk_month_by_timestamp(SOME(end_time))        as billing_record_msk_month,
    $format_msk_quarter_by_timestamp(SOME(end_time))      as billing_record_msk_quarter,
    $format_msk_half_year_by_timestamp(SOME(end_time))    as billing_record_msk_half_year,
    $format_msk_year_by_timestamp(SOME(end_time))         as billing_record_msk_year,
  FROM (
    SELECT
        br.* ,
        $to_origin_service(labels_json) as billing_record_origin_service
    FROM RANGE($billing_records_folder) as br
    )
  GROUP BY
    billing_account_id,
    folder_id,
    billing_record_origin_service,
    currency,
    `date`,
    sku_id,
    labels_hash,
    resource_id,
    cloud_id
);

$billing_records_enriched_currency = (
  SELECT
    br.*,
    ba.currency                                                           AS currency,
    $get_vat_decimal_35_15($ONE, br.billing_record_msk_date, ba.currency) AS billing_account_vat,
  FROM $billing_records AS br
  LEFT JOIN $billing_accounts AS ba USING (billing_account_id)
);

$billing_records_enriched_rates = (
  SELECT
    br.*,
    NVL(rates.quote, $ONE) AS quote,
  FROM $billing_records_enriched_currency AS br
  LEFT JOIN $currency_rates_table AS rates
    ON (br.currency == rates.currency AND br.billing_record_msk_date == rates.`date`)
);

$billing_records_full = (
  SELECT
    billing_account_id,
    billing_record_origin_service,
    sku_id,
    resource_id,
    folder_id,
    cloud_id,
    billing_record_labels_hash,
    billing_record_msk_date,
    billing_record_msk_month,
    billing_record_msk_quarter,
    billing_record_msk_half_year,
    billing_record_msk_year,
    currency                                                                                                as billing_account_currency,
    $to_double(quote)                                                                                       as billing_record_currency_exchange_rate,
    $to_double(billing_account_vat)                                                                         as billing_account_vat,
    $to_double(billing_record_pricing_quantity)                                                             as billing_record_pricing_quantity,
    $to_double(billing_record_cost)                                                                         as billing_record_cost,
    $to_double($convert_amount_to_rub(billing_record_cost, quote))                                          as billing_record_cost_rub,
    $to_double($convert_amount_to_rub(billing_record_cost, quote) * billing_account_vat)                    as billing_record_cost_rub_vat,
    $to_double(billing_record_credit)                                                                       as billing_record_credit,
    $to_double($convert_amount_to_rub(billing_record_credit, quote))                                        as billing_record_credit_rub,
    $to_double($convert_amount_to_rub(billing_record_credit, quote) * billing_account_vat)                  as billing_record_credit_rub_vat,
    $to_double(billing_record_credit_monetary_grant)                                                        as billing_record_credit_monetary_grant,
    $to_double($convert_amount_to_rub(billing_record_credit_monetary_grant, quote))                         as billing_record_credit_monetary_grant_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_monetary_grant, quote) * billing_account_vat)   as billing_record_credit_monetary_grant_rub_vat,
    $to_double(billing_record_credit_cud)                                                                   as billing_record_credit_cud,
    $to_double($convert_amount_to_rub(billing_record_credit_cud, quote))                                    as billing_record_credit_cud_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_cud, quote) * billing_account_vat)              as billing_record_credit_cud_rub_vat,
    $to_double(billing_record_credit_volume_incentive)                                                      as billing_record_credit_volume_incentive,
    $to_double($convert_amount_to_rub(billing_record_credit_volume_incentive, quote))                       as billing_record_credit_volume_incentive_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_volume_incentive, quote) * billing_account_vat) as billing_record_credit_volume_incentive_rub_vat,
    $to_double(billing_record_credit_trial)                                                                 as billing_record_credit_trial,
    $to_double($convert_amount_to_rub(billing_record_credit_trial, quote))                                  as billing_record_credit_trial_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_trial, quote) * billing_account_vat)            as billing_record_credit_trial_rub_vat,
    $to_double(billing_record_credit_disabled)                                                              as billing_record_credit_disabled,
    $to_double($convert_amount_to_rub(billing_record_credit_disabled, quote))                               as billing_record_credit_disabled_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_disabled, quote) * billing_account_vat)         as billing_record_credit_disabled_rub_vat,
    $to_double(billing_record_credit_service)                                                               as billing_record_credit_service,
    $to_double($convert_amount_to_rub(billing_record_credit_service, quote))                                as billing_record_credit_service_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_service, quote) * billing_account_vat)          as billing_record_credit_service_rub_vat,
    $to_double(billing_record_total)                                                                        as billing_record_total,
    $to_double($convert_amount_to_rub(billing_record_total, quote))                                         as billing_record_total_rub,
    $to_double($convert_amount_to_rub(billing_record_total, quote) * billing_account_vat)                   as billing_record_total_rub_vat,
    $to_double(billing_record_expense)                                                                      as billing_record_expense,
    $to_double($convert_amount_to_rub(billing_record_expense, quote))                                       as billing_record_expense_rub,
    $to_double($convert_amount_to_rub(billing_record_expense, quote) * billing_account_vat)                 as billing_record_expense_rub_vat,
    $to_double(billing_record_var_reward)                                                                   as billing_record_var_reward,
    $to_double($convert_amount_to_rub(billing_record_var_reward, quote))                                    as billing_record_var_reward_rub,
    $to_double($convert_amount_to_rub(billing_record_var_reward, quote) * billing_account_vat)              as billing_record_var_reward_rub_vat,
    -- DEPRECATED
    $to_double(billing_record_vat)                                                                          as billing_record_vat,
    $to_double(billing_record_real_consumption)                                                             as billing_record_real_consumption,
    $to_double($convert_amount_to_rub(billing_record_real_consumption, quote))                              as billing_record_real_consumption_rub,
    $to_double($convert_amount_to_rub(billing_record_real_consumption, quote) * billing_account_vat)        as billing_record_real_consumption_rub_vat
  FROM $billing_records_enriched_rates
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  *
FROM $billing_records_full
ORDER BY billing_record_msk_date, billing_account_id;
