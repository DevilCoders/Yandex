import scipy as sp
import scipy.stats as sps
import seaborn as sns
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
warnings.filterwarnings("ignore")

observe_previous_weeks = 1
window_in_days_to_observe_earlier = 30
proc_of_using_to_think_that_feature_is_active = 1/2
max_non_act_days_to_believe_that_active = 5
num_of_days_to_observe_in_future = 30
min_consumption_by_day =1
treshold_activity = [(0, 0), (67, 1)]
treshold_paid = [(0, 0), (33, 1), (67, 2), (333, 3), (1000, 4)]
max_active_days_in_future_to_say_that_not_active = 3
passport_current_path = "//home/cloud_analytics/import/iam/cloud_owners/1h/2019-08-25T20:00:45"
import requests

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
            if i > 5:
                print('Break Excecution')
                break

cluster = "hahn"
alias = "*cloud_analytics"
token = ''


cores_q = """
SELECT
    event_time,
    timezones,
    multiIf(
            account_name IS NOT NULL AND account_name != 'unknown',account_name,
            crm_client_name IS NOT NULL AND crm_client_name != 'unknown',crm_client_name,
            balance_name IS NOT NULL AND balance_name != 'unknown',balance_name,
            promocode_client_name IS NOT NULL AND promocode_client_name != 'unknown',promocode_client_name,
            CONCAT(first_name, ' ', last_name)
        ) as client_name,
    puid,
    billing_account_id,
    email,
    phone,
    first_name,
    last_name,
    age,
    sex,
    city,
    country,
    payment_type,
    ba_payment_cycle_type,
    ba_person_type,
    ba_state,
    segment,
    ba_usage_status,

    device_type,
    os,

    event,
    trial_consumption,
    first_first_paid_consumption_datetime,
    first_first_trial_consumption_datetime,
    real_consumption,

    name,
    service_long_name,
    platform,
    core_fraction,
    preemptible,
    database,
    subservice_name
FROM "//home/cloud_analytics_test/cubes/acquisition_cube/cube" as a
INNER JOIN
(
    SELECT max(timezone) as timezones, passport_uid FROM "{}"
    GROUP BY passport_uid
) as b
ON a.puid == b.passport_uid
WHERE
    puid NOT IN (SELECT puid FROM "//home/cloud_analytics_test/cubes/acquisition_cube/cube"
            WHERE  segment like 'isv_large' OR
                   segment like 'enterprise' OR
                   ba_usage_status LIKE 'service') AND
    puid in (SELECT puid FROM "//home/cloud_analytics_test/cubes/acquisition_cube/cube"
            WHERE first_first_paid_consumption_datetime IS NOT NULL AND
                name NOT LIKE '%public_fips%' AND
                name NOT LIKE '%public_ip%' AND
                name NOT LIKE '%image%' AND
                name NOT LIKE '%snapshot%' AND
                name NOT LIKE '%network-hdd%' AND
                name NOT LIKE '%network-nvme%' AND
                event like 'day_use' AND
                event_time >= first_first_paid_consumption_datetime)
    AND
	(isNull(name) or (
        name NOT LIKE '%public_fips%' AND
        name NOT LIKE '%public_ip%' AND
        name NOT LIKE '%image%' AND
        name NOT LIKE '%snapshot%' AND
        name NOT LIKE '%network-hdd%' AND
        name NOT LIKE '%network-nvme%')) AND
        first_first_paid_consumption_datetime  IS NOT NULL
ORDER BY puid, event_time;
""".format(passport_current_path)

df_columns = [
            'event_time', 'timezone', 'client_name', 'puid',
            'billing_account_id', 'mail', 'phone', 'first_name',
            'last_name', 'age', 'sex', 'city', 'country', 'payment_type',
            'ba_payment_cycle_type', 'ba_person_type', 'ba_state', 'segment',
            'ba_usage_status', 'device_type', 'os', 'event', 'trial_consumption',
            'first_first_paid_consumption_datetime',
            'first_first_trial_consumption_datetime', 'real_consumption',
            'name', 'service_long_name', 'platform', 'core_fraction', 'preemptible',
            'database', 'subservice_name']

def create_df(df_columns):
    df = chyt_execute_query(query=cores_q, cluster=cluster, alias=alias, token=token, columns=df_columns)
    df = df.replace('\\N', np.NaN)
    df["event_time"] =  pd.to_datetime(df["event_time"])
    df["first_first_paid_consumption_datetime"] =  pd.to_datetime(df["first_first_paid_consumption_datetime"])
    df["first_first_trial_consumption_datetime"] =  pd.to_datetime(df["first_first_trial_consumption_datetime"])
    df = df.sort_values(by=['event_time'])
    df["real_consumption"] =pd.to_numeric(df["real_consumption"])
    df["trial_consumption"] =pd.to_numeric(df["trial_consumption"])
    df["puid"] = pd.to_numeric(df["puid"])
    df["all_consumption"] = df["real_consumption"] + df["trial_consumption"]
    df["year"] = df["event_time"].dt.year
    df["month"] = df["event_time"].dt.month
    df["core_fraction"] = df["core_fraction"].astype(float)
    core = pd.get_dummies(df["core_fraction"])

    core.columns = ["core_" + str(i) for i in core.columns]

    database = pd.get_dummies(df["database"])
    database.columns = ["database_" + str(i) for i in database.columns]


    platform = pd.get_dummies(df["platform"])
    platform.columns = ["platform_" + str(i) for i in platform.columns]

    preemptible = pd.get_dummies(df["preemptible"])
    preemptible.columns = ["preemtible_is"]

    service_long_name = pd.get_dummies(df["service_long_name"])

    subservice = pd.get_dummies(df["subservice_name"])
    subservice.columns = ["subservice_" + str(i) for i in subservice.columns]
    df = pd.concat([df, core, database, platform, preemptible, service_long_name, subservice], axis = 1)

    services = service_long_name.columns
    subservices = np.concatenate([core.columns, platform.columns,
                             database.columns,
                             preemptible.columns,
                             subservice.columns])

    return df, services, subservices


