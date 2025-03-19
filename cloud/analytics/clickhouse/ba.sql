CREATE TABLE %(table_name)s
(
    billing_account_id String,
    date DateTime,
    client_name String,
    state String,
    block_reason String,
    sales_person String,
    segment String,
    first_ending_grant_initial_amount Float64,
    first_ending_grant_end_time DateTime,
    isv UInt8,
    var UInt8,
    master_account_id String
    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(billing_account_id) 
