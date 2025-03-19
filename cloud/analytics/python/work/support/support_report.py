import os, sys, pandas as pd, datetime, urllib2, urllib, requests, urlparse, json
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)

from global_variables import (
    metrika_clickhouse_param_dict,
    cloud_clickhouse_param_dict
)

from vault_client import instances

from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)

def load_stat_data(url, date_min, date_max, token):
    headers = {
        'Authorization': 'OAuth %s' % (token)
    }

    params = {'date_min':date_min, 'date_max':date_max, 'type':'json', '_raw_data':1}

    url_parts = list(urllib2.urlparse.urlparse(url))
    query = dict(urllib2.urlparse.parse_qsl(url_parts[4]))
    query.update(params)
    url_parts[4] = urllib.urlencode(query)
    url= urlparse.urlunparse(url_parts)

    req = requests.get(url, headers = headers)
    res = json.loads(req.text)
    df_raw = pd.DataFrame.from_dict( res['values']  )
    return df_raw

def apply_types_in_project(schema_):
    apply_types_dict = {}
    for col in schema_:

        if schema_[col] == str:
            apply_types_dict[col] = ne.custom(lambda x: str(x) if x not in ['', None] else None, col)

        elif schema_[col] == int:
            apply_types_dict[col] = ne.custom(lambda x: int(x) if x not in ['', None] else None, col)

        elif schema_[col] == float:
            apply_types_dict[col] = ne.custom(lambda x: float(x) if x not in ['', None] else None, col)
    return apply_types_dict

def get_hour_cat(x):
    h = int(x/60) + 1
    if h <= 1:
        return 'less then 1 hour'
    if h <= 2:
        return '1-2 hour'
    if h <= 4:
        return '3-4 hour'
    if h <= 5:
        return '5-8 hour'
    if h <= 24:
        return '9-24 hour'
    else:
        return '25+ hour'

def main():

    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
    stat_creds = client.get_version('ver-01d68cyvv3gpdz3apwzagry0ck')
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    start_date = '2018-10-01'
    end_date = str(datetime.date.today())

    paths_dict_test = {
        'support_tasks':'//home/cloud_analytics/import/tracker/support_tasks'
    }
    paths_dict_prod = {
        'support_tasks':'//home/cloud_analytics/import/tracker/support_tasks'
    }

    mode = 'test'
    if mode == 'test':
        paths_dict = paths_dict_test
    elif mode == 'prod':
        paths_dict = paths_dict_prod

    data = load_stat_data('https://upload.stat.yandex-team.ru/_api/statreport/json/Adhoc/yc_support/support_stat_extended?scale=s&_incl_fields=key&_incl_fields=time&_incl_fields=type_issue', start_date, end_date, stat_creds['value']['token'])

    data = data.drop('fielddate__ms', axis = 1).rename(columns = {'fielddate':'task_time', 'key':'task', 'time':'response_time','type_issue':'task_type'})

    data['response_time_group'] = data['response_time'].apply(get_hour_cat)
    data['in_time_response'] = data['response_time'].apply(lambda x: 'in_time' if x < 480 else 'out_of_time')

    cluster.write(paths_dict['support_tasks'] + '_temp', data)
    schema = {
        "response_time": int,
        "response_time_group": str,
        "task": str,
        "task_time": str,
        "task_type": str,
        "in_time_response": str
    }
    job = cluster.job()
    job.table(paths_dict['support_tasks'] + '_temp') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(paths_dict['support_tasks'], schema = schema)
    job.run()

    cluster.driver.remove(paths_dict['support_tasks'] + '_temp')

if __name__ == '__main__':
    main()
