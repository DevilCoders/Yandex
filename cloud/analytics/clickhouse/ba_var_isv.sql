CREATE TABLE %(table_name)s
(
    billing_account_id String,
    client_name String,
    isv UInt8,
    var UInt8
    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(billing_account_id) 
