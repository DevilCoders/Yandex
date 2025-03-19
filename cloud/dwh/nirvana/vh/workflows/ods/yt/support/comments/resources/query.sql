PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        `id`                        AS id,
        `ticket_id`                 AS ticket_id,
        `iam_user_id`               AS iam_user_id,
        `direction`                 AS direction,
        `state`                     AS state,
        $get_datetime(`created_at`) AS created_at,
        $get_datetime(`export_ts`)  AS export_ts,
        $get_datetime(`updated_at`) AS updated_at,
        `attachments`               AS attachments,
        `visible`                   AS is_visible
    FROM
        $src_table
    ORDER BY
        id
;
