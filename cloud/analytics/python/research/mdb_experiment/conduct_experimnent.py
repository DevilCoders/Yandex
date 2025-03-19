import numpy as np
from tqdm import tqdm_notebook as tqdm
import scipy as sp
import scipy.stats as sps
from sklearn.decomposition import PCA
from sklearn.model_selection import train_test_split
import gc
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import datetime
import re
import math
import numpy as np
import pandas as pd
import scipy.stats as sps
import matplotlib.pyplot as plt
from numpy.linalg import inv
from numpy import linalg as LA
from scipy.linalg import eigvals as eig
import seaborn as sns
from collections import Counter
from statsmodels.sandbox.stats.multicomp import multipletests
from tqdm import tqdm_notebook
import gc
from sklearn.linear_model import LinearRegression
from sklearn.linear_model import Ridge
from sklearn.linear_model import Lasso
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error
from sklearn.datasets import load_boston
from scipy.linalg import eigvals
import warnings
from collections import defaultdict
from dateutil.parser import parse
import operator

import requests
import pandas as pd
import time
import numpy as np


id_of_experiment = input("please, enter id of experiment: ")

def execute_query(query, cluster, alias, token, timeout=600):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}&enable_optimize_predicate_expression=0".format(proxy=proxy, alias=alias, token=token)
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
            if i > 1:
                print('Break Excecution')
                break

cluster = "hahn"
alias = "*cloud_analytics"
token = ''


def works_with_emails(mail_):
    mail_parts = str(mail_).split('@')
    if len(mail_parts) > 1:
        if 'yandex.' in mail_parts[1].lower() or 'ya.' in mail_parts[1].lower():
            domain = 'yandex.ru'
            login = mail_parts[0].lower().replace('.', '-')
            return login + '@' + domain
        else:
            return mail_.lower()


def read_first_group():
    try:
        first_df = pd.read_excel("first_sample_experiment_mdb.xlsx")
    except Exception:
        raise Exception("No file 'first_sample_experiment_mdb.xlsx'. Please, add this file in this folder.")
    first_df["email"] = first_df["email"].apply(works_with_emails)
    cols = ['puid', 'last_service', 'dt_last_visit_create_cluster_old',
           'billing_account_id', 'email', 'cloud_id', 'trial_started_old',
           'paid_started_old', 'mdb_consumption_old', 'group']
    if "influenced (id of experiment)" in first_df.columns:
        cols.append("influenced (id of experiment)")

    first_df.columns = cols
    first_df["puid"] = first_df["puid"].astype(str)
    first_df["mdb_consumption_old"] = first_df["mdb_consumption_old"].astype(float)
    return first_df


def read_second_group():
    try:
        second_df = pd.read_excel("second_sample_experiment_mdb.xlsx")
    except Exception:
        raise Exception("No file 'second_sample_experiment_mdb.xlsx'. Please, add this file in this folder.")
    second_df["email"] = second_df["email"].apply(works_with_emails)
    cols = ['puid', 'last_service', 'visited_pages', 'billing_account_id', 'email',
           'cloud_id', 'trial_started_old', 'paid_started_old', 'mdb_consumption_old',
           'group']
    if "influenced (id of experiment)" in second_df.columns:
        cols.append("influenced (id of experiment)")
    second_df.columns = cols
    second_df["puid"] = second_df["puid"].astype(str)
    second_df["mdb_consumption_old"] = second_df["mdb_consumption_old"].astype(float)
    return second_df


def get_emailing_result(id_of_experiment):
    mails_req = f"""
    SELECT
        email,
        max(mailing_name),
        max(if(isNotNull(open_time), 1, 0)) as is_opened,
        max(if(isNotNull(click_time), 1, 0)) as is_cliked
    FROM "//home/cloud_analytics_test/cubes/emailing/cube"
    WHERE mailing_name like '%MDB-exp%-{id_of_experiment}'

    GROUP BY email
    """
    columns = ["email", "mailing_name", "is_opened", "is_cliked"]
    df = chyt_execute_query(query=mails_req, cluster=cluster,
                       alias=alias, token=token, columns=columns)
    if df is None:
        raise Exception(f"Wrong number {id_of_experiment!r} of experiment, found 0 writes")
    df["email"] = df["email"].apply(works_with_emails)
    return df


