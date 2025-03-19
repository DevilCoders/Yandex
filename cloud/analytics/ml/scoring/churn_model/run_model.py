#!/usr/bin/env python
# coding: utf-8

# In[1]:


import calendar
import gc
import json
import math
import os
import os.path
import random
import sys
import time
import typing as tp
import warnings
from collections import defaultdict
from datetime import datetime, timedelta

import numpy as np
import pandas as pd
import requests
import scipy.stats as sps
from dateutil.parser import *
from joblib import Parallel, delayed
from sklearn.base import BaseEstimator, TransformerMixin
from sklearn.linear_model import LinearRegression
from sklearn.metrics import (accuracy_score, f1_score, fbeta_score,
                             make_scorer, precision_score, recall_score)
from sklearn.model_selection import GridSearchCV, train_test_split
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import (MinMaxScaler, Normalizer, StandardScaler,
                                   normalize)

import yt.wrapper as yt
from catboost import CatBoostClassifier

warnings.filterwarnings("ignore")


# In[7]:


# In[8]:


experiment_start_date = '2020-09-25'


# In[9]:


treshold_paid = [(0, 0), (33, 1), (67, 2), (333, 3), (1000, 4)]
treshold_paid_log = [(np.log(x), ind) for x, ind in treshold_paid]
treshold_paid_log


# ## -1

# In[10]:


try:
    from vault_client.instances import Production as VaultClient

    client = VaultClient(decode_files=True)
    secret_uuid = "sec-01e2dyrwvwrmnk1r2q7rnyejcy"
    tokens = client.get_version(secret_uuid)
    yt_token = tokens["value"]["clickhouse_token"]
except Exception:
    yt_token = sys.argv[1]

yt.config["proxy"]["url"] = "hahn"
yt.config["token"] = yt_token
cluster = "hahn"
alias = "*cloud_analytics"


def _raw_execute_yt_query(query: str, timeout=600) -> tp.List[str]:
    '''
    Executes chyt query, returns array of strings
    :param query: query to execute on cluster
    :return: list of strings (rows of table)
    '''
    token = yt_token
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = f"{proxy}/query?database={alias}&password={token}"        "&enable_optimize_predicate_expression=0"

    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.text.strip().split('\n')
    return rows


def _raw_chyt_execute_query(query: str,
                            columns: tp.Optional[str] = None) -> pd.DataFrame:
    '''
    Executes chyt query, returns pd.DataFrame
    :param query: query to execute on cluster
    :return: pd.DataFrame (final table)
    '''
    counter = 0
    while True:
        try:
            result = _raw_execute_yt_query(query=query)
            if columns is None:
                df_raw = pd.DataFrame([row.split('\t')
                                       for row in result[1:]],
                                      columns=result[0].split('\t'))
            else:
                df_raw = pd.DataFrame([row.split('\t')
                                       for row in result], columns=columns)
            return df_raw
        except Exception as err:
            print(err)
            counter += 1
            if counter > 5:
                print('Break Excecution')
                break


def _update_automatically_types(df_raw: pd.DataFrame) -> pd.DataFrame:
    """
    Update all string types in table to list, int, float, str.
    if 'id' in column name and column type may
    be like int - does not update this column.

    :param df_raw: raw dataframe which column types need to be updated
    :return: pd.DataFrame (updated table)
    """
    df = df_raw.copy()
    for column in df.columns:
        # list updater
        try:
            df[column] = df[column].apply(lambda x: json.loads(x)
                                          if isinstance(json.loads(x), list)
                                          else throw_exeption_in_code)
            continue
        except Exception:
            pass
        try:
            df[column] = df[column].apply(lambda x:
                                          json.loads(x.replace("'", '"'))
                                          if isinstance(
                                              json.loads(x.replace("'", '"')),
                                              list)
                                          else throw_exeption_in_code
                                          )
            continue
        except Exception:
            pass
        # id checker
        try:
            if ("id" in column and "paid" not in column) and not pd.isnull(df[column].astype(int).sum()):
                continue
        except Exception:
            pass
        # int updater
        try:
            df[column] = df[column].astype(int)
            continue
        except Exception:
            pass

        # float updater
        try:
            df[column] = df[column].astype(float)
            continue
        except Exception:
            pass
    return df


def works_with_emails(mail_):
    """mail processing
    :param mail_: mail string
    :return: processed string
    """
    mail_parts = str(mail_).split('@')
    if len(mail_parts) > 1:
        if 'yandex.' in mail_parts[1].lower() or 'ya.' in mail_parts[1].lower():
            domain = 'yandex.ru'
            login = mail_parts[0].lower().replace('.', '-')
            return login + '@' + domain
        else:
            return mail_


def execute_query(query: str,
                  columns: tp.Optional[str] = None) -> pd.DataFrame:
    """Execute query, returns pandas dataframe as result
    :param query: query to execute on cluster
    :param columns: name of dataframe columns
    :return: pandas dataframe, the result of query
    """
    df = _raw_chyt_execute_query(query, columns)
    df = df.replace('\\N', np.NaN)
    df = _update_automatically_types(df)
    if "email" in df.columns:
        df["email"] = df["email"].apply(lambda x: works_with_emails(x))
    return df


def time_to_unix(time_str: str) -> int:
    """
    String time to int
    """
    dt = parse(time_str)
    timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
    return int(timestamp)


schema_type = tp.List[tp.Dict[str, tp.Union[str, tp.Dict[str, str]]]]


def _apply_type(raw_schema: tp.Optional[schema_type],
                df: pd.DataFrame) -> tp.List[schema_type]:
    """
    Create schema for dataset in YT.
    Supports datetime, int64, double, list with string and int and float.
    """
    if raw_schema is not None:
        for key in raw_schema:
            if 'list:' not in raw_schema[key] and raw_schema[key] != 'datetime':
                df[key] = df[key].astype(raw_schema[key])
            if raw_schema[key] == 'datetime':
                df[key] = df[key].apply(lambda x: time_to_unix(x))

    schema: tp.List[tp.Dict[str, tp.Union[str, tp.Dict[str, str]]]] = []
    for col in df.columns:
        if raw_schema is not None and raw_schema.get(col) is not None and raw_schema[col] == 'datetime':
            schema.append({"name": col, 'type': 'datetime'})
            continue
        if df[col].dtype == int:
            schema.append({"name": col, 'type': 'int64'})
        elif df[col].dtype == float:
            schema.append({"name": col, 'type': 'double'})
        elif raw_schema is not None and raw_schema.get(col) is not None and 'list:' in raw_schema[col]:
            second_type = raw_schema[col].split("list:")[-1]
            schema.append({"name": col, 'type_v3':
                           {"type_name": 'list', "item":
                            {"type_name": "optional", "item": second_type}}})
        else:
            schema.append({"name": col, 'type': 'string'})
    return schema


def save_table(file_to_write: str, path: str,
               table: pd.DataFrame,
               schema: tp.Optional[schema_type] = None,
               append: str = False) -> None:
    """
    Save table in HAHN.
    :param file_to_write: table name in HAHN
    :param path: '//path/to/folder', folder, where to save table
    :param schema: schema for table
    :param append: append to the end or not.
    Creates new table if does not exisists
    :return: None
    """
    assert(path[-1] != '/')

    df = table.copy()
    real_schema = _apply_type(schema, df)
    json_df_str = df.to_json(orient='records')
    path = path + "/" + file_to_write
    json_df = json.loads(json_df_str)
    if not yt.exists(path) or not append:
        yt.create(type="table", path=path, force=True,
                  attributes={"schema": real_schema})
    tablepath = yt.TablePath(path, append=append)
    yt.write_table(tablepath, json_df,
                   format=yt.JsonFormat(attributes={"encode_utf8": False}))


