PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `tables` SYMBOLS $select_datatransfer_snapshot_table;

$cluster = {{ cluster -> table_quote() }};
$source_folder_path = {{ param["source_folder_path"] -> quote() }};
$destination_path = {{ input1 -> table_quote() }};
$offset = {{ param["transfer_offset"] }};

$snapshot = SELECT * FROM $select_datatransfer_snapshot_table($source_folder_path, $cluster, $offset);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
    sku_id                    AS sku_id,
    real_service_id           AS real_service_id,
    subservice                AS subservice,
    visibility                AS visibility,
    updated_by                AS updated_by,
    $get_datetime(created_at) AS created_at,
    $get_datetime(updated_at) AS updated_at,
FROM $snapshot
ORDER BY sku_id;
