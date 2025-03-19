CREATE TABLE cloud_analytics.compute_stats(
date DateTime,
zone_id String,
node_name String,
cores_free Float32,
cores_used Float32,
cores_total Float32,
memory_free Float32,
memory_used Float32,
memory_total Float32
) ENGINE = MergeTree() PARTITION BY toYYYYMM(date) ORDER BY (date, node_name)

