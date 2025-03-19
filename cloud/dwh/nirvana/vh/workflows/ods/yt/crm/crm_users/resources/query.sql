PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $get_datetime, $get_timestamp_from_days;
IMPORT `helpers` SYMBOLS $get_md5;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};
$pii_dst_table = {{ param["pii_destination_path"] -> quote() }};

$toString = ($data) -> (CAST($data AS String));
$decode_utf8 = ($data) -> (nvl(cast(String::Base64StrictDecode($data) as Utf8),$data));

$get_timestamp = ($ts) -> (CAST($ts AS TimeStamp));
$from_utc_ts_to_msk_dt = ($ts) -> (DateTime::MakeDatetime(DateTime::Update(AddTimeZone($ts, "Europe/Moscow"), "GMT"  as Timezone)));

$result = (
    SELECT
        $toString(`acl_role_set_id`)                                                            AS acl_role_set_id,
        $toString(`acl_team_set_id`)                                                            AS acl_crm_team_set_id,
        CAST(`add_to_bcc_for_outgoing_email` AS bool)                                           AS add_to_bcc_for_outgoing_email,
        $toString(`address_city`)                                                               AS address_city,
        $toString(`address_country`)                                                            AS address_country,
        $toString(`address_postalcode`)                                                         AS address_postalcode,
        $toString(`address_state`)                                                              AS address_state,
        $toString(`address_street`)                                                             AS address_street,
        $toString(`authenticate_id`)                                                            AS authenticate_id,
        $toString(`created_by`)                                                                 AS created_by,
        $get_timestamp(`date_entered`)                                                          AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                                  AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                                         AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                                 AS date_modified_dttm_local,
        $toString(`default_team`)                                                               AS default_team,
        CAST(`deleted` AS bool)                                                                 AS deleted,
        $toString(`department`)                                                                 AS department,
        $toString(`description`)                                                                AS crm_user_description,
        $toString(`employee_status`)                                                            AS employee_status,
        CAST(`external_auth_only` AS bool)                                                      AS external_auth_only,
        $toString(`first_name`)                                                                 AS first_name,
        $toString(`first_name_en`)                                                              AS first_name_en,
        $toString(`id`)                                                                         AS crm_user_id,
        CAST(`import_email_from_robot` AS bool)                                                 AS import_email_from_robot,
        $toString(`internal_phone`)                                                             AS internal_phone,
        CAST(`is_admin` AS bool)                                                                AS is_admin,
        CAST(`is_group` AS bool)                                                                AS is_group,
        $get_timestamp(`last_login`)                                                            AS last_login_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`last_login`))                                    AS last_login_dttm_local,
        $toString(`last_name`)                                                                  AS last_name,
        $toString(`last_name_en`)                                                               AS last_name_en,
        $toString(`messenger_id`)                                                               AS crm_messenger_id,
        $toString(`messenger_type`)                                                             AS messenger_type,
        $toString(`modified_user_id`)                                                           AS modified_user_id,
        $toString(`phone_fax`)                                                                  AS phone_fax,
        $toString(`phone_home`)                                                                 AS phone_home,
        $toString(`phone_mobile`)                                                               AS phone_mobile,
        $toString(`phone_other`)                                                                AS phone_other,
        $toString(`phone_work`)                                                                 AS phone_work,
        $toString(`preferred_language`)                                                         AS preferred_language,
        $get_timestamp(`pwd_last_changed`)                                                      AS pwd_last_changed_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`pwd_last_changed`))                              AS pwd_last_changed_dttm_local,
        CAST(`receive_import_notifications` AS bool)                                            AS receive_import_notifications,
        CAST(`receive_notifications` AS bool)                                                   AS receive_notifications,
        $toString(`reports_to_id`)                                                              AS reports_to_id,
        CAST(`show_on_employees` AS bool)                                                       AS show_on_employees,
        CAST(`show_popup_card` AS bool)                                                         AS show_popup_card,
        $toString(`status`)                                                                     AS status,
        CAST(`sugar_login` AS bool)                                                             AS sugar_login,
        $toString(`team_set_id`)                                                                AS crm_team_set_id,
        $toString(`title`)                                                                      AS title,
        $toString(`title_en`)                                                                   AS title_en,
        CAST(`use_click_to_call` AS bool)                                                       AS use_click_to_call,
        $toString(`user_name`)                                                                  AS crm_user_name
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT 
    acl_role_set_id                                                                             AS acl_role_set_id,
    acl_crm_team_set_id                                                                         AS acl_crm_team_set_id,
    add_to_bcc_for_outgoing_email                                                               AS add_to_bcc_for_outgoing_email,
    address_city                                                                                AS address_city,
    $get_md5(address_country)                                                                   AS address_country_hash,
    $get_md5(address_postalcode)                                                                AS address_postalcode_hash,
    $get_md5(address_state)                                                                     AS address_state_hash,
    $get_md5(address_street)                                                                    AS address_street_hash,
    authenticate_id                                                                             AS authenticate_id,
    created_by                                                                                  AS created_by,
    date_entered_ts                                                                             AS date_entered_ts,
    date_entered_dttm_local                                                                     AS date_entered_dttm_local,
    date_modified_ts                                                                            AS date_modified_ts,
    date_modified_dttm_local                                                                    AS date_modified_dttm_local,
    default_team                                                                                AS default_team,
    deleted                                                                                     AS deleted,
    department                                                                                  AS department,
    crm_user_description                                                                        AS crm_user_description,
    employee_status                                                                             AS employee_status,
    external_auth_only                                                                          AS external_auth_only,
    $get_md5(first_name)                                                                        AS first_name_hash,
    $get_md5(first_name_en)                                                                     AS first_name_en_hash,
    crm_user_id                                                                                 AS crm_user_id,
    import_email_from_robot                                                                     AS import_email_from_robot,
    $get_md5(internal_phone)                                                                    AS internal_phone_hash,
    is_admin                                                                                    AS is_admin,
    is_group                                                                                    AS is_group,
    last_login_ts                                                                               AS last_login_ts,
    last_login_dttm_local                                                                       AS last_login_dttm_local,
    $get_md5(last_name)                                                                         AS last_name_hash,
    $get_md5(last_name_en)                                                                      AS last_name_en_hash,
    crm_messenger_id                                                                            AS crm_messenger_id,
    messenger_type                                                                              AS messenger_type,
    modified_user_id                                                                            AS modified_user_id,
    $get_md5(phone_fax)                                                                         AS phone_fax_hash,
    $get_md5(phone_home)                                                                        AS phone_home_hash,
    $get_md5(phone_mobile)                                                                      AS phone_mobile_hash,
    $get_md5(phone_other)                                                                       AS phone_other_hash,
    $get_md5(phone_work)                                                                        AS phone_work_hash,
    preferred_language                                                                          AS preferred_language,
    pwd_last_changed_ts                                                                         AS pwd_last_changed_ts,
    pwd_last_changed_dttm_local                                                                 AS pwd_last_changed_dttm_local,
    receive_import_notifications                                                                AS receive_import_notifications,
    receive_notifications                                                                       AS receive_notifications,
    reports_to_id                                                                               AS reports_to_id,
    show_popup_card                                                                             AS show_popup_card,
    status                                                                                      AS status,
    sugar_login                                                                                 AS sugar_login,
    crm_team_set_id                                                                             AS crm_team_set_id,
    title                                                                                       AS title,
    title_en                                                                                    AS title_en,
    use_click_to_call                                                                           AS use_click_to_call,
    crm_user_name                                                                               AS crm_user_name
FROM $result
ORDER BY `crm_user_id`;

/* Save result in ODS PII*/
INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT 
    crm_user_id                                                                                 AS crm_user_id,
    address_country                                                                             AS address_country,
    address_postalcode                                                                          AS address_postalcode,
    address_state                                                                               AS address_state,
    address_street                                                                              AS address_street,
    first_name                                                                                  AS first_name,
    first_name_en                                                                               AS first_name_en,
    internal_phone                                                                              AS internal_phone,
    last_name                                                                                   AS last_name,
    last_name_en                                                                                AS last_name_en,
    phone_fax                                                                                   AS phone_fax,
    phone_home                                                                                  AS phone_home,
    phone_mobile                                                                                AS phone_mobile,
    phone_other                                                                                 AS phone_other,
    phone_work                                                                                  AS phone_work
FROM $result
ORDER BY `crm_user_id`;
