CREATE VIEW cloud_analytics_testing.w2w_churn (
  week_next Date,
  billing_account_id Nullable(String),
  balance_name Nullable(String),
  segment Nullable(String),
  ba_state Nullable(String),
  block_reason Nullable(String),
  product Nullable(String),
  ba_usage_status Nullable(String),
  ba_person_type Nullable(String),
  sales Nullable(String),
  week_delta Int32,
  promocode_client_name Nullable(String),
  first_name Nullable(String),
  last_name Nullable(String),
  real_consumption Nullable(Float64),
  real_consumption_next_period Nullable(Float64),
  total_real_consumption Nullable(Float64),
  trial_consumption Nullable(Float64),
  trial_consumption_next_period Nullable(Float64),
  total_trial_consumption Nullable(Float64),
  total_consumption Nullable(Float64),
  total_consumption_next_period Nullable(Float64),
  consumption Nullable(Float64)
) AS
SELECT
  week_next,
  billing_account_id,
  multiIf(
    isNotNull(balance_name)
    AND (balance_name != 'unknown'),
    balance_name,
    isNotNull(promocode_client_name)
    AND (promocode_client_name != 'unknown'),
    promocode_client_name,
    CONCAT(first_name, ' ', last_name)
  ) AS balance_name,
  segment,
  ba_state,
  block_reason,
  product,
  ba_usage_status,
  multiIf(
    isNull(ba_person_type),
    'unknown',
    ba_person_type
  ) AS ba_person_type,
  sales,
  week_delta,
  promocode_client_name,
  first_name,
  last_name,
  real_consumption,
  real_consumption_next_period,
  real_consumption_cum AS total_real_consumption,
  trial_consumption,
  trial_consumption_next_period,
  trial_consumption_cum AS total_trial_consumption,
  real_consumption + trial_consumption AS total_consumption,
  trial_consumption_next_period + real_consumption_next_period AS total_consumption_next_period,
  real_consumption_cum + trial_consumption_cum AS consumption
FROM
  cloud_analytics_testing.retention_cube_test
WHERE
  week_delta = 1
UNION ALL
SELECT
  week AS week_next,
  billing_account_id,
  multiIf(
    isNotNull(balance_name)
    AND (balance_name != 'unknown'),
    balance_name,
    isNotNull(promocode_client_name)
    AND (promocode_client_name != 'unknown'),
    promocode_client_name,
    CONCAT(first_name, ' ', last_name)
  ) AS balance_name,
  segment,
  ba_state,
  block_reason,
  product,
  ba_usage_status,
  multiIf(
    isNull(ba_person_type),
    'unknown',
    ba_person_type
  ) AS ba_person_type,
  sales,
  week_delta,
  promocode_client_name,
  first_name,
  last_name,
  real_consumption,
  real_consumption_next_period,
  real_consumption_cum AS total_real_consumption,
  trial_consumption,
  trial_consumption_next_period,
  trial_consumption_cum AS total_trial_consumption,
  real_consumption + trial_consumption AS total_consumption,
  trial_consumption_next_period + real_consumption_next_period AS total_consumption_next_period,
  real_consumption_cum + trial_consumption_cum AS consumption
FROM
  cloud_analytics_testing.retention_cube_test
WHERE
  (week_delta = 0)
  AND (
    toMonday(toDate(first_first_trial_consumption_datetime)) = toDate(week)
  )
  AND (
    toMonday(toDate(first_first_paid_consumption_datetime)) = toDate(week)
  )