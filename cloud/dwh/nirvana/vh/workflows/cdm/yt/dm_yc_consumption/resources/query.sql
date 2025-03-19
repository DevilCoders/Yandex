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
$NOW = CurrentUtcDateTime();

-- Utils
$convert_amount_to_rub = ($amount, $rate) -> (IF($rate != $ONE, $amount * $rate, $amount));
$to_double = ($value) -> (CAST($value AS Double));
$is_mk8s = ($labels) -> (
    DictContains(
        Yson::LookupDict(Yson::ParseJson($labels, Yson::Options()), "user_labels"),
        "managed-kubernetes-cluster-id"
    )
);
$to_origin_service = ($labels) -> (IF( $is_mk8s($labels),'mk8s', 'undefined'));

-- Input
$ba_crm_tags_table = {{ param["ba_crm_tags_table"] -> quote() }};
$billing_conversion_rates_table = {{ param["billing_conversion_rates_table"] -> quote() }};
$billing_records_folder = {{ param["billing_records_folder"] -> quote() }};
$currency_rates_table = {{ param["currency_rates_table"] -> quote() }};
$services_table = {{ param["services_table"] -> quote() }};
$sku_tags_table = {{ param["sku_tags_table"] -> quote() }};
$sku_labels_table = {{ param["sku_labels_table"] -> quote() }};
$sku_pricing_versions_table = {{ param["sku_pricing_versions_table"] -> quote() }};
$skus_table = {{ param["skus_table"] -> quote() }};
$mdb_clusters = {{ param["mdb_clusters"] -> quote() }};
$mdb_subclusters = {{ param["mdb_subclusters"] -> quote() }};
$mdb_sku_dedicated_hosts_mapping = {{ param["mdb_sku_dedicated_hosts_mapping"] -> quote() }};
$mdb_hosts = {{ param["mdb_hosts"] -> quote() }};
$person_data = {{ param["person_data"] -> quote() }};

-- Output
$destination_path = {{ param["destination_path"] -> quote() }};


$mdb_dedic_hosts = (
        SELECT
            h.fqdn                                                      AS mdb_host_id,
            c.host_group_ids                                            AS mdb_host_group_id,
            String::ReplaceLast(NVL(type,'undefined'),'_cluster','')    AS mdb_host_group_type
        FROM $mdb_clusters AS c
        INNER JOIN $mdb_subclusters AS s ON c.cid = s.cid
        INNER JOIN $mdb_hosts AS h ON h.subcid = s.subcid
        WHERE c.host_group_ids IS NOT NULL
);

$mdb_sku_dedic_mdb_ids = (
        SELECT
            sku_dedic_compute.sku_id                        AS mdb_sku_dedic_compute_id,
            sku_mapping.mdb_sku_dedic_compute_name          AS mdb_sku_dedic_compute_name,
            sku_dedic_mdb.sku_id                            AS mdb_sku_dedic_mdb_id,
            sku_mapping.mdb_sku_dedic_mdb_name              AS mdb_sku_dedic_mdb_name
        FROM $mdb_sku_dedicated_hosts_mapping as sku_mapping
        INNER JOIN $skus_table  AS sku_dedic_compute  ON sku_mapping.mdb_sku_dedic_compute_name = sku_dedic_compute.name
        INNER JOIN $skus_table  AS sku_dedic_mdb      ON sku_mapping.mdb_sku_dedic_mdb_name = sku_dedic_mdb.name
        WHERE sku_mapping.end_dttm = CAST(Date('2099-12-31') AS DateTime)
);

