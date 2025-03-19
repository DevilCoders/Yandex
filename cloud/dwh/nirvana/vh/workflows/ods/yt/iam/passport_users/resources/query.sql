PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("tables.sql");

IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string, $lookup_bool, $get_md5_without_pickle;
IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};
$src_folder = {{ param["source_folder_path"] -> quote() }};
$history_src_folder = {{ param["history_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$PII_dst_table = {{ param["PII_destination_path"] -> quote() }};

$DEFAULT_MAIL_SUBSCRIPTION = True;
$parse_json = ($field) -> (Yson::ParseJson($field));
$get_settings = ($settings, $name) -> ($lookup_string($parse_json($settings), $name, NULL));
$get_settings_flag = ($settings, $name, $default) -> ($lookup_bool($parse_json($settings), $name, $default));
$get_settings_hash = ($settings, $name) -> ($get_md5_without_pickle($get_settings($settings, $name)));
$get_settings_dict = ($settings, $name) -> (Yson::LookupDict(Yson::ParseJson($settings), $name, Yson::Options(false AS Strict)));

$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);
$history_snapshot = SELECT * FROM $select_transfer_manager_table($history_src_folder, $cluster);



$actual_users = (
    SELECT
        id,
        passport_id,
        passport_login,
        `settings`,
        created_clouds,
        cloud_creation_limit,
        modified_at,
        NULL                        AS deleted_at
    FROM
        $snapshot
);

$deleted_users = (
    SELECT
        id,
        passport_id,
        passport_login,
        `settings`,
        created_clouds,
        cloud_creation_limit,
        deleted_at,
        modified_at,
    FROM (
        SELECT
            id,
            passport_id,
            passport_login,
            `settings`,
            created_clouds,
            cloud_creation_limit,
            modified_at                                                     AS modified_at,
            NVL(deleted_at, modified_at)                                    AS deleted_at,
            ROW_NUMBER() over (PARTITION BY id ORDER BY modified_at DESC)   AS change_id
        FROM
            $history_snapshot
        WHERE id NOT IN (
            SELECT id
            FROM $actual_users
        )
    )
    WHERE change_id = 1
);

$unified_users = (
    SELECT * FROM $actual_users
    UNION ALL
    SELECT * FROM $deleted_users
);


$user_creations = (
    SELECT
        id,
        modified_at as created_at
    FROM (
        SELECT
            id,
            modified_at,
            ROW_NUMBER() over (PARTITION BY id ORDER BY modified_at ASC)   AS change_id
        FROM
            $history_snapshot
    ) AS deleted_users_hist_rank
    WHERE change_id = 1
);

$users_with_creation_time = (
    SELECT
        users.*,
        NVL(creations.created_at, users.modified_at) as created_at
    FROM $unified_users as users
    LEFT JOIN $user_creations as creations
    USING (id)
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
    created_clouds,
    cloud_creation_limit,
    created_at,
    deleted_at,
    modified_at,
    mail_tech,
    mail_billing,
    mail_testing,
    mail_info,
    mail_feature,
    mail_event,
    mail_promo,
    mail_alerting
FROM $dst_table
WHERE iam_uid NOT IN (SELECT id FROM $users_with_creation_time)
UNION ALL
SELECT
    id                                                                          AS iam_uid,
    passport_id                                                                 AS passport_uid,
    $get_md5_without_pickle(passport_login)                                     AS passport_login_hash,
    $get_settings(`settings`, 'language')                                       AS language,
    $get_settings_hash(`settings`, 'phone')                                     AS phone_hash,
    $get_settings_hash(`settings`, 'email')                                     AS email_hash,
    $get_settings_dict(`settings`, 'experiments')                               AS experiments,
    NVL(created_clouds, 0)                                                      AS created_clouds,
    NVL(cloud_creation_limit, 0)                                                AS cloud_creation_limit,
    $get_datetime(created_at)                                                   AS created_at,
    $get_datetime(deleted_at)                                                   AS deleted_at,
    $get_datetime(modified_at)                                                  AS modified_at,
    $get_settings_flag(`settings`, 'mail_tech', $DEFAULT_MAIL_SUBSCRIPTION)     AS mail_tech,
    $get_settings_flag(`settings`, 'mail_billing', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_billing,
    $get_settings_flag(`settings`, 'mail_testing', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_testing,
    $get_settings_flag(`settings`, 'mail_info', $DEFAULT_MAIL_SUBSCRIPTION)     AS mail_info,
    $get_settings_flag(`settings`, 'mail_feature', $DEFAULT_MAIL_SUBSCRIPTION)  AS mail_feature,
    $get_settings_flag(`settings`, 'mail_event', $DEFAULT_MAIL_SUBSCRIPTION)    AS mail_event,
    $get_settings_flag(`settings`, 'mail_promo', $DEFAULT_MAIL_SUBSCRIPTION)    AS mail_promo,
    $get_settings_flag(`settings`, 'mail_alerting', $DEFAULT_MAIL_SUBSCRIPTION) AS mail_alerting,
FROM
    $users_with_creation_time
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
        created_clouds,
        cloud_creation_limit,
        created_at,
        deleted_at,
        modified_at,
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
        iam_uid;

INSERT INTO $PII_dst_table WITH TRUNCATE
SELECT
    iam_uid,
    phone,
    email,
    passport_login
FROM (
    SELECT
        iam_uid, 
        phone, 
        email,
        passport_login
    FROM $PII_dst_table
    WHERE iam_uid NOT IN (SELECT id FROM $users_with_creation_time)

    UNION ALL
    
    SELECT
        id                                  AS iam_uid,
        $get_settings(`settings`, 'phone')  AS phone,
        $get_settings(`settings`, 'email')  AS email,
        passport_login                      AS passport_login
    FROM
        $users_with_creation_time
    )
ORDER BY
    iam_uid;
