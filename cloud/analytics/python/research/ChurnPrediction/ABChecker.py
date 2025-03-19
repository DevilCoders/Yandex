import scipy as sp
import scipy.stats as sps
import seaborn as sns
from tqdm import tqdm_notebook as tqdm
import datetime
import re
from dateutil.parser import parse
from datetime import datetime, timedelta
import numpy as np
import pandas as pd
import scipy.stats as sps
import seaborn as sns
from collections import Counter
import gc
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error
from scipy.linalg import eigvals
import warnings
from sklearn.preprocessing import StandardScaler
from sklearn.preprocessing import Normalizer
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
from sklearn.metrics import roc_auc_score
from sklearn.metrics import roc_curve
from datetime import timedelta
import os
os.environ['KMP_DUPLICATE_LIB_OK']='True'
import xgboost as xgb
from xgboost import XGBClassifier
from xgboost import XGBRegressor
from sklearn.model_selection import GridSearchCV
from sklearn.model_selection import RandomizedSearchCV
import calendar
import sys
import os.path
import datetime
import time
warnings.filterwarnings("ignore")

string_curr_date = sys.argv[1]
curr_date = parse(string_curr_date)
start_date = parse("2019-08-19")
observe_previous_weeks = 1
window_in_days_to_observe_earlier = 30
proc_of_using_to_think_that_feature_is_active = 1/2
max_non_act_days_to_believe_that_active = 5
num_of_days_to_observe_in_future = 30
min_consumption_by_day =1
treshold_activity = [(0, 0), (67, 1)]
treshold_paid = [(0, 0), (33, 1), (67, 2), (333, 3), (1000, 4)]
max_active_days_in_future_to_say_that_not_active = 3
person_df_file = "person_df.csv"
import requests
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
            if i > 5:
                print('Break Excecution')
                break

cluster = "hahn"
alias = "*cloud_analytics"
token = ''

def arr_cocnat(arr):
    arr = [x for x in arr if x is not None]
    if len(arr) == 0:
        return None
    ans = np.concatenate(arr)
    return ans

call_req = """
    select DISTINCT billing_account_id
    FROM "//home/cloud_analytics/churn_prediction_test/test-{}_churn_answers"
    """
full_req = """
    select DISTINCT billing_account_id
    FROM "//home/cloud_analytics/churn_prediction/full-{}_churn_answers"
    """

def get_call_bad_bids(string_date):
    col = ["billing_account_id"]
    curr_call_req = call_req.format(string_date)
    df_bids = chyt_execute_query(query=curr_call_req,
                         cluster=cluster, alias=alias,
                          token=token, columns=["billing_account_id"])
    return np.array(df_bids["billing_account_id"])

def get_full_bad_bids(string_date):
    col = ["billing_account_id"]
    curr_full_req = full_req.format(string_date)
    df_bids = chyt_execute_query(query=curr_full_req,
                         cluster=cluster, alias=alias,
                           token=token, columns=["billing_account_id"])
    return np.array(df_bids["billing_account_id"])

y_name = "is_not_active_next_{}_days(except_{}_days)".format(
                                num_of_days_to_observe_in_future,
                                max_active_days_in_future_to_say_that_not_active)

def wald(n, m, p1, p2):
    sigma = np.sqrt(p1*(1 - p1) / n + p2 * (1 - p2) / m)
    return (p1 - p2) / sigma

def wald_test(X, Y):
    ans = sps.norm.cdf(wald(X.shape[0], Y.shape[0], np.mean(X), np.mean(Y)))
    return ans

def two_sided_wald_test(X, Y):
    if wald_test(X, Y) < 0.05:
        return (True, wald_test(X, Y))
    if wald_test(Y, X) < 0.05:
        return (True, wald_test(Y, X))
    return False, min(wald_test(Y, X), wald_test(X, Y))

def get_res_table(bid_arr, curr_person_df):
    results = pd.DataFrame(columns = ["bid"])
    results["bid"] = bid_arr
    results = pd.merge(results["bid"].to_frame(),
                        curr_person_df[["billing_account_id", y_name]],
                        left_on = "bid",
                        right_on = "billing_account_id",
                        how = "inner")
    results["y"] = results[y_name]
    results.drop(columns = ["billing_account_id", y_name], inplace = True)
    return results

