CREATE TABLE cloud_analytics.trials_by_weeks (
  tag String,
  ba_curr_state String,
  date DateTime,
  type String,
  cum_count Float32,
  tag_count Float32,
  cum_count_pct Float32
) ENGINE = MergeTree() PARTITION BY toYYYYMM(date) ORDER BY(type, tag, date)