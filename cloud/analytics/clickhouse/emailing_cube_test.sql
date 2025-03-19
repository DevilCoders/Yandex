CREATE TABLE %(table_name)s
(
        ba_became_paid_time Nullable(DateTime),
        ba_created_time Nullable(DateTime),
        billing_account_id Nullable(String),
        ba_state Nullable(String),
        block_reason Nullable(String),
        click_time Nullable(DateTime),
        cloud_created_time Nullable(DateTime),
        delivery_time Nullable(DateTime),
        email Nullable(String),
        event String,
        event_time DateTime,
        first_paid_consumption_time Nullable(DateTime),
        first_trial_consumption_time Nullable(DateTime),
        is_ba_became_paid Nullable(Int64),
        is_ba_created Nullable(Int64),
        is_cloud_created Nullable(Int64),
        is_first_paid_consumption Nullable(Int64),
        is_first_trial_consumption Nullable(Int64),
        mail_id Nullable(String),
        mailing_name Nullable(String),
        open_time Nullable(DateTime),
        puid Nullable(String),
        paid_consumption Nullable(Float64),
        is_paid_more_then_10_rur Nullable(Int64),
        is_trial_more_then_10_rur Nullable(Int64),
        trial_consumption Nullable(Float64)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(event_time) PARTITION BY toYYYYMM(event_time)