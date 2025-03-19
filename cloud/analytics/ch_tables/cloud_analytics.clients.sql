CREATE TABLE cloud_analytics.clients(
  passport_uid String,
  billing_account_id String,
  source1 String,
  source2 String
) ENGINE = MergeTree() ORDER BY(passport_uid)