$mdb_dedic_resources = (
    SELECT -- хост
        host.mdb_host_id                    AS mdb_dedic_resource_id,
        host.mdb_host_id                    AS mdb_host_id,
        host.mdb_host_group_id              AS mdb_host_group_id,
        host.mdb_host_group_type            AS mdb_host_group_type,
        NULL                                AS mdb_billing_record_origin_subservice,
        NULL                                AS mdb_billing_record_origin_service,
        sku.mdb_sku_dedic_mdb_id            AS mdb_sku_dedic_id,
        sku.mdb_sku_dedic_mdb_id            AS mdb_sku_dedic_mdb_id,
        sku.mdb_sku_dedic_mdb_name          AS mdb_sku_dedic_mdb_name,
        sku.mdb_sku_dedic_compute_id        AS mdb_sku_dedic_compute_id,
        sku.mdb_sku_dedic_compute_name      AS mdb_sku_dedic_compute_name,
        TRUE                                AS mdb_host_is_host,
        host.mdb_host_group_id || '#' || host.mdb_host_group_type || '#' || sku.mdb_sku_dedic_compute_id
                                            AS mdb_host_group_w_type_code,
        host.mdb_host_group_id || '#' || sku.mdb_sku_dedic_compute_id
                                            AS mdb_host_group_wo_type_code
    FROM $mdb_dedic_hosts AS host
    CROSS JOIN $mdb_sku_dedic_mdb_ids AS sku
    UNION ALL
    SELECT DISTINCT -- хост группа для новой разметки
        host.mdb_host_group_id              AS mdb_dedic_resource_id,
        NULL                                AS mdb_host_id,
        host.mdb_host_group_id              AS mdb_host_group_id,
        host.mdb_host_group_type            AS mdb_host_group_type,
        host.mdb_host_group_type            AS mdb_billing_record_origin_subservice,
        'mdb'                               AS mdb_billing_record_origin_service,
        sku.mdb_sku_dedic_compute_id        AS mdb_sku_dedic_id,
        NULL                                AS mdb_sku_dedic_mdb_id,
        NULL                                AS mdb_sku_dedic_mdb_name,
        sku.mdb_sku_dedic_compute_id        AS mdb_sku_dedic_compute_id,
        sku.mdb_sku_dedic_compute_name      AS mdb_sku_dedic_compute_name,
        FALSE                               AS mdb_host_is_host,
        host.mdb_host_group_id || '#' || host.mdb_host_group_type || '#' || sku.mdb_sku_dedic_compute_id
                                            AS mdb_host_group_w_type_code,
        host.mdb_host_group_id || '#' || sku.mdb_sku_dedic_compute_id
                                            AS mdb_host_group_wo_type_code
    FROM $mdb_dedic_hosts AS host
    CROSS JOIN $mdb_sku_dedic_mdb_ids AS sku
    UNION ALL
    SELECT DISTINCT -- хост группа для сохранения старого значения
        host.mdb_host_group_id              AS mdb_dedic_resource_id,
        NULL                                AS mdb_host_id,
        host.mdb_host_group_id              AS mdb_host_group_id,
        host.mdb_host_group_type            AS mdb_host_group_type,
        NULL                                AS mdb_billing_record_origin_subservice,
        NULL                                AS mdb_billing_record_origin_service,
        sku.mdb_sku_dedic_compute_id        AS mdb_sku_dedic_id,
        NULL                                AS mdb_sku_dedic_mdb_id,
        NULL                                AS mdb_sku_dedic_mdb_name,
        sku.mdb_sku_dedic_compute_id        AS mdb_sku_dedic_compute_id,
        sku.mdb_sku_dedic_compute_name      AS mdb_sku_dedic_compute_name,
        FALSE                               AS mdb_host_is_host,
        host.mdb_host_group_id || '#' || host.mdb_host_group_type || '#' || sku.mdb_sku_dedic_compute_id
                                            AS mdb_host_group_w_type_code,
        host.mdb_host_group_id || '#' || sku.mdb_sku_dedic_compute_id
                                            AS mdb_host_group_wo_type_code
    FROM $mdb_dedic_hosts AS host
    CROSS JOIN $mdb_sku_dedic_mdb_ids AS sku
);

$mdb_dedic_resources_dis = (
    SELECT DISTINCT mdb_dedic_resource_id FROM $mdb_dedic_resources
);