def daydaterange(end_date, start_date):
    for n in range(int ((end_date - start_date).days) // 7):
        yield start_date + timedelta(n * 7)

def make_results_for_y(bid_arr, curr_person_df):
    results = pd.DataFrame(columns = ["bid"])
    results["bid"] = bid_arr
    results = pd.merge(results["bid"].to_frame(),
                        curr_person_df[["billing_account_id", y_name]],
                        left_on = "bid",
                        right_on = "billing_account_id",
                        how = "inner")
    results["y"] = results[y_name]
    results.drop(columns = ["billing_account_id", y_name], inplace = True)
    if -1 in results["y"].unique():
        return None
    else:
        return np.array(results["y"])

def make_results_for_metrics(bid_arr, person_df):
    is_used = []
    for bid in bid_arr:
        if person_df[person_df["billing_account_id"] == bid].shape[0] > 0:
            is_used.append(1)
        else:
            is_used.append(0)
    return is_used

def print_res(col, final_call_y, final_control_y, row):
    if final_call_y is None:
        row.append(-1)
        row.append(-1)
        row.append(False)
        row.append(-1)
        return
    cols = ["date", "probability for " + col + " for call group",
            "probability for " + col + " for call group"]
    print("probability for " + col + " for call group", np.mean(final_call_y))
    print("probability for " + col + " for control group", np.mean(final_control_y))
    res, pvalue = two_sided_wald_test(final_call_y, final_control_y)
    print("has influence: {}; pvalue: {}".format(res, pvalue))
    row.append(np.mean(final_call_y))
    row.append(np.mean(final_control_y))
    row.append(res)
    row.append(pvalue)
    print("=========")


def abchecker():
    res = pd.read_csv("res_ab_test.csv")
    final_call_y = None
    final_control_y = None
    final_call_14 = None
    final_control_14 = None
    final_call_28 = None
    final_control_28 = None
    person_df = pd.read_csv(person_df_file)

    person_df = person_df[person_df["curr_date_to_predict"] <= string_curr_date]
    prev_call_bids = None
    for d in daydaterange(curr_date, start_date):
        string_date = d.strftime('%Y-%m-%d')
        curr_person_df = person_df[person_df["curr_date_to_predict"] == string_date]
        next_person_df = person_df[person_df["curr_date_to_predict"] > string_date]
        d_2_weeks_next = d + timedelta(16)
        d_30_days_next = d + timedelta(30)
        string_date_2_weeks_next = d_2_weeks_next.strftime('%Y-%m-%d')
        string_date_30_days_next = d_30_days_next.strftime('%Y-%m-%d')
        week_2_next_person_df = person_df[(person_df["curr_date_to_predict"] > string_date) & \
                                          (person_df["curr_date_to_predict"] < string_date_2_weeks_next)]
        next_28_days_next_person_df = person_df[(person_df["curr_date_to_predict"] > string_date) &
                                          (person_df["curr_date_to_predict"] < string_date_30_days_next)]


        call_bids = get_call_bad_bids(string_date)
        prev_call_bids = arr_cocnat([prev_call_bids, call_bids])
        full_bids = get_full_bad_bids(string_date)
        control_bids = full_bids[~np.in1d(full_bids, prev_call_bids)]

        #print(call_bids.shape[0], call_bids.shape[0] + control_bids.shape[0],  full_bids.shape[0])

        call_res_y = make_results_for_y(call_bids, curr_person_df)
        control_res_y = make_results_for_y(control_bids, curr_person_df)
        final_call_y = arr_cocnat([final_call_y, call_res_y])
        final_control_y = arr_cocnat([final_control_y, control_res_y])

        call_14 = make_results_for_metrics(call_bids, week_2_next_person_df)
        control_14 = make_results_for_metrics(control_bids, week_2_next_person_df)
        final_call_14 = arr_cocnat([final_call_14, call_14])
        final_control_14 = arr_cocnat([final_control_14, control_14])

        call_28 = make_results_for_metrics(call_bids, next_28_days_next_person_df)
        control_28 = make_results_for_metrics(control_bids, next_28_days_next_person_df)
        final_call_28 = arr_cocnat([final_call_28, call_28])
        final_control_28 = arr_cocnat([final_control_28, control_28])
    row = [curr_date.strftime('%Y-%m-%d'), len(final_call_14), len(final_control_14)]
    print_res("y", final_call_y, final_control_y, row)
    print_res("is_used_next_2_weeks", final_call_14,  final_control_14, row)
    print_res("is_used_next_28_days",  final_call_28,  final_control_28, row)
    res.loc[res.shape[0] + 1] = row
    res.to_csv("res_ab_test.csv", index = False)

abchecker()