# In[11]:


def find_tables_in_hahn_folder(path):
    tables = []
    for table in yt.search(path, node_type=["table"],
                           attributes=["account"]):
        tables.append(table)
    return tables


def get_current_date_as_str():
    return str(datetime.date(datetime.now()))


def date_to_string(date):
    return datetime.strftime(date, "%Y-%m-%d")


def date_range(end_date, freq_interval=None, start_date=None, freq='D'):
    if freq_interval is None:
        return pd.date_range(start=start_date, end=end_date, freq=freq).to_pydatetime().tolist()
    return pd.date_range(end=end_date, periods=freq_interval, freq=freq).to_pydatetime().tolist()


def replace_column_in_df(df_raw, column, where="front"):
    df = df_raw.copy()
    cols = df.columns.tolist()
    cols.remove(column)
    if where == "front":
        columns = [column] + cols
    else:
        columns = cols + [column]
    return df[columns]


def concatenate_tables(df_array):
    """Concatenates array of dataframes with possible Nan inside array
    :param df_array: array of dataframes
    :return: Nane or dataframe
    """

    df_array = [df for df in df_array if df is not None]
    if len(df_array) == 0:
        return None
    res_df = pd.concat(df_array, axis=0)
    res_df.index = np.arange(0, res_df.shape[0], 1)
    return res_df


# In[12]:


def get_group(index_to_check, dict_of_groups):
    """raw function for addABTestingGroup, returns group name depends of index"""
    for key in dict_of_groups:
        if index_to_check in dict_of_groups[key]:
            return key
    return ""


def add_AB_testing_group(df_raw, dict_of_groups):
    """ Add new column with group for AB testing
    :param df_raw: raw dataframe, does not change
    :param dict_of_groups: dict of groups, for example {"A": list_of_df_indexes_in_group_A}
    :return: new dataframe with column "Group", where groups are written
    """
    df = df_raw.copy()
    df["Group"] = df.index.map(lambda x: get_group(x, dict_of_groups))
    return df


# # 0. Создание общего индекса

# - get_id_by_cloud_id
# - get_id_by_billing_id
# - get_biliing_by_id

# In[13]:


"""============================================================================================================"""


# META ID

class IdGenerator:
    def __init__(self, number_of_digits: int = 9):
        self.given_id: tp.Set[int] = set()
        self.number_of_digits: int = number_of_digits

    def __generate_new_id(self) -> int:
        if len(self.given_id) == 10 ** (self.number_of_digits) - 10 ** (self.number_of_digits - 1):
            raise ValueError("Sorry, no free id")
        while True:
            current_id = np.random.randint(
                10 ** (self.number_of_digits - 1), 10 ** (self.number_of_digits))
            if current_id not in self.given_id:
                self.given_id.add(current_id)
                return current_id

    __call__ = __generate_new_id


class MetaInformationClass:

    def __init__(self, interested_columns=[]):
        self.dict_cloud: tp.Dict[str, int] = {}  # by cloud_id get id
        self.dict_billing: tp.Dict[str, int] = {}  # by billing_id get id
        self.dict_id_to_folders: tp.Dict[int, tp.Set[str]] = defaultdict(
            set)  # by meta_id get floders
        self.dict_id_to_billing: tp.Dict[int, tp.List[str]] = defaultdict(
            list)  # by id get get all billings
        self.dict_id_to_cloud: tp.Dict[int, tp.List[str]] = defaultdict(
            list)  # by id get get all clouds
        # by id get get last billing by time in cube
        self.dict_id_to_last_billing: tp.Dict[int, str] = {}
        self.gen_id = IdGenerator()
        self.visited: tp.Set[str] = set()
        # matching billing to last billing
        self.billing_to_new_billing: tp.Dict[str, str] = {}
        self.columns = interested_columns

    def __get_for_billing_id(self) -> pd.DataFrame:
        part_req = [f"max(if (event == 'ba_created', {col}, '')) as {col}," for col in self.columns
                    if col != "company_name"]
        if "company_name" in self.columns:
            part_req.append("""
            max(multiIf(account_name IS NOT NULL AND account_name != 'unknown',account_name,
                    balance_name IS NOT NULL AND balance_name != 'unknown',balance_name,
                    promocode_client_name IS NOT NULL AND promocode_client_name !='unknown',
                    promocode_client_name, CONCAT(first_name, ' ', last_name)
                    )) as company_name,
            """)
        part_req = "\n".join(part_req)
        request = f"""
    SELECT
        billing_account_id as billing,
        groupUniqArray(cloud_id) as array,
        {part_req}
        max(event_time) as date
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
    WHERE billing_account_id != ''
    AND event == 'ba_created'
    AND segment != 'VAR'
    AND cloud_id != ''
    GROUP BY billing_account_id
    ORDER BY date DESC
    FORMAT TabSeparatedWithNames
        """
        df = execute_query(request)
        return df

    def __get_for_cloud_id(self) -> pd.DataFrame:
        folder_paths = find_tables_in_hahn_folder(
            "//home/cloud_analytics/import/iam/cloud_folders/1h")
        folder_path = max(folder_paths, key=lambda x: yt.row_count(x))
        request = f"""
    SELECT
        cloud_id as cloud,
        groupUniqArray(billing_account_id) as array,
        max(folders) as folders
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as a
    LEFT JOIN (
        SELECT
            groupUniqArray(folder_id) as folders,
            cloud_id
        FROM "{folder_path}"
        WHERE cloud_status == 'ACTIVE'
        GROUP BY cloud_id
    ) as b
    ON a.cloud_id == b.cloud_id
    WHERE billing_account_id != ''
    AND event == 'ba_created'
    AND cloud_id != ''
    AND segment != 'VAR'
    GROUP BY cloud_id
    FORMAT TabSeparatedWithNames
        """
        df = execute_query(request)
        return df

    def __add_info(self, billing_or_cloud: str, current_id: int, where_to_add: str) -> None:
        getattr(self, "dict_id_to_" +
                where_to_add)[current_id].append(billing_or_cloud)
        getattr(self, "dict_" + where_to_add)[billing_or_cloud] = current_id

    def __dfs_visit(self, billing: str, current_id: int) -> None:
        if billing in self.visited:
            return
        self.visited.add(billing)
        self.__add_info(billing, current_id, "billing")
        self.billing_to_new_billing[billing] = self.dict_id_to_last_billing[current_id]

        for cloud in self.billing_as_vertex_dict[billing]:
            self.__add_info(cloud, current_id, "cloud")
            curr_add = self.cloud_to_folders_dict[cloud]
            self.dict_id_to_folders[current_id].update(curr_add)
            for chld_billing in self.cloud_as_vertex_dict[cloud]:
                self.__dfs_visit(chld_billing, current_id)

    def __create_dict(self, where_to_add: str) -> pd.DataFrame:
        vertex_df = getattr(
            self, f"_MetaInformationClass__get_for_{where_to_add}_id")()
        vertex_df.index = vertex_df[where_to_add]
        vertex_df.drop(columns=[where_to_add], inplace=True)
        curr_dict = vertex_df.to_dict()["array"]
        if where_to_add == 'cloud':
            self.cloud_to_folders_dict = vertex_df.to_dict()["folders"]
        setattr(self, f"{where_to_add}_as_vertex_dict", curr_dict)
        return vertex_df

    def create_users_id(self):
        cloud_as_vertex_df = self.__create_dict("cloud")
        billing_as_vertex_df = self.__create_dict("billing")

        for col in self.columns:
            setattr(self, f"billing_to_{col}", {})

        for billing, row in billing_as_vertex_df.iterrows():
            for col in self.columns:
                getattr(self, f"billing_to_{col}")[billing] = row[col]
            if billing not in self.visited:
                current_id = self.gen_id()
                self.dict_id_to_last_billing[current_id] = billing
                self.__dfs_visit(billing, current_id)

    def get_id_by_cloud_id(self, cloud_id: str) -> str:
        return str(self.dict_cloud[cloud_id])

    def get_id_by_billing_id(self, billing_id: str) -> str:
        return str(self.dict_billing[billing_id])

    def get_billing_by_id(self, meta_id: str) -> str:
        return self.dict_id_to_last_billing[int(meta_id)]

    def get_all_accosiated_billings(self, billing_id: str) -> tp.List[str]:
        try:
            meta_id = self.dict_billing[billing_id]
            return self.dict_id_to_billing[meta_id]
        except Exception:
            return ['billing_id']

    def get_info_from_column(self, billing_id: str, column: str):
        assert column in self.columns
        meta_id = self.dict_billing[billing_id]
        billings = self.dict_id_to_billing[meta_id]
        answer_list = []
        for billing in billings:
            answer_list.append(getattr(self, f"billing_to_{column}")[billing])
        answer_list = set(answer_list)
        bad_words = ["unknown", "-", ""]
        for word in bad_words:
            if word in answer_list:
                answer_list.remove(word)
        return list(answer_list)

    def get_dataframe_with_grouped_information(self):
        result_dict = defaultdict(list)
        for billing in self.dict_billing:
            meta_id = self.dict_billing[billing]
            result_dict["billing_account_id"].append(billing)
            result_dict["last_active_billing"].append(
                self.billing_to_new_billing[billing])
            result_dict["associated_billings"].append(
                list(set(self.dict_id_to_billing[meta_id])))
            result_dict["associated_clouds"].append(list(set(
                self.dict_id_to_cloud[meta_id])))
            result_dict["associated_folders"].append(
                list(self.dict_id_to_folders[meta_id]))
            for column in self.columns:
                result_dict[column].append(
                    self.get_info_from_column(billing, column))
        result_df = pd.DataFrame.from_dict(result_dict)
        return result_df


