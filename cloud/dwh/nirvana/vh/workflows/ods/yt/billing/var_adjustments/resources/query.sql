PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_date;
IMPORT `numbers` SYMBOLS $to_decimal_35_9;
IMPORT `helpers` SYMBOLS $to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$toString = ($utf8) -> (CAST($utf8 AS String));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $to_str(billing_account_id)                         AS billing_account_id,
    $get_date(`date`)                                   AS `date`,
    $to_decimal_35_9(amount)                            AS amount,
    $to_str(created_by)                                 AS created_by
FROM $select_transfer_manager_table($src_folder, $cluster)
ORDER BY billing_account_id, `date`
