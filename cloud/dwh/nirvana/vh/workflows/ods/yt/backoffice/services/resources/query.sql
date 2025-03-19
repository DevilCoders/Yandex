PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $parse_iso8601_to_datetime_msk;


$src_table = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$result = (
    SELECT
    `id`,
    `name`,
    `is_product`,
    `icon_name`,
    `slug`,
    `full_name`,
    `iam_flag`,
    `status`,
    `page_id`,
    `doc_url`,
    `prices_url`,
    `order_number`,
    `category_id`,
    `description_ru`,
    `description_en`,
    `tag`,
    `console_url`,
    `icon`,
    `has_en_fallback`,
    $parse_iso8601_to_datetime_msk(created_at) AS created_at_msk,
    $parse_iso8601_to_datetime_msk(updated_at) AS updated_at_msk
    FROM $src_table
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `id`;
