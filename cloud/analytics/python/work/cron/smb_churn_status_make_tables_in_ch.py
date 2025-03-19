import os, sys, pandas as pd, datetime
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)
from dateutil import relativedelta
from global_variables import (
    metrika_clickhouse_param_dict,
    cloud_clickhouse_param_dict
)
from data_loader import clickhouse
from creds import (
    yt_creds,
    metrika_creds,
    yc_ch_creds,
    crm_sql_creds,
    stat_creds,
    telebot_creds
)

from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)

def main():
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    metrika_clickhouse_param_dict['user'] = metrika_creds['value']['login']
    metrika_clickhouse_param_dict['password'] = metrika_creds['value']['pass']

    cloud_clickhouse_param_dict['user'] = yc_ch_creds['value']['login']
    cloud_clickhouse_param_dict['password'] = yc_ch_creds['value']['pass']
    query = 'DROP TABLE IF EXISTS cloud_analytics_testing.churn_weekly'
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

    query = '''
    CREATE TABLE cloud_analytics_testing.churn_weekly_temp
    ENGINE = MergeTree()
    ORDER BY(week_next) PARTITION BY toYYYYMM(week_next)
     AS
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
      cloud_analytics_testing.retention_cube_weekly
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
      cloud_analytics_testing.retention_cube_weekly
    WHERE
      (week_delta = 0)
      AND (
        toMonday(toDate(first_first_trial_consumption_datetime)) = toDate(week)
      )
      AND (
        toMonday(toDate(first_first_paid_consumption_datetime)) = toDate(week)
      )
    '''
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

    query = 'RENAME TABLE cloud_analytics_testing.churn_weekly_temp TO cloud_analytics_testing.churn_weekly'
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

    query = 'DROP TABLE IF EXISTS cloud_analytics_testing.churn_weekly_temp'
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

    query = 'DROP TABLE IF EXISTS cloud_analytics_testing.churn_monthly'
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

    query = '''
    CREATE TABLE cloud_analytics_testing.churn_monthly_temp
    ENGINE = MergeTree()
    ORDER BY(week_next) PARTITION BY toYYYYMM(week_next)
     AS
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
      cloud_analytics_testing.retention_cube_monthly
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
      cloud_analytics_testing.retention_cube_monthly
    WHERE
      (week_delta = 0)
      AND (
        toStartOfMonth(toDate(first_first_trial_consumption_datetime)) = toDate(week)
      )
      AND (
        toStartOfMonth(toDate(first_first_paid_consumption_datetime)) = toDate(week)
      )
    '''
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

    query = 'RENAME TABLE cloud_analytics_testing.churn_monthly_temp TO cloud_analytics_testing.churn_monthly'
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

    query = 'DROP TABLE IF EXISTS cloud_analytics_testing.churn_monthly_temp'
    cloud_clickhouse_param_dict['query'] = query

    clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)

if __name__ == '__main__':
    main()
