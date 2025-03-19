PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_datetime, $DATETIME_INF;
IMPORT `helpers` SYMBOLS $yson_to_str;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"]->quote() }};
$dst_table = {{ input1 -> table_quote() }};

$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

/* Делаем преобразования */
$result = (
    SELECT
        `billing_account_id`                    AS `billing_account_id`,
        $get_datetime($yson_to_str(created_at)) AS `created_at`,
        $get_datetime(`effective_time`)         AS `start_time`,
        COALESCE($get_datetime(LEAD(`effective_time`) OVER `w` - 1), $DATETIME_INF) AS `end_time`,
        `service_instance_id`                   AS `service_instance_id`,
        `service_instance_type`                 AS `service_instance_type`
    FROM $snapshot
    WINDOW `w` AS (
        PARTITION BY `service_instance_type`, `service_instance_id`
        ORDER BY `effective_time`
    )
);

/* Сохраняем результат в ODS слое */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `service_instance_type`, `service_instance_id`, `start_time`;
