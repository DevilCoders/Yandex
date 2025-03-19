USE hahn;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

$res_table_path = '//home/cloud_analytics/ml/capacity_planning/fip4/historical_data';
$rep_date = DateTime::Format("%Y-%m-%d")(CurrentUTCTimeStamp());

$consumption =
    SELECT
        billing_record_msk_date,
        billing_account_id,
        crm_segment,
        billing_record_pricing_quantity,
        billing_account_state,
        billing_account_usage_status,
        billing_record_real_consumption_rub_vat
    FROM `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption`
    WHERE sku_name IN ('network.public_fips', 'network.public_fips.lb', 'mdb.cluster.mongo.public_ip', 
                       'mdb.cluster.clickhouse.public_ip', 'mdb.cluster.mysql.public_ip', 'mdb.cluster.postgresql.public_ip')
    AND billing_record_msk_date < $rep_date
    AND billing_record_msk_date >= '2020-04-01'
    AND billing_account_is_suspended_by_antifraud_current = False
    AND billing_account_state IN ('active', 'payment_required', 'suspended')
    AND billing_account_usage_status IN ('trial', 'paid', 'service')
;

$res_table =
    SELECT
        billing_record_msk_date AS rep_date,
        crm_segment,
        billing_account_state,
        billing_account_usage_status,
        sum(billing_record_pricing_quantity)/24 AS ip_count,
        sum(billing_record_real_consumption_rub_vat) AS real_consumption_vat
    FROM $consumption
    GROUP BY billing_record_msk_date, crm_segment, billing_account_state, billing_account_usage_status
;

INSERT INTO $res_table_path WITH TRUNCATE 
    SELECT
        rep_date,
        crm_segment,
        billing_account_state,
        billing_account_usage_status,
        Math::Round(ip_count, -2) AS ip_count,
        Math::Round(real_consumption_vat, -2) AS real_consumption_vat
    FROM $res_table
    ORDER BY rep_date, crm_segment, billing_account_state, billing_account_usage_status
;
