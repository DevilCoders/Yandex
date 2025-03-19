#!/usr/bin/env python3
import numpy as np
import scipy as sp
import pandas as pd
from dateutil.parser import *
import requests
from datetime import datetime
import re
from io import StringIO
import yt.wrapper as yt
import json
from datetime import datetime, timedelta
import operator
import requests, sys, pymysql, time
from library.python.vault_client.instances import Production as VaultClient


try:
    client = VaultClient(decode_files=True)
    secret_uuid = "sec-01e2dyrwvwrmnk1r2q7rnyejcy"
    tokens = client.get_version(secret_uuid)
    clickhouse_token = tokens["value"]["clickhouse_token"]
    metrika_password = tokens["value"]["metrika"]
    crm_password = tokens["value"]["crm"]
except Exception:
    clickhouse_token = sys.argv[1]
    metrika_password = sys.argv[2]
    crm_password = sys.argv[3]


def read_password_from_file(filename):
    """
    Read token from file
    :param filename: path to file with token
    :return: token
    """
    with open(filename) as f:
        token = f.readlines()
    return token[0].split()[0]


yt.config["proxy"]["url"] = "hahn"
yt.config["token"] = clickhouse_token


def raw_execute_query(query, cluster, alias, token, timeout=600):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}&enable_optimize_predicate_expression=0".format(proxy=proxy,
                                                                                            alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.text.strip().split('\n')
    return rows


def raw_chyt_execute_query(query, cluster, alias, token, columns = None):
    i = 0
    while True:
        try:
            result = raw_execute_query(query=query,
                                       cluster=cluster,
                                       alias=alias, token=token)
            if columns is None:
                users = pd.DataFrame([row.split('\t') for row in result[1:]], columns = result[0].split('\t'))
            else:
                users = pd.DataFrame([row.split('\t') for row in result], columns=columns)
            return users
        except Exception as err:
            print(err)
            i += 1
            if i > 5:
                print('Break Excecution')
                break


cluster = "hahn"
alias = "*cloud_analytics"


def update_automatically_types(df_raw):
    df = df_raw.copy()
    for column in df.columns:
        if ("id" in column and "paid" not in column) or \
        (len(df[column].replace(np.nan, "").max()) > 9 and
                              ("." not in df[column].replace(np.nan, "").max())):
            continue
        try:
            df[column] = df[column].astype(int)
            continue
        except Exception:
            pass
        try:
            df[column] = df[column].astype(float)
            continue
        except Exception:
            pass
        try:
            df[column] = df[column].apply(lambda x: json.loads(x))
            continue
        except Exception:
            pass
        try:
            df[column] = df[column].apply(lambda x: json.loads(x.replace("'", '"')))
            continue
        except Exception:
            pass
    return df


def execute_query(query, columns = None):
    """Execute query, returns pandas dataframe with result
    :param query: query to execute on cluster
    :param columns: name of dataframe columns
    :return: pandas dataframe, the result of query
    """
    df = raw_chyt_execute_query(query, cluster, alias, clickhouse_token, columns)
    df = df.replace('\\N', np.NaN)
    df = update_automatically_types(df)
    return df

USER = 'lunin-dv'
PASS = metrika_password

HOST = 'http://clickhouse.metrika.yandex.net:8123/'


def raw_get_metrika_data(query, host=HOST, connection_timeout = 1500, **kwargs):
    """Execute raw query to mtmega
    """
    params = kwargs

    r = requests.post(host, params = params, auth=(USER, PASS), timeout = connection_timeout, data=query)
    if r.status_code == 200:
        return r.text
    else:
        raise ValueError(r.text)


def get_metrika_df(query, host=HOST, connection_timeout = 1500):
    """Get df from mtmega"""
    data = raw_get_metrika_data(query, host, connection_timeout)
    df = pd.read_csv(StringIO(data), sep = '\t', low_memory=False)
    return df

def concatenate_tables(df_array):
    """Concatenates array of dataframes with possible Nan inside array
    :param df_array: array of dataframes
    :return: Nane or dataframe
    """

    df_array = [df for df in df_array if df is not None]
    if len(df_array) == 0:
        return None
    res_df = pd.concat(df_array, axis = 0)
    res_df.index = np.arange(0, res_df.shape[0], 1)
    return res_df

