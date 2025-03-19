PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime, $get_timestamp_from_days;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$toString = ($data) -> (CAST($data AS String));
$decode_utf8 = ($data) -> (nvl(cast(String::Base64StrictDecode($data) as Utf8),$data));

$get_timestamp = ($ts) -> (CAST($ts AS TimeStamp));
$from_utc_ts_to_msk_dt = ($ts) -> (DateTime::MakeDatetime(DateTime::Update(AddTimeZone($ts, "Europe/Moscow"), "GMT"  as Timezone)));

$result = (
    SELECT
        $toString(`assigned_user_id`)                                                                   AS assigned_user_id,
        $toString(`balance_id`)                                                                         AS crm_balance_id,
        `base_rate`                                                                                     AS base_rate,
        $toString(`billing_id`)                                                                         AS crm_billing_id,
        $toString(`category_id`)                                                                        AS crm_category_id,
        `cost_price`                                                                                    AS cost_price,
        `cost_usdollar`                                                                                 AS cost_usdollar,
        $toString(`created_by`)                                                                         AS created_by,
        $toString(`currency_id`)                                                                        AS crm_currency_id,
        $get_timestamp(`date_available`)                                                                AS date_available_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_available`))                                        AS date_available_dttm_local,
        $get_timestamp(`date_cost_price`)                                                               AS date_cost_price_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_cost_price`))                                       AS date_cost_price_dttm_local,
        $get_timestamp(`date_entered`)                                                                  AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                                          AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                                                 AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                                         AS date_modified_dttm_local,
        $get_timestamp($get_datetime(`date_modified_from_yt`))                                          AS date_modified_from_yt_ts,
        $from_utc_ts_to_msk_dt($get_datetime(`date_modified_from_yt`))                                  AS date_modified_from_yt_dttm_local,
        CAST(`deleted` AS bool)                                                                         AS deleted,
        $toString(`description`)                                                                        AS crm_product_template_description,
        `discount_price`                                                                                AS discount_price,
        `discount_usdollar`                                                                             AS discount_usdollar,
        $toString(`ext_category_id`)                                                                    AS ext_category_id,
        $toString(`id`)                                                                                 AS crm_product_template_id,
        `list_order`                                                                                    AS list_order,
        `list_price`                                                                                    AS list_price,
        `list_usdollar`                                                                                 AS list_usdollar,
        $toString(`modified_user_id`)                                                                   AS modified_user_id,
        $toString(`name`)                                                                               AS crm_product_template_name,
        `pricing_factor`                                                                                AS pricing_factor,
        $toString(`pricing_formula`)                                                                    AS pricing_formula,
        $toString(`product_name_en`)                                                                    AS product_name_en,
        $toString(`sku_id`)                                                                             AS crm_sku_id,
        $toString(`sku_name`)                                                                           AS crm_sku_name,
        $toString(`status`)                                                                             AS status,
        $toString(`tax_class`)                                                                          AS tax_class,
        $toString(`type_id`)                                                                            AS crm_type_id,
        $toString(`units`)                                                                              AS units

    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_product_template_id`;