# In[14]:


user = MetaInformationClass(interested_columns=[])


# In[15]:


user.create_users_id()


# In[16]:


res_df = user.get_dataframe_with_grouped_information()


# In[17]:


res_df = res_df[['billing_account_id', 'last_active_billing']]


# In[18]:


res_df.index = res_df['billing_account_id']


# In[19]:


builling_to_last_active_billing = res_df['last_active_billing'].to_dict()


# In[ ]:


# # 1. Создание таблиц

# Не забыть:
#
# 1) Создать запросы, зависящие от времени
#
# 2) самому прописать все типы

# ## 1.1 VM_CUBE

# In[14]:


def create_vm_request(date: str) -> str:
    prod_part_req = ""
    for prod in cube_products:
        prod_part_req += "max(if(vm_product_name == '" + \
            prod + "'" + ', 1, 0)) as "' + prod + '",\n'
        vm_cube_request = f"""
SELECT
    cloud_id,
    avg(vm_cores) as num_of_cores_at_vm,
    length(groupUniqArray(vm_id)) as num_of_vm,
    count(DISTINCT node_az) as az_num,
    avg(vm_cores_real) as cores_real,
    avg(vm_memory_real) as vm_memory_real,
    avg(vm_memory_to_cores_ratio) as vm_memory_to_cores_ratio,
    avg(vm_preemptible) as preemptible,
    max(vm_age) as  vm_age,
    avg(vm_core_fraction) as core_fraction,
    {prod_part_req}
    max(if(vm_is_service == 'service', 1, 0)) as service,
    max(if(vm_is_service == 'customer', 1, 0)) as customer
FROM "//home/cloud_analytics/compute_logs/vm_cube/vm_cube"
WHERE toDate(vm_finish) < '{date}'
AND toDate(vm_finish) >= addDays(toDate('{date}'), -7)
GROUP BY cloud_id
FORMAT TabSeparatedWithNames
"""
    return vm_cube_request


# In[15]:


def core_fraction_type(core: float) -> str:
    if core < 25:
        return "small"
    if core < 100:
        return "medium"
    return "all"


# In[16]:


def create_vm_cube_info_df(date: str) -> pd.DataFrame:
    vm_cube_request = create_vm_request(date)
    vm_cube_df = execute_query(vm_cube_request)
    vm_cube_df["core_fraction"] = vm_cube_df["core_fraction"].apply(
        lambda x: core_fraction_type(x))
    return vm_cube_df


# ## 1.2 Managed Databes ON VM

# In[17]:


def create_mdb_on_vm_list(date: str) -> tp.Set[str]:
    request = f"""
SELECT
billing_account_id
FROM "//home/cloud_analytics/import/network-logs/db-on-vm/data"
WHERE toDate(date) < '{date}'
AND toDate(date) >= addDays(toDate('{date}'), -14)
GROUP BY billing_account_id
FORMAT TabSeparatedWithNames
    """
    df = execute_query(request)
    return set(df["billing_account_id"])


# ## 1.3 Managed Databes types

# ## 1.3.1 artificial intelligence

# ## 1.4  Paid consumption information

# In[18]:


def predict_tan(target: tp.List[float]) -> float:
    target = normalize(np.array(target)[:, np.newaxis], axis=0).ravel()
    x = np.arange(0, target.shape[0])
    model = LinearRegression()
    model.fit(x.reshape(-1, 1), np.array(target))
    return model.coef_[0]


def find_angle(tan1: float, tan2: float) -> float:
    return -math.degrees(math.asin((tan1 - tan2) / math.sqrt((tan2 * tan2 + 1) * (tan1 * tan1 + 1))))


def log_apply(val: float) -> float:
    return np.log(val)


# In[19]:


def create_paid_req(date: str, days: int) -> str:
    req = f"""
SELECT
    billing_account_id,
    groupArray(paid) as paid_arr_{days}
FROM (
    SELECT
        billing_account_id,
        date,
        sum(if(toDate(date) == toDate(event_time), real_consumption, 0)) as paid
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as a,
    (
        SELECT
            dt as date
        FROM (
            SELECT arrayMap(x -> addDays(toDate('{date}'), -x-1), range({days})) as dt
        )
        ARRAY JOIN dt
    ) as b
    WHERE sku_lazy == 0
    GROUP BY billing_account_id, date
    ORDER BY billing_account_id, date
)
GROUP BY billing_account_id
HAVING arraySum(paid_arr_{days}) > 0
FORMAT TabSeparatedWithNames
"""
    return req


def create_paid_df(date: str) -> pd.DataFrame:
    df_30 = execute_query(create_paid_req(date, 30))
    df_7 = execute_query(create_paid_req(date, 7))
    paid_df = pd.merge(df_7, df_30, on="billing_account_id", how="inner")
    for col in paid_df.columns:
        if "paid_arr" in col:
            paid_df["paid_coeff_" +
                    col.split("_")[-1]] = paid_df[col].apply(lambda x: predict_tan(x))
    paid_df["delta"] = paid_df[["paid_coeff_7", "paid_coeff_30"]].apply(lambda row:
                                                                        find_angle(row["paid_coeff_30"],
                                                                                   row["paid_coeff_7"]), axis=1)
    paid_df["cons_avg_last_week"] = paid_df["paid_arr_7"].apply(
        lambda val: np.mean(val))
    paid_df["cons_std_last_week"] = paid_df["paid_arr_7"].apply(
        lambda val: np.std(val))
    paid_df["consumer_plateau"] = (
        paid_df["cons_std_last_week"] / paid_df["cons_avg_last_week"]) < 0.15
    paid_df["cons_avg_last_week"] = np.log(paid_df["cons_avg_last_week"])
    paid_df["consumer_plateau"] = paid_df["consumer_plateau"].astype(int)
    paid_df.drop(columns=["paid_arr_7", "paid_arr_30"], inplace=True)
    return paid_df


