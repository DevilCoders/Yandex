CREATE TABLE %(table_name)s
(
    date DateTime,
    metric String,
    node_name String,
    zone_id String,
    node_type String, 
    node_mem_type String,
    min Float32,
    avg Float32,
    max Float32

)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(date, metric, node_name, zone_id) PARTITION BY toYYYYMM(date)
