PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime_ms;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        `id`                        AS component_id,
        `hashKey`                   AS hash_key,
        `shortId`                   AS short_id,
        `version`                   AS version,
        `queue`                     AS queue,
        `name`                      AS name,
        `description`               AS description,
        `lead`                      AS lead,
        `assignAuto`                AS assign_auto,
        `permissions`               AS permissions,
        `followers`                 AS followers,
        `followingGroups`           AS following_groups,
        `followingMaillists`        AS following_maillists,
        `deleted`                   AS deleted,
        $get_datetime_ms(`created`) AS created_at,
        $get_datetime_ms(`updated`) AS updated_at,
        `orgId`                     AS org_id
    FROM
        $src_table
    ORDER BY
        component_id
