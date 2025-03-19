CREATE VIEW cloud_analytics.acquisition_cube_wtf_view AS
SELECT
    assumeNotNull(multiIf(segment IS NULL OR segment = '', 'unknown', segment)) as segment,
    assumeNotNull(multiIf(ba_person_type IS NULL OR ba_person_type = '', 'unknown', ba_person_type)) as ba_person_type,
    assumeNotNull(multiIf(board_segment IS NULL OR board_segment = '', 'unknown', board_segment)) as board_segment,
    assumeNotNull(multiIf(potential IS NULL OR potential = '', 'unknown', potential)) as potential,
    assumeNotNull(multiIf(service_name IS NULL OR service_name = '', 'unknown', service_name)) as service_name,
    assumeNotNull(multiIf(name IS NULL OR name = '', 'unknown', name)) as sku_name,
    assumeNotNull(multiIf(block_reason IS NULL OR block_reason = '', 'unknown', block_reason)) as block_reason,
    assumeNotNull(multiIf(is_fraud IS NULL, 'unknown', toString(is_fraud))) as is_fraud,
    assumeNotNull(multiIf(ba_usage_status IS NULL OR ba_usage_status = '', 'unknown', ba_usage_status)) as ba_usage_status,
    assumeNotNull(multiIf(channel IS NULL OR channel = '', 'unknown', channel)) as channel,
    assumeNotNull(multiIf(first_ba_created_datetime IS NULL, toDate('2018-01-01'), toStartOfMonth(toDate(first_ba_created_datetime)))) as cohort,
    assumeNotNull(event) as event,
    assumeNotNull(event_time) as event_time,
    assumeNotNull(multiIf(real_consumption IS NULL, 0, real_consumption)) as real_consumption,
    assumeNotNull(multiIf(trial_consumption IS NULL, 0, trial_consumption)) as trial_consumption,
    assumeNotNull(multiIf(real_consumption + trial_consumption IS NULL, 0, real_consumption + trial_consumption)) as all_consumption,
    assumeNotNull(multiIf(first_first_trial_consumption_datetime IS NULL, toDateTime('0000-00-00 00:00:00'), first_first_trial_consumption_datetime)) as first_trial_consumption_datetime,
    assumeNotNull(multiIf(first_first_paid_consumption_datetime IS NULL, toDateTime('0000-00-00 00:00:00'), first_first_paid_consumption_datetime)) as first_paid_consumption_datetime,
    assumeNotNull(multiIf(first_ba_created_datetime IS NULL, toDateTime('0000-00-00 00:00:00'), first_ba_created_datetime)) as first_ba_created_datetime,
    assumeNotNull(multiIf(first_cloud_created_datetime IS NULL, toDateTime('0000-00-00 00:00:00'), first_cloud_created_datetime)) as first_cloud_created_datetime
FROM cloud_analytics.acquisition_cube