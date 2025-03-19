PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $parse_iso8601_to_datetime_msk;


$src_table = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$result = (
    SELECT
        event_id,
        service_id,
        $parse_iso8601_to_datetime_msk(created_at) AS created_at_msk,
        $parse_iso8601_to_datetime_msk(updated_at) AS updated_at_msk
    FROM $src_table
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `event_id`;
