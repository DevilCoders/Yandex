CREATE TABLE cloud_analytics.week_events (
  week DateTime,
  week_str String,
  ba_curr_state String,
  source String, 
  registered_in_console Float32,
  ba_created Float32,
  first_consumption Float32,
  ba_paid Float32,
  first_paid_consumption Float32
) ENGINE = MergeTree() PARTITION BY toYYYYMM(week) ORDER BY(week, source)