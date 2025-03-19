PRAGMA Library("datetime.sql");
PRAGMA Library("currency.sql");
IMPORT `currency` SYMBOLS $get_vat_decimal_35_15;

$billing_records_folder = {{ param["billing_records_folder"] -> quote() }};
$attribution_table = {{ param["attribution_table"] -> quote() }};
$currency_rates_table = {{ param["currency_rates_table"] -> quote() }};
$skus_table = {{ param["skus_table"] -> quote() }};
$services_table = {{ param["services_table"] -> quote() }};
$sku_labels_table = {{ param["sku_labels_table"] -> quote() }};
$ba_crm_tags_table = {{ param["ba_crm_tags_table"] -> quote() }};

$destination_path = {{ input1 -> table_quote() }};

$ONE = Decimal('1', 35, 15);
$ZERO = Decimal('0', 35, 15);

$billing_records = (
    SELECT
        billing_account_id                                      AS billing_account_id,
        cloud_id                                                AS cloud_id,
        sku_id                                                  AS sku_id,
        SUM(expense - NVL(reward, $ZERO))                       AS total,
        SUM(NVL(monetary_grant_credit, $ZERO))                  AS credit_monetary_grant,
        `date`                                                  AS `date`,
        $get_vat_decimal_35_15($ONE, `date`, currency)          AS billing_record_vat,
        currency                                                AS currency,
    FROM
        RANGE($billing_records_folder)
    GROUP BY
        billing_account_id,
        cloud_id,
        `date`,
        sku_id,
        currency
);

$billing_records_enriched_quote = (
  SELECT
    br.billing_account_id                                                                           AS billing_account_id,
    br.cloud_id                                                                                     AS cloud_id,
    br.sku_id                                                                                       AS sku_id,
    br.`date`                                                                                       AS date_msk,
    CAST(br.total * NVL(rates.quote, $ONE) * br.billing_record_vat AS double)                       AS total_rub_vat,
    CAST(br.credit_monetary_grant * NVL(rates.quote, $ONE) * br.billing_record_vat AS double)       AS credit_monetary_grant_rub_vat,
  FROM $billing_records           AS br
  LEFT JOIN $currency_rates_table AS rates
  USING (currency, `date`)
);

$billing_records_enriched_crm = (
    SELECT
        br.*,
        crm.segment                             AS crm_segment,
        crm.is_suspended_by_antifraud_current   AS billing_account_is_suspended_by_antifraud_current,
        crm.person_type                         AS person_type,
        crm.billing_account_month_cohort        AS billing_account_month_cohort,
        crm.billing_account_name                AS billing_account_name
    FROM $billing_records_enriched_quote AS br
    LEFT JOIN $ba_crm_tags_table         AS crm
    ON br.date_msk = crm.`date` AND br.billing_account_id = crm.billing_account_id
);

$enriched_sku_tags = (
    SELECT
        skus.sku_id             AS sku_id,
        skus.name               AS sku_name,
        services.name           AS sku_service_name,
        services.group          AS sku_service_group,
        labels.subservice       AS sku_subservice_name
    FROM $skus_table AS skus
    LEFT JOIN $sku_labels_table AS labels   ON (skus.sku_id = labels.sku_id)
    LEFT JOIN $services_table   AS services ON (labels.real_service_id = services.service_id)
);

$billing_records_enriched_sku_tags = (
    SELECT
        br.*,
        sku_name,
        sku_service_name,
        sku_service_group,
        sku_subservice_name,
    FROM $billing_records_enriched_crm AS br
    LEFT JOIN $enriched_sku_tags       AS sku_tags
    USING(sku_id)
);

$weights_for_billing_account = (
    SELECT
        billing_account_id,
        channel_marketing_influenced,
        channel,
        utm_source,
        utm_medium,
        utm_campaign_name,
        utm_term,
        utm_campaign_country,
        SUM(event_first_weight)                         AS event_first_weight,
        SUM(event_last_weight)                          AS event_last_weight,
        SUM(event_uniform_weight)                       AS event_uniform_weight,
        SUM(event_u_shape_weight)                       AS event_u_shape_weight,
        SUM(event_exp_7d_half_life_time_decay_weight)   AS event_exp_7d_half_life_time_decay_weight
    FROM $attribution_table
    GROUP BY
        billing_account_id,
        channel_marketing_influenced,
        channel,
        utm_source,
        utm_medium,
        utm_campaign_name,
        utm_term,
        utm_campaign_country
);