# In[20]:


def create_services_cons_last_week_req(date: str) -> str:
    req = f"""
    SELECT
        billing_account_id,
        SUM(real_consumption) as all_consumption,
        SUM(if(database == 'clickhouse', real_consumption, 0)) / all_consumption * 100 as clickhouse_pct,
        SUM(if(database == 'postgres', real_consumption, 0)) / all_consumption * 100 as pg_pct,
        SUM(if(database == 'redis', real_consumption, 0)) / all_consumption * 100 as redis_pct,
        SUM(if(database == 'mongo', real_consumption, 0)) / all_consumption * 100 as mongo_pct,
        SUM(if(database like '%sql%', real_consumption, 0)) / all_consumption * 100 as mysql_pct,
        SUM(if(sku_name like '%speech%', real_consumption, 0)) / all_consumption * 100 as speech_pct,
        SUM(if(sku_name like '%translate%', real_consumption, 0)) / all_consumption * 100 as translate_pct,
        SUM(if(sku_name like '%vision%', real_consumption, 0)) / all_consumption * 100 as vision_pct,
    
        SUM(if(lower(service_long_name) like '%compute%', real_consumption, 0)) / all_consumption * 100 as compute_pct,
        SUM(if(lower(service_long_name) like '%managed_databases%', real_consumption, 0)) / all_consumption * 100 
        as managed_databases_pct,
        SUM(if(lower(service_long_name) like '%artificial_intelligence%', real_consumption, 0)) / all_consumption * 100 
        as artificial_intelligence_pct,
        SUM(if(lower(service_long_name) like '%marketplace%', 
        real_consumption, 0)) / all_consumption * 100 as marketplace_pct,
        SUM(if(lower(service_long_name) like '%storage%', real_consumption, 0)) / all_consumption * 100 as storage_pct,
        SUM(if(lower(service_long_name) like '%network%', real_consumption, 0)) / all_consumption * 100 as network_pct,
        SUM(if(lower(service_long_name) not like '%network%' and
               lower(service_long_name) not like '%storage%' and
               lower(service_long_name) not like '%marketplace%' and
               lower(service_long_name) not like '%artificial_intelligence%' and
               lower(service_long_name) not like '%managed_databases%' and
               lower(service_long_name) not like '%compute%', real_consumption, 0)) / all_consumption * 100 as other_pct
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
    WHERE 
        toDate(event_time) < '{date}'
    AND toDate(event_time) >= addDays(toDate('{date}'), -7)
    AND sku_lazy == 0
    GROUP BY billing_account_id
    HAVING all_consumption > 0
    FORMAT TabSeparatedWithNames
    """
    return req


def create_services_cons_last_week_df(date: str) -> pd.DataFrame:
    service_df = execute_query(create_services_cons_last_week_req(date))
    service_df.drop(columns=["all_consumption"], inplace=True)
    return service_df


# ## 1.5  Visit information

# In[21]:


def create_visits_req(req_date: str) -> str:
    part_req_1 = ""
    days = 365
    req_date = date_to_string(parse(req_date) - timedelta(1))
    for date in date_range(req_date, freq_interval=days):
        str_date = date_to_string(date)
        part_req_1 += f"""if(toDate('{str_date}') == date and cons > 0, 1, 0) as "{str_date} tmp",\n"""
    part_req_1 = part_req_1[:-2]

    part_req_2 = ""
    for date in date_range(req_date, freq_interval=days):
        str_date = date_to_string(date)
        part_req_2 += f"""max("{str_date} tmp"),\n"""
    part_req_2 = part_req_2[:-2]

    req = f"""
SELECT
    billing_account_id,
    arrayFilter((x, i) -> i >= start_ind, visits_arr, arrayEnumerate(visits_arr)) as visits,
    total_consumption
FROM (
    SELECT
        billing_account_id,
        array({part_req_2}) as visits_arr,
        arrayFirstIndex(x -> (x != 0), visits_arr) as start_ind,
        sum(cons) as total_consumption
    FROM (
        SELECT
            billing_account_id,
            toDate(event_time) as date,
            sum(real_consumption) as cons,
            {part_req_1}
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE sku_lazy == 0
        GROUP BY billing_account_id, date
    )
    GROUP BY billing_account_id
    HAVING arraySum(visits_arr) > 0
)
ORDER BY billing_account_id
FORMAT TabSeparatedWithNames
"""
    return req


# In[22]:


def get_days_arr(curr_visits_arr: tp.List[int], active_type: int = 1) -> tp.List[int]:
    curr_visits = 0
    ans_arr = []
    for elem in curr_visits_arr:
        if elem == 1 - active_type:
            if curr_visits > 0:
                ans_arr.append(curr_visits)
            curr_visits = 0
        else:
            curr_visits += 1
    ans_arr.append(curr_visits)
    return ans_arr


# In[23]:


def refresh_probobas(curr_visits_arr: tp.List[int],
                     num_of_non_visits: np.array,
                     num_of_non_visits_after_30: np.array) -> None:
    curr_zeros = 0
    for elem in curr_visits_arr:
        if elem == 0:
            curr_zeros += 1
        else:
            num_of_non_visits[np.arange(1, 8) <= curr_zeros] += 1
            num_of_non_visits_after_30[np.arange(1, 8) + 30 <= curr_zeros] += 1
            curr_zeros = 0
    num_of_non_visits[np.arange(1, 8) + 30 <= curr_zeros] += 1
    num_of_non_visits_after_30[np.arange(1, 8) + 30 <= curr_zeros] += 1


# In[24]:


def create_non_active_probabilities(df: pd.DataFrame) -> np.array:
    num_of_non_visits = np.zeros(7)
    num_of_non_visits_after_30 = np.zeros(7)
    for _, row in df.iterrows():
        refresh_probobas(row["visits"], num_of_non_visits,
                         num_of_non_visits_after_30)
    return num_of_non_visits_after_30 / num_of_non_visits


# In[25]:


def get_cogort_type(consumption: float) -> int:  # type
    for threshold, ind in treshold_paid_log:
        if threshold >= consumption:
            return ind - 1
    return treshold_paid_log[-1][1]


# In[26]:


def create_cogorta_churn_probabilty_column(df_raw) -> tp.List[float]:
    df = df_raw.copy()
    df["cogort"] = df["avg_consumption_in_active_day"].apply(
        lambda x: get_cogort_type(x))
    probas_matrix = np.zeros((len(treshold_paid_log), 7))
    for _, cogort_type in treshold_paid_log:
        cogort_df = df[(df["cogort"] == cogort_type) &
                       (df["total_active_days"] >= 7)]
        probas_matrix[cogort_type] = create_non_active_probabilities(
            cogort_df).tolist()
    proba_column = []
    for _, row in df.iterrows():
        cogort = row["cogort"]
        non_visited_days = row["last_non_active_period"]
        if non_visited_days == 0:
            proba_column.append(0)
            continue
        proba_column.append(
            probas_matrix[cogort, min(non_visited_days - 1, 6)])
    return proba_column


# In[27]:


