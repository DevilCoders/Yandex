PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $concat_path, $get_missing_or_updated_tables;
IMPORT `datetime` SYMBOLS $parse_iso8601_string, $from_utc_ts_to_msk_dt;

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_folder = {{ input1 -> table_quote() }};

$date_parse = DateTime::Parse("%Y-%m-%d");

$cluster = {{ cluster -> table_quote() }};
$start_of_history = {{ param["start_of_history"] -> quote() }};
$updates_start_dt = CAST(CurrentUtcDate() - DateTime::IntervalFromDays(2) as DateTime);
$load_iteration_limit = 30;

$tables_to_load = (
    SELECT *
    FROM $get_missing_or_updated_tables($cluster, $src_folder, $dst_folder, $start_of_history, $updates_start_dt, $load_iteration_limit)
);

DEFINE ACTION $insert_partition($date) AS
    $source_table = $concat_path($src_folder, $date);
    $destination_table = $concat_path($dst_folder, $date);

    INSERT INTO $destination_table WITH TRUNCATE
    SELECT
        id as id,
        cloud_id as cloud_id,
        folder_id as folder_id,
        source_id as host_id,
        Yson::LookupString(labels, 'managed-kubernetes-cluster-id')                 AS managed_kubernetes_cluster_id,
        Yson::LookupString(labels, 'managed-kubernetes-node-group-id')              AS managed_kubernetes_node_group_id,
        Yson::LookupUint64(tags, 'cores')                                           AS cores,
        Yson::LookupUint64(tags, 'gpus')                                            AS gpus,
        Yson::LookupUint64(tags, 'core_fraction')                                   AS core_fraction,
        Yson::LookupUint64(tags, 'memory')                                          AS memory,
        Yson::LookupString(tags, 'platform_id')                                     AS platform_id,
        Yson::LookupBool(tags, 'preemptible')                                       AS preemptible,
        Yson::LookupUint64(tags, 'public_fips')                                     AS public_fips,
        Yson::ConvertToStringList(Yson::Lookup(tags, 'product_ids'))                AS product_ids,
        Yson::LookupString(tags, 'cluster_id')                                      AS cluster_id,
        Yson::LookupString(tags, 'cluster_type')                                    AS cluster_type,
        Yson::LookupUint64(tags, 'disk_size')                                       AS disk_size,
        Yson::LookupString(tags, 'disk_type_id')                                    AS disk_type_id,
        Yson::LookupUint64(tags, 'online')                                          AS online,
        Yson::LookupString(tags, 'resource_preset_id')                              AS resource_preset_id,
        Yson::LookupString(tags, 'compute_instance_id')                             AS compute_instance_id,
        Yson::LookupUint64(usage, 'start')                                          AS usage_start,
        Yson::LookupUint64(usage, 'finish')                                         AS usage_finish,
        Yson::LookupUint64(usage, 'quantity', Yson::Options(true as AutoConvert))   AS usage_quantity,
        Yson::LookupString(usage, 'type')                                           AS usage_type,
        Yson::LookupString(usage, 'unit')                                           AS usage_unit,
        resource_id                                                                 AS vm_id,
        schema                                                                      AS schema,
        $parse_iso8601_string(iso_eventtime)                                        AS eventtime_ts,
        $from_utc_ts_to_msk_dt($parse_iso8601_string(iso_eventtime))                AS eventtime_dttm_local
    FROM $source_table
    WHERE schema in ('compute.vm.generic.v1', 'mdb.db.generic.v1') or schema like 'mk8s%'
END DEFINE;


EVALUATE FOR $date IN $tables_to_load DO BEGIN
    DO $insert_partition($date)
END DO;
