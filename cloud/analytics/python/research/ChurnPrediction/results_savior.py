from sklearn.model_selection import train_test_split
import numpy as np
import requests
import pandas as pd
import sys
import re
from dateutil.parser import parse
from datetime import datetime, timedelta

date = sys.argv[1]

from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
cluster_yt = clusters.yt.Hahn(
    token = '',
    pool = "cloud_analytics_pool",
)

def execute_query(query, cluster, alias, token, timeout=600):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}&enable_optimize_predicate_expression=0".format(\
                                                        proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.text.strip().split('\n')
    return rows

def chyt_execute_query(query, cluster, alias, token, columns):
    i = 0
    while True:
        try:
            result = execute_query(query=query, cluster=cluster, alias=alias, token=token)
            users = pd.DataFrame([row.split('\t') for row in result], columns = columns)
            return users
        except Exception as err:
            print(err)
            i += 1
            if i > 10:
                print('Break Excecution')
                break

cluster = "hahn"
alias = "*cloud_analytics"
token = ''
pool = "cloud_analytics"
cores_q = """
    select DISTINCT billing_account_id
    FROM "//home/cloud_analytics/churn_prediction_test/test-{}_churn_answers"
    """

def daydaterange(end_date, start_date):
    for n in range(int ((end_date - start_date).days) // 7):
        yield start_date + timedelta(n * 7)

def get_bad_bids():
    col = ["billing_account_id"]
    arr = []
    start_date = parse("2019-08-19")
    end_date = parse(date)
    for d in daydaterange(end_date, start_date):
        print(d)
        curr_cores_q = cores_q.format(d.strftime('%Y-%m-%d'))
        curr = chyt_execute_query(query=curr_cores_q,
                             cluster=cluster, alias=alias,
                               token=token, columns=["billing_account_id"])
        arr.append(curr)
    df_bids = pd.concat(arr)
    return np.array(df_bids["billing_account_id"])

def get_good_bids():
    bad_bids = get_bad_bids()
    file_to_read = "{}_churn_answers.csv".format(date)
    df = pd.read_csv(file_to_read)
    return df[(df["puid"] % 2 == 0) & (~df["billing_account_id"].isin(bad_bids))]["billing_account_id"].unique()

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

schema = {
    'billing_account_id': str,
    'mail': str,
    'phone': str,
    'mail': str,
    'phone': str,
    'first_name': str,
    'last_name': str,
    'client_name': str,
    'timezone': str,
    'weight':int,
    'description': str
}

def into_hanh_push(file_to_write, path, table, schema):
    path = path + file_to_write
    raw_path = path + "_"
    cluster_yt = clusters.yt.Hahn(
        token = token,
        pool = pool
    )
    cluster_yt.write(raw_path, table)

    job = cluster_yt.job()
    job.table(raw_path) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(path, schema = schema)
    job.run()
    cluster_yt.driver.remove(raw_path)

def server_savior(date):
    file_to_call_read = "{}call_churn_answers.csv".format(date)
    answers = pd.read_csv(file_to_call_read)
    answers = answers[answers["weight"] >= 50]
    call_bids = get_good_bids()
    call_answers = answers[answers["billing_account_id"].isin(call_bids)]
    full_answers = answers.copy()
    full_file_to_write = "full-{}_churn_answers".format(date)
    full_path = '//home/cloud_analytics/churn_prediction/'
    into_hanh_push(full_file_to_write, full_path, full_answers, schema)
    call_file_to_write = "test-{}_churn_answers".format(date)
    call_path = '//home/cloud_analytics/churn_prediction_test/'
    into_hanh_push(call_file_to_write, call_path, call_answers, schema)

server_savior(date)