def create_visits_df(date: str) -> pd.DataFrame:
    req = create_visits_req(date)
    visits_df = execute_query(req)
    if visits_df.shape[0] == 0:
        return visits_df
    visits_df["total_consumption"] = visits_df["total_consumption"].astype(
        float)
    visits_df["active_periods"] = visits_df["visits"].apply(
        lambda row: get_days_arr(row, 1))
    visits_df["non_active_periods"] = visits_df["visits"].apply(
        lambda row: get_days_arr(row, 0))
    visits_df["last_active_period"] = visits_df["active_periods"].apply(
        lambda x: x[-1])
    visits_df["last_non_active_period"] = visits_df["non_active_periods"].apply(
        lambda x: x[-1])
    visits_df["avg_non_active_period"] = visits_df["non_active_periods"].apply(lambda x: np.mean(x) if x[-1] != 0
                                                                               else np.mean(x[:-1]))
    visits_df["avg_active_period"] = visits_df["active_periods"].apply(lambda x: np.mean(x) if x[-1] != 0
                                                                       else np.mean(x[:-1]))
    visits_df["summary_active_days_pct"] = visits_df[["active_periods", "visits"]].apply(
        lambda row: np.sum(row["active_periods"]) / len(row["visits"]) * 100, axis=1)
    visits_df["total_active_days"] = visits_df["active_periods"].apply(
        lambda x: sum(x))
    visits_df["avg_non_active_period_greater_30"] = visits_df["avg_non_active_period"].apply(
        lambda x: int(x > 30))
    visits_df.replace(np.nan, 0, inplace=True)
    visits_df["avg_consumption_in_active_day"] = np.log(
        visits_df["total_consumption"] / visits_df["total_active_days"])
    churn_prob_column = create_cogorta_churn_probabilty_column(visits_df)
    visits_df["churn_probabilty"] = churn_prob_column
    visits_df.drop(columns=["visits",
                            "total_consumption",
                            "active_periods", "non_active_periods"], inplace=True)
    return visits_df


# ## 1.6 Main information DataFrame (as model features)

# In[28]:


def create_main_infromation_req(date: str) -> str:
    common_info_req = f"""
SELECT
    billing_account_id,
    cloud_id,
    age,
    sex,
    region,
    payment_type,
    person_type,
    if (state in ('suspended', 'inactive', 'deleted'), 
    'inactive', state) as state,
    multiIf (os like '%Mac%', 'Mac',
             os like '%Linux%' or os like '%Ubuntu%', 'Linux',
             os like '%Windows%', 'Windows', 
             'other') as os,
    is_suspended_now,
    from_desktop
FROM (
    SELECT
    billing_account_id,
    max(cloud_id) as cloud_id,
    max(age) as age, -- leak, but not important
    max(sex) as sex,-- leak, but not important
    max(multiIf(city == 'Москва', 'Moscow',
                city == 'Санкт-Петербург', 'Saint Petersburg',
                country == 'Россия', 'Russia', 
                isNotNull(country), 'Other countries',
                'undefined')) as region,  -- leak, but not important
    max(ba_payment_cycle_type) as payment_type,  -- leak, but not important
    max(if(ba_person_type like '%company%', 
                               'company',
                               ba_person_type)) as person_type,  -- leak, but not important
    argMax(ba_state, event_time) as state,
    if(state in ('suspended', 'inactive', 'deleted'), 1, 0)
    as is_suspended_now,
    max(segment) as segment,  -- leak, but not important
    max(If(device_type like '%desktop%', 1, 0)) as from_desktop,  -- leak, but not important
    max(if (isNotNull(os), splitByChar(' ', assumeNotNull(os))[1], Null)) as os, -- leak, but not important
    sum(if(
            toDate(event_time) < toDate('{date}') AND
            toDate(event_time) >= addDays(toDate('{date}'), -7) 
           AND sku_lazy == 0, real_consumption, 0)) as cons,
    max(multiIf(
    first_first_paid_consumption_datetime == '0000-00-00 00:00:00', '2070-01-01 00:00:00',
    first_first_paid_consumption_datetime)) as first_first_paid_consumption_datetime
FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
WHERE
    toDate(event_time) < toDate('{date}')
AND ba_usage_status != 'service'
GROUP BY billing_account_id
HAVING segment not in ('Enterprise', 'Large ISV', 'ISV ML', 'Medium', 'Yandex Projects', 'Yandex Staff')
AND cons > 0
AND toDate((first_first_paid_consumption_datetime)) < 
addDays(toDate('{date}'), -14)
)
FORMAT TabSeparatedWithNames
""".encode('utf-8')
    return common_info_req


# In[29]:


def create_main_infromation_df(date: str) -> pd.DataFrame:
    main_info_df = execute_query(create_main_infromation_req(date))
    main_info_df["is_suspended_now"] = main_info_df["is_suspended_now"].astype(
        float)
    main_info_df["from_desktop"] = main_info_df["from_desktop"].astype(float)
    main_info_df.replace(np.nan, "undefined", inplace=True)
    return main_info_df


# ## 1.7 Main information DataFrame (ONLY for mails info)

# In[30]:


def create_mail_info_req() -> str:
    passport_current_path = f"//home/cloud_analytics/import/iam/cloud_owners_history"
    main_request = f"""
    SELECT
        billing_account_id,
        first_name,
        last_name,
        account_name,
        email,
        phone,
        timezone,
        person_type,
        paid_consumption_per_last_month
    FROM (
        SELECT
            billing_account_id,
            account_name as account_name,
            first_name as first_name,
            last_name as last_name,
            user_settings_email as email,
            ba_person_type as person_type,
            phone as phone,
            timezone
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as a
        LEFT JOIN (
            SELECT
                max(timezone) as timezone, 
                passport_uid 
            FROM "{passport_current_path}"
            GROUP BY passport_uid
        ) as b
        ON a.puid == b.passport_uid
        WHERE 
            event == 'ba_created'
        AND segment not in ('Enterprise', 'Large ISV', 'ISV ML', 'Medium', 'Yandex Projects')
        ) as a
    LEFT JOIN (
        SELECT
            billing_account_id,
            SUM(if(toDate(event_time) >= addDays(NOW(), -30), real_consumption, 0)) as paid_consumption_per_last_month
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        GROUP BY billing_account_id
    ) as b
    on a.billing_account_id == b.billing_account_id
    FORMAT TabSeparatedWithNames
    """
    return main_request


# In[31]:


def create_mail_info_df() -> pd.DataFrame:
    df = execute_query(create_mail_info_req())

    df['last_active_id'] = df["billing_account_id"].apply(lambda x: builling_to_last_active_billing[x]
                                                          if builling_to_last_active_billing.get(x) is not None
                                                          else x)
    consum_dict = df.groupby('last_active_id')[
        'paid_consumption_per_last_month'].sum()
    df['paid_consumption_per_last_month'] = df['last_active_id'].apply(
        lambda x: consum_dict[x])
    return df


# ## 1.8 Добавить ответ

# In[32]:


def create_evaluation_precision_billings_request(date: str) -> str:
    req = f"""
SELECT
    DISTINCT billing_account_id
FROM (
    SELECT
        billing_account_id,
        groupUniqArray(toDate(event_time)) as visited_days
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
    WHERE sku_lazy = 0
    AND real_consumption > 0
    AND toDate(event_time) >= addDays(toDate('{date}'), -33)
    AND toDate(event_time) < toDate('{date}')
    GROUP BY billing_account_id
)
WHERE length(visited_days) > 3
FORMAT TabSeparatedWithNames
"""
    return req


# In[33]:


def create_target_billings_request(date: str) -> str:
    req = f"""
    SELECT
        DISTINCT billing_account_id
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
    WHERE sku_lazy = 0
    AND real_consumption > 0
    AND toDate(event_time) >= addDays(toDate('{date}'), -33)
    AND toDate(event_time) < toDate('{date}')
    FORMAT TabSeparatedWithNames
    """
    return req


# In[34]:


