$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$result = (
    SELECT
        CAST(id AS String)                               AS flavor_id,
        cpu_guarantee                                    AS cpu_guarantee,
        cpu_limit                                        AS cpu_limit,
        memory_guarantee                                 AS memory_guarantee,
        memory_limit                                     AS memory_limit,
        network_guarantee                                AS network_guarantee,
        network_limit                                    AS network_limit,
        io_limit                                         AS io_limit,
        CAST(name AS String)                             AS name,
        visible                                          AS visible,
        Yson::ConvertToString(vtype)                     AS vtype,
        CAST(platform_id AS String)                      AS platform_id,
        CAST(type AS String)                             AS type,
        generation                                       AS generation,
        gpu_limit                                        AS gpu_limit,
        io_cores_limit                                   AS io_cores_limit,
        CAST(cloud_provider_flavor_name AS String)       AS cloud_provider_flavor_name
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