def make_columns(observe_previous_weeks,
                 window_in_days_to_observe_earlier,
                 proc_of_using_to_think_that_feature_is_active,
                 max_non_act_days_to_believe_that_active,
                 num_of_days_to_observe_in_future,
                 min_consumption_by_day,
                 services,
                 subservices):
    all_features = []
    for service in services:
        all_features.append(service)
    for subservice in subservices:
        all_features.append(subservice)
    common_columns = ["puid",
                      "billing_account_id",
                      'mail',
                      'phone',
                      'timezone',
                      'client_name',
                      'first_name',
                      'last_name',
                      "region", "age", "sex", "payment_type", "segment", "ba_person_type",
                      "ba_usage_status", "avg_blocked_by_billing_per_active_month",
                      "os", "device_type",
                      "num_of_visits_per_{}_week(s)".format(observe_previous_weeks),
                      "num_of_clicks_per_{}_week(s)".format(observe_previous_weeks),
                      "num_of_calls_per_{}_week(s)".format(observe_previous_weeks)]
    common_pattern_subservice = ["is_used_{}(>={}_of_{})"]

    for feature in all_features:
        for pattern in common_pattern_subservice:
            common_columns.append(pattern.format(feature,
                                                 proc_of_using_to_think_that_feature_is_active,
                                                 window_in_days_to_observe_earlier))
    ################################################################################################
    activity_columns = ["days_in_cloud", "active_days_in_cloud",
                        "proc_of_active_day_in_cloud",
                        "num_of_{}_days_non_act_tog".format(num_of_days_to_observe_in_future),
                "prob_to_be_not_active_next_{}_days(depend_on_cogorta)".format(num_of_days_to_observe_in_future),
                        "num_of_non_act_days_before_tog",
                        "num_of_act_periods(non_act<={}_day(s))".format(max_non_act_days_to_believe_that_active),
                    "avg_num_of_act_days_tog(non_act<={}_day(s))".format(max_non_act_days_to_believe_that_active),
                        "avg_num_of_non_act_days_tog",
                        "max_num_of_non_act_days_tog", "num_of_non_act_periods", "avg_act_days_in_act_month",
                        "act_days_in_trial"]

    #activity_pattern_service = ["avg_num_of_non_act_days_tog_in_{}"]

    #for feature in all_features:
    #    for pattern in activity_pattern_service:
    #        activity_columns.append(pattern.format(feature))
    ################################################################################################
    consumption_columns = ["avg_consump_per_act_day", "cogorta_paid", "variance_per_day",
                           "sum_trial_cons", "sum_real_cons", "avg_trial_cons_per_act_day",
                           "cogorta_trial", "previous_{}_week(s)_cons".format(observe_previous_weeks)]
    consumption_pattern_service = ["proc_of_{}_cons_to_cons_last_{}_days"]

    for feature in all_features:
        for pattern in consumption_pattern_service:
            consumption_columns.append(pattern.format(feature, window_in_days_to_observe_earlier))

    res_columns = []
    res_columns.append("curr_date_to_predict")
    for col in common_columns:
        res_columns.append(col)
    for col in activity_columns:
        res_columns.append(col)
    for col in consumption_columns:
        res_columns.append(col)
    return res_columns, \
            common_columns, \
            activity_columns, \
            consumption_columns,\
            common_pattern_subservice,\
            consumption_pattern_service,\
            all_features

def get_common_info_from_df(df, col):
    val = np.array(df[col])
    val = val[~pd.isnull(val)]
    val = val[~(val == "undefined")]
    if val.shape[0] == 0:
        return "неизвестно"
    return val[-1]
def get_mail(df):
    return get_common_info_from_df(df, "mail")
def get_first_name(df):
    return get_common_info_from_df(df, "first_name")
def get_last_name(df):
    return get_common_info_from_df(df, "last_name")
def get_phone(df):
    return get_common_info_from_df(df, "phone")
def get_billing_account_id(df):
    return df["billing_account_id"].iloc[-1]
def get_name(df):
    return get_common_info_from_df(df, "client_name")
def get_timezone(df):
    return get_common_info_from_df(df, "timezone")

def get_puid(df):
    return df["puid"].iloc[-1]

def get_region(df):
    val = np.array(df["city"])
    val = val[~pd.isnull(val)]
    if val.shape[0] != 0 and (val[-1] == "Москва" or val[-1] == "Санкт-Петербург"):
        return val[-1]
    val = np.array(df["country"].unique())
    val = val[~pd.isnull(val)]
    if val.shape[0] == 0:
        return "неизвестно"
    if val[-1] == "Россия":
        return val[-1]
    return "иностранец"


def get_age(df):
    return get_common_info_from_df(df, "age")

def get_sex(df):
    return get_common_info_from_df(df, "sex")

def get_payment_type(df):
    return get_common_info_from_df(df, "payment_type")

def get_segment(df):
    return get_common_info_from_df(df, "segment")

def get_ba_person_type(df):
    return get_common_info_from_df(df, "ba_person_type")

def get_ba_usage_status(df):
    return get_common_info_from_df(df, "ba_usage_status")

def get_device_type(df):
    return get_common_info_from_df(df, "device_type")

def get_os(df):
    os = get_common_info_from_df(df, "os")
    os = os.split(" ")[0]
    return os


