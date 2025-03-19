PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string, $lookup_bool, $get_md5;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{input1 -> table_quote()}};

$PII_dst_table = {{ param["PII_destination_path"] -> quote() }};

$DEFAULT_MAIL_SUBSCRIPTION = True;
$parse_json = ($field) -> (Yson::ParseJson($field));
$get_settings = ($settings, $name) -> ($lookup_string($parse_json($settings), $name, NULL));
$get_settings_flag = ($settings, $name, $default) -> ($lookup_bool($parse_json($settings), $name, $default));
$get_settings_hash = ($settings, $name) -> ($get_md5($get_settings($settings, $name)));
$get_settings_dict = ($settings, $name) -> (Yson::LookupDict(Yson::ParseJson($settings), $name, Yson::Options(false AS Strict)));


$passport_users_history = (
    SELECT
        a.id                                                                          AS iam_uid,
        a.passport_id                                                                 AS passport_uid,
        a.passport_login                                                              AS passport_login,
        $get_md5(a.passport_login)                                                    AS passport_login_hash,
        $get_settings(a.`settings`, 'language')                                       AS language,
        $get_settings_hash(a.`settings`, 'phone')                                     AS phone_hash,
        $get_settings_hash(a.`settings`, 'email')                                     AS email_hash,
        $get_settings_dict(a.`settings`, 'experiments')                               AS experiments,
        $get_datetime(a.modified_at)                                                  AS modified_at,
        $get_datetime(a.deleted_at)                                                   AS deleted_at,
        NVL(a.created_clouds, 0)                                                      AS created_clouds,
        NVL(a.cloud_creation_limit, 0)                                                AS cloud_creation_limit,
        $get_settings_flag(a.`settings`, 'mail_tech', $DEFAULT_MAIL_SUBSCRIPTION)     AS mail_tech,
        $get_settings_flag(a.`settings`, 'mail_billing', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_billing,
        $get_settings_flag(a.`settings`, 'mail_testing', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_testing,
        $get_settings_flag(a.`settings`, 'mail_info', $DEFAULT_MAIL_SUBSCRIPTION)     AS mail_info,
        $get_settings_flag(a.`settings`, 'mail_feature', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_feature,
        $get_settings_flag(a.`settings`, 'mail_event', $DEFAULT_MAIL_SUBSCRIPTION)    AS mail_event,
        $get_settings_flag(a.`settings`, 'mail_promo', $DEFAULT_MAIL_SUBSCRIPTION)    AS mail_promo,
        $get_settings_flag(a.`settings`, 'mail_alerting', $DEFAULT_MAIL_SUBSCRIPTION) AS mail_alerting,
        $get_settings(a.`settings`, 'phone')                                          AS phone,
        $get_settings(a.`settings`, 'email')                                          AS email,
    FROM $select_transfer_manager_table($src_folder, $cluster) as a
        LEFT JOIN $dst_table as b
            ON a.id=b.iam_uid AND $get_datetime(a.modified_at)=b.modified_at
    WHERE b.iam_uid IS NULL
);

$result = (
    SELECT
        iam_uid,
        passport_uid,
        passport_login_hash,
        language,
        phone_hash,
        email_hash,
        experiments,
        modified_at,
        deleted_at,
        created_clouds,
        cloud_creation_limit,
        mail_tech,
        mail_billing,
        mail_testing,
        mail_info,
        mail_feature,
        mail_event,
        mail_promo,
        mail_alerting
    FROM $dst_table

    UNION ALL
    
    SELECT
        iam_uid,
        passport_uid,
        passport_login_hash,
        language,
        phone_hash,
        email_hash,
        experiments,
        modified_at,
        deleted_at,
        created_clouds,
        cloud_creation_limit,
        mail_tech,
        mail_billing,
        mail_testing,
        mail_info,
        mail_feature,
        mail_event,
        mail_promo,
        mail_alerting
    FROM $passport_users_history
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_uid,
    passport_uid,
    passport_login_hash,
    language,
    phone_hash,
    email_hash,
    experiments,
    modified_at,
    deleted_at,
    created_clouds,
    cloud_creation_limit,
    mail_tech,
    mail_billing,
    mail_testing,
    mail_info,
    mail_feature,
    mail_event,
    mail_promo,
    mail_alerting
FROM
    $result
ORDER BY
    iam_uid, modified_at;

INSERT INTO $PII_dst_table WITH TRUNCATE
SELECT
    iam_uid,
    modified_at,
    phone,
    email,
    passport_login
FROM (
    SELECT
        iam_uid,
        modified_at,
        phone,
        email,
        passport_login
    FROM $PII_dst_table
    WHERE iam_uid NOT IN (SELECT iam_uid FROM $passport_users_history)
    
    UNION ALL
    
    SELECT
        iam_uid,
        modified_at,
        phone,
        email,
        passport_login
    FROM $passport_users_history
)
ORDER BY
    iam_uid, modified_at;
