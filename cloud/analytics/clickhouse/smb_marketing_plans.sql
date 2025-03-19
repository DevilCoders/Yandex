CREATE TABLE %(table_name)s
(
    metric String,
    ba_person_type String,
    traffic_source String,
    date Date,
    value Float64
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(date) PARTITION BY toYYYYMM(date)