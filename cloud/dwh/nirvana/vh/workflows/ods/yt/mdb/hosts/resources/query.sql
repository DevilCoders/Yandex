PRAGMA Library("datetime.sql");
IMPORT `datetime` SYMBOLS $parse_iso8601_string, $from_utc_ts_to_msk_dt;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$result = (
    SELECT
        CAST(subcid AS String)                                                AS subcid,
        CAST(shard_id AS String)                                              AS shard_id,
        CAST(flavor AS String)                                                AS flavor_id,
        space_limit                                                           AS disk_space_limit,
        CAST(fqdn AS String)                                                  AS fqdn,
        CAST(vtype_id AS String)                                              AS vtype_id,
        geo_id                                                                AS geo_id,
        disk_type_id                                                          AS disk_type_id,
        CAST(subnet_id AS String)                                             AS subnet_id,
        assign_public_ip                                                      AS assign_public_ip,
        $from_utc_ts_to_msk_dt($parse_iso8601_string(created_at))             AS create_dttm_local,
        $parse_iso8601_string(created_at)                                     AS create_ts
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
