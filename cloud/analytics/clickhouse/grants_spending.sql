CREATE TABLE %(table_name)s
(
    id String,
    billing_account_id String,
    start_time UInt64,
    end_time UInt64,
    initial_amount Float64,
    consumed_amount Float64    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(id) 
