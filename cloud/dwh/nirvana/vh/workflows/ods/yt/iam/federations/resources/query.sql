PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_msk_datetime;

$cluster = {{ cluster -> table_quote() }};
$federations_src_folder = {{ param["federations_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$federations_snapshot = SELECT * FROM $select_transfer_manager_table($federations_src_folder, $cluster);

$federations_snapshot = (
    SELECT
        id                                  AS federation_id,
        autocreate_users                    AS autocreate_users,
        case_insensitive_name_ids           AS case_insensitive_name_ids,
        cookie_max_age                      AS cookie_max_age,
        description                         AS description,
        encrypted_assertions                AS encrypted_assertions,
        issuer                              AS issuer,
        name                                AS name,
        organization_id                     AS organization_id,
        set_sso_url_username                AS set_sso_url_username,
        sso_binding                         AS sso_binding,
        sso_url                             AS sso_url,
        status                              AS status,
        $get_msk_datetime(created_at)       AS created_at_msk
    FROM
        $federations_snapshot
);

$result = (
SELECT
    federation_id,
    autocreate_users,
    case_insensitive_name_ids,
    cookie_max_age,
    description,
    encrypted_assertions,
    issuer,
    name,
    organization_id,
    set_sso_url_username,
    sso_binding,
    sso_url,
    status,
    created_at_msk
FROM $dst_table
WHERE federation_id NOT IN (SELECT federation_id FROM $federations_snapshot)
UNION ALL
SELECT
    federation_id,
    autocreate_users,
    case_insensitive_name_ids,
    cookie_max_age,
    description,
    encrypted_assertions,
    issuer,
    name,
    organization_id,
    set_sso_url_username,
    sso_binding,
    sso_url,
    status,
    created_at_msk
FROM $federations_snapshot
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    federation_id,
    autocreate_users,
    case_insensitive_name_ids,
    cookie_max_age,
    description,
    encrypted_assertions,
    issuer,
    name,
    organization_id,
    set_sso_url_username,
    sso_binding,
    sso_url,
    status,
    created_at_msk
FROM
    $result
ORDER BY
    federation_id;
