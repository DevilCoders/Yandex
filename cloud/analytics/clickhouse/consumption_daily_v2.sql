CREATE TABLE %(table_name)s
(
    date DateTime,
    billing_account_id String,
    cloud_id String,
    service_name String,
    subservice_name String,
    core_fraction String,
    preemptible String,
    platform String,
    sku_name String,
    block_reason String,
    cons_sum Float64,
    cons_type String,
    state String,
    client_name String,
    segment String,
    size String,
    last_sales_name String,
    last_state String,
    last_block_reason String,
    sales_name String,
    master_account_id String,
    database String,
    person_type String,
    created_at DateTime,
    channel String,
    usage_status String,
    cons_dyn_status String,
    paid_cons_dyn_status String,
    total String

)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(date, billing_account_id, cloud_id, sku_name) PARTITION BY toYYYYMM(date)

--gggg
