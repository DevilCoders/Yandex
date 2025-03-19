CREATE TABLE cloud_analytics.smb_plans
(
    date Date,
    segment String,
    paid_consumption_plan Float64
) ENGINE = MergeTree()
ORDER BY date PARTITION BY toYYYYMM(date);