def get_grouped_consumption(df, consumption_column, freq, need_to_delete_starting_zeros = True):
    consumption_by_freq = df.groupby([pd.Grouper(key='event_time', freq = "d")]).agg({
        consumption_column:"sum"
    }).reset_index().sort_values('event_time')
    arr = np.array(consumption_by_freq[consumption_column])
    if need_to_delete_starting_zeros:
        try:
            ind = consumption_by_freq[consumption_by_freq[consumption_column] > 0.1].index.values[0]
            consumption_by_freq = \
            consumption_by_freq.iloc[ind:]
        except Exception:
            return consumption_by_freq.iloc[0:0]
    consumption_by_freq["year"] = consumption_by_freq["event_time"].dt.year
    consumption_by_freq["month"] = consumption_by_freq["event_time"].dt.month
    return consumption_by_freq

def get_grouped_active_consumption(df, consumption_column, freq, min_consumption):
    consumption_by_freq = get_grouped_consumption(df, consumption_column, freq, need_to_delete_starting_zeros = False)
    consumption_by_freq = consumption_by_freq[consumption_by_freq[consumption_column] >= min_consumption]
    return consumption_by_freq

def calculate_active_days(active_consumption_by_day):
    return active_consumption_by_day.shape[0]

def counter(df, month_cons_by_day):
    return month_cons_by_day.shape[0]

def cons_real_sum(df, month_cons_by_day):
    return month_cons_by_day["real_consumption"].sum()
def cons_trial_sum(df, month_cons_by_day):
    return month_cons_by_day["real_consumption"].sum()
def cons_all_sum(df, month_cons_by_day):
    return month_cons_by_day["all_consumption"].sum()

def pattern_finder(df, month_cons_by_day, pattern, column):
    df_curr = df[(df["year"] == month_cons_by_day["year"].iloc[0]) &
                 (df["month"] == month_cons_by_day["month"].iloc[0])]
    if np.sum(df_curr[column].isin(pattern)) > 0:
        return 1
    return 0

def ba_state_finder_suspended(df, month_cons_by_day):
    return pattern_finder(df, month_cons_by_day, ["suspended", "inactive", 'deleted'], "ba_state")

def calculate_info_in_active_month(df, active_consumption_by_day, func):
    years = active_consumption_by_day["year"].unique()
    info_in_active_month = []
    for year in years:
        year_consumption = active_consumption_by_day[active_consumption_by_day["year"] == year]
        months = year_consumption["month"].unique()
        for month in months:
            month_consumption = year_consumption[year_consumption["month"] == month]
            info_in_active_month.append(func(df, month_consumption))
    return np.array(info_in_active_month)

def get_avg_blocked_by_billing_per_active_month(df, active_consumption_by_day):
    return np.mean(calculate_info_in_active_month(df,
                                                  active_consumption_by_day,
                                                  ba_state_finder_suspended))

def calc_sum_of_pattern_in_period(df, finish_dt, start_dt, col, pattern):
    df_curr = df[(df["event_time"] >= start_dt) & (df["event_time"] <= finish_dt)]
    return np.sum(df_curr[col].isin(pattern))

def get_num_of_visits_per_period(df, finish_dt, start_dt):
    return calc_sum_of_pattern_in_period(df, finish_dt, start_dt, "event", ["visit"])
def get_num_of_clicks_per_period(df, finish_dt, start_dt):
    return calc_sum_of_pattern_in_period(df, finish_dt, start_dt, "event", ["click"])
def get_num_of_calls_per_period(df, finish_dt, start_dt):
    return calc_sum_of_pattern_in_period(df, finish_dt, start_dt, "event", ["call"])
def get_num_of_blocked_by_billing_per_period(df, finish_dt, start_dt):
    return calc_sum_of_pattern_in_period(df, finish_dt, start_dt, "ba_state", ["suspended", "inactive", 'deleted'])
def get_df_in_period(df, finish_dt, start_dt):
    df_curr = df[(df["event_time"] >= start_dt) & (df["event_time"] < finish_dt)]
    return df_curr

def get_is_used_feature(df, curr_date_to_predict,
                                feature,
                                proc_of_using_to_think_that_feature_is_active,
                                window_in_days_to_observe_earlier_dt,
                                all_active_consumption_by_day, min_consumption_by_day):
    df_curr = get_df_in_period(df, curr_date_to_predict, window_in_days_to_observe_earlier_dt)
    all_days = get_df_in_period(all_active_consumption_by_day,
                                curr_date_to_predict, window_in_days_to_observe_earlier_dt).shape[0]
    feature_df = df_curr[df_curr[feature] == 1]
    feature_cons = get_grouped_active_consumption(feature_df, "all_consumption", "d",
                                                                min_consumption_by_day)
    feature_days = feature_cons.shape[0]
    if all_days == 0:
        return 0
    if feature_days / all_days >= proc_of_using_to_think_that_feature_is_active:
        return 1
    return 0

def get_common_feature_info(df,
                            curr_date_to_predict,
                            feature,
                            proc_of_using_to_think_that_feature_is_active,
                            window_in_days_to_observe_earlier_dt,
                            min_consumption_by_day,
                            all_active_consumption_by_day,
                            pattern_num):
    assert(pattern_num < 2)
    if pattern_num == 1:
        return get_is_used_feature(df, curr_date_to_predict,
                                         feature,
                                         proc_of_using_to_think_that_feature_is_active,
                                         window_in_days_to_observe_earlier_dt,
                                         all_active_consumption_by_day,
                                         min_consumption_by_day,)

