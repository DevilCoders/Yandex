PRAGMA Library("helpers.sql");

IMPORT `helpers` SYMBOLS $lookup_int64, $lookup_string, $get_md5;

$src_table = {{ param["source_path"] -> quote() }};
$pii_dst_table = {{ param["pii_destination_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_int = ($container, $field) -> ($lookup_int64($container, $field, NULL));
$get_string = ($custom_fields, $field) -> ($lookup_string($custom_fields, $field, NULL));
$get_field = ($container, $field) -> (IF(Yson::Contains($container, $field), $container.$field, NULL));

$CLOUD_ID = 70618;
$CLOUD_DEPARTMENT_LEVEL_SHIFT = 1;

$AGG_DEPARTMENT_PATH = ($base, $addon) -> (IF($base IS NOT NULL, $base || ' / ', '') || $addon);

$parse_ancestors_yson = (
    SELECT
        t.*,
        Yson::ConvertToList(Yson::ParseJson(`department_ancestors`)) AS ancestors
    FROM $src_table AS t
);

$flatten_ancestors = (
    SELECT
        id,
        ancestors
    FROM $parse_ancestors_yson
    FLATTEN LIST BY ancestors
);

$cloud_level = (
    SELECT DISTINCT
        CAST($get_int(ancestors, 'level') AS Uint64) AS cloud_level
    FROM $flatten_ancestors AS t
    WHERE $get_int(ancestors, 'id') = $CLOUD_ID
);

$cloud_level_department = (
    SELECT *
    FROM $flatten_ancestors AS t
    WHERE $get_int(ancestors, 'level') = $cloud_level + $CLOUD_DEPARTMENT_LEVEL_SHIFT
);


$direct_departments = (
    SELECT
        t.id                                                                            AS id,
        $get_string(t.department_yson, 'url')                                           AS department_url,
        $get_string($get_field($get_field(t.department_yson, 'name'), 'full'), 'ru')    AS department_name,
        $get_int(t.department_yson, 'id')                                               AS department_id,
        ListConcat(
            ListMap(
                ListSkip(Yson::ConvertToList(Yson::ParseJson(`department_ancestors`)), Unwrap($cloud_level)),
                ($x) -> { RETURN $get_string($x, 'name') }
            ),
            ' / '
        ) AS department_fullpath
    FROM (
        SELECT
            t.*,
            Yson::ParseJson(`department`) AS department_yson
        FROM $src_table AS t
    ) AS t
);


$result_data = (
    SELECT
        persons.`id`                                                                                    AS staff_user_id,
        persons.`uid`                                                                                   AS staff_user_uid,
        persons.`login`                                                                                 AS staff_user_login,
        persons.`firstname_en`                                                                          AS firstname_en,
        persons.`firstname_ru`                                                                          AS firstname_ru,
        persons.`lastname_en`                                                                           AS lastname_en,
        CAST(persons.`is_deleted` AS Bool?)                                                             AS is_deleted,
        persons.`lastname_ru`                                                                           AS lastname_ru,
        persons.`official_affiliation`                                                                  AS official_affiliation,
        cast(persons.`official_quit_at` AS Date)                                                        AS official_quit_dt,
        CAST(persons.`official_is_dismissed` AS Bool)                                                   AS official_is_dismissed,
        CAST(persons.`official_is_homeworker` AS Bool)                                                  AS official_is_homeworker,
        CAST(persons.`official_is_robot` AS Bool)                                                       AS official_is_robot,
        persons.`yandex_login`                                                                          AS yandex_login,
        persons.`ancestors`                                                                             AS department_ancestors,
        direct_department.`department_id`                                                               AS department_id,
        direct_department.`department_name`                                                             AS department_name,
        direct_department.`department_url`                                                              AS department_url,
        $AGG_DEPARTMENT_PATH(direct_department.department_fullpath, direct_department.department_name)  AS department_fullname,
        $get_int(cloud_deps.`ancestors`, 'id')                                                          AS cloud_department_id,
        $get_string(cloud_deps.`ancestors`, 'name')                                                     AS cloud_department_name,
        $get_string(cloud_deps.`ancestors`, 'url')                                                      AS cloud_department_url,
    FROM $parse_ancestors_yson AS persons
        INNER JOIN $direct_departments AS direct_department
            ON persons.id = direct_department.id
        LEFT JOIN $cloud_level_department AS cloud_deps
            ON persons.id = cloud_deps.id
);

INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT
    $get_md5(staff_user_id)                     AS staff_user_hash,
    staff_user_id,
    staff_user_uid,
    firstname_en,
    firstname_ru,
    lastname_en,
    lastname_ru,
    staff_user_login,
    yandex_login,
    official_quit_dt
FROM $result_data;

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    $get_md5(staff_user_id)                     AS staff_user_hash,
    staff_user_id                               AS staff_user_id,
    staff_user_uid                              AS staff_user_uid,
    $get_md5(firstname_en)                      AS firstname_en_hash,
    $get_md5(firstname_ru)                      AS firstname_ru_hash,
    $get_md5(lastname_en)                       AS lastname_en_hash,
    $get_md5(lastname_ru)                       AS lastname_ru_hash,
    $get_md5(staff_user_login)                  AS staff_user_login_hash,
    $get_md5(yandex_login)                      AS yandex_login_hash,
    is_deleted                                  AS is_deleted,
    official_affiliation                        AS official_affiliation,
    official_quit_dt                            AS official_quit_dt,
    official_is_dismissed                       AS official_is_dismissed,
    official_is_homeworker                      AS official_is_homeworker,
    official_is_robot                           AS official_is_robot,
    department_ancestors                        AS department_ancestors,
    department_id                               AS department_id,
    department_name                             AS department_name,
    department_fullname                         AS department_fullname,
    department_url                              AS department_url,
    NVL(cloud_department_id, department_id)     AS cloud_department_id,
    NVL(cloud_department_name, department_name) AS cloud_department_name,
    NVL(cloud_department_url, department_url)   AS cloud_department_url,
FROM $result_data;
