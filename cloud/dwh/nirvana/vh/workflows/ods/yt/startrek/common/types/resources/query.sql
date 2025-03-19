PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        `id`,
        `orgId`,
        `hashKey`,
        `shortId`,
        `version`,
        `key`,
        `name`,
        `description`,
        `deleted`,
        `created`,
        `updated`
    FROM
        $src_table
    ORDER BY
        id
