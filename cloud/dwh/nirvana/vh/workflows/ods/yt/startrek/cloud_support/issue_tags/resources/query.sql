PRAGMA Library("helpers.sql");

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

IMPORT `helpers` SYMBOLS $lookup_string;
$get_string = ($container, $field) -> ($lookup_string($container, $field, NULL));

$tags = (
    SELECT
        id                          AS issue_id,
        Yson::ConvertToList(`tags`) AS tags
    FROM $src_table
);

$flatten_tags = (
    SELECT
        issue_id    AS issue_id,
        tags        AS tag
    FROM $tags
    FLATTEN LIST BY tags
);

$cast_tags = (
    SELECT
        issue_id,
        Yson::ConvertToString(tag) AS tag
    FROM $flatten_tags
);

INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        issue_id,
        tag
    FROM $cast_tags
    ORDER BY issue_id, tag;
