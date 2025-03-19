-- Input
$dm_yc_consumption = {{ param["dm_yc_consumption"] -> quote() }};

-- Output
$destination_table = {{input1 -> table_quote()}};


$infrastructure_consumption_raw = (
    SELECT
        billing_account_id,
        billing_account_name,
        crm_account_name,
        billing_account_usage_status,
        crm_architect_current,
        crm_account_owner_current,
        crm_segment,
        crm_segment_current,
        billing_account_is_var,
        billing_account_is_var_current,
        billing_account_is_subaccount,

        billing_record_msk_date,

        sku_service_name,
        sku_subservice_name,
        sku_service_name || '.' || sku_subservice_name          AS sku_service_subservice_name,
        sku_name,
        sku_pricing_unit,
        sku_lazy,
        billing_record_origin_service,
        billing_record_origin_subservice,
        sum(billing_record_pricing_quantity)                    AS billing_record_pricing_quantity,

        sum(billing_record_total_redistribution_rub_vat)        AS billing_record_total_redistribution_rub_vat,
        sum(billing_record_total_rub_vat)                       AS billing_record_total_rub_vat,
        sum(billing_record_credit_monetary_grant_rub_vat)       AS billing_record_credit_monetary_grant_rub_vat
    FROM $dm_yc_consumption
    WHERE
        sku_service_group = 'Infrastructure'
    GROUP BY
        billing_account_id,
        billing_account_name,
        crm_account_name,
        billing_account_usage_status,
        crm_architect_current,
        crm_account_owner_current,
        crm_segment,
        crm_segment_current,
        billing_account_is_var,
        billing_account_is_var_current,
        billing_account_is_subaccount,

        billing_record_msk_date,

        sku_service_name,
        sku_subservice_name,
        sku_name,
        sku_pricing_unit,
        sku_lazy,
        billing_record_origin_service,
        billing_record_origin_subservice
);

$billing_account_is_lazy = (
    SELECT
        billing_account_id,
        billing_record_msk_date,
        MIN (sku_lazy)                                          AS billing_account_is_lazy
    FROM $dm_yc_consumption
    GROUP BY
        billing_account_id,
        billing_record_msk_date
);

$infrastructure_consumption = (
    SELECT
        iaas_raw.billing_account_id                             AS billing_account_id,
        iaas_raw.billing_account_name                           AS billing_account_name,
        iaas_raw.crm_account_name                               AS crm_account_name,
        iaas_raw.billing_account_usage_status                   AS billing_account_usage_status,
        iaas_raw.crm_architect_current                          AS crm_architect_current,
        iaas_raw.crm_account_owner_current                      AS crm_account_owner_current,
        iaas_raw.crm_segment                                    AS crm_segment,
        iaas_raw.crm_segment_current                            AS crm_segment_current,
        iaas_raw.billing_account_is_var                         AS billing_account_is_var,
        iaas_raw.billing_account_is_var_current                 AS billing_account_is_var_current,
        iaas_raw.billing_account_is_subaccount                  AS billing_account_is_subaccount,
        ba_lazy.billing_account_is_lazy                         AS billing_account_is_lazy,

        iaas_raw.billing_record_msk_date                        AS billing_record_msk_date,

        iaas_raw.sku_service_name                               AS sku_service_name,
        iaas_raw.sku_subservice_name                            AS sku_subservice_name,
        iaas_raw.sku_service_subservice_name                    AS sku_service_subservice_name,
        iaas_raw.sku_name                                       AS sku_name,
        iaas_raw.sku_pricing_unit                               AS sku_pricing_unit,
        iaas_raw.sku_lazy                                       AS sku_lazy,
        iaas_raw.billing_record_origin_service                  AS billing_record_origin_service,
        iaas_raw.billing_record_origin_subservice               AS billing_record_origin_subservice,

        iaas_raw.billing_record_pricing_quantity                AS billing_record_pricing_quantity,
        iaas_raw.billing_record_total_redistribution_rub_vat    AS billing_record_total_redistribution_rub_vat,
        iaas_raw.billing_record_total_rub_vat                   AS billing_record_total_rub_vat,
        iaas_raw.billing_record_credit_monetary_grant_rub_vat   AS billing_record_credit_monetary_grant_rub_vat

    FROM $infrastructure_consumption_raw                        AS iaas_raw
    LEFT JOIN $billing_account_is_lazy                          AS ba_lazy
        ON iaas_raw.billing_account_id = ba_lazy.billing_account_id
            AND iaas_raw.billing_record_msk_date = ba_lazy.billing_record_msk_date
);


INSERT INTO $destination_table WITH TRUNCATE
    SELECT
        *
    FROM $infrastructure_consumption;
