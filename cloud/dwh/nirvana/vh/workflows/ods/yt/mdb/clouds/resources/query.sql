$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$result = (
    SELECT
        cloud_id                      AS cloud_id,
        CAST(cloud_ext_id AS String)  AS cloud_ext_id,
        cpu_quota                     AS cpu_quota,
        memory_quota                  AS memory_quota,
        clusters_quota                AS clusters_quota,
        cpu_used                      AS cpu_used,
        memory_used                   AS memory_used,
        clusters_used                 AS clusters_used,
        actual_cloud_rev              AS actual_cloud_rev,
        ssd_space_quota               AS ssd_space_quota,
        ssd_space_used                AS ssd_space_used,
        hdd_space_quota               AS hdd_space_quota,
        hdd_space_used                AS hdd_space_used,
        gpu_quota                     AS gpu_quota,
        gpu_used                      AS gpu_used
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
  SELECT * FROM $result
