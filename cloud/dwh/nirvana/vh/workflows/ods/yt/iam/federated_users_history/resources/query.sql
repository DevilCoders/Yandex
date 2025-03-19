PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string, $lookup_bool, $get_md5;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$PII_dst_table = {{ param["PII_destination_path"] -> quote() }};

$DEFAULT_MAIL_SUBSCRIPTION = True;
$parse_json = ($field) -> (Yson::ParseJson($field));
$get_settings = ($settings, $name) -> ($lookup_string($parse_json($settings), $name, NULL));
$get_settings_flag = ($settings, $name, $default) -> ($lookup_bool($parse_json($settings), $name, $default));
$get_settings_hash = ($settings, $name) -> ($get_md5($get_settings($settings, $name)));
$get_settings_dict = ($settings, $name) -> (Yson::LookupDict(Yson::ParseJson($settings), $name, Yson::Options(false AS Strict)));


$federated_users_history = (
SELECT
    a.id AS id,
    a.federation_id AS federation_id,
    a.created_at AS created_at,
    a.modified_at AS modified_at,
    a.deleted_at AS deleted_at,
    a.settings AS settings,
FROM $select_transfer_manager_table($src_folder, $cluster) as a
LEFT JOIN $dst_table as b
ON a.id=b.iam_user_id AND $get_datetime(a.modified_at)=b.modified_at
WHERE b.iam_user_id IS NULL
);

$result = (
SELECT
    iam_user_id,
    iam_federation_id,
    created_at,
    modified_at,
    deleted_at,
    phone_hash,
    email_hash,
    experiments,
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
    id                                                                          AS iam_user_id,
    federation_id                                                               AS iam_federation_id,
    $get_datetime(created_at)                                                   AS created_at,
    $get_datetime(modified_at)                                                  AS modified_at,
    $get_datetime(deleted_at)                                                   AS deleted_at,
    $get_settings_hash(`settings`, 'phone')                                     AS phone_hash,
    $get_settings_hash(`settings`, 'email')                                     AS email_hash,
    $get_settings_dict(`settings`, 'experiments')                               AS experiments,
    $get_settings_flag(`settings`, 'mail_tech', $DEFAULT_MAIL_SUBSCRIPTION)     AS mail_tech,
    $get_settings_flag(`settings`, 'mail_billing', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_billing,
    $get_settings_flag(`settings`, 'mail_testing', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_testing,
    $get_settings_flag(`settings`, 'mail_info', $DEFAULT_MAIL_SUBSCRIPTION)     AS mail_info,
    $get_settings_flag(`settings`, 'mail_feature', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_feature,
    $get_settings_flag(`settings`, 'mail_event', $DEFAULT_MAIL_SUBSCRIPTION)    AS mail_event,
    $get_settings_flag(`settings`, 'mail_promo', $DEFAULT_MAIL_SUBSCRIPTION)    AS mail_promo,
    $get_settings_flag(`settings`, 'mail_alerting', $DEFAULT_MAIL_SUBSCRIPTION) AS mail_alerting
FROM $federated_users_history
);

$pii_result = (
SELECT
    iam_user_id,
    modified_at,
    phone,
    email
FROM $PII_dst_table
UNION ALL
SELECT
    id                                              AS iam_user_id,
    $get_datetime(modified_at)                      AS modified_at,
    $get_settings(`settings`, 'phone')              AS phone,
    $get_settings(`settings`, 'email')              AS email,
FROM
    $federated_users_history
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    iam_user_id,
    iam_federation_id,
    created_at,
    modified_at,
    deleted_at,
    phone_hash,
    email_hash,
    experiments,
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
    iam_user_id, modified_at;

INSERT INTO $PII_dst_table WITH TRUNCATE
SELECT iam_user_id, modified_at, phone, email
FROM $pii_result
ORDER BY
    iam_user_id, modified_at;
