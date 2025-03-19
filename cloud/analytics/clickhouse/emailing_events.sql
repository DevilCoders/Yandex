CREATE TABLE %(table_name)s
(
    billing_account_id Nullable(String),
    event String,
    event_time DateTime,
    mail_id String,
    mailing_name String,
    program_name String,
    stream_name String,
    mailing_id Int64,
    email String,
    puid Nullable(String),
    ba_state Nullable(String),
    block_reason Nullable(String),
    is_fraud Nullable(Int64),
    ba_usage_status Nullable(String),
    segment Nullable(String),
    account_name Nullable(String),
    cloud_created_time Nullable(String),
    ba_created_time Nullable(String),
    first_trial_consumption_time Nullable(String),
    first_paid_consumption_time Nullable(String)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(event_time, event, mail_id, program_name, stream_name) PARTITION BY toYYYYMM(event_time)