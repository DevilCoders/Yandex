PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{ cluster -> table_quote() }};

$folder_history_source_dir = {{ param["folder_history_source_dir"] -> quote() }};
$folders_source_dir = {{ param["folders_source_dir"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_user_id = ($metadata) -> (JSON_VALUE(cast($metadata as JSON), '$.user_id'));

$folder_history_snapshot = (
    SELECT
        id                                                              AS id,
        name                                                            AS name,
        cloud_id                                                        AS cloud_id,
        IF(deleted_at IS NOT NULL, "DELETED", status)                   AS status,
        $get_user_id(metadata)                                          AS created_by_iam_uid,
        modified_at                                                     AS modified_at,
        created_at                                                      AS created_at,
        deleted_at                                                      AS deleted_at
    FROM (
        SELECT
            id                         AS id,
            name                       AS name,
            cloud_id                   AS cloud_id,
            status                     AS status,
            metadata                   AS metadata,
            modified_at                AS modified_at,
            created_at                 AS created_at,
            deleted_at                 AS deleted_at,
            ROW_NUMBER() over (PARTITION BY id ORDER BY modified_at DESC) AS change_id
        FROM
            $select_transfer_manager_table($folder_history_source_dir, $cluster)
    ) AS folder_history_ranked
    WHERE change_id = 1
);

$unified_folders = (
    SELECT
        id,
        name,
        cloud_id,
        status,
        created_by_iam_uid,
        modified_at,
        created_at,
        deleted_at
    FROM $folder_history_snapshot

    UNION ALL

    SELECT
        folders.id              AS id,
        folders.name            AS name,
        folders.cloud_id        AS cloud_id,
        folders.status          AS status,
        NULL                    AS created_by_iam_uid,
        folders.modified_at     AS modified_at,
        folders.created_at      AS created_at,
        NULL                    AS deleted_at
    FROM $select_transfer_manager_table($folders_source_dir, $cluster) AS folders
        LEFT ONLY JOIN $folder_history_snapshot AS folder_hist
            ON folders.id = folder_hist.id
);

$new_unified_folders = (
SELECT
    a.id                                                                            AS folder_id,
    a.name                                                                          AS folder_name,
    a.cloud_id                                                                      AS cloud_id,
    a.status                                                                        AS status,
    a.created_by_iam_uid                                                            AS created_by_iam_uid,
    $get_datetime(a.modified_at)                                                    AS modified_at,
    $get_datetime(a.created_at)                                                     AS created_at,
    $get_datetime(IF(a.status = "DELETED", NVL(a.deleted_at, a.modified_at), NULL)) AS deleted_at,
FROM
    $unified_folders as a
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
WHERE folder_id NOT IN (SELECT folder_id FROM $new_unified_folders)
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
FROM $new_unified_folders
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
    folder_id
;
