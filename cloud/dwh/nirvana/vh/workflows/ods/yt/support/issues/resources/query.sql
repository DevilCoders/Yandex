PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
            `access_type`                   AS access_type,
            `attachments`                   AS attachments,
            `cloud_id`                      AS cloud_id,
            `comments_metadata`             AS comments_metadata,
            $get_datetime(`created_at`)     AS created_at,
            `description`                   AS description,
            $get_datetime(`export_ts`)      AS export_ts,
            `folder_id`                     AS folder_id,
            `iam_user_id`                   AS iam_user_id,
            `id`                            AS id,
            `meta`                          AS meta,
            $get_datetime(`resolved_at`)    AS resolved_at,
            `st_id`                         AS startrek_issue_id,
            `st_key`                        AS startrek_issue_key,
            `st_version`                    AS st_version,
            `state`                         AS issue_state,
            `summary`                       AS summary,
            $get_datetime(`synced_at`)      AS synced_at,
            `type`                          AS issue_type,
            $get_datetime(`updated_at`)     AS updated_at
    FROM
        $src_table
    ORDER BY
        id
;
