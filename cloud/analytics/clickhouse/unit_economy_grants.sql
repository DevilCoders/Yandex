CREATE TABLE %(table_name)s
(
    billing_account_id Nullable(String),
    amount Float64,
    start_time DateTime,
    end_time DateTime
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(start_time) PARTITION BY toYYYYMM(start_time)