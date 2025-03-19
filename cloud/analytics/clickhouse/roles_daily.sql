CREATE TABLE %(table_name)s
(
    billing_account_id String,
    date Date,
    role String,
    user_name String,
    share Int32
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(date) PARTITION BY toYYYYMM(date)