def common_info_calc(
                    df,
                    row,
                    curr_date_to_predict,
                    observe_previous_weeks,
                    observe_previous_weeks_dt,
                    window_in_days_to_observe_earlier,
                    window_in_days_to_observe_earlier_dt,
                    proc_of_using_to_think_that_feature_is_active,
                    max_non_act_days_to_believe_that_active,
                    num_of_days_to_observe_in_future,
                    min_consumption_by_day,
                    treshold_activity,
                    treshold_paid,
                    common_columns,
                    common_pattern_subservice,
                    real_active_consumption_by_day,
                    real_consumption_by_day,
                    all_active_consumption_by_day,
                    all_features):

    if df.shape[0] == 0:
        return None
    ans = {}
    ans["timezone"] = get_timezone(df)
    ans["client_name"] = get_name(df)
    ans["puid"] = get_puid(df)
    ans["billing_account_id"] = get_billing_account_id(df)
    ans["mail"] = get_mail(df)
    ans["phone"] = get_phone(df)
    ans["first_name"] = get_first_name(df)
    ans["last_name"] = get_last_name(df)
    ans["region"] = get_region(df)
    ans["age"] = get_age(df)
    ans["sex"] = get_sex(df)
    ans["payment_type"] = get_payment_type(df)
    ans["segment"] = get_segment(df)
    ans["ba_person_type"] = get_ba_person_type(df)
    ans["ba_usage_status"] = get_ba_usage_status(df)

    ans["avg_blocked_by_billing_per_active_month"] = \
                get_avg_blocked_by_billing_per_active_month(df, real_active_consumption_by_day)
    ans["os"] = get_os(df)
    ans["device_type"] = get_device_type(df)
    ans["num_of_visits_per_{}_week(s)".format(observe_previous_weeks)] = get_num_of_visits_per_period(df,
                                                                          curr_date_to_predict,
                                                                          observe_previous_weeks_dt)
    ans["num_of_clicks_per_{}_week(s)".format(observe_previous_weeks)] = get_num_of_clicks_per_period(df,
                                                                          curr_date_to_predict,
                                                                          observe_previous_weeks_dt)
    ans["num_of_calls_per_{}_week(s)".format(observe_previous_weeks)] = get_num_of_calls_per_period(df,
                                                                    curr_date_to_predict,
                                                                    observe_previous_weeks_dt)

    ans["num_of_blocked_by_billing_per_{}_week(s)".format(observe_previous_weeks)] = \
    get_num_of_blocked_by_billing_per_period(df, curr_date_to_predict, observe_previous_weeks_dt)

    for feature in all_features:
        for ind, pattern in enumerate(common_pattern_subservice):
            ans[pattern.format(feature,
                               proc_of_using_to_think_that_feature_is_active,
                               window_in_days_to_observe_earlier)] = get_common_feature_info(
                                                                        df,
                                                                        curr_date_to_predict,
                                                                        feature,
                                                                        proc_of_using_to_think_that_feature_is_active,
                                                                        window_in_days_to_observe_earlier_dt,
                                                                        min_consumption_by_day,
                                                                        all_active_consumption_by_day,
                                                                        ind + 1)
    for column in common_columns:
        row.append(ans[column])
        ##print("column: {} --> {}".format(column, ans[column]))

def calculate_info_with_non_act_tog(real_consumption_by_day, min_consumption_by_day, func):
    cnt = 0
    ans = []
    for _, row in real_consumption_by_day.iterrows():
        if row['real_consumption'] <= min_consumption_by_day: # Не юзал сервис в этот день
            cnt += 1
        if row['real_consumption'] > min_consumption_by_day: # юзал сервис в этот день
            if cnt > 0:
                #print(row)
                ans.append(cnt)
            cnt = 0
    #print(ans)
    if np.array(ans).shape[0] == 0:
        return 0
    return func(np.array(ans))

def calculate_info_with_act_tog(real_consumption_by_day,
                                max_non_act_days_to_believe_that_active,
                                min_consumption_by_day,
                                func):
    cnt = 0
    non_act = 0
    ans = []
    for _, row in real_consumption_by_day.iterrows():
        if row['real_consumption'] >= min_consumption_by_day:
            cnt += 1
            non_act = 0
        else: # юзал сервис в этот день
            non_act += 1
            if non_act > max_non_act_days_to_believe_that_active and cnt > 0:
                ans.append(cnt)
                cnt = 0
    if cnt > 0:
        ans.append(cnt)
        cnt = 0
    if np.array(ans).shape[0] == 0:
        return 0
    return func(np.array(ans))

def get_last_date(df):
    return parse(df["event_time"].iloc[-1].strftime('%Y-%m-%d'))

def get_days_in_cloud(real_consumption_by_day):
    return real_consumption_by_day.shape[0]

def get_active_days_in_cloud(real_active_consumption_by_day):
    return real_active_consumption_by_day.shape[0]

def get_proc_of_active_day_in_cloud(real_active_consumption_by_day, real_consumption_by_day):
    return get_active_days_in_cloud(real_active_consumption_by_day) / get_days_in_cloud(real_consumption_by_day)

def get_num_of_n_days_non_act_tog(real_consumption_by_day,
                                  num_of_days_to_observe_in_future,
                                  min_consumption_by_day):
    func = lambda arr: np.sum(arr > num_of_days_to_observe_in_future)
    return calculate_info_with_non_act_tog(real_consumption_by_day, min_consumption_by_day, func)

def get_num_of_non_act_days_before_tog(df, curr_date_to_predict):
    last_active_datetime = get_last_date(df)
    delta = (curr_date_to_predict - last_active_datetime).days - 1
    return delta

def calculate_avg_real_cons_in_active_day(real_active_consumption_by_day):
    return np.mean(real_active_consumption_by_day["real_consumption"])

def find_cogorta(elem, treshold):
    treshold = sorted(treshold, key = lambda x: x[1], reverse = True)
    for (x, ans) in treshold:
        if elem > x:
            return ans

def get_cogorta(real_active_consumption_by_day, treshold):
    summ = calculate_avg_real_cons_in_active_day(real_active_consumption_by_day)
    return find_cogorta(summ, treshold)

def get_prob_to_be_not_active_next_n_days(df,
                                          curr_date_to_predict,
                                          real_active_consumption_by_day,
                                          min_consumption_by_day,
                                          treshold):
    cogorta = get_cogorta(real_active_consumption_by_day, treshold)
    days = get_num_of_non_act_days_before_tog(df, curr_date_to_predict)
    #print(cogorta, days)
    return days * len(treshold) +  cogorta

