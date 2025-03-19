PRAGMA Library("helpers.sql");
PRAGMA Library("datetime.sql");
IMPORT `helpers` SYMBOLS $lookup_string;
IMPORT `datetime` SYMBOLS $get_datetime_ms;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_field = ($container, $field) -> (IF(Yson::Contains($container, $field), $container.$field, NULL));
$get_string = ($container, $field) -> ($lookup_string($container, $field, NULL));
$cast_dt = ($field) -> (DateTime::MakeDatetime($field));

$event_changes = (
    SELECT
        e.*,
        Yson::ConvertToList(`changes`) AS change
    FROM $src_table AS e
);

$flatten_event_changes = (
    SELECT
        e.*
    FROM $event_changes AS e
    FLATTEN LIST BY change
);

$parsed_changes = (
    SELECT
        e.issue                                                 AS issue_id,
        $cast_dt($get_datetime_ms(e.`date`))                    AS dttm,
        e.orgId                                                 AS org_id,
        e.author                                                AS author_id,
        $get_string(`change`, 'field')                          AS field,
        $get_field($get_field(`change`, 'newValue'), 'value')   AS new_value,
        $get_field($get_field(`change`, 'oldValue'), 'value')   AS old_value
    FROM $flatten_event_changes AS e
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT *
FROM $parsed_changes;