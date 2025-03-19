PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_msk_datetime;

$cluster = {{ cluster -> table_quote() }};
$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$bindings_snapshot = (
SELECT
    resource_id,
    role_slug,
    subject_id,
    id,
    is_system,
    resource_cloud_id,
    resource_type,
    subject_type,
    $get_msk_datetime(modified_at)  AS modified_at_msk,
    managed_by,
    top_level_resource_id,
    top_level_resource_type
FROM $select_transfer_manager_table($src_folder, $cluster)
);

$result = (
SELECT
    resource_id,
    role_slug,
    subject_id,
    id,
    is_system,
    resource_cloud_id,
    resource_type,
    subject_type,
    modified_at_msk,
    managed_by,
    top_level_resource_id,
    top_level_resource_type
FROM $dst_table
WHERE id NOT IN (SELECT id FROM $bindings_snapshot)
UNION ALL
SELECT
    resource_id,
    role_slug,
    subject_id,
    id,
    is_system,
    resource_cloud_id,
    resource_type,
    subject_type,
    modified_at_msk,
    managed_by,
    top_level_resource_id,
    top_level_resource_type
FROM $bindings_snapshot
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    resource_id,
    role_slug,
    subject_id,
    id,
    is_system,
    resource_cloud_id,
    resource_type,
    subject_type,
    modified_at_msk,
    managed_by,
    top_level_resource_id,
    top_level_resource_type
FROM
    $result
ORDER BY
    resource_id, role_slug, subject_id;
