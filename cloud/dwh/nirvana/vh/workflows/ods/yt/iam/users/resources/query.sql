PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

PRAGMA DqEngine="disable";

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$snapshot = (
SELECT
    id                              as iam_uid,
    subject_type                    as subject_type,
    cloud_id                        as subject_cloud_id,
    $get_datetime(created_at)       as created_at,
    $get_datetime(modified_at)      as modified_at
FROM $select_transfer_manager_table($src_folder, $cluster)
);

$result = (
SELECT
    iam_uid,
    subject_type,
    subject_cloud_id,
    created_at,
    modified_at
FROM $dst_table
WHERE iam_uid NOT IN (SELECT iam_uid FROM $snapshot)
UNION ALL
SELECT
    iam_uid,
    subject_type,
    subject_cloud_id,
    created_at,
    modified_at
FROM $snapshot
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_uid,
    subject_type,
    subject_cloud_id,
    created_at,
    modified_at
FROM
    $result
ORDER BY
    iam_uid
