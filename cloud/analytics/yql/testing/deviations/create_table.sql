CREATE TABLE testing.consumption_deviation
(
    date DateTime,
    service String,
    billing_account_id String,
    delta_total_pct Float32,
    usage_status String,
    delta_cost Float32,
    name String,
    generated_at DateTime,
    state String,
    delta_cost_pct Float32,
    delta_total Float32,
    longname String

 


)
ENGINE = MergeTree()
ORDER BY(date, service, billing_account_id) PARTITION BY toYYYYMM(date)