def get_avg_num_of_act_days_tog(real_consumption_by_day,
                                max_non_act_days_to_believe_that_active,
                                min_consumption_by_day):
    func = lambda arr: np.sum(arr)
    return calculate_info_with_act_tog(real_consumption_by_day,
                                max_non_act_days_to_believe_that_active,
                                min_consumption_by_day,
                                func)

def get_avg_num_of_non_act_days_tog(real_consumption_by_day,
                                  min_consumption_by_day):
    func = lambda arr: np.mean(arr)
    return calculate_info_with_non_act_tog(real_consumption_by_day, min_consumption_by_day, func)

def get_max_num_of_non_act_days(real_consumption_by_day, min_consumption_by_day):
    func = lambda arr: np.max(arr)
    return calculate_info_with_non_act_tog(real_consumption_by_day, min_consumption_by_day, func)

def get_num_of_non_act_periods(real_consumption_by_day, min_consumption_by_day):
    func = lambda arr: arr.shape[0]
    return calculate_info_with_non_act_tog(real_consumption_by_day, min_consumption_by_day, func)


def get_num_of_act_periods(real_consumption_by_day,
                           max_non_act_days_to_believe_that_active,
                           min_consumption_by_day):
    func = lambda arr: arr.shape[0]
    return calculate_info_with_act_tog(real_consumption_by_day,
                                max_non_act_days_to_believe_that_active,
                                min_consumption_by_day,
                                func)


def get_avg_act_days_in_act_month(df, real_active_consumption_by_day):
    arr = calculate_info_in_active_month(df, real_active_consumption_by_day, counter)
    if arr.shape[0] <= 2:
        return np.max(arr)
    if arr[0] < 20:
        arr = arr[1:]
    if get_last_date(df).day < 20 and arr.shape[0] > 0:
        arr = arr[:-1]
    return np.mean(arr)

def get_act_days_in_trial(trial_active_consumption_by_day):
    return trial_active_consumption_by_day.shape[0]
def activity_info_calc(
                    df,
                    row,
                    curr_date_to_predict,
                    observe_previous_weeks,
                    observe_previous_weeks_dt,
                    window_in_days_to_observe_earlier,
                    window_in_days_to_observe_earlier_dt,
                    proc_of_using_to_think_that_feature_is_active,
                    max_non_act_days_to_believe_that_active,
                    num_of_days_to_observe_in_future,
                    min_consumption_by_day,
                    treshold_activity,
                    activity_columns,
                    real_active_consumption_by_day,
                    real_consumption_by_day,
                    all_active_consumption_by_day,
                    trial_active_consumption_by_day,
                    all_features):

    if df.shape[0] == 0:
        return None
    ans = {}
    ans["days_in_cloud"] = get_days_in_cloud(real_consumption_by_day)
    ans["active_days_in_cloud"] = get_active_days_in_cloud(real_active_consumption_by_day)
    ans["proc_of_active_day_in_cloud"] = get_proc_of_active_day_in_cloud(real_active_consumption_by_day,
                                                                        real_consumption_by_day)

    ans["num_of_{}_days_non_act_tog".format(num_of_days_to_observe_in_future)] = \
            get_num_of_n_days_non_act_tog(real_consumption_by_day,
                                  num_of_days_to_observe_in_future,
                                  min_consumption_by_day)

    ans["prob_to_be_not_active_next_{}_days(depend_on_cogorta)".format(\
                            num_of_days_to_observe_in_future)] = \
            get_prob_to_be_not_active_next_n_days(df, curr_date_to_predict,
                                                      real_active_consumption_by_day,
                                                      min_consumption_by_day,
                                                      treshold_activity)

    ans["num_of_non_act_days_before_tog"] = \
            get_num_of_non_act_days_before_tog(df, curr_date_to_predict)

    ans["avg_num_of_act_days_tog(non_act<={}_day(s))".format(max_non_act_days_to_believe_that_active)] = \
        get_avg_num_of_act_days_tog(real_consumption_by_day,
                                max_non_act_days_to_believe_that_active,
                                min_consumption_by_day)
    ############################
    ans["avg_num_of_non_act_days_tog"] = get_avg_num_of_non_act_days_tog(real_consumption_by_day,
                                  min_consumption_by_day)

    ans["max_num_of_non_act_days_tog"] = get_max_num_of_non_act_days(real_consumption_by_day,
                                  min_consumption_by_day)

    ans["num_of_non_act_periods"] = get_num_of_non_act_periods(real_consumption_by_day,
                                  min_consumption_by_day)

    ans["avg_act_days_in_act_month"] = get_avg_act_days_in_act_month(df, real_active_consumption_by_day)

    ans["num_of_act_periods(non_act<={}_day(s))".format(max_non_act_days_to_believe_that_active)] = \
            get_num_of_act_periods(real_consumption_by_day,
                           max_non_act_days_to_believe_that_active,
                           min_consumption_by_day)

    ans["act_days_in_trial"] = get_act_days_in_trial(trial_active_consumption_by_day)
   # print(activity_columns)
    for column in activity_columns:
        row.append(ans[column])
        #print("column: {} --> {}".format(column, ans[column]))

def get_avg_consump_per_act_day(real_active_consumption_by_day):
    return np.mean(real_active_consumption_by_day["real_consumption"])

def get_variance_per_day(real_active_consumption_by_day):
    return np.var(real_active_consumption_by_day["real_consumption"])

def get_sum_trial_cons(df):
    return df["trial_consumption"].sum()

def get_sum_real_cons(df):
    return df["real_consumption"].sum()

def get_avg_trial_cons_per_act_day(trial_active_consumption_by_day):
    return np.mean(trial_active_consumption_by_day["trial_consumption"])

