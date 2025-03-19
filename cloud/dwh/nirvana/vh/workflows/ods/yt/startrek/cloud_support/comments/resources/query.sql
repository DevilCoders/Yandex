PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime_ms;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$cast_dt = ($field) -> (DateTime::MakeDatetime($field));


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        `id`                                    AS id,
        `issue`                                 AS issue_id,
        `hashKey`                               AS hash_key,
        `shortId`                               AS short_id,
        `version`                               AS version,
        `text`                                  AS text,
        `author`                                AS author,
        `modifier`                              AS modifier,
        `summonees`                             AS summonees,
        `maillistSummonees`                     AS maillist_summonees,
        $cast_dt($get_datetime_ms(`created`))   AS created_at,
        $cast_dt($get_datetime_ms(`updated`))   AS updated_at,
        `deleted`                               AS is_deleted,
        `email`                                 AS email,
        `emailsInfo`                            AS emails_info,
        `reactions`                             AS reactions,
        `orgId`                                 AS org_id
    FROM
        $src_table
    ORDER BY
        id
