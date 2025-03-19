CREATE TABLE %(table_name)s
(

    billing_account_id String,
    event_time DateTime,
    event_type String,
    is_fake String,
    passport_uid String,
    week String,
    month String,
    segment String,
    sales_person String,
    source String,
    source_detailed String,
    cloud_status String,
    ba_curr_state String
    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(passport_uid,billing_account_id,event_type,event_time) PARTITION BY toYYYYMM(event_time)
