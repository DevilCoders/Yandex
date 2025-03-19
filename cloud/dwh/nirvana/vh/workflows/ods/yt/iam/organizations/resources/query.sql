PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_msk_datetime;

$cluster = {{ cluster -> table_quote() }};
$organizations_src_folder = {{ param["organizations_folder_path"] -> quote() }};
$organizations_history_src_folder = {{ param["organizations_history_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$lookup_size = ($settings) -> ( Yson::LookupString(Yson::ParseJson($settings), "size", Yson::Options(false as Strict)) );

$organizations_snapshot = SELECT * FROM $select_transfer_manager_table($organizations_src_folder, $cluster);

$organizations = (
    SELECT
        id                          AS organization_id,
        directory_organization_id   AS directory_organization_id,
        description                 AS description,
        name                        AS name,
        status                      AS status,
        display_name                AS display_name,
        created_at                  AS created_at,
        NULL                        AS deleted_at,
        $lookup_size(settings)      AS size
    FROM
        $organizations_snapshot
);


$organizations_history_snapshot = SELECT * FROM $select_transfer_manager_table($organizations_history_src_folder, $cluster);
$deleted_organizations = (
    SELECT
        organization_id,
        directory_organization_id,
        description,
        name,
        status,
        display_name,
        size,
        created_at,
        deleted_at
    FROM (
        SELECT
            id                                                              AS organization_id,
            directory_organization_id                                       AS directory_organization_id,
            description                                                     AS description,
            name                                                            AS name,
            status                                                          AS status,
            display_name                                                    AS display_name,
            $lookup_size(settings)                                          AS size,
            created_at                                                      AS created_at,
            NVL(deleted_at, modified_at)                                    AS deleted_at,
            ROW_NUMBER() over (PARTITION BY id ORDER BY modified_at DESC)   AS change_id
        FROM
            $organizations_history_snapshot
        WHERE id NOT IN (
            SELECT organization_id
            FROM $organizations
        )
    ) AS deleted_organizations_hist_rank
    WHERE change_id = 1
);

$unified_organizations = (
SELECT
    organization_id                                 AS organization_id,
    directory_organization_id                       AS directory_organization_id,
    description                                     AS description,
    name                                            AS name,
    status                                          AS status,
    display_name                                    AS display_name,
    size                                            AS size,
    $get_msk_datetime(created_at)                   AS created_at_msk,
    $get_msk_datetime(deleted_at)                   AS deleted_at_msk
FROM $organizations
    UNION ALL
SELECT
    organization_id                                 AS organization_id,
    directory_organization_id                       AS directory_organization_id,
    description                                     AS description,
    name                                            AS name,
    status                                          AS status,
    display_name                                    AS display_name,
    size                                            AS size,
    $get_msk_datetime(created_at)                   AS created_at_msk,
    $get_msk_datetime(deleted_at)                   AS deleted_at_msk
FROM $deleted_organizations
);

$new_unified_organizations = (
SELECT
    organization_id,
    directory_organization_id,
    description,
    name,
    status,
    display_name,
    size,
    created_at_msk,
    deleted_at_msk
FROM $dst_table
WHERE organization_id NOT IN (SELECT organization_id from $unified_organizations)
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
    deleted_at_msk
FROM $unified_organizations
)
;
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
    deleted_at_msk
FROM
    $new_unified_organizations
ORDER BY
    organization_id;
