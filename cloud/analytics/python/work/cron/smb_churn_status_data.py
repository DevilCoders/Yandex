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

def get_start_date_of_week(date_str):
    try:
        date_ = datetime.datetime.strptime(date_str, '%Y-%m-%d %H:%M:%S')
        return str((date_ - datetime.timedelta(days=date_.weekday() % 7)).date())

    except:
        pass

def get_start_date_of_month(date_str):
    try:
        date_ = datetime.datetime.strptime(date_str, '%Y-%m-%d %H:%M:%S')
        return str(date_.replace(day=1).date())
    except:
        pass

def get_date(date_str):
    try:
        return str(datetime.datetime.strptime(date_str, '%Y-%m-%d %H:%M:%S').date())

    except:
        pass

def get_week_delta(new_date, old_date):
    try:
        return (datetime.datetime.strptime(new_date, '%Y-%m-%d') - datetime.datetime.strptime(old_date, '%Y-%m-%d')).days/7

    except:
        pass

def get_month_delta(new_date, old_date):
    try:
        return relativedelta.relativedelta(datetime.datetime.strptime(new_date, '%Y-%m-%d'), datetime.datetime.strptime(old_date, '%Y-%m-%d')).months

    except:
        pass

def get_changes(new, old):
    if old == 0:
        old = 0.00001
    return (new - old)*100/old

def date_range_by_weeks(start_str, end_str):
    start = datetime.datetime.strptime(start_str, '%Y-%m-%d')
    end = datetime.datetime.strptime(end_str, '%Y-%m-%d')
    delta = int((end - start).days/7) + 1
    date_list = []

    for i in range(delta):
        date_list.append( str((start + datetime.timedelta(days = i*7)).date()) )

    return date_list

def date_range_by_months(start_str, end_str):
    start = datetime.datetime.strptime(start_str, '%Y-%m-%d')
    end = datetime.datetime.strptime(end_str, '%Y-%m-%d')
    delta = int(get_month_delta(end_str ,start_str)) + 1
    date_list = []

    for i in range(delta):
        date_list.append( str((start + relativedelta.relativedelta(months = i)).date()) )

    return date_list

def is_last_week(week_str, last_week):
    if get_week_delta(last_week, week_str) == 0:
        return 'Last Week'
    return 'Not Last Week'

def is_last_month(week_str, last_week):
    if get_month_delta(last_week, week_str) == 0:
        return 'Last Week'
    return 'Not Last Week'

def get_last_not_empty_table(folder_path):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    last_table_rows = 0
    last_table = ''
    for table in tables_list:
        try:
            table_ = job.driver.read(table)
        except:
            continue

        if table_.row_count > last_table_rows:
            last_table_rows =  table_.row_count
            last_table = table
    if last_table:
        return last_table
    else:
        return tables_list[0]

def get_table_list(folder_path):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    return '{%s}' % (','.join(tables_list))


def get_fact_of_consumption(result_dict_, row_, metric):
    if result_dict_[metric + '_next_period'] > 0:
        result_dict_['is_' + metric + '_next_period'] = 1
    else:
        result_dict_['is_' + metric + '_next_period'] = 0

    if result_dict_[metric] > 0:
        result_dict_['is_' + metric] = 1
    else:
        result_dict_['is_' + metric] = 0

    return result_dict_

def get_consumption_category(result_dict_, row_, metric):
    if result_dict_[metric + '_next_period'] <= 0 and row_[metric] > 0:
        result_dict_[metric + '_client_type'] = 'Churn'

    elif result_dict_[metric + '_next_period'] > row_[metric]  and row_[metric] > 0:
        result_dict_[metric + '_client_type'] = 'Positive'

    elif result_dict_[metric + '_next_period'] > row_[metric]  and row_[metric] <= 0:
        result_dict_[metric + '_client_type'] = 'New'

    elif result_dict_[metric + '_next_period'] < row_[metric] and row_[metric] > 0:
        result_dict_[metric + '_client_type'] = 'Negative'

    elif result_dict_[metric + '_next_period'] == row_[metric] and row_[metric] > 0:
        result_dict_[metric + '_client_type'] = 'Same'

    elif result_dict_[metric + '_next_period'] == row_[metric] and row_[metric] <= 0:
        result_dict_[metric + '_client_type'] = 'Non-consuming'

    return result_dict_

