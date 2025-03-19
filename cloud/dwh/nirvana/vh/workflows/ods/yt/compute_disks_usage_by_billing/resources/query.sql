PRAGMA Library("datetime.sql");
PRAGMA Library("tables.sql");

IMPORT `tables` SYMBOLS $get_max_date_from_table_path;
IMPORT `datetime` SYMBOLS $format_msk_hour_by_timestamp;
IMPORT `datetime` SYMBOLS $get_date_range_inclusive;

$cluster = {{cluster->table_quote()}};

/* На входе таблица потребления от биллинга */
$src_table_analytics_cube = {{ param["analytics_cube"]->quote() }};
$dst_table_disks_usage_by_billing = {{ param["destination_folder_path"]->quote() }};

$default_min_date = "2020-07-01"; -- переход на realtime

/* Получаем название DC по префиксу диска */
$get_datacenter_by_prefix = ($resource_id) -> { RETURN CASE
    WHEN String::StartsWith($resource_id, 'ef3') THEN 'myt'
    WHEN String::StartsWith($resource_id, 'epd') THEN 'sas'
    WHEN String::StartsWith($resource_id, 'fhm') THEN 'vla'
    ELSE "unknown" END
};

/*
Определяем границы для забора данных:
    `$end_date`   - текущий день
    `$start_date` - сдвинутый на -1 месяц последний посчитанный день либо начало 2020-07-01
*/
$end_date = CurrentUtcDate();
$start_date = (
    SELECT NVL(
        DateTime::MakeDate(DateTime::ShiftMonths(max_date, -1)),
        CAST($default_min_date as Date) -- Default
    ) FROM $get_max_date_from_table_path($dst_table_disks_usage_by_billing, $cluster)
);

/* Извлекаем только потребление по дискам из общей детализации по выбранному date range */
$result = (
    SELECT
        billing_account_id,
        billing_account_usage_status_at_moment as billing_account_status,
        billing_account_state_at_moment as billing_account_state,
        billing_record_date as `date`,
        $format_msk_hour_by_timestamp(billing_record_end_time) as `hour`,
        billing_record_resource_id as disk_id,
        billing_record_nbs_size as disk_size,
        billing_record_nbs_type as disk_type,
        billing_record_cloud_id as cloud_id,
        $get_datacenter_by_prefix(billing_record_resource_id) as datacenter
    FROM $src_table_analytics_cube WHERE `billing_record_nbs_size` is not null
    AND `billing_record_date` >= cast($start_date as String)
);

/* Сохраняем результат, записываем дневные таблицы */
EVALUATE FOR $date IN $get_date_range_inclusive($start_date, $end_date) DO BEGIN
    $dst_table = ($dst_table_disks_usage_by_billing || "/" || cast($date as String));
    INSERT INTO $dst_table WITH TRUNCATE
        SELECT * from $result WHERE `date` = cast($date as String)
END DO;
