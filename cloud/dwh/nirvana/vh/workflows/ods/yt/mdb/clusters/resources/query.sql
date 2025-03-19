PRAGMA Library("datetime.sql");
IMPORT `datetime` SYMBOLS $parse_iso8601_string, $from_utc_ts_to_msk_dt;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$result = (
    SELECT
        CAST(cid AS String)                                                   AS cid,
        Yson::ConvertToString(type, Yson::Options(false as Strict))           AS type,
        Yson::ConvertToString(env, Yson::Options(false as Strict))            AS env,
        folder_id                                                             AS folder_id,
        CAST(network_id AS String)                                            AS network_id,
        CAST(ListConcat(Yson::ConvertToStringList(host_group_ids,
            Yson::Options(false as Strict)), ";") AS String)                  AS host_group_ids,
        CAST(name AS String)                                                  AS name,
        Yson::ConvertToString(status, Yson::Options(false as Strict))         AS status,
        deletion_protection                                                   AS deletion_protection,
        CAST(monitoring_cloud_id AS String)                                   AS monitoring_cloud_id,
        $from_utc_ts_to_msk_dt($parse_iso8601_string(created_at))             AS create_dttm_local,
        $parse_iso8601_string(created_at)                                     AS create_ts
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
