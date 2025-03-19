PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string, $lookup;

$cluster =    {{ cluster -> table_quote() }} ;

$src_folder = {{ param["source_folder_path"] -> quote()       }} ;
$dst_table =  {{ input1 -> table_quote() }} ;

/* Читаем последний снепшот снапшот */
$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

/* Делаем преобразования */
$result = (
    SELECT
        `balance_client_id`                             AS `balance_client_id`,
        `balance_contract_id`                           AS `balance_contract_id`,
        `balance_person_id`                             AS `balance_person_id`,
        `balance_person_type`                           AS `balance_person_type`,
        $get_datetime(`created_at`)                     AS `created_at`,
        `currency`                                      AS `currency`,
        `name`                                          AS `name`,
        `owner_id`                                      AS `owner_id`,
        `payment_cycle_type`                            AS `payment_cycle_type`,

        `id`                                            AS `publisher_account_id`,

        `state`                                         AS `state`,
        $get_datetime(`updated_at`)                     AS `updated_at`,
    FROM $snapshot
);

/* Сохраняем результат в ODS слое */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `publisher_account_id`;
