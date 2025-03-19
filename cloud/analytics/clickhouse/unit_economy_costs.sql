CREATE TABLE %(table_name)s
(
    billing_account_id String,
    date Date,
    costs_type String,
    costs Float64,
    account_name String,
    ba_person_type String,
    ba_usage_status String,
    ba_type String,
    sales_name String,
    segment String,
    is_fraud UInt8,
    board_segment String,
    ba_name String,
    block_reason String,
    architect String,
    costs_group String,
    ba_created_date Date,
    m_cohort String,
    ba_paid_date Date,
    sales_cycle_step String
    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(billing_account_id, date, costs_type) PARTITION BY toYYYYMM(date)