def get_cogorta_trial(trial_active_consumption_by_day, treshold):
    summ = get_avg_trial_cons_per_act_day(trial_active_consumption_by_day)
    return find_cogorta(summ, treshold)

def get_previous_n_week_cons(df, curr_date_to_predict, observe_previous_weeks_start_dt):
    df_curr = get_df_in_period(df, curr_date_to_predict, observe_previous_weeks_start_dt)
    return df_curr["real_consumption"].sum()

def get_consumption_feature_info(df, curr_date_to_predict,
                                             window_in_days_to_observe_earlier_dt, feature, pattern_ind):
    assert(pattern_ind <= 1)
    if pattern_ind == 1:
        return get_proc_of_feature_cons_to_cons_last_n_day(df, curr_date_to_predict,
                                             window_in_days_to_observe_earlier_dt, feature)

def get_proc_of_feature_cons_to_cons_last_n_day(df, curr_date_to_predict,
                                             window_in_days_to_observe_earlier_dt, feature):
    df_curr = get_df_in_period(df, curr_date_to_predict, window_in_days_to_observe_earlier_dt)
    feature_df = df_curr[df_curr[feature] == 1]
    return feature_df["real_consumption"].sum() / df_curr["real_consumption"].sum()

def consumption_info_calc(df,
                            row,
                            curr_date_to_predict,
                            observe_previous_weeks,
                            observe_previous_weeks_dt,
                            window_in_days_to_observe_earlier,
                            window_in_days_to_observe_earlier_dt,
                            proc_of_using_to_think_that_feature_is_active,
                            max_non_act_days_to_believe_that_active,
                            num_of_days_to_observe_in_future,
                            min_consumption_by_day,
                            treshold_paid,
                            consumption_columns,
                            consumption_pattern_service,
                            real_active_consumption_by_day,
                            trial_active_consumption_by_day,
                            all_features):
    if df.shape[0] == 0:
        return None
    ans = {}
    ans["avg_consump_per_act_day"] = get_avg_consump_per_act_day(real_active_consumption_by_day)
    ans['cogorta_paid'] = get_cogorta(real_active_consumption_by_day, treshold_paid)
    ans["variance_per_day"] = get_variance_per_day(real_active_consumption_by_day)
    ans['sum_trial_cons'] = get_sum_trial_cons(df)
    ans['sum_real_cons'] = get_sum_real_cons(df)
    ans["avg_trial_cons_per_act_day"] = get_avg_trial_cons_per_act_day(trial_active_consumption_by_day)
    ans["cogorta_trial"] = get_cogorta_trial(trial_active_consumption_by_day, treshold_paid)
    ans["previous_{}_week(s)_cons".format(observe_previous_weeks)] = get_previous_n_week_cons(df,
                                                                                curr_date_to_predict,
                                                                                observe_previous_weeks_dt)
    for feature in all_features:
        for ind, pattern in enumerate(consumption_pattern_service):
            ans[pattern.format(feature, window_in_days_to_observe_earlier)] = \
                                                    get_consumption_feature_info(df, curr_date_to_predict,
                                             window_in_days_to_observe_earlier_dt, feature, ind + 1)
    for column in consumption_columns:
        row.append(ans[column])
        #print("column: {} --> {}".format(column, ans[column]))


def probab_to_be_non_active_next_n_days_for_acc(
                                        df, number,
                                        more_than_n,
                                        curr_date_to_predict,
                                        num_of_days_to_observe_in_future,
                                        observe_previous_weeks,
                                        min_consumption_by_day,
                                        treshold):

    consumption_by_day = get_grouped_consumption(df, "real_consumption", "d")
    real_active_consumption_by_day = get_grouped_active_consumption(df, "real_consumption", "d",
                                                                min_consumption_by_day)
    if consumption_by_day.shape[0] < 7:
        return
    cogorta = get_cogorta(real_active_consumption_by_day, treshold)
    observe_previous_days = observe_previous_weeks * 7
    cnt = 0
    for _, row in consumption_by_day.iterrows():
        if row['real_consumption'] <= min_consumption_by_day: # Не юзал сервис в этот день
            cnt += 1
        if row['real_consumption'] > min_consumption_by_day: # юзал сервис в этот день
            for i in range (0, observe_previous_days, 1):
                if cnt >= i + num_of_days_to_observe_in_future:
                    #print(df["puid"].unique())
                    more_than_n[i][cogorta] += 1
                if cnt >= i:
                    number[i][cogorta] += 1
            cnt = 0
    cnt = (curr_date_to_predict - get_last_date(df)).days
    for i in range (0, observe_previous_days, 1):
        if cnt >= i + num_of_days_to_observe_in_future:
            more_than_n[i][cogorta] += 1
            number[i][cogorta] += 1

def probab_to_be_non_active_next_n_days(df_all,
                                        curr_date_to_predict,
                                        num_of_days_to_observe_in_future,
                                        observe_previous_weeks,
                                        min_consumption_by_day,
                                        treshold):
    #print(curr_date_to_predict)
    #print(df_all["event_time"].head())
    df = df_all[df_all["event_time"] < curr_date_to_predict]
    observe_previous_days = observe_previous_weeks * 7
    number = np.zeros((observe_previous_days, len(treshold)))
    more_than_n = np.zeros((observe_previous_days, len(treshold)))

    df_grouped = df.groupby(["puid"])
    for puid, df_account in df_grouped:
        probab_to_be_non_active_next_n_days_for_acc(
                                        df_account,
                                        number,
                                        more_than_n,
                                        curr_date_to_predict,
                                        num_of_days_to_observe_in_future,
                                        observe_previous_weeks,
                                        min_consumption_by_day,
                                        treshold)
        #print(more_than_n)
    print("Cogorta probabilities calculated!")
    #print(more_than_n)
    #print("========")
    #print(number)
    cogorta = more_than_n / number
    more_than_n_df = pd.DataFrame(more_than_n)
    more_than_n_df.to_csv("cogorta_numerator.csv", index = False)
    number = pd.DataFrame(number)
    number.to_csv("cogorta_denominator.csv", index = False)
    return cogorta

