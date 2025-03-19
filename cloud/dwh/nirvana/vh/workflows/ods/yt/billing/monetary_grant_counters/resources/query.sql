PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `numbers` SYMBOLS $to_double;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$result = (
    SELECT
        CAST(billing_account_id as String)        AS billing_account_id,
        CAST(monetary_grant_id as String)         AS monetary_grant_id,
        $get_datetime(updated_at)                 AS updated_at,
        $to_double(value)                           AS value
    FROM $select_transfer_manager_table($src_folder, $cluster)
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY billing_account_id, monetary_grant_id, updated_at