def get_date_of_experiment(id_of_experiment):
    data_req = f"""
    SELECT
        max(delivery_time) as delivery_time
    FROM "//home/cloud_analytics_test/cubes/emailing/cube"
    WHERE mailing_name like '%MDB-exp%-{id_of_experiment}'
    """
    columns = ["curr_date"]
    df = chyt_execute_query(query=data_req, cluster=cluster,
                       alias=alias, token=token, columns=columns)
    if df is None:
        raise Exception(f"Wrong number {id_of_experiment!r} of experiment, found 0 writes")
    return str(df["curr_date"].iloc[0])


def make_info_df(date, id_of_experiment):
    info_request = f"""
    SELECT
        puid,
        max(if(isNotNull(first_first_trial_consumption_datetime) and
                first_first_trial_consumption_datetime > {date!r}, \
                first_first_trial_consumption_datetime, NULL)) as trial_dt,
        max(if(isNotNull(first_first_paid_consumption_datetime) and
                first_first_paid_consumption_datetime > {date!r}, \
                first_first_paid_consumption_datetime, NULL)) as paid_dt,
        max(mdb_consumption_before),
        sum(if (service_name == 'mdb', trial_consumption + real_consumption, 0)) as mdb_consumption
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
    INNER JOIN (
        SELECT
            puid,
            sum(if (service_name == 'mdb', trial_consumption + real_consumption, 0)) as mdb_consumption_before
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE event_time < {date!r}
        GROUP BY puid
    )
    on puid == puid
    WHERE event_time >= {date!r}
    GROUP BY puid
    """
    columns = ["puid",
               f"trial_started_after_{id_of_experiment}_experiment",
               f"paid_started_after_{id_of_experiment}_experiment",
               f"mdb_consumption_before_{id_of_experiment}_experiment",
               f"mdb_consumption_after_{id_of_experiment}_experiment"]
    df = chyt_execute_query(query=info_request, cluster=cluster,
                       alias=alias, token=token, columns=columns)
    df["puid"] = df["puid"].astype(str)
    df[f"mdb_consumption_after_{id_of_experiment}_experiment"] = \
    df[f"mdb_consumption_after_{id_of_experiment}_experiment"].astype(float)
    df[f"mdb_consumption_before_{id_of_experiment}_experiment"] = \
    df[f"mdb_consumption_before_{id_of_experiment}_experiment"].astype(float)
    df.replace("\\N", np.nan, inplace = True)
    return df


def get_last_create_cluster_visit(date, id_of_experiment):
    request = f"""
    SELECT
        puid,
        max(toDateTime(ts)) as timestamp
    FROM "//home/cloud_analytics/import/console_logs/events"
    WHERE
        response >= '200'
    and
        response < '300'
    and
        puid != ''
    and
        event like '%create-cluster%'
    GROUP BY puid
    HAVING timestamp > toDateTime({date!r});
    """
    columns = ["puid",
               f"dt_last_visit_create_cluster_after_{id_of_experiment}_experiment"]
    df = chyt_execute_query(query=request, cluster=cluster,
                       alias=alias, token=token, columns=columns)
    df["puid"] = df["puid"].astype(str)
    df.replace("\\N", np.nan, inplace = True)
    return df


