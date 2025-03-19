PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$parse_json = ($field) -> (Yson::ParseJson($field));
$get_settings = ($settings, $name) -> ($lookup_string($parse_json($settings), $name, NULL));

$src_cloud_history_dir = {{ param["cloud_history_source_dir"] -> quote() }};
$src_clouds_dir = {{ param["clouds_source_dir"] -> quote() }};

$dst_table = {{ input1 -> table_quote() }};

$MDB_SQLSERVER_CLUSTER = 'MDB_SQLSERVER_CLUSTER';

$str_contains = ($field, $val) -> (String::Contains($field, $val));

$cloud_history_snapshot = (
    SELECT
        id,
        name,
        organization_id,
        status,
        created_by,
        modified_at,
        created_at,
        deleted_at,
        settings,
        permission_stages
    FROM (
        SELECT
            id                                                              AS id,
            name                                                            AS name,
            organization_id                                                 AS organization_id,
            IF(deleted_at IS NOT NULL, "DELETED", status)                   AS status,
            created_by                                                      AS created_by,
            modified_at                                                     AS modified_at,
            created_at                                                      AS created_at,
            deleted_at                                                      AS deleted_at,
            settings                                                        AS settings,
            permission_stages                                               AS permission_stages,
            ROW_NUMBER() OVER (PARTITION BY id ORDER BY modified_at DESC)   AS change_id
        FROM
            $select_transfer_manager_table($src_cloud_history_dir, $cluster) AS cloud_history
    ) AS cloud_history_ranked
    WHERE change_id = 1
);

$raw_clouds_snapshot = (
    SELECT
        id,
        name,
        organization_id,
        status,
        created_by,
        modified_at,
        created_at,
        deleted_at,
        settings,
        permission_stages
    FROM $cloud_history_snapshot

    UNION ALL

    SELECT
        clouds.id                   AS id,
        clouds.name                 AS name,
        clouds.organization_id      AS organization_id,
        clouds.status               AS status,
        clouds.created_by           AS created_by,
        clouds.modified_at          AS modified_at,
        clouds.created_at           AS created_at,
        NULL                        AS deleted_at,
        settings                    AS settings,
        permission_stages           AS permission_stages
    FROM $select_transfer_manager_table($src_clouds_dir, $cluster) AS clouds
        LEFT ONLY JOIN $cloud_history_snapshot AS cloud_hist
            ON clouds.id = cloud_hist.id
);

$raw_clouds_ext = (
    SELECT
        id                                                                                          AS cloud_id,
        name                                                                                        AS cloud_name,
        organization_id                                                                             AS organization_id,
        status                                                                                      AS status,
        created_by                                                                                  AS created_by_iam_uid,
        $get_datetime(modified_at)                                                                  AS modified_at,
        $get_datetime(created_at)                                                                   AS created_at,
        $get_datetime(IF(status = "DELETED", NVL(deleted_at, modified_at), NULL))                   AS deleted_at,
        NVL($get_settings(`settings`, 'defaultZone'), $get_settings(`settings`, 'default_zone'))    AS default_zone,
        permission_stages                                                                           AS permission_stages,
        $str_contains(`permission_stages`, $MDB_SQLSERVER_CLUSTER)                                  AS permission_mdb_sql_server_cluster
    FROM
        $raw_clouds_snapshot
);

$result = (
    SELECT
        cloud_id,
        cloud_name,
        organization_id,
        status,
        created_by_iam_uid,
        modified_at,
        created_at,
        deleted_at,
        default_zone,
        permission_stages,
        permission_mdb_sql_server_cluster
    FROM $dst_table
    WHERE cloud_id NOT IN (SELECT cloud_id FROM $raw_clouds_ext)

    UNION ALL
    
    SELECT
        cloud_id,
        cloud_name,
        organization_id,
        status,
        created_by_iam_uid,
        modified_at,
        created_at,
        deleted_at,
        default_zone,
        permission_stages,
        permission_mdb_sql_server_cluster
    FROM $raw_clouds_ext
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    cloud_id,
    cloud_name,
    organization_id,
    status,
    created_by_iam_uid,
    modified_at,
    created_at,
    deleted_at,
    default_zone,
    permission_stages,
    permission_mdb_sql_server_cluster
FROM
    $result
ORDER BY cloud_id;