$billing_records = (
  SELECT
    billing_account_id                                    AS billing_account_id,
    billing_record_origin_service                         AS billing_record_origin_service,
    sku_id                                                AS sku_id,
    currency                                              AS billing_record_currency,
    SUM(cost)                                             AS billing_record_cost,
    SUM(credit)                                           AS billing_record_credit,
    SUM(expense)                                          AS billing_record_expense,
    SUM(expense - NVL(reward, $ZERO))                     AS billing_record_total,
    SUM(NVL(monetary_grant_credit, $ZERO))                AS billing_record_credit_monetary_grant,
    SUM(NVL(cud_credit, $ZERO))                           AS billing_record_credit_cud,
    SUM(NVL(volume_incentive_credit, $ZERO))              AS billing_record_credit_volume_incentive,
    SUM(NVL(trial_credit, $ZERO))                         AS billing_record_credit_trial,
    SUM(NVL(disabled_credit, $ZERO))                      AS billing_record_credit_disabled,
    SUM(NVL(service_credit, $ZERO))                       AS billing_record_credit_service,
    SUM(NVL(cud_compensated_pricing_quantity, $ZERO))     AS billing_record_cud_compensated_pricing_quantity,
    SUM(expense - NVL(reward, $ZERO))                     AS billing_record_real_consumption,
    SUM(pricing_quantity)                                 AS billing_record_pricing_quantity,
    NVL(SUM(reward), $ZERO)                               AS billing_record_var_reward,
    `date`                                                AS billing_record_msk_date,
    $get_vat_decimal_35_15($ONE, `date`, currency)        AS billing_record_vat,
    $format_msk_month_by_timestamp(SOME(end_time))        AS billing_record_msk_month,
    $format_msk_quarter_by_timestamp(SOME(end_time))      AS billing_record_msk_quarter,
    $format_msk_half_year_by_timestamp(SOME(end_time))    AS billing_record_msk_half_year,
    $format_msk_year_by_timestamp(SOME(end_time))         AS billing_record_msk_year,
    nvl(mdb_billing_record_origin_subservice,'undefined') AS billing_record_origin_subservice,
    mdb_host_group_has_dedic,
    CASE
        WHEN mdb_host_group_has_dedic AND mdb_billing_record_origin_service  IS NULL     AND sum(pricing_quantity) > $ZERO THEN $ONE-(mdb_dedic_pricing_quantity / SUM(pricing_quantity))
        WHEN mdb_host_group_has_dedic AND mdb_billing_record_origin_service  IS NOT NULL AND sum(pricing_quantity) > $ZERO THEN mdb_dedic_type_pricing_quantity / SUM(pricing_quantity)
        WHEN mdb_host_group_has_dedic AND mdb_billing_record_origin_service  IS NOT NULL THEN $ZERO
        ELSE $ONE
        END AS mdb_pricing_quantity_rate,
    CAST(MAX(end_time) AS Timestamp) AS billing_record_max_end_time
  FROM (
     SELECT
        br.* ,
        $to_origin_service(labels_json) AS billing_record_origin_service,
        NULL                            AS mdb_billing_record_origin_subservice,
        FALSE                           AS mdb_host_group_has_dedic,
        $ZERO                           AS mdb_dedic_type_pricing_quantity,
        $ZERO                           AS mdb_dedic_pricing_quantity,
        NULL                            AS mdb_billing_record_origin_service
    FROM RANGE($billing_records_folder) AS br
    WHERE `date` < '2022-03-14' OR `date` >= '2022-03-14' and br.resource_id NOT IN (
            SELECT mdb_dedic_resource_id FROM $mdb_dedic_resources_dis
        )
    UNION ALL
    SELECT
        br.* ,
        mdbr.mdb_billing_record_origin_subservice   AS mdb_billing_record_origin_subservice,
        mdbr.mdb_host_id IS NULL                    AS mdb_host_group_has_dedic,
        SUM(IF(mdbr.mdb_host_is_host, br.pricing_quantity, $ZERO)) OVER (PARTITION BY
            br.billing_account_id,
            br.`date`,
            br.currency,
            mdbr.mdb_host_group_w_type_code
            )                                       AS mdb_dedic_type_pricing_quantity,
        SUM(IF(mdbr.mdb_host_is_host, br.pricing_quantity, $ZERO)) OVER (PARTITION BY
            br.billing_account_id,
            br.`date`,
            br.currency,
            mdbr.mdb_host_group_wo_type_code
            )                                       AS mdb_dedic_pricing_quantity,
        mdbr.mdb_billing_record_origin_service      AS mdb_billing_record_origin_service
    FROM
    (SELECT
        br.* ,
        $to_origin_service(labels_json) as billing_record_origin_service,
    FROM RANGE($billing_records_folder,'2022-03-01') as br
    WHERE `date` >= '2022-03-14' and br.resource_id IN (
           SELECT mdb_dedic_resource_id FROM $mdb_dedic_resources_dis
       )
    ) AS br
    LEFT JOIN $mdb_dedic_resources AS mdbr on br.resource_id =  mdbr.mdb_dedic_resource_id AND br.sku_id = mdbr.mdb_sku_dedic_id
  )
  WHERE mdb_billing_record_origin_subservice IS NOT NULL AND mdb_dedic_type_pricing_quantity> 0 OR mdb_billing_record_origin_subservice IS NULL
  GROUP BY
    billing_account_id,
    nvl(mdb_billing_record_origin_service,billing_record_origin_service) AS billing_record_origin_service,
    currency,
    `date`,
    sku_id,
    mdb_host_group_has_dedic,
    mdb_billing_record_origin_subservice,
    mdb_dedic_pricing_quantity,
    mdb_dedic_type_pricing_quantity,
    mdb_billing_record_origin_service
);

$skus_inform = (
  SELECT
    skus.sku_id,
    skus.name,
    skus.pricing_unit,
    skus.translation_en,
    skus.translation_ru,
    String::SplitToList(skus.name,'.') AS sku_name_list,
    CASE WHEN (String::SplitToList(skus.name,'.'))[1] ='commitment' THEN 1 ELSE 0 END AS sku_is_commitment,
  FROM $skus_table AS skus
);

$enriched_sku_tags = (
  SELECT
    skus.sku_id             AS sku_id,
    skus.name               AS sku_name,
    skus.pricing_unit       AS sku_pricing_unit,
    skus.translation_en     AS sku_name_eng,
    skus.translation_ru     AS sku_name_rus,
    services.name           AS sku_service_name,
    services.description    AS sku_service_name_eng,
    services.group          AS sku_service_group,
    labels.subservice       AS sku_subservice_name,
    tags.sku_lazy           AS sku_lazy,
    skus.sku_is_commitment  AS sku_is_commitment,
    CASE WHEN sku_is_commitment=1 THEN String::ReplaceAll((ListConcat(ListSkip(sku_name_list, 3), "#")),'#standard#','#')
        ELSE
            ListConcat(sku_name_list, "#")
        END AS sku_commitment_join_key,
  FROM $skus_inform AS skus
  LEFT JOIN $sku_labels_table AS labels ON (skus.sku_id = labels.sku_id)
  LEFT JOIN $services_table AS services ON (labels.real_service_id = services.service_id)
  LEFT JOIN $sku_tags_table AS tags ON (skus.sku_id = tags.sku_id)
);

