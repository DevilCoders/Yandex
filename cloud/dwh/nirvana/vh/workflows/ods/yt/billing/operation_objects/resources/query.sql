PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$result = (
    SELECT
        operation_id,
        object_type,
        object_id,
        object_role
    FROM $select_transfer_manager_table($src_folder, $cluster)
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY operation_id