def apply_types_in_project(schema_):
    apply_types_dict = {}
    for col in schema_:

        if schema_[col] == str:
            apply_types_dict[col] = ne.custom(lambda x: str(x).replace('"', '').replace("'", '').replace('\\','') if x not in ['', None] else None, col)

        elif schema_[col] == int:
            apply_types_dict[col] = ne.custom(lambda x: int(x) if x not in ['', None] else None, col)

        elif schema_[col] == float:
            apply_types_dict[col] = ne.custom(lambda x: float(x) if x not in ['', None] else None, col)
    return apply_types_dict


def calc_cohort(groups):
    for key, records in groups:
        records = list(records)

        con_metrics = [
            'real_consumption',
            'real_payment',
            'trial_consumption'
        ]
        consumption_dict = {}

        for metric in con_metrics:
            consumption_dict[metric+'_cum'] = 0.0

        for rec in records:
            last_week = str((datetime.datetime.now().date() - datetime.timedelta(days=datetime.datetime.now().date().weekday() % 7)) - datetime.timedelta(days=7))
            week_range = date_range_by_weeks(rec['week'], str(datetime.datetime.now().date()))

            for metric in con_metrics:
                consumption_dict[metric+'_cum'] += rec[metric]

            for rec_new in records:

                if rec_new['week'] >= rec['week']:

                    if rec_new['week'] in week_range:
                        week_range.remove(rec_new['week'])

                    result_dict = rec.to_dict().copy()

                    result_dict['week_next'] = rec_new['week']

                    result_dict['week_delta'] = get_week_delta(rec_new['week'], rec['week'])

                    for metric in con_metrics:
                        result_dict[metric + '_next_period'] = rec_new[metric]

                    for metric in con_metrics:
                        result_dict[metric + '_delta'] = rec_new[metric] - rec[metric]

                    result_dict = get_consumption_category(result_dict, rec, 'real_consumption')
                    result_dict = get_consumption_category(result_dict, rec, 'real_payment')
                    result_dict = get_consumption_category(result_dict, rec, 'trial_consumption')

                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_consumption')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_payment')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'trial_consumption')

                    for metric in con_metrics:
                        result_dict[metric+'_cum'] = consumption_dict[metric+'_cum']

                    yield Record(key, **result_dict)


            if week_range:

                for week in week_range:

                    result_dict = rec.to_dict().copy()
                    result_dict['week_next'] = week

                    result_dict['week_delta'] = get_week_delta(week, rec['week'])

                    for metric in con_metrics:
                        result_dict[metric + '_next_period'] = 0.0

                    for metric in con_metrics:
                        result_dict[metric + '_delta'] = 0.0 - rec[metric]

                    result_dict = get_consumption_category(result_dict, rec, 'real_consumption')
                    result_dict = get_consumption_category(result_dict, rec, 'real_payment')
                    result_dict = get_consumption_category(result_dict, rec, 'trial_consumption')

                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_consumption')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_payment')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'trial_consumption')

                    for metric in con_metrics:
                        result_dict[metric+'_cum'] = consumption_dict[metric+'_cum']

                    yield Record(key, **result_dict)

def calc_cohort_monthly(groups):
    for key, records in groups:
        records = list(records)

        con_metrics = [
            'real_consumption',
            'real_payment',
            'trial_consumption'
        ]
        consumption_dict = {}

        for metric in con_metrics:
            consumption_dict[metric+'_cum'] = 0.0

        for rec in records:
            last_month = get_start_date_of_month(str(datetime.datetime.now()).split('.')[0])
            month_range = date_range_by_months(rec['week'], str(datetime.datetime.now().date()))

            for metric in con_metrics:
                consumption_dict[metric+'_cum'] += rec[metric]

            for rec_new in records:

                if rec_new['week'] >= rec['week']:

                    if rec_new['week'] in month_range:
                        month_range.remove(rec_new['week'])

                    result_dict = rec.to_dict().copy()

                    result_dict['week_next'] = rec_new['week']

                    result_dict['week_delta'] = get_month_delta(rec_new['week'], rec['week'])

                    for metric in con_metrics:
                        result_dict[metric + '_next_period'] = rec_new[metric]

                    for metric in con_metrics:
                        result_dict[metric + '_delta'] = rec_new[metric] - rec[metric]

                    result_dict = get_consumption_category(result_dict, rec, 'real_consumption')
                    result_dict = get_consumption_category(result_dict, rec, 'real_payment')
                    result_dict = get_consumption_category(result_dict, rec, 'trial_consumption')

                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_consumption')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_payment')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'trial_consumption')

                    for metric in con_metrics:
                        result_dict[metric+'_cum'] = consumption_dict[metric+'_cum']

                    yield Record(key, **result_dict)


            if month_range:

                for week in month_range:

                    result_dict = rec.to_dict().copy()
                    result_dict['week_next'] = week

                    result_dict['week_delta'] = get_month_delta(week, rec['week'])

                    for metric in con_metrics:
                        result_dict[metric + '_next_period'] = 0.0

                    for metric in con_metrics:
                        result_dict[metric + '_delta'] = 0.0 - rec[metric]

                    result_dict = get_consumption_category(result_dict, rec, 'real_consumption')
                    result_dict = get_consumption_category(result_dict, rec, 'real_payment')
                    result_dict = get_consumption_category(result_dict, rec, 'trial_consumption')

                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_consumption')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'real_payment')
                    result_dict = get_fact_of_consumption(result_dict, rec, 'trial_consumption')

                    for metric in con_metrics:
                        result_dict[metric+'_cum'] = consumption_dict[metric+'_cum']

                    yield Record(key, **result_dict)
