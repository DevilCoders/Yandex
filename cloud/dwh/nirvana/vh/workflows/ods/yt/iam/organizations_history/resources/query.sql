PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_msk_datetime;

$cluster = {{ cluster -> table_quote() }};
$organizations_history_src_folder = {{ param["organizations_history_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};


$organizations_history = (
SELECT
    a.id                                                                                      AS organization_id,
    a.directory_organization_id                                                               AS directory_organization_id,
    a.description                                                                             AS description,
    a.name                                                                                    AS name,
    a.status                                                                                  AS status,
    a.display_name                                                                            AS display_name,
    Yson::LookupString(Yson::ParseJson(a.settings), "size", Yson::Options(false as Strict))   AS size,
    $get_msk_datetime(a.created_at)                                                           AS created_at_msk,
    $get_msk_datetime(a.modified_at)                                                          AS modified_at_msk,
    $get_msk_datetime(a.deleted_at)                                                           AS deleted_at_msk
FROM
    $select_transfer_manager_table($organizations_history_src_folder, $cluster) as a
LEFT JOIN $dst_table as b
ON a.id=b.organization_id AND $get_msk_datetime(a.modified_at)=b.modified_at_msk
WHERE b.organization_id IS NULL
);

$result = (
SELECT
    organization_id,
    directory_organization_id,
    description,
    name,
    status,
    display_name,
    size,
    created_at_msk,
    modified_at_msk,
    deleted_at_msk
FROM $dst_table
UNION ALL
SELECT
    organization_id,
    directory_organization_id,
    description,
    name,
    status,
    display_name,
    size,
    created_at_msk,
    modified_at_msk,
    deleted_at_msk
FROM $organizations_history
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    organization_id,
    directory_organization_id,
    description,
    name,
    status,
    display_name,
    size,
    created_at_msk,
    modified_at_msk,
    deleted_at_msk
FROM
    $result
ORDER BY
    organization_id, modified_at_msk;
