PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");
PRAGMA Library("billing_accounts.sql");


IMPORT `tables` SYMBOLS $select_datatransfer_snapshot_table;
IMPORT `numbers` SYMBOLS $to_decimal_35_15;
IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_bool, $lookup_string_list, $lookup_int64, $lookup_string;
IMPORT `billing_accounts` SYMBOLS $is_suspended_by_antifraud;


$cluster = {{ cluster -> table_quote() }};
$src_folder = {{ param["source_folder_path"] -> quote() }};
$offset = {{ param["transfer_offset"] }};
$dst_table = {{ input1 -> table_quote() }};

/* Хелпепы для преобразований */
-- Достаем нужный feature_flag. Дефолтное значение - false.
$get_feature_flag = ($yson, $feature_flag) -> ($lookup_bool($yson, $feature_flag, FALSE));

-- Обработка метадаты
$get_bool_from_metadata = ($metadata, $field, $default) -> ($lookup_bool($metadata, $field, $default));
$get_string_list_from_metadata = ($metadata, $field) -> ($lookup_string_list($metadata, $field));
$get_string_from_metadata = ($metadata, $field, $default) -> ($lookup_string($metadata, $field, $default));

$get_autopay_failures = ($metadata) -> ($lookup_int64($metadata, "autopay_failures", 0));
$get_is_fraud = ($metadata) -> (ListHasItems($get_string_list_from_metadata($metadata, "fraud_detected_by")));

$snapshot = SELECT * FROM $select_datatransfer_snapshot_table($src_folder, $cluster, $offset);

/* Делаем преобразования */
$result = (
    SELECT
        $to_decimal_35_15(`balance`)                                            AS `balance`,
        `balance_client_id`                                                     AS `balance_client_id`,
        `balance_contract_id`                                                   AS `balance_contract_id`,
        `id`                                                                    AS `billing_account_id`,
        $to_decimal_35_15(`billing_threshold`)                                  AS `billing_threshold`,
        `country_code`                                                          AS `country_code`,
        $get_datetime(`created_at`)                                             AS `created_at`,
        `currency`                                                              AS `currency`,
        $get_datetime(`disabled_at`)                                            AS `disabled_at`,

        -- feature_flags
        $get_feature_flag(`feature_flags`, "partner_access")                    AS `has_partner_access`,
        $get_feature_flag(`feature_flags`, "isv")                               AS `is_isv`,
        $get_feature_flag(`feature_flags`, "referral")                          AS `is_referral`,
        $get_feature_flag(`feature_flags`, "referrer")                          AS `is_referrer`,
        $get_feature_flag(`feature_flags`, "var")                               AS `is_var`,

        `master_account_id`                                                     AS `master_account_id`,
        NVL(`master_account_id`, '') != ''                                      AS `is_subaccount`,

        -- metadata
        $get_string_list_from_metadata(`metadata`, "auto_grant_policies")       AS `auto_grant_policies`,
        $get_autopay_failures(`metadata`)                                       AS `autopay_failures`,
        $get_string_from_metadata(`metadata`, "block_comment", NULL)            AS `block_comment`,
        $get_string_from_metadata(`metadata`, "block_ticket", NULL)             AS `block_ticket`,
        $get_string_from_metadata(`metadata`, "block_reason", NULL)             AS `block_reason`,
        $get_string_list_from_metadata(`metadata`, "fraud_detected_by")         AS `fraud_detected_by`,
        $get_bool_from_metadata(`metadata`, "floating_threshold", FALSE)        AS `has_floating_threshold`,
        $get_bool_from_metadata(`metadata`, "verified", FALSE)                  AS `is_verified`,
        $get_is_fraud(`metadata`)                                               AS `is_fraud`,
        $get_string_list_from_metadata(`metadata`, "idempotency_checks")        AS `idempotency_checks`,
        $get_string_from_metadata(`metadata`, "registration_ip", NULL)          AS `registration_ip`,
        $get_string_from_metadata(`metadata`, "registration_user_iam", NULL)    AS `registration_iam_uid`,
        $get_string_from_metadata(`metadata`, "unblock_reason", NULL)           AS `unblock_reason`,

        `name`                                                                  AS `name`,
        `owner_id`                                                              AS `owner_passport_uid`,
        `owner_iam_id`                                                          AS `owner_iam_uid`,
        $get_datetime(`paid_at`)                                                AS `paid_at`,
        `payment_cycle_type`                                                    AS `payment_cycle_type`,
        `payment_method_id`                                                     AS `payment_method_id`,
        `payment_type`                                                          AS `payment_type`,
        `person_type`                                                           AS `person_type`,
        `state`                                                                 AS `state`,
        `type`                                                                  AS `type`,
        $get_datetime(`updated_at`)                                             AS `updated_at`,
        `usage_status`                                                          AS `usage_status`
    FROM $snapshot
);


$result = (
    SELECT
        result.*,
        $is_suspended_by_antifraud(TableRow())        AS is_suspended_by_antifraud
    FROM $result as result
);

/* Сохраняем результат в ODS слое */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `billing_account_id`;
