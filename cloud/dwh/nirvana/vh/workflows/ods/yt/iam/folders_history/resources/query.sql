PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA DqEngine="disable";

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{input1 -> table_quote()}};

$get_user_id = ($metadata) -> (JSON_VALUE(cast($metadata as JSON), '$.user_id'));

$folders_history = (
SELECT
    a.id                         as folder_id,
    a.name                       as folder_name,
    a.cloud_id                   as cloud_id,
    a.status                     as status,
    $get_user_id(a.metadata)     as created_by_iam_uid,
    $get_datetime(a.modified_at) as modified_at,
    $get_datetime(a.created_at)  as created_at,
    $get_datetime(a.deleted_at)  as deleted_at
FROM $select_transfer_manager_table($src_folder, $cluster) as a
LEFT JOIN $dst_table as b
ON a.id=b.folder_id AND $get_datetime(a.modified_at)=b.modified_at
WHERE b.folder_id IS NULL
);

$result = (
SELECT
    folder_id,
    folder_name,
    cloud_id,
    status,
    created_by_iam_uid,
    modified_at,
    created_at,
    deleted_at,
FROM $dst_table
UNION ALL
SELECT
    folder_id,
    folder_name,
    cloud_id,
    status,
    created_by_iam_uid,
    modified_at,
    created_at,
    deleted_at,
FROM $folders_history
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    folder_id           AS folder_id,
    folder_name         AS folder_name,
    cloud_id            AS cloud_id,
    status              AS status,
    created_by_iam_uid  AS created_by_iam_uid,
    modified_at         AS modified_at,
    created_at          AS created_at,
    deleted_at          AS deleted_at
FROM
    $result
ORDER BY
    folder_id, modified_at;
