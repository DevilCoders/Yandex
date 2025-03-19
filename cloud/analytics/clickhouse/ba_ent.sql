CREATE TABLE %(table_name)s
(
    billing_account_id String,
    client_name String,
    state_extended String,
    sales_person String,
    segment String,
    first_ending_grant_initial_amount Float64,
    first_ending_grant_end_time DateTime
    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(billing_account_id) 
