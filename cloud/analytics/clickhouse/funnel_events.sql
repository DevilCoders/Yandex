CREATE TABLE %(table_name)s
(
    t DateTime,
    total String,
    billing_account_id String,
    event_time DateTime,
    event_day DateTime,
    event_week DateTime,
    event_month DateTime,
    event_type String,
    is_fake String,
    passport_uid String,
    week String,
    month String,
    segment String,
    sales_person String,
    channel String,
    source String,
    cloud_status String,
    ba_curr_state String,
    ba_curr_state_extended String,
    person_type String,
    total_trial_consumption Float32,
    total_paid_consumption Float32,
    total_consumption Float32
    
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(passport_uid,billing_account_id,event_type,event_time) PARTITION BY toYYYYMM(event_time)