def apply_experiment_to_sample(df, mail_df, info_df, create_cluster_df, id_of_experiment):
    mail_and_df = pd.merge(df, mail_df, on = 'email', how = 'left')
    info_mail_and_df = pd.merge(mail_and_df, info_df, on = 'puid', how = 'left')
    cluster_mail_and_df = pd.merge(info_mail_and_df, create_cluster_df, on = 'puid', how = 'left')
    if "influenced (id of experiment)" in cluster_mail_and_df.columns:
        ys = cluster_mail_and_df["influenced (id of experiment)"]
        cluster_mail_and_df.drop(columns = ["influenced (id of experiment)"], inplace = True)
        cluster_mail_and_df["influenced (id of experiment)"] = ys

    trial_col = f"trial_started_after_{id_of_experiment}_experiment"
    paid_col = f"paid_started_after_{id_of_experiment}_experiment"
    mdb_after_col = f"mdb_consumption_after_{id_of_experiment}_experiment"
    mdb_before_col = f"mdb_consumption_before_{id_of_experiment}_experiment"
    cluster_col = f"dt_last_visit_create_cluster_after_{id_of_experiment}_experiment"

    cluster_mail_and_df["influenced_after_experiment"] = \
    ((~pd.isnull(cluster_mail_and_df[trial_col])) | \
        (~pd.isnull(cluster_mail_and_df[paid_col])) | \
        (cluster_mail_and_df[mdb_after_col] > 1) | \
        (~pd.isnull(cluster_mail_and_df[cluster_col]))) & (cluster_mail_and_df[mdb_before_col] < 1)

    def get_final_influence(row):
        if row["influenced_after_experiment"]:
            row["influenced_after_experiment"] = str(id_of_experiment)
        else:
            row["influenced_after_experiment"] = "no influence"
        if row["influenced (id of experiment)"] == "no influence":
            return row["influenced_after_experiment"]
        if row["influenced_after_experiment"] == "no influence":
            return row["influenced (id of experiment)"]
        return row["influenced (id of experiment)"] + ', ' + row["influenced_after_experiment"]

    if "influenced (id of experiment)" not in cluster_mail_and_df.columns:
        cluster_mail_and_df["influenced (id of experiment)"] = ["no influence"] * cluster_mail_and_df.shape[0]

    cluster_mail_and_df[f"has consumption before experiment {id_of_experiment}"] = \
    cluster_mail_and_df[mdb_before_col] > 1

    cluster_mail_and_df["influenced (id of experiment)"] = \
        cluster_mail_and_df.apply(get_final_influence, axis = 1)
    cluster_mail_and_df.drop(columns = ["influenced_after_experiment"], inplace = True)

    return cluster_mail_and_df

def get_experiment_results(id_of_experiment, \
                             need_to_remove_influence_column_in_start_excel = False, \
                             need_to_copy_file = False):
    try:
        id_of_experiment = int(id_of_experiment)
    except Exception:
        raise ValueError("Cant cast id '{id_of_experiment}' of experiment to int")

    mail_df = get_emailing_result(id_of_experiment)
    date = get_date_of_experiment(id_of_experiment)
    info_df = make_info_df(date, id_of_experiment)
    create_cluster_df = get_last_create_cluster_visit(date, id_of_experiment)
    first_df = read_first_group()
    second_df = read_second_group()

    if need_to_remove_influence_column_in_start_excel \
    and "influenced (id of experiment)" in first_df.columns:
        first_df.drop(columns = ["influenced (id of experiment)"], inplace = True)
        second_df.drop(columns = ["influenced (id of experiment)"], inplace = True)

    experiment_first_df = apply_experiment_to_sample(first_df, mail_df, info_df,
                                                     create_cluster_df, id_of_experiment)
    experiment_second_df = apply_experiment_to_sample(second_df, mail_df, info_df,
                                                      create_cluster_df, id_of_experiment)
    first_df["influenced (id of experiment)"] = experiment_first_df["influenced (id of experiment)"]
    second_df["influenced (id of experiment)"] = experiment_second_df["influenced (id of experiment)"]

    experiment_first_df = experiment_first_df.sort_values(by = ["influenced (id of experiment)"])
    experiment_second_df = experiment_second_df.sort_values(by = ["influenced (id of experiment)"])
    experiment_first_df.to_excel(f"first_sample_AFTER_experiment_{id_of_experiment}_mdb.xlsx", index = False)
    experiment_second_df.to_excel(f"second_sample_AFTER_experiment_{id_of_experiment}_mdb.xlsx", index = False)

    first_df = first_df.sort_values(by = ["influenced (id of experiment)"])
    second_df = second_df.sort_values(by = ["influenced (id of experiment)"])
    first_df.to_excel("first_sample_experiment_mdb.xlsx", index = False)
    second_df.to_excel("second_sample_experiment_mdb.xlsx", index = False)
    if need_to_copy_file:
        first_df.to_excel(f"first_sample_experiment_mdb_copy_after_{id_of_experiment}.xlsx", index = False)
        second_df.to_excel(f"second_sample_experiment_mdb_copy_after_{id_of_experiment}.xlsx", index = False)
    return experiment_first_df, experiment_second_df


experiment_first_df, experiment_second_df = \
get_experiment_results(id_of_experiment, False, True)
