CREATE TABLE invalid_metrics
(
    `source_name` String,
    `uploaded_at` DateTime DEFAULT NOW(),
    `date` Date DEFAULT toDate(uploaded_at),
    `reason` String,
    `metric_id` String,
    `hostname` String,
    `source_id` String,
    `reason_comment` String,
    `raw_metric` Nullable(String),
    `metric` Nullable(String),
    `metric_source_id` Nullable(String),
    `metric_schema` Nullable(String),
    `metric_resource_id` Nullable(String)
)
ENGINE = MergeTree()
PARTITION BY toYYYYMMDD(date)
ORDER BY (date, source_name)
TTL date + INTERVAL 1 MONTH;