$billing_records_enriched_sku_tags = (
  SELECT
    billing_account_id,
    billing_record_origin_service,
    br.sku_id                                                                                                           AS sku_id,
    billing_record_cost * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                                   AS billing_record_cost,
    billing_record_credit * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                                 AS billing_record_credit,
    billing_record_expense * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                                AS billing_record_expense,
    billing_record_total * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                                  AS billing_record_total,
    billing_record_credit_monetary_grant * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                  AS billing_record_credit_monetary_grant,
    billing_record_credit_cud * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                             AS billing_record_credit_cud,
    billing_record_credit_volume_incentive * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                AS billing_record_credit_volume_incentive,
    billing_record_credit_trial * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                           AS billing_record_credit_trial,
    billing_record_credit_disabled * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                        AS billing_record_credit_disabled,
    billing_record_credit_service * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                         AS billing_record_credit_service,
    billing_record_cud_compensated_pricing_quantity * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)       AS billing_record_cud_compensated_pricing_quantity,
    billing_record_real_consumption * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                       AS billing_record_real_consumption,
    billing_record_pricing_quantity * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                       AS billing_record_pricing_quantity,
    billing_record_var_reward * IF(mdb_host_group_has_dedic,mdb_pricing_quantity_rate,$ONE)                             AS billing_record_var_reward,
    billing_record_msk_date,
    billing_record_vat                                                                                                  AS billing_record_vat,
    billing_record_msk_month,
    billing_record_msk_quarter,
    billing_record_msk_half_year,
    billing_record_msk_year,
    billing_record_origin_subservice,
    billing_record_currency,
    sku_name,
    sku_pricing_unit,
    sku_name_eng,
    sku_name_rus,
    sku_service_name,
    sku_service_name_eng,
    IF(br.billing_record_origin_service = 'mk8s', 'Kubernetes', sku_service_group)                                      AS sku_service_group,
    sku_subservice_name,
    sku_lazy,
    sku_tags.sku_is_commitment                                                                                          AS sku_is_commitment,
    sku_tags.sku_commitment_join_key                                                                                    AS sku_commitment_join_key,
    NVL(rates.quote, $ONE)                                                                                              AS billing_record_quote,
    billing_record_max_end_time
  FROM $billing_records AS br
  JOIN $enriched_sku_tags AS sku_tags ON br.sku_id=sku_tags.sku_id
  LEFT JOIN $currency_rates_table AS rates ON br.billing_record_currency=rates.currency AND br.billing_record_msk_date=rates.`date`
);

$crm_tags_with_currency = (
  SELECT
    crm.*,
    $get_vat_decimal_35_15($ONE, crm.`date`, crm.currency)        AS billing_account_vat,
    NVL(rates.quote, $ONE)                                        AS quote,
  FROM $ba_crm_tags_table AS crm
  LEFT JOIN $currency_rates_table AS rates USING (currency, `date`)
);

$person_names = (SELECT billing_account_id, name FROM $person_data);

$billing_currency = (
    SELECT
        multiplier,
        target_currency,
    FROM
        (SELECT
            A.*,
            NVL(LEAD(start_ts) OVER `w` ,CAST(Date('2099-12-31') AS DateTime))  - DateTime::IntervalFromMilliseconds(1)  AS end_ts
        FROM
            $billing_conversion_rates_table AS A
        WHERE source_currency = 'RUB'
        WINDOW w AS (
            PARTITION BY source_currency, target_currency
            ORDER BY start_ts
        )
        )
    WHERE $NOW BETWEEN start_ts AND end_ts
    UNION ALL
    SELECT
        CAST(1 AS Decimal(35,9))   AS multiplier
        ,'RUB'                     AS target_currency
);

$sku_cur_prices = (
SELECT
    aggregation_info_interval                                                   AS sku_cur_price_aggregation_info_interval,
    aggregation_info_level                                                      AS sku_cur_price_aggregation_info_level,
    start_pricing_quantity                                                      AS sku_cur_price_start_pricing_quantity,
    end_pricing_quantity                                                        AS sku_cur_price_end_pricing_quantity,
    spv.sku_id                                                                  AS sku_cur_price_sku_id,
    rates.target_currency                                                       AS sku_cur_price_currency,
    CAST(unit_price AS Decimal(35, 15)) * CAST(multiplier AS Decimal(35, 15))   AS sku_cur_price_unit_price,
FROM (SELECT * FROM $skus_table WHERE deprecated == false) AS skus
JOIN (
    SELECT
        A.*,
        ROW_NUMBER() OVER(PARTITION BY sku_id, start_time, start_pricing_quantity) AS RN
    FROM $sku_pricing_versions_table AS A
    WHERE $NOW BETWEEN start_time AND end_time
    ) AS spv ON skus.sku_id=spv.sku_id
CROSS JOIN $billing_currency AS rates
WHERE RN=1 -- на всякий случай
);

$sku_cur_prices_wo_rates = (
SELECT
    sku_cur_price_sku_id,
    sku_cur_price_currency,
    sku_cur_price_unit_price
FROM $sku_cur_prices
WHERE sku_cur_price_aggregation_info_level IS NULL
);

