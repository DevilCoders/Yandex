PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $from_utc_ts_to_msk_dt;
IMPORT `helpers` SYMBOLS $to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_ts = ($container)  -> ($get_datetime(cast($container AS Uint64)));
$to_dttm_local = ($container)  -> ($from_utc_ts_to_msk_dt($get_datetime(cast($container AS Uint64))));

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    a.*,
    translations.en.name                            AS support_template_name_en,
    translations.ru.name                            AS support_template_name_ru
WITHOUT
    translations
FROM (
    SELECT
        $to_ts(`created_at`)                        AS created_ts,
        $to_dttm_local(`created_at`)                AS created_dttm_local,
        $to_str(`fixed_consumption_schema`)         AS fixed_consumption_schema,
        $to_str(`id`)                               AS support_template_id,
        $to_str(`name`)                             AS support_template_name,
        `is_subscription_allowed`                   AS is_subscription_allowed,
        $to_str(`percentage_consumption_schema`)    AS percentage_consumption_schema,
        `priority_level`                            AS priority_level,
        $to_str(`state`)                            AS state,
        Yson::ConvertTo(translations,
                Struct<en: Struct<name: String?>?,ru: Struct<name: String?>? >?)
                                                    AS translations,
        `updated_at`,
        $to_ts(`updated_at`)                        AS updated_ts,
        $to_dttm_local(`updated_at`)                AS updated_dttm_local,
    FROM $select_transfer_manager_table($src_folder, $cluster)
) AS a
order by support_template_id
