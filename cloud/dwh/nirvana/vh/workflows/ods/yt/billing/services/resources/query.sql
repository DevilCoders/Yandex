PRAGMA yt.UseNativeYtTypes;
PRAGMA OrderedColumns;
PRAGMA Library("tables.sql");

IMPORT `tables` SYMBOLS $select_datatransfer_snapshot_table;

$cluster = {{ cluster -> table_quote() }};
$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_path = {{ input1 -> table_quote() }};
$offset = {{ param["transfer_offset"] }};

$snapshot = SELECT * FROM $select_datatransfer_snapshot_table($src_folder, $cluster, $offset);

$result = (
    SELECT
        id          as service_id,
        group,
        name,
        description
    FROM $snapshot
);

INSERT INTO $dst_path WITH TRUNCATE
SELECT * FROM $result
ORDER BY service_id;
