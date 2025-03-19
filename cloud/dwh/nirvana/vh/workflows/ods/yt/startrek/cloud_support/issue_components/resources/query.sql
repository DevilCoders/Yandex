PRAGMA Library("helpers.sql");

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

IMPORT `helpers` SYMBOLS $lookup_string;
$get_string = ($container, $field) -> ($lookup_string($container, $field, NULL));

$components = (
    SELECT
        id                                  AS issue_id,
        Yson::ConvertToList(`components`)   AS component_ids
    FROM $src_table
);

$flatten_components = (
    SELECT
        issue_id        AS issue_id,
        component_ids   AS component_id
    FROM $components
    FLATTEN LIST BY component_ids
);

$cast_components = (
    SELECT
        issue_id,
        Yson::ConvertToString(component_id) AS component_id
    FROM $flatten_components
);

INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        issue_id,
        component_id
    FROM $cast_components
    ORDER BY issue_id, component_id;
