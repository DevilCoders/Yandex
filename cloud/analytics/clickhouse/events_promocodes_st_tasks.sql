CREATE TABLE %(table_name)s
(
    task_key String,
    task_title String,
    created_at DateTime
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(created_at) PARTITION BY toYYYYMM(created_at)
