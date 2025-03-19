CREATE TABLE %(table_name)s
(
    date DateTime,
    billing_account_id String,
    ba_curr_state String,
    cloud_id String,
    service1 String,
    service2 String,
    service3 String,
    service4 String,
    sku String,
    pricing_unit String,
    quantity Float32,
    cost Float32,
    credit Float32,
    paid_consumption Float32,
    usage_status String,
    offer_amount Float32,
    client_name String,
    source String,
    source2 String,
    segment String,
    sales_person String,
    idc_industry String

)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(date, billing_account_id, cloud_id, sku) PARTITION BY toYYYYMM(date)