def ans_for_acc_creator(df_all, puids,
                        curr_date_to_predict,
                        num_of_days_to_observe_in_future,
                        min_consumption_by_day,
                        max_active_days_in_future_to_say_that_not_active):

    df = df_all[df_all["puid"].isin(puids)]
    #print(curr_date_to_predict, timedelta(days = num_of_days_to_observe_in_future))
    finish_dt = curr_date_to_predict + timedelta(days = num_of_days_to_observe_in_future)
    df_curr = get_df_in_period(
                                df,
                                finish_dt,
                                curr_date_to_predict)
    df_grouped = df_curr.groupby(["puid"])
    ans = {}
    for puid, df_account in df_grouped:
        real_active_consumption_by_day = get_grouped_active_consumption(df_account, "real_consumption", "d",
                                                                min_consumption_by_day)
        days = real_active_consumption_by_day.shape[0]
        if days > max_active_days_in_future_to_say_that_not_active:
            ans[puid] = 0
        else:
            ans[puid] = 1
    return ans

def curr_info_table_prepare(df_all, observe_previous_weeks_dt, curr_date_to_predict, min_consumption_by_day):
    df = df_all[df_all["event_time"] < curr_date_to_predict]
    df_curr = get_df_in_period(df[df["real_consumption"] >= min_consumption_by_day],
                               curr_date_to_predict,
                               observe_previous_weeks_dt)

    puids = df_curr["puid"].unique()
    df = df[df["puid"].isin(puids)]
    return df

def row_creator(df,
                curr_date_to_predict,
                observe_previous_weeks,
                observe_previous_weeks_dt,
                window_in_days_to_observe_earlier,
                window_in_days_to_observe_earlier_dt,
                proc_of_using_to_think_that_feature_is_active,
                max_non_act_days_to_believe_that_active,
                num_of_days_to_observe_in_future,
                min_consumption_by_day,
                treshold_activity,
                treshold_paid,
                common_columns,
                activity_columns,
                consumption_columns,
                common_pattern_subservice,
                consumption_pattern_service,
                real_active_consumption_by_day,
                real_consumption_by_day,
                all_active_consumption_by_day,
                trial_active_consumption_by_day,
                all_features
                ):
    row = []
    row.append(curr_date_to_predict)
    #print(activity_columns)
    common_info_calc(
                            df,
                            row,
                            curr_date_to_predict,
                            observe_previous_weeks,
                            observe_previous_weeks_dt,
                            window_in_days_to_observe_earlier,
                            window_in_days_to_observe_earlier_dt,
                            proc_of_using_to_think_that_feature_is_active,
                            max_non_act_days_to_believe_that_active,
                            num_of_days_to_observe_in_future,
                            min_consumption_by_day,
                            treshold_activity,
                            treshold_paid,
                            common_columns,
                            common_pattern_subservice,
                            real_active_consumption_by_day,
                            real_consumption_by_day,
                            all_active_consumption_by_day,
                            all_features)

    activity_info_calc(
                            df,
                            row,
                            curr_date_to_predict,
                            observe_previous_weeks,
                            observe_previous_weeks_dt,
                            window_in_days_to_observe_earlier,
                            window_in_days_to_observe_earlier_dt,
                            proc_of_using_to_think_that_feature_is_active,
                            max_non_act_days_to_believe_that_active,
                            num_of_days_to_observe_in_future,
                            min_consumption_by_day,
                            treshold_activity,
                            activity_columns,
                            real_active_consumption_by_day,
                            real_consumption_by_day,
                            all_active_consumption_by_day,
                            trial_active_consumption_by_day,
                            all_features)

    consumption_info_calc(
                            df,
                            row,
                            curr_date_to_predict,
                            observe_previous_weeks,
                            observe_previous_weeks_dt,
                            window_in_days_to_observe_earlier,
                            window_in_days_to_observe_earlier_dt,
                            proc_of_using_to_think_that_feature_is_active,
                            max_non_act_days_to_believe_that_active,
                            num_of_days_to_observe_in_future,
                            min_consumption_by_day,
                            treshold_paid,
                            consumption_columns,
                            consumption_pattern_service,
                            real_active_consumption_by_day,
                            trial_active_consumption_by_day,
                            all_features)
    return row

