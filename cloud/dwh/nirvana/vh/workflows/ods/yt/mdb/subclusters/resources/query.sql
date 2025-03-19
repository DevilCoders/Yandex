PRAGMA Library("datetime.sql");
IMPORT `datetime` SYMBOLS $parse_iso8601_string, $from_utc_ts_to_msk_dt;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$result = (
    SELECT
        CAST(cid AS String)                                                   AS cid,
        CAST(name AS String)                                                  AS name,
        CAST(subcid AS String)                                                AS subcid,
        CAST(ListConcat(Yson::ConvertToStringList(roles), ";") AS String)     AS roles,
        $from_utc_ts_to_msk_dt($parse_iso8601_string(created_at))             AS create_dttm_local,
        $parse_iso8601_string(created_at)                                     AS create_ts
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