$billing_records_account_sku_day_agg = (
select
    sku_id,
    billing_account_id,
    billing_record_currency,
    billing_record_msk_date,
    billing_record_msk_month,
    sum(billing_record_pricing_quantity) as billing_record_pricing_quantity,
FROM $billing_records_enriched_sku_tags AS br
JOIN (SELECT DISTINCT sku_cur_price_sku_id FROM $sku_cur_prices WHERE sku_cur_price_aggregation_info_interval = 'month' AND sku_cur_price_aggregation_info_level = 'billing_account')  AS skus ON br.sku_id=skus.sku_cur_price_sku_id
WHERE br.billing_record_pricing_quantity <> $ZERO
GROUP BY br.sku_id as sku_id,
    br.billing_account_id as billing_account_id,
    br.billing_record_currency as billing_record_currency,
    br.billing_record_msk_date as billing_record_msk_date,
    br.billing_record_msk_month as billing_record_msk_month
);

$billing_records_account_sku_day_agg_w_running_sum = (
select
    sku_id,
    billing_account_id,
    billing_record_currency,
    billing_record_msk_date,
    billing_record_msk_month,
    billing_record_pricing_quantity ,
    sum(billing_record_pricing_quantity) OVER(PARTITION BY br.sku_id, br.billing_account_id,br.billing_record_currency, br.billing_record_msk_month ORDER BY br.billing_record_msk_date) - billing_record_pricing_quantity
                                                                                                                                            AS billing_record_pricing_quantity_running_sum_prev,
    sum(billing_record_pricing_quantity) OVER(PARTITION BY br.sku_id, br.billing_account_id,br.billing_record_currency, br.billing_record_msk_month ORDER BY br.billing_record_msk_date)
                                                                                                                                            AS billing_record_pricing_quantity_running_sum_cur
FROM $billing_records AS br
);

$sku_cur_prices_w_rates = (
SELECT
    sku_cur_price_sku_id,
    sku_cur_price_billing_account_id,
    sku_cur_price_currency,
    sku_cur_price_msk_date,
    sum((LEAST(br.billing_record_pricing_quantity_running_sum_cur,prc.sku_cur_price_end_pricing_quantity) -
        GREATEST(br.billing_record_pricing_quantity_running_sum_prev,prc.sku_cur_price_start_pricing_quantity)) *  prc.sku_cur_price_unit_price)
                                                                                                                                            AS sku_cur_price_unit_amt,
    sum(LEAST(br.billing_record_pricing_quantity_running_sum_cur,prc.sku_cur_price_end_pricing_quantity) -
        GREATEST(br.billing_record_pricing_quantity_running_sum_prev,prc.sku_cur_price_start_pricing_quantity))                             AS sku_cur_price_unit_quantity
FROM $sku_cur_prices as prc
JOIN $billing_records_account_sku_day_agg_w_running_sum as br
    ON prc.sku_cur_price_sku_id = br.sku_id
    AND prc.sku_cur_price_currency = br.billing_record_currency
where   br.billing_record_pricing_quantity_running_sum_cur    >= prc.sku_cur_price_start_pricing_quantity
    and br.billing_record_pricing_quantity_running_sum_prev   <= prc.sku_cur_price_end_pricing_quantity
GROUP BY
    br.sku_id                                                                                                                               AS sku_cur_price_sku_id,
    br.billing_account_id                                                                                                                   AS sku_cur_price_billing_account_id,
    br.billing_record_currency                                                                                                              AS sku_cur_price_currency,
    br.billing_record_msk_date                                                                                                              AS sku_cur_price_msk_date
);

