PRAGMA Library("tables.sql");
PRAGMA DqEngine="disable";

IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{input1 -> table_quote()}};


$resource_memberships = (
SELECT
    a.subject_id                  AS iam_uid,
    a.resource_type               AS resource_type,
    a.resource_id                 AS resource_id
FROM
    $select_transfer_manager_table($src_folder, $cluster) AS a
LEFT JOIN $dst_table AS b
ON a.subject_id=b.iam_uid AND a.resource_id=b.resource_id
WHERE b.iam_uid IS NULL
);

$result = (
SELECT
    iam_uid,
    resource_type,
    resource_id
FROM $dst_table
UNION ALL
SELECT
    iam_uid,
    resource_type,
    resource_id
FROM $resource_memberships
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_uid,
    resource_type,
    resource_id
FROM
    $result
ORDER BY
    iam_uid
