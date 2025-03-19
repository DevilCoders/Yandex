PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_msk_datetime, $DATETIME_INF, $MSK_TIMEZONE;
IMPORT `numbers` SYMBOLS $to_decimal_35_15, $DECIMAL_35_15_INF, $DECIMAL_35_15_EPS;

$msk_inf = AddTimeZone($DATETIME_INF, $MSK_TIMEZONE);
$cluster = {{cluster->table_quote()}};

/* Logfeller's log */
$src_folder = {{ param["source_folder_path"]->quote() }};
$billing_accounts_table = {{ param["billing_accounts_table"]->quote() }};
$dst_table = {{input1->table_quote()}};

/* Aggregate snapshot */
$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

/* Make transformations */
$overrides = (
    SELECT
        `billing_account_id`                                AS `billing_account_id`,
        `sku_id`                                            AS `sku_id`,
        Yson::ConvertToList(`expirable_pricing_versions`)   AS `expirable_pricing_versions`,
        `local_currency`                                    AS `local_currency`
    FROM $snapshot
);

$overrides_with_currency = (
    SELECT
        overrides.`billing_account_id`                                      AS billing_account_id,
        overrides.`sku_id`                                                  AS sku_id,
        overrides.`expirable_pricing_versions`                              AS expirable_pricing_versions,
        IF (overrides.`local_currency`, billing_accounts.`currency`, "RUB") AS currency
    FROM $overrides as overrides
    LEFT JOIN $billing_accounts_table as billing_accounts
    USING (`billing_account_id`)
);

$overrides_unpacked_by_time = (
    SELECT
        `billing_account_id`,
        `sku_id`,
        `currency`,
    -- effective_time field in pricing_version ingested with as Yson (Double)
    -- It happens due to ingestion via Data Transfer (it has it's own conversion rules)
        $get_msk_datetime(CAST(Yson::LookupString(`expirable_pricing_versions`, 'effective_time', Yson::Options(true as AutoConvert) ) AS Uint64))                  AS `start_time_msk`,
        NVL($get_msk_datetime(CAST(Yson::LookupString(`expirable_pricing_versions`, 'expiration_time', Yson::Options(true as AutoConvert)) AS Uint64)), $msk_inf)   AS `end_time_msk`,
        ListEnumerate(Yson::ConvertToList(`expirable_pricing_versions`["pricing_expression"]["rates"]))                                                             AS `rates_with_ids`,
        Yson::YPathString(`expirable_pricing_versions`, '/aggregation_info/interval')                                                                               AS aggregation_info_interval,
        Yson::YPathString(`expirable_pricing_versions`, '/aggregation_info/level')                                                                                  AS aggregation_info_level
    FROM (
        SELECT *
        FROM $overrides_with_currency
        FLATTEN BY `expirable_pricing_versions`
    )
);

$sku_overrides_unpacked_by_rate =
SELECT
    `billing_account_id`,
    `sku_id`,
    `currency`,
    `start_time_msk`,
    `end_time_msk`,
    CAST(`rates_with_ids`.0 AS UInt32) AS `rate_id`,
    $to_decimal_35_15(Yson::ConvertToString(`rates_with_ids`.1["start_pricing_quantity"])) AS `start_pricing_quantity`,
    $to_decimal_35_15(Yson::ConvertToString(`rates_with_ids`.1["unit_price"])) AS `unit_price`,
    `aggregation_info_interval`,
    `aggregation_info_level`
FROM (
    SELECT *
    FROM $overrides_unpacked_by_time
    FLATTEN BY `rates_with_ids`
);

$overrides_updated_end_price =
SELECT
    `billing_account_id`,
    `sku_id`,
    `currency`,
    `start_time_msk`,
    `end_time_msk`,
    `rate_id`,
    `start_pricing_quantity`,
    COALESCE(LEAD(`start_pricing_quantity`) OVER `w` - $DECIMAL_35_15_EPS, $DECIMAL_35_15_INF) AS `end_pricing_quantity`,
    `unit_price`,
    `aggregation_info_interval`,
    `aggregation_info_level`
FROM $sku_overrides_unpacked_by_rate
WINDOW `w` AS (
    PARTITION BY `billing_account_id`, `sku_id`, `start_time_msk`
    ORDER BY `billing_account_id`, `sku_id`, `start_time_msk`, `rate_id`
);


/* Save result */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $overrides_updated_end_price
ORDER BY `billing_account_id`, `sku_id`, `start_time_msk`, `rate_id`;
