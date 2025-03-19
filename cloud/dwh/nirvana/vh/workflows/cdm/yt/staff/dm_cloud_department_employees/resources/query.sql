PRAGMA Library("helpers.sql");

IMPORT `helpers` SYMBOLS $lookup_int64, $lookup_string;

$staff_persons_table = {{ param["staff_persons_ods_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_int = ($container, $field) -> ($lookup_int64($container, $field, NULL));
$get_string = ($custom_fields, $field) -> ($lookup_string($custom_fields, $field, NULL));


$departments = (
    SELECT
        persons.staff_user_id                               AS staff_user_id,
        $get_string(persons.department_ancestors, 'name')   AS department_name,
        $get_string(persons.department_ancestors, 'type')   AS department_type,
        $get_string(persons.department_ancestors, 'url')    AS department_url,
        $get_int(persons.department_ancestors, 'id')        AS department_id,
        $get_int(persons.department_ancestors, 'level')     AS department_level,
        False                                               AS is_direct
    FROM $staff_persons_table AS persons
    FLATTEN LIST BY persons.department_ancestors

    UNION ALL 

    SELECT
        persons.staff_user_id           AS staff_user_id,
        persons.department_name         AS department_name,
        --persons.department_type         AS department_type,
        'department'                    AS department_type,
        persons.department_url          AS department_url,
        persons.department_id           AS department_id,
        --persons.department_level        AS department_level,
        100                             AS department_level,
        True                            AS is_direct
    FROM $staff_persons_table AS persons
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT *
FROM $departments
;