INSERT INTO $destination_path WITH TRUNCATE
SELECT
    br.billing_account_id                                                               AS billing_account_id,
    br.cloud_id                                                                         AS cloud_id,
    br.sku_id                                                                           AS sku_id,
    br.date_msk                                                                         AS billing_record_msk_date,

    br.crm_segment                                                                      AS crm_segment,
    br.billing_account_is_suspended_by_antifraud_current                                AS billing_account_is_suspended_by_antifraud_current,
    br.person_type                                                                      AS person_type,
    br.billing_account_month_cohort                                                     AS billing_account_month_cohort,
    br.billing_account_name                                                             AS billing_account_name,

    br.sku_name                                                                         AS sku_name,
    br.sku_service_name                                                                 AS sku_service_name,
    br.sku_service_group                                                                AS sku_service_group,
    br.sku_subservice_name                                                              AS sku_subservice_name,

    weights.event_first_weight                                                                                                  AS event_first_weight,
    NVL(br.total_rub_vat * weights.event_first_weight, br.total_rub_vat)                                                        AS billing_record_total_rub_vat_by_event_first_weight_model,
    NVL(br.credit_monetary_grant_rub_vat * weights.event_first_weight, br.credit_monetary_grant_rub_vat)                        AS billing_record_credit_monetary_grant_rub_vat_by_event_first_weight_model,

    weights.event_last_weight                                                                                                   AS event_last_weight,
    NVL(br.total_rub_vat * weights.event_last_weight, br.total_rub_vat)                                                         AS billing_record_total_rub_vat_by_event_last_weight_model,
    NVL(br.credit_monetary_grant_rub_vat * weights.event_last_weight, br.credit_monetary_grant_rub_vat)                         AS billing_record_credit_monetary_grant_rub_vat_by_event_last_weight_model,

    weights.event_uniform_weight                                                                                                AS event_uniform_weight,
    NVL(br.total_rub_vat * weights.event_uniform_weight, br.total_rub_vat)                                                      AS billing_record_total_rub_vat_by_event_uniform_weight_model,
    NVL(br.credit_monetary_grant_rub_vat * weights.event_uniform_weight, br.credit_monetary_grant_rub_vat)                      AS billing_record_credit_monetary_grant_rub_vat_by_event_uniform_weight_model,

    weights.event_u_shape_weight                                                                                                AS event_u_shape_weight,
    NVL(br.total_rub_vat * weights.event_u_shape_weight, br.total_rub_vat)                                                      AS billing_record_total_rub_vat_by_event_u_shape_weight_model,
    NVL(br.credit_monetary_grant_rub_vat * weights.event_u_shape_weight, br.credit_monetary_grant_rub_vat)                      AS billing_record_credit_monetary_grant_rub_vat_by_event_u_shape_weight_model,

    weights.event_exp_7d_half_life_time_decay_weight                                                                            AS event_exp_7d_half_life_time_decay_weight,
    NVL(br.total_rub_vat * weights.event_exp_7d_half_life_time_decay_weight, br.total_rub_vat)                                  AS billing_record_total_rub_vat_by_event_exp_7d_half_life_time_decay_weight_model,
    NVL(br.credit_monetary_grant_rub_vat * weights.event_exp_7d_half_life_time_decay_weight, br.credit_monetary_grant_rub_vat)  AS billing_record_credit_monetary_grant_rub_vat_by_event_exp_7d_half_life_time_decay_weight_model,

    weights.channel                                                                     AS channel,
    NVL(weights.channel_marketing_influenced, "Unmatched")                              AS channel_marketing_influenced,
    weights.utm_source                                                                  AS utm_source,
    weights.utm_medium                                                                  AS utm_medium,
    weights.utm_campaign_name                                                           AS utm_campaign_name,
    weights.utm_term                                                                    AS utm_term,
    weights.utm_campaign_country                                                        AS utm_campaign_country

FROM $billing_records_enriched_sku_tags AS br
    LEFT JOIN $weights_for_billing_account AS weights
        ON  br.billing_account_id = weights.billing_account_id

ORDER BY billing_account_id, billing_record_msk_date