def main():
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    paths_dict_test = {
        'acquisition_cube': '//home/cloud_analytics_test/cubes/acquisition_cube/cube',
        'retention_cube_weekly':'//home/cloud_analytics_test/cubes/retention/weekly_by_product',
         'retention_cube_monthly':'//home/cloud_analytics_test/cubes/retention/monthly_by_product'
    }
    paths_dict_prod = {
        'acquisition_cube': '//home/cloud_analytics/cubes/acquisition_cube/cube',
        'retention_cube_weekly':'//home/cloud_analytics/cubes/retention/weekly_by_product',
        'retention_cube_monthly':'//home/cloud_analytics/cubes/retention/monthly_by_product'
    }

    mode = 'test'
    if mode == 'test':
        paths_dict = paths_dict_test
    elif mode == 'prod':
        paths_dict = paths_dict_prod

    schema = {
        "ba_state": str,
        "ba_type": str,
        "balance_name": str,
        "billing_account_id": str,
        "cloud_id": str,
        "email": str,
        "first_name": str,
        "is_real_consumption": int,
        "is_real_consumption_next_period": int,
        "is_real_payment": int,
        "is_real_payment_next_period": int,
        "is_trial_consumption": int,
        "is_trial_consumption_next_period": int,
        "last_name": str,
        "phone": str,
        "product": str,
        "promocode_client_type": str,
        "promocode_source": str,
        "promocode_type": str,
        "puid": str,
        "real_consumption": float,
        "real_consumption_client_type": str,
        "real_consumption_cum": float,
        "real_consumption_delta": float,
        "real_consumption_next_period": float,
        "real_payment": float,
        "real_payment_client_type": str,
        "real_payment_cum": float,
        "real_payment_delta": float,
        "real_payment_next_period": float,
        "segment": str,
        "trial_consumption": float,
        "trial_consumption_client_type": str,
        "trial_consumption_cum": float,
        "trial_consumption_delta": float,
        "trial_consumption_next_period": float,
        "week": str,
        "week_delta": int,
        "week_next": str,
        "first_ba_created_datetime": str,
        "first_ba_became_paid_datetime": str,
        "first_cloud_created_datetime": str,
        "first_first_paid_consumption_datetime": str,
        "first_first_trial_consumption_datetime": str,
        "first_visit_datetime": str,
        "promocode_client_name": str,
        "crm_client_name": str,
        "sales": str,
        "ba_usage_status": str,
        "ba_person_type": str,
        "block_reason": str
    }

    job = cluster.job()
    consumption = job.table(paths_dict['acquisition_cube']) \
        .filter(
            nf.custom(lambda x: x == 'day_use', 'event')
        ) \
        .project(
            ne.all(),
            week = ne.custom(get_start_date_of_week, 'event_time'),
            product = ne.custom(lambda x: str(x).split('.')[0], 'name')
        ) \
        .groupby(
            'balance_name',
            'billing_account_id',
            'cloud_id',
            'email',
            'first_name',
            'last_name',
            'phone',
            'product',
            'promocode_client_type',
            'promocode_source',
            'promocode_type',
            'puid',
            'week',
            'first_ba_created_datetime',
            'first_ba_became_paid_datetime',
            'first_cloud_created_datetime',
            'first_first_paid_consumption_datetime',
            'first_first_trial_consumption_datetime',
            'first_visit_datetime',
        ) \
        .aggregate(
            real_consumption = na.sum('real_consumption', missing = 0),
            real_payment = na.sum('real_payment', missing = 0),
            trial_consumption = na.sum('trial_consumption', missing = 0),
            block_reason = na.last('block_reason', by = 'event_time'),
            ba_person_type = na.last('ba_person_type', by = 'event_time'),
            ba_usage_status = na.last('ba_usage_status', by = 'event_time'),
            sales = na.last('sales', by = 'event_time'),
            ba_state = na.last('ba_state', by = 'event_time'),
            ba_type = na.last('ba_type', by = 'event_time'),
            crm_client_name = na.last('crm_client_name', by = 'event_time'),
            promocode_client_name = na.last('promocode_client_name', by = 'event_time'),
            segment = na.last('segment', by = 'event_time')
        ) \
        .groupby(
            'puid',
            'billing_account_id',
            'product'
        ) \
        .sort(
            'week'
        ) \
        .reduce(
            calc_cohort
        ) \
        .project(**apply_types_in_project(schema)) \
        .sort(
            'puid',
            'week',
            'week_next'
        ) \
        .put(paths_dict['retention_cube_weekly'], schema = schema)
    job.run()

    schema = {
        "ba_state": str,
        "ba_type": str,
        "balance_name": str,
        "billing_account_id": str,
        "cloud_id": str,
        "email": str,
        "first_name": str,
        "is_real_consumption": int,
        "is_real_consumption_next_period": int,
        "is_real_payment": int,
        "is_real_payment_next_period": int,
        "is_trial_consumption": int,
        "is_trial_consumption_next_period": int,
        "last_name": str,
        "phone": str,
        "product": str,
        "promocode_client_type": str,
        "promocode_source": str,
        "promocode_type": str,
        "puid": str,
        "real_consumption": float,
        "real_consumption_client_type": str,
        "real_consumption_cum": float,
        "real_consumption_delta": float,
        "real_consumption_next_period": float,
        "real_payment": float,
        "real_payment_client_type": str,
        "real_payment_cum": float,
        "real_payment_delta": float,
        "real_payment_next_period": float,
        "segment": str,
        "trial_consumption": float,
        "trial_consumption_client_type": str,
        "trial_consumption_cum": float,
        "trial_consumption_delta": float,
        "trial_consumption_next_period": float,
        "week": str,
        "week_delta": int,
        "week_next": str,
        "first_ba_created_datetime": str,
        "first_ba_became_paid_datetime": str,
        "first_cloud_created_datetime": str,
        "first_first_paid_consumption_datetime": str,
        "first_first_trial_consumption_datetime": str,
        "first_visit_datetime": str,
        "promocode_client_name": str,
        "crm_client_name": str,
        "sales": str,
        "ba_usage_status": str,
        "ba_person_type": str,
        "block_reason": str
    }

    job = cluster.job()
    consumption = job.table(paths_dict['acquisition_cube']) \
        .filter(
            nf.custom(lambda x: x == 'day_use', 'event')
        ) \
        .project(
            ne.all(),
            week = ne.custom(get_start_date_of_month, 'event_time'),
            product = ne.custom(lambda x: str(x).split('.')[0], 'name')
        ) \
        .groupby(
            'balance_name',
            'billing_account_id',
            'cloud_id',
            'email',
            'first_name',
            'last_name',
            'phone',
            'product',
            'promocode_client_type',
            'promocode_source',
            'promocode_type',
            'puid',
            'week',
            'first_ba_created_datetime',
            'first_ba_became_paid_datetime',
            'first_cloud_created_datetime',
            'first_first_paid_consumption_datetime',
            'first_first_trial_consumption_datetime',
            'first_visit_datetime',
        ) \
        .aggregate(
            real_consumption = na.sum('real_consumption', missing = 0),
            real_payment = na.sum('real_payment', missing = 0),
            trial_consumption = na.sum('trial_consumption', missing = 0),
            block_reason = na.last('block_reason', by = 'event_time'),
            ba_person_type = na.last('ba_person_type', by = 'event_time'),
            ba_usage_status = na.last('ba_usage_status', by = 'event_time'),
            sales = na.last('sales', by = 'event_time'),
            ba_state = na.last('ba_state', by = 'event_time'),
            ba_type = na.last('ba_type', by = 'event_time'),
            crm_client_name = na.last('crm_client_name', by = 'event_time'),
            promocode_client_name = na.last('promocode_client_name', by = 'event_time'),
            segment = na.last('segment', by = 'event_time')
        ) \
        .groupby(
            'puid',
            'billing_account_id',
            'product'
        ) \
        .sort(
            'week'
        ) \
        .reduce(
            calc_cohort_monthly
        ) \
        .project(**apply_types_in_project(schema)) \
        .sort(
            'puid',
            'week',
            'week_next'
        ) \
        .put(paths_dict['retention_cube_monthly'], schema = schema)
    job.run()

if __name__ == '__main__':
    main()