def create_df_of_billings_which_visits(date: str) -> pd.DataFrame:
    funcs = [lambda x: execute_query(create_target_billings_request(x)),
             lambda x: execute_query(create_evaluation_precision_billings_request(x))]
    data = Parallel(n_jobs=-1)(delayed(func)(date) for func in funcs)
    target_df, evaluation_precision_df = data[0], data[1]
    string_date_to_add_answers = date_to_string(
        parse(date) - timedelta(days=35))
    target_df["date"] = [string_date_to_add_answers] * target_df.shape[0]
    evaluation_precision_df["date"] = [
        string_date_to_add_answers] * evaluation_precision_df.shape[0]
    return target_df, evaluation_precision_df


# ## 1.9 Склеить таблицы

# In[35]:


def create_week_df(date: str) -> pd.DataFrame:
    df_funcs = [create_vm_cube_info_df,
                create_main_infromation_df,
                create_visits_df,
                create_paid_df,
                create_services_cons_last_week_df,
                create_mdb_on_vm_list,
                create_df_of_billings_which_visits]
    dfs = Parallel(n_jobs=-1)(delayed(func)(date) for func in df_funcs)
    vm_cube_df = dfs[0]
    main_info_df = dfs[1]
    visits_df = dfs[2]
    paid_df = dfs[3]
    service_df = dfs[4]
    mdb_on_vm_bids = dfs[5]
    target_df, evaluation_precision_df = dfs[6]

    df = pd.merge(main_info_df, visits_df,
                  on="billing_account_id", how="inner")
    df = pd.merge(df, paid_df, on="billing_account_id", how="inner")
    df = pd.merge(df, service_df, on="billing_account_id", how="inner")
    df = pd.merge(df, vm_cube_df, on="cloud_id", how="left")
    df["cogort"] = df['cons_avg_last_week'].apply(lambda x: get_cogort_type(x))
    df["using_mdb_on_vm"] = (
        df["billing_account_id"].isin(mdb_on_vm_bids)).astype(int)
    df["date"] = [date] * df.shape[0]
    df.replace(np.nan, 0, inplace=True)
    new_col_list = ["date"] + list(df.columns[:-1])
    df = df[new_col_list]
    df["core_fraction"].replace("0", "undefined", inplace=True)
    df["core_fraction"].replace(0, "undefined", inplace=True)
    df = replace_column_in_df(df, "date")
    return df, target_df, evaluation_precision_df


# In[ ]:


# In[36]:


segments = ["compute", "managed_databases", "artificial_intelligence",
            "marketplace", "storage", "network", "other"]
cube_products = ["Ubuntu", "Windows", "CentOS", "Debian"]
databases = ["clickhouse", "pg", "redis", "mongo"]
ais = ["speech", "translate", "vision"]


# ## 1.10  финальная таблица (Для создания первого датасета с фичами)

# In[37]:


def clean_target_column(date, target, answers_max_date):
    if pd.isnull(target):
        if date <= answers_max_date:
            return 1
    return target


# In[38]:


def create_train_table(start_date, end_date):
    def loop_body(date):
        string_date = date_to_string(date)
        return create_week_df(string_date)
    data = Parallel(n_jobs=-1)(delayed(loop_body)(date) for date in date_range(end_date,
                                                                               start_date=start_date,
                                                                               freq="7D"))
    evaluation_array = []
    weeks_arrays = []
    target_array = []
    weeks_arrays = []
    for week_df, target_df, evaluation_precision_df in data:
        evaluation_array.append(evaluation_precision_df)
        weeks_arrays.append(week_df)
        target_array.append(target_df)

    person_df = concatenate_tables(weeks_arrays)
    target_df = concatenate_tables(target_array)
    evaluation_precision_df = concatenate_tables(evaluation_array)
    target_df["target"] = [0] * target_df.shape[0]
    evaluation_precision_df["evaluate"] = [
        0] * evaluation_precision_df.shape[0]
    res_df = pd.merge(person_df.replace(np.nan, 0), target_df, on=[
                      "date", "billing_account_id"], how="left")
    res_df = pd.merge(res_df, evaluation_precision_df, on=[
                      "date", "billing_account_id"], how="left")
    res_df["core_fraction"].replace("0", "undefined", inplace=True)
    res_df["core_fraction"].replace(0, "undefined", inplace=True)
    max_date = max(res_df["date"])
    answers_max_date = date_to_string(parse(max_date) - timedelta(days=35))
    res_df["target"] = res_df[["date", "target"]].apply(lambda row:
                                                        clean_target_column(row["date"],
                                                                            row["target"],
                                                                            answers_max_date), axis=1)
    res_df["precision_evaluate"] = res_df[["date", "evaluate"]].apply(lambda row:
                                                                      clean_target_column(row["date"],
                                                                                          row["evaluate"],
                                                                                          answers_max_date), axis=1)
    res_df.drop(columns=["evaluate"], inplace=True)
    res_df = replace_column_in_df(res_df, "date")
    return res_df


# In[39]:


# start_date = '2020-01-06'
# end_date = '2020-06-01'


# In[40]:


# %%time
# res_df = create_train_table(start_date, end_date)


# In[41]:


# temp_path = "//home/cloud_analytics/churn_prediction"
# file_name = "person_info"
# save_table(file_name, temp_path, res_df)


# In[42]:


# res_df["precision_evaluate"].sum()


# # 2. Создание скрипта обновления

# ## 2.0 Перекопирование всех файлов внутри

# In[43]:


def copy_all(from_path, to_path):
    assert ("churn_prediction" in from_path and "churn_prediction_copy/churn_prediction" in to_path) or (
        "churn_prediction" in to_path and "churn_prediction_copy/churn_prediction" in from_path)

    yt.copy(from_path, to_path, force=True)


# In[44]:


def read_person_df_without_types(path):
    curr_path = path + "/person_info"
    req = f"""
    SELECT
        *
    FROM "{curr_path}"
    ORDER BY date
    FORMAT TabSeparatedWithNames
    """
    df = execute_query(req)
    return df


# In[45]:


def update_answers(date, bid, target, last_answers_date, visiting_billings):
    if date == last_answers_date:
        if bid in visiting_billings:
            return 0
        return 1
    if not pd.isnull(target):
        return target
    else:
        return np.nan


# In[46]:


def update_table_with_new_week(from_path, to_path):
    copy_all(from_path, to_path)
    df = read_person_df_without_types(from_path)
    max_date = df["date"].max()
    next_date = date_to_string(parse(max_date) + timedelta(7))
    week_df, target_df, evaluation_precision_df = create_week_df(next_date)
    last_answers_date = target_df["date"].iloc[0]
    target_billings = set(target_df["billing_account_id"])
    evaluation_precision_billings = set(
        evaluation_precision_df["billing_account_id"])
    for column in evaluate_columns:
        if column == "precision_evaluate":
            billings = evaluation_precision_billings
        else:
            billings = target_billings
        df[column] = df[["date", "billing_account_id", column]].apply(lambda row:
                                                                      update_answers(row["date"],
                                                                                     row["billing_account_id"],
                                                                                     row[column],
                                                                                     last_answers_date, billings), axis=1)
        week_df[column] = [np.nan] * week_df.shape[0]
    df = concatenate_tables([df, week_df])
    return df, last_answers_date


# In[47]:


def update_table_with_new_week(from_path, to_path):
    copy_all(from_path, to_path)
    df = read_person_df_without_types(from_path)
    max_date = df["date"].max()
#     print(max_date)
    next_date = date_to_string(parse(max_date) + timedelta(7))
    print(next_date)
    assert parse(next_date) <= parse(get_current_date_as_str())
    week_df, target_df, evaluation_precision_df = create_week_df(next_date)
    last_answers_date = target_df["date"].iloc[0]
    target_billings = set(target_df["billing_account_id"])
    evaluation_precision_billings = set(
        evaluation_precision_df["billing_account_id"])
    for column in evaluate_columns:
        if column == "precision_evaluate":
            billings = evaluation_precision_billings
        else:
            billings = target_billings
        df[column] = df[["date", "billing_account_id", column]].apply(lambda row:
                                                                      update_answers(row["date"],
                                                                                     row["billing_account_id"],
                                                                                     row[column],
                                                                                     last_answers_date, billings), axis=1)
        week_df[column] = [np.nan] * week_df.shape[0]
    df = concatenate_tables([df, week_df])
    return df, last_answers_date


