PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");

IMPORT `tables`   SYMBOLS $select_datatransfer_snapshot_table;
IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `datetime` SYMBOLS $get_date;
IMPORT `numbers`  SYMBOLS $to_decimal_35_15;

$src_folder = {{ param["source_folder_path"] -> quote() }};
$cluster = {{cluster -> table_quote()}};
$offset = {{ param["transfer_offset"] }};
$dst_table = {{ input1 -> table_quote() }};

$snapshot = SELECT * FROM $select_datatransfer_snapshot_table($src_folder, $cluster, $offset);

$result = (
    SELECT
        `billing_account_id`                    AS `billing_account_id`,
        `sku_id`                                AS `sku_id`,
        $get_date(`date`)                       AS `date`,
        $to_decimal_35_15(`pricing_quantity`)   AS `pricing_quantity`,
        $to_decimal_35_15(`cost`)               AS `cost`,
        $to_decimal_35_15(`credit`)             AS `credit`,
        $get_datetime(`created_at`)             AS `created_at`
    FROM $snapshot
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `billing_account_id`, `date`;
