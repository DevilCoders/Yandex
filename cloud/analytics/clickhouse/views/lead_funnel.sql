CREATE VIEW cloud_analytics_testing.lead_funnel (
  lead_id Nullable(String),
  lead_email Nullable(String),
  billing_account_id Nullable(String),
  puid Nullable(String),
  lead_source Nullable(String),
  ba_person_type Nullable(String),
  ba_state Nullable(String),
  ba_usage_status Nullable(String),
  block_reason Nullable(String),
  sales_name Nullable(String),
  events Array(String),
  events_times Array(DateTime),
  events_times_index Array(UInt8),
  calls UInt64,
  trial_comsumption_totals Array(Float64),
  paid_comsumption_totals Array(Float64),
  time DateTime,
  new UInt8,
  new_assigned UInt8,
  assigned_inprocess UInt8,
  inprocess_recycled UInt8,
  inprocess_paid_consumption UInt8,
  inprocess_converted UInt8,
  converted_paid_consumption UInt8,
  recycled_paid_consumption UInt8
) AS
SELECT
  lead_id,
  lead_email,
  billing_account_id,
  puid,
  lead_source,
  ba_person_type,
  ba_state,
  ba_usage_status,
  block_reason,
  sales_name,
  groupArray(lead_state) AS events,
  groupArray(event_time) AS events_times,
  arrayMap(
    x - > (x > toDate('2017-01-01')),
    groupArray(event_time)
  ) AS events_times_index,
  arraySum(
    arrayMap(x - > (x = 'call'), groupArray(lead_state))
  ) AS calls,
  groupArray(trial_comsumption_total) AS trial_comsumption_totals,
  groupArray(paid_comsumption_total) AS paid_comsumption_totals,
  arrayFilter(
    (time, name) - > (name = 'New'),
    events_times,
    events
  ) [1] AS time,
  arrayFilter(
    (events_times_index, time, name) - > (name = 'New'),
    events_times_index,
    events_times,
    events
  ) [1] AS new,
  arrayFilter(
    (events_times_index, time, name) - > (
      (name = 'Assigned')
      AND (time >= new)
      AND (new != 0)
    ),
    events_times_index,
    events_times,
    events
  ) [1] AS new_assigned,
  arrayFilter(
    (events_times_index, time, name) - > (
      (name = 'call')
      AND (time >= new_assigned)
      AND (new_assigned != 0)
    ),
    events_times_index,
    events_times,
    events
  ) [1] AS assigned_inprocess,
  arrayFilter(
    (events_times_index, time, name) - > (
      (name = 'Recycled')
      AND (time >= assigned_inprocess)
      AND (assigned_inprocess != 0)
    ),
    events_times_index,
    events_times,
    events
  ) [1] AS inprocess_recycled,
  arrayFilter(
    (events_times_index, time, name) - > (
      (name = 'Paid Consumption')
      AND (time >= assigned_inprocess)
      AND (assigned_inprocess != 0)
    ),
    events_times_index,
    events_times,
    events
  ) [1] AS inprocess_paid_consumption,
  arrayFilter(
    (events_times_index, time, name) - > (
      (name = 'Converted')
      AND (time >= assigned_inprocess)
      AND (assigned_inprocess != 0)
    ),
    events_times_index,
    events_times,
    events
  ) [1] AS inprocess_converted,
  arrayFilter(
    (events_times_index, time, name) - > (
      (name = 'Paid Consumption')
      AND (time >= inprocess_converted)
      AND (inprocess_converted != 0)
    ),
    events_times_index,
    events_times,
    events
  ) [1] AS converted_paid_consumption,
  arrayFilter(
    (events_times_index, time, name) - > (
      (name = 'Paid Consumption')
      AND (time >= inprocess_recycled)
      AND (inprocess_recycled != 0)
    ),
    events_times_index,
    events_times,
    events
  ) [1] AS recycled_paid_consumption
FROM
  (
    SELECT
      *
    FROM
      cloud_analytics_testing.crm_lead_cube_test
    ORDER BY
      event_time ASC,
      lead_state DESC
  )
GROUP BY
  lead_id,
  lead_email,
  billing_account_id,
  puid,
  lead_source,
  ba_person_type,
  ba_state,
  ba_usage_status,
  block_reason,
  sales_name