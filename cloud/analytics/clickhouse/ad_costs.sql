CREATE TABLE %(table_name)s
(
    campaign String,
    clicks Int64,
    client String,
    cost Float64,
    cost_usd Float64,
    date Date,
    goals Int64,
    impressions Int64,
    medium String,
    source String
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(date) PARTITION BY toYYYYMM(date)
