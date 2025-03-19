PRAGMA Library("currency.sql");

IMPORT `currency` SYMBOLS $get_vat_decimal_35_15;

-- Input
$base_consumption_table = {{ param["base_consumption_table"] -> quote() }};
$folders_table = {{ param["folders_table"] -> quote() }};
$skus_table = {{ param["skus_table"] -> quote() }};
$services_table = {{ param["services_table"] -> quote() }};
$sku_labels_table = {{ param["sku_labels_table"] -> quote() }};
$ba_crm_tags_table = {{ param["ba_crm_tags_table"] -> quote() }};
$person_data = {{ param["person_data"] -> quote() }};

-- Output
$destination_path = {{ param["destination_path"] -> quote() }};
$PII_destination_path = {{ param["PII_destination_path"] -> quote() }};

$skus = (
  SELECT
    skus.sku_id             AS sku_id,
    skus.name               AS sku_name,
    skus.translation_en     AS sku_name_eng,
    skus.translation_ru     AS sku_name_rus,
    services.name           AS sku_service_name,
    services.description    AS sku_service_name_eng,
    labels.subservice       AS sku_subservice_name,
    skus.pricing_unit       AS sku_pricing_unit
  FROM $skus_table AS skus
  LEFT JOIN $sku_labels_table AS labels ON (skus.sku_id = labels.sku_id)
  LEFT JOIN $services_table AS services ON (labels.real_service_id = services.service_id)
);

$billing_records_enriched_skus = (
  SELECT
    br.*,
    sku_name,
    sku_name_eng,
    sku_name_rus,
    sku_service_name,
    sku_service_name_eng,
    sku_subservice_name,
    sku_pricing_unit
  FROM $base_consumption_table AS br
  LEFT JOIN $skus AS skus
  USING(sku_id)
);

$billing_records_enriched_folders = (
  SELECT
    br.*,
    folder_name
  FROM $billing_records_enriched_skus AS br
  LEFT JOIN $folders_table AS folders
  USING (folder_id)
);

$display_names = (
  SELECT
    COALESCE(String::ReplaceAll(crm.crm_account_name, '\"', "\'"), person.name) AS account_display_name,
    crm.`date`                                                                  AS billing_record_msk_date,
    crm.billing_account_id                                                      AS billing_account_id
  FROM $ba_crm_tags_table AS crm
  LEFT JOIN $person_data AS person
  USING (billing_account_id)
);

$billing_records_enriched_display_names = (
  SELECT
    br.*,
    account_display_name,
    Digest::Md5Hex(account_display_name) AS account_display_name_hash
  FROM $billing_records_enriched_folders AS br
  JOIN $display_names AS names
  USING (billing_record_msk_date, billing_account_id)
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
    base.folder_id                                          as folder_id,
    base.folder_name                                        as folder_name,
    base.billing_account_id                                 as billing_account_id,
    base.billing_record_origin_service                      as billing_record_origin_service,
    base.sku_id                                             as sku_id,
    base.billing_record_msk_date                            as billing_record_msk_date,
    base.billing_record_msk_month                           as billing_record_msk_month,
    base.billing_record_msk_quarter                         as billing_record_msk_quarter,
    base.billing_record_msk_half_year                       as billing_record_msk_half_year,
    base.billing_record_msk_year                            as billing_record_msk_year,
    base.billing_account_currency                           as billing_account_currency,
    base.billing_record_labels_hash                         as billing_record_labels_hash,
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
    SUM(base.billing_record_real_consumption_rub_vat)        as billing_record_real_consumption_rub_vat,
    sku_name,
    sku_pricing_unit,
    sku_name_eng,
    sku_name_rus,
    sku_service_name,
    sku_service_name_eng,
    sku_subservice_name,
    account_display_name_hash,
    cloud_id
  FROM $billing_records_enriched_display_names as base
  GROUP BY
    billing_account_id,
    billing_record_origin_service,
    sku_id,
    billing_record_labels_hash,
    sku_name,
    sku_pricing_unit,
    sku_name_eng,
    sku_name_rus,
    sku_service_name,
    sku_service_name_eng,
    sku_subservice_name,
    account_display_name_hash,
    folder_id,
    folder_name,
    cloud_id,
    billing_record_msk_date,
    billing_record_msk_month,
    billing_record_msk_quarter,
    billing_record_msk_half_year,
    billing_record_msk_year,
    billing_account_currency
ORDER BY billing_record_msk_date, billing_account_id, cloud_id, folder_id, sku_id;

INSERT INTO $PII_destination_path WITH TRUNCATE
SELECT
  billing_account_id,
  billing_record_msk_date,
  account_display_name
FROM $display_names
ORDER BY billing_record_msk_date, billing_account_id;
