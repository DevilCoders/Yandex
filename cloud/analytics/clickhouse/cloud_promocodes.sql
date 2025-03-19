CREATE TABLE %(table_name)s
(
    duration UInt64,
    promocode_id String,
    promocode_reason String,
    promocode_amount Float64,
    expiration_time DateTime,
    puid String,
    proposed_to String,
    event_time DateTime
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(event_time) PARTITION BY toYYYYMM(event_time)