def concat_names(first_name, last_name):
    if pd.isnull(first_name):
        first_name = "_"
    if pd.isnull(last_name):
        last_name = "_"
    return last_name + ", " + first_name

def string_answer_to_boolean(string):
    """Finds good words like "yes" in string"""
    try:
        return not pd.isnull(string) and ("Да" in string or "да" in string or "Yes" in string or "yes" in string)
    except Exception:
        print(string)
        return False

def from_3_constants_to_date(year, month, day):
    return datetime.strftime(parse(f"{year}-{month}-{day}"), "%Y-%m-%d")


def get_number_of_days_in_month(year, month):
    return monthrange(year, month)[1]

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
            return mail_.lower()

def apply_type(raw_schema, df):
    if raw_schema is not None:
        for key in raw_schema:
            if raw_schema[key] != 'list':
                df[key] = df[key].astype(raw_schema[key])
    schema = []
    for col in df.columns:
        if df[col].dtype == int:
            schema.append({"name":col, 'type':'int64'})
        elif df[col].dtype == float:
            schema.append({"name":col, 'type':'double'})
        elif raw_schema is not None and raw_schema.get(col) is not None and raw_schema[col] == 'list':
            schema.append({"name": col, 'type_v3':
                {"type_name": 'list', "item": {"type_name":"optional", "item":"string"}}})
        else:
            schema.append({"name":col, 'type':'string'})
    return schema

def save_table(file_to_write, path, table, schema = None, append=False):
    df = table.copy()
    real_schema = apply_type(schema, df)
    json_df_str = df.to_json(orient='records')
    path = path + file_to_write
    json_df = json.loads(json_df_str)
    if not yt.exists(path) or not append:
        yt.create(type="table", path=path, force=True,
              attributes={"schema": real_schema})
    tablepath = yt.TablePath(path, append=append)
    yt.write_table(tablepath, json_df,
               format = yt.JsonFormat(attributes={"encode_utf8": False}))

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
"""============================================================================================================"""

### Обработка dimensions ###

query = '''
    SELECT
        ba_id as billing_account_id,
        dimen_name as dimension,
        user_name,
        date
    FROM(SELECT
            d.name AS dimen_name,
            act.parent_type as type,
            act.date_entered as date,
            ba.name AS ba_id,
            u.user_name
        FROM
            cloud8.dimensions AS d
        INNER JOIN cloud8.dimensions_bean_rel AS bean
            ON d.id = bean.dimension_id
        LEFT JOIN cloud8.leads AS l
            ON bean.bean_id = l.id
        LEFT JOIN cloud8.leads_billing_accounts AS l_bean
            ON l_bean.leads_id = l.id
        LEFT JOIN cloud8.billingaccounts AS ba
            ON l_bean.billingaccounts_id = ba.id
        LEFT JOIN cloud8.activities as act
            ON act.parent_id = l.id
        LEFT JOIN cloud8.users AS u
            ON l.assigned_user_id = u.id
        WHERE
            bean.bean_module = 'Leads'
        AND l.deleted = 0
        AND ba.name IS NOT NULL
        AND act.activity_type = 'link'
        AND act.parent_type = 'Leads'
        AND act.data like '%dimensions%') as t0
    GROUP BY
    ba_id, dimen_name, date
    ORDER BY date
    '''
cnx = pymysql.connect(
    user='cloud8',
    password=crm_password,
    host='c-mdb8t5pqa6cptk82ukmc.rw.db.yandex.net',
    port = 3306,
    database='cloud8'
)
dim_df = pd.read_sql_query(query, cnx)
cnx.close()

# 1.1 Найдем buisness billings
bids = set(dim_df[dim_df["dimension"] == "Business"]["billing_account_id"])
business_df = dim_df[dim_df["billing_account_id"].isin(bids)]
business_df = business_df.sort_values(by = ["billing_account_id", "date"])

# 1.2 Уберем тех, у кого был поменен dimensions последние 3 месяца
business_df["less_four_month_before"] = business_df["date"].apply(lambda x: int((datetime.now() - x).days / 30) < 3)
no_update_last_3_month = business_df[~business_df["less_four_month_before"]]

