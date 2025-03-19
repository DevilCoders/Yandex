PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $to_str, $yson_to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_str(committed_use_discount_id)                                  AS committed_use_discount_id,
    $to_str(subscription_id)                                            AS subscription_id,
    Yson::ConvertToString(`__dummy`, Yson::Options(false as Strict))    AS `__dummy`
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY committed_use_discount_id, subscription_id
