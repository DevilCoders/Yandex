PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt, $from_int64_to_ts;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

PRAGMA yt.InferSchema = '1';

$result = (
SELECT
    CAST(cloud_id AS String)                                    AS cloud_id,
    CAST(cluster AS String)                                     AS cluster,
    CAST(cpu_name as String)                                    AS cpu_name,
    CAST(folder_id AS String)                                   AS folder_id,
    CAST(host AS String)                                        AS host,
    CAST(instance_id AS String)                                 AS instance_id,
    CAST(metric AS String)                                      AS metric,
    CAST(project AS String)                                     AS project,
    CAST(service AS String)                                     AS service,
    CAST(value AS Double)                                       AS value,
    $from_utc_ts_to_msk_dt($from_int64_to_ts(`timestamp`))      AS metric_dttm_local,
    $from_int64_to_ts(`timestamp`)                              AS metric_ts

 FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
