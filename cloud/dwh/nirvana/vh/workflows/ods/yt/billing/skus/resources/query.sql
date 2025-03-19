PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_datatransfer_snapshot_table;
IMPORT `datetime` SYMBOLS $get_datetime;
IMPORT `helpers` SYMBOLS $lookup_string, $lookup;

$cluster = {{cluster -> table_quote()}};
$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
$offset = {{ param["transfer_offset"] }};

/* Хелпепы для преобразований */
$get_translation = ($translations, $lang) -> ($lookup_string($lookup($translations, $lang), "name", NULL));

/* Аггрегируем снапшот */
$snapshot = SELECT * FROM $select_datatransfer_snapshot_table($src_folder, $cluster, $offset);

/* Делаем преобразования */
$result = (
    SELECT
        `balance_product_id`                            AS `balance_product_id`,
        $get_datetime(`created_at`)                     AS `created_at`,
        `deprecated`                                    AS `deprecated`,
        `formula`                                       AS `formula`,
        `metric_unit`                                   AS `metric_unit`,
        `name`                                          AS `name`,
        `pricing_unit`                                  AS `pricing_unit`,
        `priority`                                      AS `priority`,
        `publisher_account_id`                          AS `publisher_account_id`,
        `rate_formula`                                  AS `rate_formula`,
        `resolving_policy`                              AS `resolving_policy`,
        `service_id`                                    AS `service_id`,

        `id`                                            AS `sku_id`,

        $get_translation(`translations`, "en")          AS `translation_en`,
        $get_translation(`translations`, "ru")          AS `translation_ru`,
        $get_datetime(`updated_at`)                     AS `updated_at`,
        `updated_by`                                    AS `updated_by`,
        `usage_type`                                    AS `usage_type`,
        `usage_unit`                                    AS `usage_unit`
    FROM $snapshot
);

/* Сохраняем результат в ODS слое */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `sku_id`;
