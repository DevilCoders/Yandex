CREATE TABLE %(table_name)s
(
    date String,
    created_by String,
    amount Float64,
    billing_account_id String, 
    account_name String, 
    sales_name String
    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(date) PARTITION BY toYYYYMM(toDate(date))

