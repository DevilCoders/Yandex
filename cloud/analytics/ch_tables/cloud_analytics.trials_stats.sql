CREATE TABLE cloud_analytics.trials_stats(
  source String,
  ba_curr_state String,
  reg_week_monday DateTime,
  reg_week String,
  promocode_count Float32,
  email_opened_count Float32,
  activated_count Float32,
  created_ba_count Float32,
  consumes_trial_count Float32,
  ba_paid_count Float32,
  consumes_paid_count Float32
) ENGINE = MergeTree() PARTITION BY toYYYYMM(reg_week_monday) ORDER BY (source, reg_week_monday)

