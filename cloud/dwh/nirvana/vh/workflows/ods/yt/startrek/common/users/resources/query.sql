PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $get_datetime_ms;
IMPORT `helpers` SYMBOLS $lookup_string;
IMPORT `helpers` SYMBOLS $get_md5;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$pii_dst_table = {{ param["pii_destination_path"] -> quote() }};

$get_string = ($container, $field) -> ($lookup_string($container, $field, NULL));
$cast_dt = ($field) -> (DateTime::MakeDatetime($field));


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        $cast_dt ($get_datetime_ms(`firstLoginDate`))   AS first_login_dttm,
        $get_md5($get_string(`firstName`, 'ENGLISH'))   AS firstname_en_hash,
        $get_md5($get_string(`firstName`, 'RUSSIAN'))   AS firstname_ru_hash,
        `hasLicense`                                    AS has_license,
        `hashKey`                                       AS hash_key,
        `dismissed`                                     AS is_dismissed,
        `external`                                      AS is_external,
        `orgAdmin`                                      AS is_org_admin,
        `robot`                                         AS is_robot,
        `langUi`                                        AS lang_ui,
        $get_md5($get_string(`lastName`, 'ENGLISH'))    AS lastname_en_hash,
        $get_md5($get_string(`lastName`, 'RUSSIAN'))    AS lastname_ru_hash,
        `orgId`                                         AS org_id,
        `uid`                                           AS st_user_id,
        $get_md5(`login`)                               AS st_user_login_hash,
        `webPushSubscriptionState`                      AS web_push_subscription_state
    FROM
        $src_table
    ORDER BY
        st_user_id
;

INSERT INTO $pii_dst_table WITH TRUNCATE
    SELECT
        $get_string(`firstName`, 'ENGLISH') AS firstname_en,
        $get_string(`firstName`, 'RUSSIAN') AS firstname_ru,
        $get_string(`lastName`, 'ENGLISH')  AS lastname_en,
        $get_string(`lastName`, 'RUSSIAN')  AS lastname_ru,
        `uid`                               AS st_user_id,
        `login`                             AS st_user_login
    FROM
        $src_table
    ORDER BY
        st_user_id
;