def ml_table_creator(
                    df_all,
                    curr_date_to_predict,
                    observe_previous_weeks,
                    window_in_days_to_observe_earlier,
                    proc_of_using_to_think_that_feature_is_active,
                    max_non_act_days_to_believe_that_active,
                    num_of_days_to_observe_in_future,
                    min_consumption_by_day,
                    treshold_activity,
                    treshold_paid,
                    services,
                    subservices,
                    TESTED = False,
                    max_active_days_in_future_to_say_that_not_active = 3):

    window_in_days_to_observe_earlier_dt = curr_date_to_predict - \
                                timedelta(days = window_in_days_to_observe_earlier)

    observe_previous_weeks_dt = curr_date_to_predict - timedelta(days = observe_previous_weeks * 7)
    res_columns, \
    common_columns, \
    activity_columns, \
    consumption_columns,\
    common_pattern_subservice,\
    consumption_pattern_service, \
    all_features                 = make_columns(
                                                observe_previous_weeks,
                                                window_in_days_to_observe_earlier,
                                                proc_of_using_to_think_that_feature_is_active,
                                                max_non_act_days_to_believe_that_active,
                                                num_of_days_to_observe_in_future,
                                                min_consumption_by_day,
                                                services,
                                                subservices)
    #print(activity_columns)

    df = curr_info_table_prepare(df_all, observe_previous_weeks_dt, curr_date_to_predict, min_consumption_by_day)
    df_grouped = df.groupby(["puid"])
    puids = df["puid"].unique()
    if TESTED == True:
        answers = ans_for_acc_creator(df_all, puids,
                        curr_date_to_predict,
                        num_of_days_to_observe_in_future,
                        min_consumption_by_day,
                        max_active_days_in_future_to_say_that_not_active)
    res_columns.append("is_not_active_next_{}_days(except_{}_days)".format(
                            num_of_days_to_observe_in_future,
                            max_active_days_in_future_to_say_that_not_active))

    person_df = pd.DataFrame(columns = res_columns)
    ind = 1
    for puid, df_account in df_grouped:

        all_active_consumption_by_day = get_grouped_active_consumption(df_account, "all_consumption", "d",
                                                                min_consumption_by_day)
        real_active_consumption_by_day = get_grouped_active_consumption(df_account, "real_consumption", "d",
                                                                min_consumption_by_day)
        real_consumption_by_day = get_grouped_consumption(df_account, "real_consumption", "d")
        trial_active_consumption_by_day = get_grouped_active_consumption(df_account, "trial_consumption", "d",
                                                                min_consumption_by_day)

        row = row_creator(
                            df_account,
                            curr_date_to_predict,
                            observe_previous_weeks,
                            observe_previous_weeks_dt,
                            window_in_days_to_observe_earlier,
                            window_in_days_to_observe_earlier_dt,
                            proc_of_using_to_think_that_feature_is_active,
                            max_non_act_days_to_believe_that_active,
                            num_of_days_to_observe_in_future,
                            min_consumption_by_day,
                            treshold_activity,
                            treshold_paid,
                            common_columns,
                            activity_columns,
                            consumption_columns,
                            common_pattern_subservice,
                            consumption_pattern_service,
                            real_active_consumption_by_day,
                            real_consumption_by_day,
                            all_active_consumption_by_day,
                            trial_active_consumption_by_day,
                            all_features)
        if TESTED == True:
            if answers.get(puid) is None:
                row.append(1)
            else:
                row.append(answers[puid])
        else:
            row.append(-1)
        person_df.loc[ind] = row
        if ind % 100 == 0:
            gc.collect()
        ind += 1
    return person_df

def get_curr_date_to_predict(person_df):
    return person_df["curr_date_to_predict"].iloc[-1]

def table_preparation(df):
    df["curr_date_to_predict"] = pd.to_datetime(df["curr_date_to_predict"])
    df.index = np.arange(1, df.shape[0] + 1, 1)
    df = df.replace("неизвестно", 'undefined')
    bool_cols = df.select_dtypes(include=[np.bool]).columns
    df[bool_cols] = df[bool_cols].astype(np.int32)
    return df

def update_answers(df_all, person_df,
                        num_of_days_to_observe_in_future,
                        min_consumption_by_day,
                        max_active_days_in_future_to_say_that_not_active, y_name):

    curr_date_to_predict = get_curr_date_to_predict(person_df)
    df = df_all[df_all["event_time"] < curr_date_to_predict]
    test_df = person_df[person_df[y_name] == -1]
    dates = sorted(test_df["curr_date_to_predict"].unique())
    for date in dates:
        #print(date)
        curr_df = test_df[test_df["curr_date_to_predict"] == date]
        puids = curr_df["puid"].unique()
        date = pd.to_datetime(date)
        currs = ans_for_acc_creator(df_all, puids,
                        date,
                        num_of_days_to_observe_in_future,
                        min_consumption_by_day,
                        max_active_days_in_future_to_say_that_not_active)
        for ind in curr_df.index:
            puid = curr_df["puid"].loc[ind]
            #print(ind, puid)
            if curr_date_to_predict >= date + timedelta(days = num_of_days_to_observe_in_future):
                #print("loch")
                if currs.get(puid) is None:
                    person_df[y_name].loc[ind] = 1
                else:
                    person_df[y_name].loc[ind] = currs[puid]
            else:
                if currs.get(puid) is not None and currs[puid] == 0:
                    person_df[y_name].loc[ind] = 0

    return person_df

df, services, subservices = create_df(df_columns)
start_train_date = pd.to_datetime('2019-03-04')
end_train_date = pd.to_datetime('2019-06-01')
date = parse('2019-06-01')
df = df[df["event_time"] < date]

#cog = probab_to_be_non_active_next_n_days(df,
#                                            date,
#                                            num_of_days_to_observe_in_future,
#                                            observe_previous_weeks,
#                                            min_consumption_by_day,
#                                            treshold_activity)
#print("cogorta done")

def daterange(start_date, end_date):
    for n in range(int ((end_date - start_date).days / 7 + 1) ):
        yield start_date + timedelta(n * 7)

df_arr = []
for curr_date in daterange(start_train_date, end_train_date):
    print(curr_date)
    curr_person_df = ml_table_creator(
                df,
                curr_date,
                observe_previous_weeks,
                window_in_days_to_observe_earlier,
                proc_of_using_to_think_that_feature_is_active,
                max_non_act_days_to_believe_that_active,
                num_of_days_to_observe_in_future,
                min_consumption_by_day,
                treshold_activity,
                treshold_paid,
                services,
                subservices,
                TESTED = False,
                max_active_days_in_future_to_say_that_not_active = max_active_days_in_future_to_say_that_not_active)
    df_arr.append(curr_person_df)
    print("========================")

y_name = "is_not_active_next_{}_days(except_{}_days)".format(
                                num_of_days_to_observe_in_future,
                                max_active_days_in_future_to_say_that_not_active)

person_df = pd.concat(df_arr)
person_df = table_preparation(person_df)
person_df = update_answers(df, person_df,
                                            num_of_days_to_observe_in_future,
                                            min_consumption_by_day,
                                            max_active_days_in_future_to_say_that_not_active, y_name)
person_df.to_csv("person_df.csv", index=False)