# 1.3 Найдем тех, у кого последний dimensions был "Сроки реализации не определены (>3 месяцев)"
last_dimension_not_active = no_update_last_3_month.groupby("billing_account_id").tail(1)
interest_billings = set(last_dimension_not_active[
                            last_dimension_not_active["dimension"] == \
                            "Сроки реализации не определены (>3 месяцев)"]["billing_account_id"])

# 1.4 Создадим финальную таблицу с нужными billings и необходимой из crm информацией
final_crm_info_df = business_df[business_df["billing_account_id"].isin(interest_billings)]
final_crm_info_df = pd.DataFrame(final_crm_info_df.groupby("billing_account_id")[["date", "user_name"]].max())
final_crm_info_df["billing_account_id"] = final_crm_info_df.index
final_crm_info_df.index = np.arange(0, final_crm_info_df.shape[0])
final_crm_info_df = final_crm_info_df[["billing_account_id", "date", "user_name"]]
final_crm_info_df.columns = ["billing_account_id", "last_dimension_update_date", "sales_name"]
final_crm_info_df = final_crm_info_df[final_crm_info_df["last_dimension_update_date"].apply(lambda x: int((datetime.now() - x).days / 30) == 3)]


### Информация из hahn ###
req = """
SELECT
    billing_account_id
FROM (
    SELECT
        DISTINCT billing_account_id,
        SUM(if(toDate(event_time) >= addDays(toDate(NOW()), -30), real_consumption, 0)) as last_month_consumption
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
    WHERE sku_lazy == 0
    GROUP BY billing_account_id
    HAVING last_month_consumption == 0
)
INNER JOIN (
    SELECT
        billing_account_id,
        1 - hasAll(['unreachible', 'unreachible,unreachible'], groupUniqArray(call_status)) as target
    FROM "//home/cloud_analytics_test/cubes/crm_leads/cube"
    WHERE 
        event == 'call'
    AND
        isNotNull(billing_account_id)
    GROUP BY billing_account_id
    HAVING target == 1
)
USING billing_account_id
FORMAT TabSeparatedWithNames
"""

df = execute_query(req)
result = pd.merge(df, final_crm_info_df, on = "billing_account_id", how = "inner")

### Mail information for billings###
def create_mail_info_req() -> str:
    tables = find_tables_in_hahn_folder("//home/cloud_analytics/import/iam/cloud_owners/1h")
    passport_current_path = f"{sorted(tables)[-2]}"
    main_request = f"""
    SELECT
        billing_account_id,
        first_name,
        last_name,
        full_company_name,
        email,
        phone,
        timezone,
        person_type,
        paid_consumption_per_last_month
    FROM (
        SELECT
            billing_account_id,
            multiIf(account_name IS NOT NULL AND account_name != 'unknown',account_name,
                   balance_name IS NOT NULL AND balance_name != 'unknown',balance_name,
                   promocode_client_name IS NOT NULL AND promocode_client_name !='unknown',     
                   promocode_client_name, '-'
                  ) as full_company_name,
            first_name as first_name,
            last_name as last_name,
            user_settings_email as email,
            ba_person_type as person_type,
            phone as phone,
            timezone
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        LEFT JOIN (
            SELECT
                max(timezone) as timezone, 
                passport_uid 
            FROM "{passport_current_path}"
            GROUP BY passport_uid
        )
        ON puid == passport_uid
        WHERE 
            event == 'ba_created'
        AND segment not in ('Enterprise', 'Large ISV', 'ISV ML', 'Medium', 'Yandex Projects')
        )
    LEFT JOIN (
        SELECT
            billing_account_id,
            SUM(if(toDate(event_time) >= addDays(NOW(), -30), real_consumption, 0)) as paid_consumption_per_last_month
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE sku_lazy == 0
        GROUP BY billing_account_id
    )
    using billing_account_id
    FORMAT TabSeparatedWithNames
    """
    return main_request


mail_df = execute_query(create_mail_info_req())
mail_df["email"] = mail_df["email"].apply(lambda x: works_with_emails(x))


result = pd.merge(result, mail_df, on = "billing_account_id", how = "inner")
result["last_dimension_update_date"] = result["last_dimension_update_date"].astype(str)

path = "//home/cloud_analytics/lunin-dv/crm_with_last_non_active_dimension/"
file_to_write = get_current_date_as_str()
save_table(file_to_write, path, result)