$billing_records_enriched_crm_tags = (
  SELECT
    br.*,
    NVL(billing_account_name          ,'UNKNOWN')                                                                                           AS billing_account_name,
    NVL(billing_account_type          ,'UNKNOWN')                                                                                           AS billing_account_type,
    NVL(billing_account_type_current  ,'UNKNOWN')                                                                                           AS billing_account_type_current,
    NVL(billing_master_account_id     ,'UNKNOWN')                                                                                           AS billing_master_account_id,
    NVL(billing_master_account_name   ,'UNKNOWN')                                                                                           AS billing_master_account_name,
    NVL(billing_account_month_cohort  ,'UNKNOWN')                                                                                           AS billing_account_month_cohort,
    NVL(crm.currency,br.billing_record_currency)                                                                                            AS billing_account_currency,
    is_subaccount                                                                                                                           AS billing_account_is_subaccount,
    NVL(country_code                  ,'UNKNOWN')                                                                                           AS billing_account_country_code,
    NVL(person_type_current           ,'UNKNOWN')                                                                                           AS billing_account_person_type_current,
    NVL(person_type                   ,'UNKNOWN')                                                                                           AS billing_account_person_type,
    NVL(usage_status_current          ,'UNKNOWN')                                                                                           AS billing_account_usage_status_current,
    NVL(usage_status                  ,'UNKNOWN')                                                                                           AS billing_account_usage_status,
    NVL(state_current                 ,'UNKNOWN')                                                                                           AS billing_account_state_current,
    NVL(state                         ,'UNKNOWN')                                                                                           AS billing_account_state,
    is_isv_current                                                                                                                          AS billing_account_is_isv_current,
    is_isv                                                                                                                                  AS billing_account_is_isv,
    is_var_current                                                                                                                          AS billing_account_is_var_current,
    is_var                                                                                                                                  AS billing_account_is_var,
    is_fraud_current                                                                                                                        AS billing_account_is_fraud_current,
    is_fraud                                                                                                                                AS billing_account_is_fraud,
    is_suspended_by_antifraud                                                                                                               AS billing_account_is_suspended_by_antifraud,
    is_suspended_by_antifraud_current                                                                                                       AS billing_account_is_suspended_by_antifraud_current,
    NVL(payment_type_current          ,'UNKNOWN')                                                                                           AS billing_account_payment_type_current,
    NVL(payment_type                  ,'UNKNOWN')                                                                                           AS billing_account_payment_type,
    COALESCE (String::ReplaceAll(crm_account_name, '\"', "\'"), person.name)                                                                AS account_display_name,
    NVL(crm.crm_account_id            ,'UNKNOWN')                                                                                           AS crm_account_id,
    NVL(crm.crm_account_name          ,'UNKNOWN')                                                                                           AS crm_account_name,
    NVL(crm.segment_current           ,'UNKNOWN')                                                                                           AS crm_segment_current,
    NVL(crm.segment                   ,'UNKNOWN')                                                                                           AS crm_segment,
    NVL(crm.crm_account_tags          ,'UNKNOWN')                                                                                           AS crm_account_tags,
    NVL(crm.crm_account_dimensions    ,'UNKNOWN')                                                                                           AS crm_account_dimensions,
    NVL(crm.crm_industry              ,'UNKNOWN')                                                                                           AS crm_industry,
    NVL(crm.crm_sub_industry          ,'UNKNOWN')                                                                                           AS crm_sub_industry,
    NVL(crm.account_owner_current     ,'UNKNOWN')                                                                                           AS crm_account_owner_current,
    NVL(crm.account_owner             ,'UNKNOWN')                                                                                           AS crm_account_owner,
    NVL(crm.architect_current         ,'UNKNOWN')                                                                                           AS crm_architect_current,
    NVL(crm.architect                 ,'UNKNOWN')                                                                                           AS crm_architect,
    NVL(crm.bus_dev_current           ,'UNKNOWN')                                                                                           AS crm_bus_dev_current,
    NVL(crm.bus_dev                   ,'UNKNOWN')                                                                                           AS crm_bus_dev,
    NVL(crm.partner_manager_current   ,'UNKNOWN')                                                                                           AS crm_partner_manager_current,
    NVL(crm.partner_manager           ,'UNKNOWN')                                                                                           AS crm_partner_manager,
    NVL(crm.tam_current               ,'UNKNOWN')                                                                                           AS crm_tam_current,
    NVL(crm.tam                       ,'UNKNOWN')                                                                                           AS crm_tam,
    NVL(crm.sales_current             ,'UNKNOWN')                                                                                           AS crm_sales_current,
    NVL(crm.sales                     ,'UNKNOWN')                                                                                           AS crm_sales,
    NVL(crm.quote, billing_record_quote)                                                                                                    AS quote,
    NVL(billing_account_vat, br.billing_record_vat)                                                                                         AS billing_account_vat,

    IF(br.billing_record_cud_compensated_pricing_quantity>0 ,br.billing_record_cud_compensated_pricing_quantity / SUM(br.billing_record_cud_compensated_pricing_quantity) over w1, $ZERO)
                                                                                                                                            AS billing_record_total_redistribution_consumption_prc,
    SUM(IF(br.sku_is_commitment = 1, br.billing_record_total, $ZERO)) over w1                                                               AS billing_record_total_redistribution_consumption_sum,
    CASE
        WHEN br.sku_is_commitment = 0 THEN br.billing_record_total
        WHEN SUM(br.billing_record_cud_compensated_pricing_quantity) over w1 = 0 THEN br.billing_record_total
        ELSE $ZERO
      END                                                                                                                                   AS billing_record_total_redistribution_consumption_main
  FROM $billing_records_enriched_sku_tags AS br
  LEFT JOIN $crm_tags_with_currency AS crm
    ON br.billing_record_msk_date = crm.`date` AND br.billing_account_id = crm.billing_account_id
  LEFT JOIN $person_names AS person
    ON br.billing_account_id = person.billing_account_id
  WINDOW w1 AS (PARTITION BY br.billing_account_id,br.billing_record_msk_date,br.sku_commitment_join_key)
);