# In[48]:


# df, last_answers_date = update_table_with_new_week(from_path, to_path)


# In[49]:


# meta_results_table = create_model_validate_and_predict(df, current_date, last_answers_date, from_path,
#                                                        append=append)
# create_answer_df_and_update_already_in_experiment_users(meta_results_table,
#                                                         current_date,
#                                                         from_path,
#                                                         append=append)

# file_name = "person_info"
# save_table(file_name, from_path, df)
# final_week_evaluation(df, last_answers_date, from_path, append=append)


# In[ ]:


# ## 2.1 Очистка (ДЛЯ ДЕБАГА)

# In[50]:


def full_request(path):
    req = f"""
SELECT
    *
FROM "{path}"
ORDER BY date
FORMAT TabSeparatedWithNames
"""
    full_df = execute_query(req)
    return full_df


# In[51]:


def clear_from_last_date(path):
    df = read_person_df_without_types(path)
    date = df["date"].max()
    print(date)
    df = df[df["date"] != date]
    val_date = date_to_string(parse(date) - timedelta(35))
    print(val_date)
    for index in df[df["date"] == val_date]["target"].index:
        df["target"].loc[index] = np.nan
    print(df[df["date"] == val_date]["target"].unique())
    file_name = "person_info"
    save_table(file_name, path, df)
    val_table = full_request(path + "/validation_table")
    val_table = val_table[val_table["date"] != val_date]
    save_table("validation_table", path, val_table)

    eval_table = full_request(path + "/evaluation_table")
    eval_table = eval_table[eval_table["date"] != val_date]
    save_table("evaluation_table", path, eval_table)

    persons_in_experiment = full_request(
        path + "/persons_in_experiment_already")
    persons_in_experiment = persons_in_experiment[persons_in_experiment["date"] != date]
    save_table("persons_in_experiment_already", path, persons_in_experiment)


# In[52]:


# from_path = "//home/cloud_analytics/churn_prediction"
# to_path = "//home/cloud_analytics/lunin-dv/churn_prediction_copy/churn_prediction_xgboost"


# In[53]:


# copy_all(to_path, from_path)


# In[54]:


# clear_from_last_date(from_path)


# # 3. ML

# ## 3.0 Preprocessing functions

# In[55]:


class DummyTransformer(BaseEstimator, TransformerMixin):
    def __init__(self):
        self.changer = {}

    def __create_dummies(df, cat_column, *, changer=None):
        dum = pd.get_dummies(df[cat_column])
        dum.columns = [cat_column + "_" + val for val in dum.columns]
        if "undefined" in df[cat_column].unique():
            dum.drop(columns=[cat_column + "_undefined"], inplace=True)
        if "Undefined" in df[cat_column].unique():
            dum.drop(columns=[cat_column + "_undefined"], inplace=True)
        df = pd.concat([df, dum], axis=1)

        if changer is None:
            changer = df.groupby(cat_column)["y"].mean()
            changer = changer.to_dict()

        df[cat_column] = df[cat_column].replace(changer)
        return df, changer

    def fit(self, X, y):
        df = X.copy()
        df["y"] = y
        for cat_column in X.columns:
            if X[cat_column].dtype == object or X[cat_column].dtype == str:
                _, self.changer[cat_column] = DummyTransformer.__create_dummies(
                    df, cat_column, changer=None)

        return self

    def transform(self, X_real):
        X = X_real.copy()
        for cat_column in self.changer:
            X, _ = DummyTransformer.__create_dummies(
                X, cat_column, changer=self.changer[cat_column])
        return X


# ## 3.1 train часть

# ## 3.3 Evaluate model

# In[56]:


def prepare_results_for_meta_id_using_raw_billing_answers(answer_column_name, date, df_raw):
    df = df_raw.copy()
    df["meta_id"] = df["billing_account_id"].apply(lambda x: builling_to_last_active_billing[x]
                                                   if builling_to_last_active_billing.get(x) is not None
                                                   else x)
    curr_meta_df = df[df["date"] == date][["meta_id", answer_column_name]]
    y = curr_meta_df[answer_column_name]
    curr_meta_df[answer_column_name] = 1 - np.array(y)

    meta_df = pd.DataFrame(curr_meta_df.groupby(
        "meta_id")[answer_column_name].sum())
    meta_df.columns = [answer_column_name]
    meta_df.columns = [answer_column_name]
    meta_df[answer_column_name] = meta_df[answer_column_name].apply(
        lambda x: 0 if x > 0 else 1)
    meta_df["meta_id"] = meta_df.index
    meta_df.index = np.arange(0, meta_df.shape[0])
    meta_df["billing_account_id"] = meta_df["meta_id"]
    meta_df.drop(columns=["meta_id"], inplace=True)
    meta_df["date"] = [date] * meta_df.shape[0]
    return meta_df


# In[57]:


def get_associated_predictions_and_answers(prediction_df_raw, df):
    prediction_df = prediction_df_raw.copy()
    answers_df = df.copy()
    date = prediction_df["date"].max()
    prediction_df = prepare_results_for_meta_id_using_raw_billing_answers("prediction", date,
                                                                          prediction_df)
    prediction = np.array(prediction_df["prediction"])
    answers_df = prepare_results_for_meta_id_using_raw_billing_answers(
        "precision_evaluate", date, answers_df)
    resulted_df = pd.merge(prediction_df, answers_df,
                           on="billing_account_id", how="inner")
    precision = np.array(resulted_df["precision_evaluate"])

    answers_df = df.copy()
    answers_df = prepare_results_for_meta_id_using_raw_billing_answers(
        "target", date, answers_df)
    resulted_df = pd.merge(prediction_df, answers_df,
                           on="billing_account_id", how="inner")
    recall = np.array(resulted_df["target"])

    return resulted_df['prediction'], precision, recall


# In[58]:


def evaluate_model(df, date, path, file_name=None, append=True, prediction_df=None,
                   X_val=None, y_val_precision=None, y_val_recall=None,  model=None):
    answers = []
    if "validation" in file_name:
        y_pred = model.predict(X_val)
        answers.append(("billing_account_id", y_pred,
                        y_val_precision, y_val_recall))
        y_pred = np.array(y_pred)
        billings = np.array(df[df["date"] == date]["billing_account_id"])

        prediction_df = pd.DataFrame.from_dict({"date": [date] * y_pred.shape[0],
                                                "billing_account_id": billings,
                                                "prediction": y_pred})
    y_meta_predictions, y_meta_precision, y_meta_recall = get_associated_predictions_and_answers(
        prediction_df, df)
    answers.append(("meta_id", y_meta_predictions,
                    y_meta_precision, y_meta_recall))
    print("date:", date)
    for elem in answers:
        curr_id_type, prediction_y, precision_y, recall_and_fscore_y = elem
        print(f'curr_id_type: {curr_id_type}')
        precision = precision_score(precision_y, prediction_y)
        recall = recall_score(recall_and_fscore_y, prediction_y)
        fscore = fbeta_score(recall_and_fscore_y, prediction_y, beta=0.5)
        print(f"id type: {curr_id_type}; fbeta(0.5): {round(fscore, 3)};"
              f" precision: {round(precision, 3)}; recall: {round(recall, 3)};")
        curr_dict = {"date": date,
                     "id type": curr_id_type,
                     "amount of user churn": int(np.sum(recall_and_fscore_y)),
                     "number of predictions": int(np.sum(prediction_y)),
                     "precision": [precision],
                     "recall": [recall],
                     "fbeta(0.5)": [fscore]}
        res_df = pd.DataFrame.from_dict(curr_dict)
        if file_name is not None:
            save_table(file_name, path, res_df, append=append)
        append = True


