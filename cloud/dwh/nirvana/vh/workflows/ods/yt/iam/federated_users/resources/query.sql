PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string, $lookup_bool, $get_md5_without_pickle;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};
$users_src_folder = {{ param["users_folder_path"] -> quote() }};
$users_history_src_folder = {{ param["users_history_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$PII_dst_table = {{ param["PII_destination_path"] -> quote() }};

$DEFAULT_MAIL_SUBSCRIPTION = True;
$parse_json = ($field) -> (Yson::ParseJson($field));
$get_settings = ($settings, $name) -> ($lookup_string($parse_json($settings), $name, NULL));
$get_settings_flag = ($settings, $name, $default) -> ($lookup_bool($parse_json($settings), $name, $default));
$get_settings_hash = ($settings, $name) -> ($get_md5_without_pickle($get_settings($settings, $name)));
$get_settings_dict = ($settings, $name) -> (Yson::LookupDict(Yson::ParseJson($settings), $name, Yson::Options(false AS Strict)));

$users_snapshot = SELECT * FROM $select_transfer_manager_table($users_src_folder, $cluster);

$users = (
    SELECT
        id                                              AS iam_user_id,
        federation_id                                   AS iam_federation_id,
        created_at                                      AS created_at,
        modified_at                                     AS modified_at,
        NULL                                            AS deleted_at,
        `settings`                                      AS `settings`
    FROM
        $users_snapshot
);


$users_history_snapshot = SELECT * FROM $select_transfer_manager_table($users_history_src_folder, $cluster);
$deleted_users = (
    SELECT
        iam_user_id,
        iam_federation_id,
        created_at,
        modified_at,
        deleted_at,
        `settings`
    FROM (
        SELECT
            id                                                              AS iam_user_id,
            federation_id                                                   AS iam_federation_id,
            created_at                                                      AS created_at,
            modified_at                                                     AS modified_at,
            NVL(deleted_at, modified_at)                                    AS deleted_at,
            `settings`                                                      AS `settings`,
            ROW_NUMBER() over (PARTITION BY id ORDER BY modified_at DESC)   AS change_id
        FROM
            $users_history_snapshot
        WHERE id NOT IN (
            SELECT iam_user_id
            FROM $users
        )
    ) AS deleted_users_hist_rank
    WHERE change_id = 1
);

$unified_users = (
SELECT
    iam_user_id,
    iam_federation_id,
    created_at,
    modified_at,
    deleted_at,
    `settings`
FROM $users
UNION ALL
SELECT
    iam_user_id,
    iam_federation_id,
    created_at,
    modified_at,
    deleted_at,
    `settings`
FROM $deleted_users
);

$new_unified_users = (
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
    mail_alerting,
FROM $dst_table
WHERE iam_user_id NOT IN (SELECT iam_user_id FROM $unified_users)
UNION ALL
SELECT
    iam_user_id                                                                 AS iam_user_id,
    iam_federation_id                                                           AS iam_federation_id,
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
    $get_settings_flag(`settings`, 'mail_alerting', $DEFAULT_MAIL_SUBSCRIPTION) AS mail_alerting,
FROM $unified_users
);

$pii_unified_users = (
SELECT
    iam_user_id,
    phone,
    email,
FROM $PII_dst_table
WHERE iam_user_id NOT IN (SELECT iam_user_id FROM $unified_users)
UNION ALL
SELECT
    iam_user_id                             AS iam_user_id,
    $get_settings(`settings`, 'phone')      AS phone,
    $get_settings(`settings`, 'email')      AS email,
FROM
    $unified_users
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
    $new_unified_users
ORDER BY
    iam_user_id;

INSERT INTO $PII_dst_table WITH TRUNCATE
SELECT
    iam_user_id,
    phone,
    email
FROM
    $pii_unified_users
ORDER BY
    iam_user_id;