INSERT INTO $destination_path WITH TRUNCATE
SELECT
    billing_account_id,
    billing_record_origin_service,
    sku_id,
    $to_double(billing_record_cud_compensated_pricing_quantity) AS billing_record_cud_compensated_pricing_quantity,
    billing_record_msk_date,
    billing_record_msk_month,
    billing_record_msk_quarter,
    billing_record_msk_half_year,
    billing_record_msk_year,
    billing_record_origin_subservice,
    billing_record_currency,
    sku_name,
    sku_pricing_unit,
    sku_name_eng,
    sku_name_rus,
    sku_service_name,
    sku_service_name_eng,
    sku_service_group,
    sku_subservice_name,
    sku_lazy,
    billing_account_name,
    billing_account_type,
    billing_account_type_current,
    billing_master_account_id,
    billing_master_account_name,
    billing_account_month_cohort,
    billing_account_currency,
    billing_account_is_subaccount,
    billing_account_country_code,
    billing_account_person_type_current,
    billing_account_person_type,
    billing_account_usage_status_current,
    billing_account_usage_status,
    billing_account_state_current,
    billing_account_state,
    billing_account_is_isv_current,
    billing_account_is_isv,
    billing_account_is_var_current,
    billing_account_is_var,
    billing_account_is_fraud_current,
    billing_account_is_fraud,
    billing_account_is_suspended_by_antifraud,
    billing_account_is_suspended_by_antifraud_current,
    billing_account_payment_type_current,
    billing_account_payment_type,
    account_display_name,
    crm_account_id,
    crm_account_name,
    crm_segment_current,
    crm_segment,
    crm_account_tags,
    crm_account_dimensions,
    crm_industry,
    crm_sub_industry,
    crm_account_owner_current,
    crm_account_owner,
    crm_architect_current,
    crm_architect,
    crm_bus_dev_current,
    crm_bus_dev,
    crm_partner_manager_current,
    crm_partner_manager,
    crm_tam_current,
    crm_tam,
    crm_sales_current,
    crm_sales,
    $to_double(quote)                                                                                                                       AS billing_record_currency_exchange_rate,
    $to_double(billing_account_vat)                                                                                                         AS billing_account_vat,
    $to_double(billing_record_pricing_quantity)                                                                                             AS billing_record_pricing_quantity,
    $to_double(billing_record_cost)                                                                                                         AS billing_record_cost,
    $to_double($convert_amount_to_rub(billing_record_cost, quote))                                                                          AS billing_record_cost_rub,
    $to_double($convert_amount_to_rub(billing_record_cost, quote) * billing_account_vat)
                                                                                                                                            AS billing_record_cost_rub_vat,
    $to_double(billing_record_credit)                                                                                                       AS billing_record_credit,
    $to_double($convert_amount_to_rub(billing_record_credit, quote))                                                                        AS billing_record_credit_rub,
    $to_double($convert_amount_to_rub(billing_record_credit, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_credit_rub_vat,
    $to_double(billing_record_credit_monetary_grant)                                                                                        AS billing_record_credit_monetary_grant,
    $to_double($convert_amount_to_rub(billing_record_credit_monetary_grant, quote))                                                         AS billing_record_credit_monetary_grant_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_monetary_grant, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_credit_monetary_grant_rub_vat,
    $to_double(billing_record_credit_cud)                                                                                                   AS billing_record_credit_cud,
    $to_double($convert_amount_to_rub(billing_record_credit_cud, quote))                                                                    AS billing_record_credit_cud_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_cud, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_credit_cud_rub_vat,
    $to_double(billing_record_credit_volume_incentive)                                                                                      AS billing_record_credit_volume_incentive,
    $to_double($convert_amount_to_rub(billing_record_credit_volume_incentive, quote))                                                       AS billing_record_credit_volume_incentive_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_volume_incentive, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_credit_volume_incentive_rub_vat,
    $to_double(billing_record_credit_trial)                                                                                                 AS billing_record_credit_trial,
    $to_double($convert_amount_to_rub(billing_record_credit_trial, quote))                                                                  AS billing_record_credit_trial_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_trial, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_credit_trial_rub_vat,
    $to_double(billing_record_credit_disabled)                                                                                              AS billing_record_credit_disabled,
    $to_double($convert_amount_to_rub(billing_record_credit_disabled, quote))                                                               AS billing_record_credit_disabled_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_disabled, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_credit_disabled_rub_vat,
    $to_double(billing_record_credit_service)                                                                                               AS billing_record_credit_service,
    $to_double($convert_amount_to_rub(billing_record_credit_service, quote))                                                                AS billing_record_credit_service_rub,
    $to_double($convert_amount_to_rub(billing_record_credit_service, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_credit_service_rub_vat,
    $to_double(billing_record_total)                                                                                                        AS billing_record_total,
    $to_double($convert_amount_to_rub(billing_record_total, quote))                                                                         AS billing_record_total_rub,
    $to_double($convert_amount_to_rub(billing_record_total, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_total_rub_vat,
    $to_double(billing_record_total_redistribution_consumption_main + billing_record_total_redistribution_consumption_sum*billing_record_total_redistribution_consumption_prc)
                                                                                                                                            AS billing_record_total_redistribution,
    $to_double($convert_amount_to_rub(billing_record_total_redistribution_consumption_main + billing_record_total_redistribution_consumption_sum*billing_record_total_redistribution_consumption_prc, quote))
                                                                                                                                            AS billing_record_total_redistribution_rub,
    $to_double($convert_amount_to_rub(billing_record_total_redistribution_consumption_main + billing_record_total_redistribution_consumption_sum*billing_record_total_redistribution_consumption_prc, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_total_redistribution_rub_vat,
    $to_double(billing_record_expense)                                                                                                      AS billing_record_expense,
    $to_double($convert_amount_to_rub(billing_record_expense, quote))                                                                       AS billing_record_expense_rub,
    $to_double($convert_amount_to_rub(billing_record_expense, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_expense_rub_vat,
    $to_double(billing_record_var_reward)                                                                                                   AS billing_record_var_reward,
    $to_double($convert_amount_to_rub(billing_record_var_reward, quote))                                                                    AS billing_record_var_reward_rub,
    $to_double($convert_amount_to_rub(billing_record_var_reward, quote) *  billing_account_vat)
                                                                                                                                            AS billing_record_var_reward_rub_vat,
    -- DEPRECATED
    $to_double(billing_record_vat)                                                                                                          AS billing_record_vat,
    $to_double(billing_record_real_consumption)                                                                                             AS billing_record_real_consumption,
    $to_double($convert_amount_to_rub(billing_record_real_consumption, quote))                                                              AS billing_record_real_consumption_rub,
    $to_double($convert_amount_to_rub(billing_record_real_consumption, quote) *  billing_account_vat)                                       AS billing_record_real_consumption_rub_vat,

    -- TEST
    CASE
        WHEN br.sku_is_commitment = 0 AND billing_account_usage_status not in ('trial','service') THEN $to_double(NANVL(NVL((sku_cur_prices_w_rates.sku_cur_price_unit_amt / sku_cur_prices_w_rates.sku_cur_price_unit_quantity), sku_cur_prices_wo_rates.sku_cur_price_unit_price, $ZERO), sku_cur_prices_wo_rates.sku_cur_price_unit_price) * br.billing_record_pricing_quantity)
        ELSE $to_double(0)
      END                                                                                                                                   AS price_list_consumption__alpha,
    CASE
        WHEN br.sku_is_commitment = 0 AND billing_account_usage_status not in ('trial','service') THEN $to_double($convert_amount_to_rub(NANVL(NVL((sku_cur_prices_w_rates.sku_cur_price_unit_amt / sku_cur_prices_w_rates.sku_cur_price_unit_quantity), sku_cur_prices_wo_rates.sku_cur_price_unit_price, $ZERO), sku_cur_prices_wo_rates.sku_cur_price_unit_price) *  br.billing_record_pricing_quantity, br.quote))
        ELSE $to_double(0)
      END                                                                                                                                   AS price_list_consumption_rub__alpha,
    CASE
        WHEN br.sku_is_commitment = 0 AND billing_account_usage_status not in ('trial','service') THEN $to_double($convert_amount_to_rub(NANVL(NVL((sku_cur_prices_w_rates.sku_cur_price_unit_amt / sku_cur_prices_w_rates.sku_cur_price_unit_quantity), sku_cur_prices_wo_rates.sku_cur_price_unit_price, $ZERO), sku_cur_prices_wo_rates.sku_cur_price_unit_price) *  br.billing_record_pricing_quantity, quote) *  br.billing_account_vat)
        ELSE $to_double(0)
      END                                                                                                                                   AS price_list_consumption_rub_vat__alpha,
    billing_record_max_end_time
  WITHOUT
    br.billing_account_vat,
    br.billing_record_vat,
    br.billing_record_expense,
    br.billing_record_pricing_quantity,
    br.billing_record_real_consumption,
    br.billing_record_cost,
    br.billing_record_credit,
    br.billing_record_total,
    br.billing_record_var_reward,
    br.billing_record_credit_cud,
    br.billing_record_credit_disabled,
    br.billing_record_credit_monetary_grant,
    br.billing_record_credit_service,
    br.billing_record_credit_trial,
    br.billing_record_credit_volume_incentive,
    br.billing_record_quote,
    br.billing_record_total_redistribution_consumption_prc,
    br.billing_record_total_redistribution_consumption_sum,
    br.billing_record_total_redistribution_consumption_main
FROM $billing_records_enriched_crm_tags AS br
  LEFT JOIN $sku_cur_prices_wo_rates    AS sku_cur_prices_wo_rates ON br.sku_id=sku_cur_prices_wo_rates.sku_cur_price_sku_id AND br.billing_account_currency = sku_cur_prices_wo_rates.sku_cur_price_currency
  LEFT JOIN $sku_cur_prices_w_rates     AS sku_cur_prices_w_rates  ON br.sku_id=sku_cur_prices_w_rates.sku_cur_price_sku_id  AND br.billing_account_currency = sku_cur_prices_w_rates.sku_cur_price_currency AND br.billing_account_id=sku_cur_prices_w_rates.sku_cur_price_billing_account_id AND br.billing_record_msk_date=sku_cur_prices_w_rates.sku_cur_price_msk_date
ORDER BY billing_record_msk_date, billing_account_id;
