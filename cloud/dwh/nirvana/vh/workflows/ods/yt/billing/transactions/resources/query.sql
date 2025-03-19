PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `numbers` SYMBOLS $to_decimal_35_15;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{cluster->table_quote()}};

/* Logfeller's log */
$src_folder = {{ param["source_folder_path"]->quote() }};
$dst_table = {{input1->table_quote()}};

/* Aggregate snapshot */
$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

/* Make transformations */

$transactions_raw =
SELECT
    `id` AS `transaction_id`,
    `type` AS `transaction_type`,
    `billing_account_id` AS `billing_account_id`,
    $to_decimal_35_15(`amount`) AS `amount`,
    `status` AS `status`,
    `description` AS `description`,
    `currency` AS `currency`,
    `context` AS `context`,
    $get_datetime(`created_at`) as `created_at`,
    $get_datetime(`modified_at`) as `modified_at`
FROM $snapshot;

$transactions_unpacked_by_context =
SELECT
    `transaction_id`,
    `transaction_type`,
    `billing_account_id`,
    `amount`,
    `status`,
    `description`,
    `currency`,
    NVL(Yson::LookupBool(context, 'aborted', Yson::Options(false as Strict)), false) as `is_aborted`,
    Yson::LookupString(context, 'contract_id', Yson::Options(false as Strict)) as `balance_contract_id`,
    Yson::LookupString(context, 'operation_id', Yson::Options(false as Strict)) as `operation_id`,
    Yson::LookupString(context, 'passport_uid', Yson::Options(false as Strict)) as `passport_uid`,
    Yson::LookupString(context, 'payment_type', Yson::Options(false as Strict)) as `payment_type`,
    Yson::LookupString(context, 'paymethod_id', Yson::Options(false as Strict)) as `paymethod_id`,
    Yson::LookupString(context, 'person_id', Yson::Options(false as Strict)) as `balance_person_id`,
    Yson::LookupString(context, 'request_id', Yson::Options(false as Strict)) as `request_id`,
    NVL(Yson::LookupBool(context, 'secure', Yson::Options(false as Strict)), false) as `is_secure`,
    Yson::YPathInt64(context, '/payload/service_force_3ds', Yson::Options(false as Strict)) as `payload_service_force_3ds`,
    `created_at`,
    `modified_at`
FROM $transactions_raw;

/* Save result */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $transactions_unpacked_by_context
ORDER BY `transaction_type`, `transaction_id`;