# ## 3.3 финальная функция

# In[59]:


append = False


# In[60]:


def create_model_validate_and_predict(df, date, last_answers_date, path, append=True):
    train_table = df[df["date"] < last_answers_date]
    validation_table = df[df["date"] == last_answers_date]
    test_table = df[df["date"] == date]
    boost_pipeline = Pipeline([('dummy_transformer', DummyTransformer()),
                               ('StandartScaler', MinMaxScaler()),
                               ('CatBoost', CatBoostClassifier(verbose=False, random_seed=8))])
    columns = ["date", "billing_account_id", "cloud_id"]
    X = train_table.drop(columns=evaluate_columns + columns)
    y = train_table["target"]
    X_val = validation_table.drop(columns=evaluate_columns + columns)
    y_recall_val = validation_table["target"]
    y_precision_val = validation_table["precision_evaluate"]
    test_X = test_table.drop(columns=evaluate_columns + columns)

    #xgboost_pipeline, params = create_best_xgboost_model_with_grid_search(xgboost_pipeline, X, y)
    boost_pipeline.fit(X, y)

    # validation
    print("VALIDATION")
    evaluate_model(df, last_answers_date, path, file_name="validation_table",
                   append=append, X_val=X_val,
                   y_val_precision=y_precision_val,
                   y_val_recall=y_recall_val,
                   model=boost_pipeline)
    X = df[df["date"] <= last_answers_date].drop(
        columns=evaluate_columns + columns)
    y = df[df["date"] <= last_answers_date]["target"]

    boost_pipeline.fit(X, y)
    y_pred = boost_pipeline.predict(test_X)
    billings = np.array(test_table["billing_account_id"])
    prediction_df = pd.DataFrame.from_dict({"date": [date] * y_pred.shape[0],
                                            "billing_account_id": billings,
                                            "prediction": y_pred})
    meta_results_table = prepare_results_for_meta_id_using_raw_billing_answers(
        "prediction", date, prediction_df)
    return meta_results_table


# In[61]:


# meta_results_table = create_model_validate_and_predict(df, current_date, last_answers_date, from_path,
#                                                        append=append)


# # 4 Сохранение результатов

# In[62]:


def read_already_in_experiment_billings(path):
    curr_path = path + "/persons_in_experiment_already"
    req = f"""
    SELECT
        *
    FROM "{curr_path}"
    FORMAT TabSeparatedWithNames
    """
    bad_persons = execute_query(req)
    return bad_persons


# In[63]:


def read_from_upsell():
    min_datetime = parse(get_current_date_as_str()) - timedelta(days=70)
    path = "//home/cloud_analytics/export/crm/upsale"
    dfs = []
    tables = []

    for table in find_tables_in_hahn_folder(path):
        dt = parse(table.split('/')[-1])
        if dt > min_datetime:
            tables.append(table)

    def req(table):
        req = f"""
        SELECT
            billing_account_id
        FROM "{table}"
        FORMAT TabSeparatedWithNames
        """
        return execute_query(req)

    data = Parallel(n_jobs=-1)(delayed(req)(table) for table in tables)
    upsell_df = concatenate_tables(data)
    return upsell_df


# In[64]:


# upsell = read_from_upsell()


# In[65]:


# upsell.shape


# In[66]:


def create_answer_df_and_update_already_in_experiment_users(meta_results_table,
                                                            current_date,
                                                            path,
                                                            append=True):
    mail_df = create_mail_info_df()
    meta_results_table = meta_results_table[[
        "date", "billing_account_id", "prediction"]]
    meta_results_table["associated_billings"] = meta_results_table["billing_account_id"].apply(
        lambda x: user.get_all_accosiated_billings(x))

    meta_results_table = pd.merge(meta_results_table, mail_df, how="inner")

    testing_billings = meta_results_table[meta_results_table["prediction"] == 1][[
        "date", "billing_account_id"]]
    file_name = "persons_in_experiment_already"
    save_table(file_name, path, testing_billings, append=append)
    bad_persons = read_already_in_experiment_billings(path)

    print("Upsell removed")
    upsell_df = read_from_upsell()
    upsell_df["date"] = [current_date] * upsell_df.shape[0]
    bad_persons = concatenate_tables([bad_persons, upsell_df])

    bad_persons = bad_persons[(bad_persons["date"] >= experiment_start_date) &
                              (bad_persons["date"] < current_date)]

    print("number of users before removing users in experiment already:",
          meta_results_table.shape[0])
    meta_results_table = meta_results_table[~meta_results_table["billing_account_id"].isin(
        bad_persons["billing_account_id"])]
    print("number of users:", meta_results_table.shape[0])
    # ab testing
    interest_group = meta_results_table[meta_results_table['prediction'] == 1].sort_values(
        by='paid_consumption_per_last_month', ascending=False)

    # 30 for calls -> 60 as control/test group
    indexes = list(interest_group.head(60).index)
    random.shuffle(indexes)
    test_indexes = indexes[::2]
    control_indexes = indexes[1::2]
    ####################################
    meta_results_table = add_AB_testing_group(meta_results_table, {"test": test_indexes,
                                                                   "control": control_indexes})
    file_name = current_date
    save_table(file_name, path + "/week_logs", meta_results_table, schema={"associated_billings":
                                                                           "list:string"})
    test_group = meta_results_table[meta_results_table["Group"] == 'test']
    save_table(file_name, path + "/churn_prediction_test_group",
               test_group, schema={"associated_billings": "list:string"})

    predicted_group = meta_results_table[meta_results_table['Group'] != '']
    save_table(file_name, path + "/full_prediction_group",
               predicted_group, schema={"associated_billings": "list:string"})


# # 5. Оценка алгоритма

# In[67]:


def take_info_from_week_log(path, date):
    curr_path = path + '/week_logs/' + date
    req = f"""
    SELECT
        date,
        billing_account_id,
        prediction
    FROM "{curr_path}"
    FORMAT TabSeparatedWithNames
    """
    evaluate_df = execute_query(req)
    return evaluate_df


# In[68]:


def final_week_evaluation(df, last_answers_date, path, append=True):
    if yt.exists(path + '/week_logs/' + last_answers_date):
        evaluate_df = take_info_from_week_log(path, last_answers_date)
        print("TEST EVALUATION")
        evaluate_model(df, last_answers_date, path, file_name="evaluation_table",
                       append=append, prediction_df=evaluate_df)


# # 6. Сборочка

# In[69]:


def create_full_final_script_function(from_path, to_path, append=True):
    try:
        df, last_answers_date = update_table_with_new_week(from_path, to_path)
        current_date = df["date"].max()
        meta_results_table = create_model_validate_and_predict(df, current_date, last_answers_date, from_path,
                                                               append=append)
        create_answer_df_and_update_already_in_experiment_users(meta_results_table,
                                                                current_date,
                                                                from_path,
                                                                append=append)

        file_name = "person_info"
        save_table(file_name, from_path, df)
        final_week_evaluation(df, last_answers_date, from_path, append=append)
    except BaseException:
        # revert
        print("Ooops, Exception!")
        copy_all(to_path, from_path)
        raise


# In[70]:


evaluate_columns = ["target", "precision_evaluate"]


# In[71]:


from_path = "//home/cloud_analytics/churn_prediction"
to_path = "//home/cloud_analytics/lunin-dv/churn_prediction_copy/churn_prediction"


# In[72]:


create_full_final_script_function(from_path, to_path, append=True)
