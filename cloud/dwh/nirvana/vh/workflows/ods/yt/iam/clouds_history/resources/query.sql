PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$parse_json = ($field) -> (Yson::ParseJson($field));
$get_settings = ($settings, $name) -> ($lookup_string($parse_json($settings), $name, NULL));

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$MDB_SQLSERVER_CLUSTER = 'MDB_SQLSERVER_CLUSTER';

$str_contains = ($field, $val) -> (String::Contains($field, $val));

$clouds_history_increment = (
    SELECT
        a.id                                                                                            AS cloud_id,
        a.name                                                                                          AS cloud_name,
        a.organization_id                                                                               AS organization_id,
        a.status                                                                                        AS status,
        a.created_by                                                                                    AS created_by_iam_uid,
        $get_datetime(a.modified_at)                                                                    AS modified_at,
        $get_datetime(a.created_at)                                                                     AS created_at,
        $get_datetime(a.deleted_at)                                                                     AS deleted_at,
        NVL($get_settings(a.`settings`, 'defaultZone'), $get_settings(a.`settings`, 'default_zone'))    AS default_zone,
        a.permission_stages                                                                             AS permission_stages,
        $str_contains(a.permission_stages, $MDB_SQLSERVER_CLUSTER)                                      AS permission_mdb_sql_server_cluster
    FROM $select_transfer_manager_table($src_folder, $cluster) as a
        LEFT JOIN $dst_table as b
            ON a.id=b.cloud_id AND $get_datetime(a.modified_at)=b.modified_at
    WHERE b.cloud_id IS NULL
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
    FROM $clouds_history_increment
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
ORDER BY
    cloud_id, modified_at;
