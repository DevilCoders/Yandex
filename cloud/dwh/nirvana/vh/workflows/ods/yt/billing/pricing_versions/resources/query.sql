PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_datatransfer_snapshot_table;
IMPORT `datetime` SYMBOLS $get_datetime, $DATETIME_INF;
IMPORT `numbers` SYMBOLS $to_decimal_35_15, $DECIMAL_35_15_INF, $DECIMAL_35_15_EPS;
IMPORT `helpers` SYMBOLS $convert_to_optional;

$cluster = {{ cluster -> table_quote() }};
$src_folder = {{ param["source_folder_path"] -> quote() }};
$offset = {{ param["transfer_offset"] }};
$dst_table = {{ input1 -> table_quote() }};

$snapshot = SELECT * FROM $select_datatransfer_snapshot_table($src_folder, $cluster, $offset);

/* Make transformations */
$pricing_versions_from_sku =
    SELECT
        `id` AS `sku_id`,
        `pricing_unit`,
        $convert_to_optional(Yson::ConvertToList(`pricing_versions`)) as `pricing_versions`
    FROM $snapshot;

$pricing_versions_unpacked_by_time =
SELECT
    `sku_id`,
    `pricing_unit`,
-- effective_time field in pricing_version ingested with as Yson (Double)
-- It happens due to ingestion via Data Transfer (it has it's own conversion rules)
    NVL(
        CAST(Yson::LookupString(`pricing_versions`, 'effective_time', Yson::Options(false as Strict)) AS Int64),
        CAST(Yson::LookupDouble(`pricing_versions`, 'effective_time') AS Uint64)
    ) AS `effective_time`,
    $convert_to_optional(Yson::ConvertToList(`pricing_versions`["pricing_expression"]["rates"])) AS `rates`,
    Yson::YPathString(`pricing_versions`, '/aggregation_info/interval') as aggregation_info_interval,
    Yson::YPathString(`pricing_versions`, '/aggregation_info/level') as aggregation_info_level
FROM (
    SELECT *
    FROM $pricing_versions_from_sku
    FLATTEN BY `pricing_versions`
);


$pricing_versions_updated_start_end_time =
SELECT
    `sku_id`,
    `pricing_unit`,
    $get_datetime(`effective_time`) AS `start_time`,
    COALESCE($get_datetime(LEAD(`effective_time`, 1) OVER `w` - 1), $DATETIME_INF) AS `end_time`,
    ListEnumerate(`rates`) AS `rates_with_ids`,
    `aggregation_info_interval`,
    `aggregation_info_level`
FROM $pricing_versions_unpacked_by_time
WINDOW `w` AS (
    PARTITION BY `sku_id`
    ORDER BY `sku_id`, `effective_time`
);

$pricing_versions_unpacked_by_rate =
SELECT
    `sku_id`,
    `pricing_unit`,
    `start_time`,
    `end_time`,
    CAST(`rates_with_ids`.0 AS UInt32) AS `rate_id`,
    $to_decimal_35_15(Yson::ConvertToString(`rates_with_ids`.1["start_pricing_quantity"])) AS `start_pricing_quantity`,
    $to_decimal_35_15(Yson::ConvertToString(`rates_with_ids`.1["unit_price"])) AS `unit_price`,
    `aggregation_info_interval`,
    `aggregation_info_level`
FROM (
    SELECT *
    FROM $pricing_versions_updated_start_end_time
    FLATTEN BY `rates_with_ids`
);

$pricing_versions_updated_end_price =
SELECT
    `sku_id`,
    `pricing_unit`,
    `start_time`,
    `end_time`,
    `rate_id`,
    `start_pricing_quantity`,
    COALESCE(LEAD(`start_pricing_quantity`) OVER `w` - $DECIMAL_35_15_EPS, $DECIMAL_35_15_INF) AS `end_pricing_quantity`,
    `unit_price`,
    `aggregation_info_interval`,
    `aggregation_info_level`
FROM $pricing_versions_unpacked_by_rate
WINDOW `w` AS (
    PARTITION BY `sku_id`, `start_time`
    ORDER BY `sku_id`, `start_time`, `rate_id`
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $pricing_versions_updated_end_price
ORDER BY `sku_id`, `start_time`, `rate_id`;
