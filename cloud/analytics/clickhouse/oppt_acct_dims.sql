CREATE TABLE %(table_name)s
(
    oppt_id Nullable(String),
    oppt_name Nullable(String),
    oppt_scenario Nullable(String),
    event_time DateTime,
    event String,
    oppt_description Nullable(String),
    oppt_sales_status Nullable(String),
    billing_account_id Nullable(String),
    dimentions Nullable(String)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(event_time) PARTITION BY toYYYYMM(event_time)