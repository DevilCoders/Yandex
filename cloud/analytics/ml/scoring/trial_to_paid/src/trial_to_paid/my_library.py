import numpy as np
import scipy as sp
from sklearn.model_selection import train_test_split
import gc
import pandas as pd
import typing as tp
from dateutil.parser import *
from transliterate import translit

"""pip install transliterate"""
import requests
from datetime import datetime
import Levenshtein
from collections import defaultdict
from datetime import timezone

"""pip install python-Levenshtein"""
import re
from io import StringIO
from calendar import monthrange
import yt.wrapper as yt
import json
from datetime import datetime, timedelta
import operator
import requests, os, sys, pymysql, time
from statsmodels.stats import multitest
from collections import namedtuple
import scipy.stats as sps
import yt.transfer_manager.client as tm

try:
    from vault_client.instances import Production as VaultClient

    client = VaultClient(decode_files=True)
    secret_uuid = "sec-01e2dyrwvwrmnk1r2q7rnyejcy"
    tokens = client.get_version(secret_uuid)
    yt_token = tokens["value"]["clickhouse_token"]
    metrika_password = tokens["value"]["metrika"]
    crm_password = tokens["value"]["crm"]
    startrek_token = tokens["value"]["startrek"]
    clickhouse_token = tokens["value"]["grafana_token"]
    mdb_auth_token = tokens["value"]["mdb_auth"]
    user_name = 'lunindv'
    user_metrika_name = 'lunin-dv'
except Exception:
    yt_token = sys.argv[1]
    metrika_password = sys.argv[2]
    crm_password = sys.argv[3]
    startrek_token = sys.argv[4]
    clickhouse_token = sys.argv[5]
    mdb_auth_token = sys.argv[6]
    user_name = sys.argv[7]
    user_metrika_name = sys.argv[8]


yt.config["proxy"]["url"] = "hahn"
yt.config["token"] = yt_token
cluster = "hahn"
alias = "*cloud_analytics"


def raw_execute_yt_query(query, timeout=600):
    token = yt_token
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}&enable_optimize_predicate_expression=0".format(proxy=proxy,
                                                                                                          alias=alias,
                                                                                                          token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.text.strip().split('\n')
    return rows


def raw_grafana_execute_query(query):
    url = 'https://{host}:8443/?database={db}&query={query}'.format(
        host='vla-2z4ktcci90kq2bu2.db.yandex.net',
        db='cloud_analytics',
        query=query)
    auth = {
        'X-ClickHouse-User': user_name,
        'X-ClickHouse-Key': clickhouse_token,
    }
    res = requests.get(
        url,
        headers=auth,
        verify='/usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt')
    res.raise_for_status()
    rows = res.text.strip().split('\n')
    return rows


def raw_chyt_execute_any_query(query, request_func, columns=None):
    i = 0
    while True:
        try:
            result = request_func(query=query)
            # print(result)
            if columns is None:
                users = pd.DataFrame([row.split('\t') for row in result[1:]], columns=result[0].split('\t'))
            else:
                users = pd.DataFrame([row.split('\t') for row in result], columns=columns)
            return users
        except Exception as err:
            # print(err)
            i += 1
            if i > 5:
                print('Break Excecution')
                break


def update_automatically_types(df_raw: pd.DataFrame) -> pd.DataFrame:
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
                                          else _raise())
            continue
        except Exception:
            pass
        try:
            df[column] = df[column].apply(lambda x:
                                          json.loads(x.replace("'", '"'))
                                          if isinstance(
                                              json.loads(x.replace("'", '"')),
                                              list)
                                          else _raise()
                                          )
            continue
        except Exception:
            pass
        # id checker
        try:
            if ("id" in column and "paid" not in column) \
                    and not pd.isnull(df[column].astype(int).sum()):
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



def execute_query_body(query, request_func, columns=None):
    df = raw_chyt_execute_any_query(query, request_func, columns)
    df = df.replace('\\N', np.NaN)
    df = update_automatically_types(df)
    if "email" in df.columns:
        df["email"] = df["email"].apply(lambda x: works_with_emails(x))
    return df


def execute_query(query, columns=None):
    """Execute query, returns pandas dataframe with result
    :param query: query to execute on cluster
    :param columns: name of dataframe columns
    :return: pandas dataframe, the result of query
    """
    return execute_query_body(query, raw_execute_yt_query, columns)


def grafana_execute_query(query, columns=None):
    """Execute query, returns pandas dataframe with result
    :param query: query to execute on cluster
    :param columns: name of dataframe columns
    :return: pandas dataframe, the result of query
    """
    return execute_query_body(query, raw_grafana_execute_query, columns)


USER = user_metrika_name
PASS = metrika_password

HOST = 'http://clickhouse.metrika.yandex.net:8123/'


def raw_get_metrika_data(query, host=HOST, connection_timeout=1500, **kwargs):
    """Execute raw query to mtmega
    """
    params = kwargs

    r = requests.post(host, params=params, auth=(USER, PASS), timeout=connection_timeout, data=query)
    if r.status_code == 200:
        return r.text
    else:
        raise ValueError(r.text)


def get_metrika_df(query, host=HOST, connection_timeout=1500):
    """Get df from mtmega"""
    data = raw_get_metrika_data(query, host, connection_timeout)
    df = pd.read_csv(StringIO(data), sep='\t', low_memory=False)
    df = update_automatically_types(df)
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


def works_with_phones(phone_):
    """phone processing
    :param phone_: phone (string or int)
    :return: processed phone as string (with only digits)
    """
    if not pd.isnull(phone_):
        phone_ = str(phone_)
        if phone_[0] == '8':
            phone_ = '7' + phone_[1:]
        return ''.join(c for c in phone_ if c.isdigit())
    return phone_


def works_with_names(name_):
    """name processing
    :return: processed name in lower letters in english literation
    """
    if not pd.isnull(name_):
        name_ = str(name_)
        return translit(str.lower(name_), 'ru', reversed=True)
    return name_


dict_of_functions = {"email": works_with_emails,
                     "phone": works_with_phones,
                     "first_name": works_with_names,
                     "last_name": works_with_names,
                     "name": works_with_names}


def df_to_standard(df_raw, columns_to_change):
    """Converts tables to one format, changes columns in columns_to_change according to dict_of_functions rules
    :param df_raw: dataframe
    :param columns_to_change: dictionary, with mathing names
    (like {'email' (new column name): "Email" (column name in raw dataframe)})
    :return: converted dataframe
    """
    df = df_raw.copy()
    df.index = np.arange(0, df.shape[0], 1)
    rename_dict = {}
    for key in columns_to_change:
        rename_dict[columns_to_change[key]] = key
    df = df.rename(columns=rename_dict)
    for key in columns_to_change:
        if key in dict_of_functions:
            df[key] = \
                df[key].apply(lambda x: dict_of_functions[key](x))
    return df


def apply_schema_to_dataframe(df_raw, schema):
    df = df_raw.copy()
    for col in df.columns:
        df[col] = df[col].astype(schema[col])
    return df


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


def calc_similarity(row_1, row_2):
    """Raw function for union_tables_by_email_and_phone, calculates how strings are similar"""
    cnt = 0
    if row_1["phone"] == row_2["phone"]:
        cnt += 100
    if row_1["email"] == row_2["email"]:
        cnt += 200
    try:
        last_name_rat = Levenshtein.ratio(row_1["last_name"],
                                          row_2["last_name"])
        if last_name_rat > 0.8:
            cnt += last_name_rat * 50
    except Exception:
        pass
    try:
        first_name_rat = Levenshtein.ratio(row_1["first_name"],
                                           row_2["first_name"])
        if first_name_rat > 0.8:
            cnt += first_name_rat * 20
    except Exception:
        pass
    try:
        name_rat = Levenshtein.ratio(row_1["name"],
                                     row_2["name"])
        if name_rat > 0.5:
            cnt += name_rat * 70
    except Exception:
        pass
    return cnt


def find_best_sutable_row_in_dataframe(row_to_check, df):
    """Raw function for union_tables_by_email_and_phone, finds best similar row in dataframe for row_to_check"""
    best_ans_ind = -1
    best_cnt = -1
    for ind, row in df.iterrows():
        cnt = calc_similarity(row_to_check, row)
        if cnt > best_cnt:
            best_ans_ind = ind
            best_cnt = cnt
    return best_ans_ind, best_cnt


def union_tables_by_email_and_phone(
        first_df_raw, first_columns_dict,
        second_df_raw, second_columns_dict):
    """Find indexes in second dataframe, which corresponds to each row in the first dataframe
    :param first_df_raw: first dataframe
    :param first_columns_dict: dict of columns for first dataframe checking similarity
    (see example in df_to_standard)
    :param second_df_raw: second dataframe
    :param second_columns_dict: dict of columns for second dataframe for checking similarity
    (see example in df_to_standard)
    :return: list of indexes to check, second dataframe, from which you need to take rows,
    -1 in answer list means that similar row was not found
    """

    first_df = df_to_standard(first_df_raw, first_columns_dict)
    second_df = df_to_standard(second_df_raw, second_columns_dict)

    second_mail_grouped_df = second_df.groupby("email")
    second_phone_grouped_df = second_df.groupby("phone")
    emails = set(second_df['email'])
    phones = set(second_df['phone'])
    ans_list_of_indexes = []
    for _, row in first_df.iterrows():
        phone = row["phone"]
        email = row["email"]
        best_cnt_phone = -1
        best_cnt_mail = -1
        best_ind_phone = -1
        best_ind_mail = -1
        if not pd.isnull(email) and email in emails:
            email_df = second_mail_grouped_df.get_group(email)
            best_ind_mail, best_cnt_mail = find_best_sutable_row_in_dataframe(row, email_df)

        if not pd.isnull(phone) and phone in phones:
            phone_df = second_phone_grouped_df.get_group(phone)
            best_ind_phone, best_cnt_phone = find_best_sutable_row_in_dataframe(row, phone_df)

        if best_cnt_mail > best_cnt_phone and best_cnt_mail > 0:
            ans_list_of_indexes.append(best_ind_mail)
        elif best_cnt_phone > 0:
            ans_list_of_indexes.append(best_ind_phone)
        else:
            ans_list_of_indexes.append(-1)
    return ans_list_of_indexes, second_df


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


def concat_names(first_name, last_name):
    if pd.isnull(first_name):
        first_name = "_"
    if pd.isnull(last_name):
        last_name = "_"
    return last_name + ", " + first_name


def string_answer_to_boolean(string) -> bool:
    """Finds good words like "yes" in string"""
    try:
        return not pd.isnull(string) and ("Да" in string or "да" in string or "Yes" in string or "yes" in string)
    except Exception:
        print(string)
        return False


def raw_agrees_from_dyn(row, mkto_flag_column: str,
                        forms_mail_marketing_column: str, forms_unsubscribe_column: str) -> bool:
    """https://a.yandex-team.ru/arc/trunk/arcadia/cloud/analytics/yql/export_leads_to_marketo.yql"""
    mkto_flag = row[mkto_flag_column]
    forms_mail_marketing = row[forms_mail_marketing_column]
    forms_unsubscribe = row[forms_unsubscribe_column]
    if forms_unsubscribe:
        return False
    if pd.isnull(mkto_flag):
        return forms_mail_marketing
    return not pd.isnull(mkto_flag) and mkto_flag


def df_apply_agrees_from_dyn(df_raw, mkto_flag_column,
                             forms_mail_marketing_column, forms_unsubscribe_column, result_column):
    """https://a.yandex-team.ru/arc/trunk/arcadia/cloud/analytics/yql/export_leads_to_marketo.yql"""
    df = df_raw.copy()
    if df[forms_mail_marketing_column].dtype != bool:
        df[forms_mail_marketing_column] = df[forms_mail_marketing_column].apply(lambda x: string_answer_to_boolean(x))
    if df[forms_unsubscribe_column].dtype != bool:
        df[forms_unsubscribe_column] = df[forms_unsubscribe_column].apply(lambda x: string_answer_to_boolean(x))

    df[forms_mail_marketing_column] = df[forms_mail_marketing_column].astype(bool)
    df[forms_unsubscribe_column] = df[forms_unsubscribe_column].astype(bool)
    df[result_column] = df[[mkto_flag_column, forms_mail_marketing_column, forms_unsubscribe_column]].apply(
        lambda row: raw_agrees_from_dyn(row, mkto_flag_column,
                                        forms_mail_marketing_column, forms_unsubscribe_column), axis=1)
    return df


def from_3_constants_to_date(year, month, day) -> str:
    return datetime.strftime(parse(f"{year}-{month}-{day}"), "%Y-%m-%d")


def get_number_of_days_in_month(year, month):
    return monthrange(year, month)[1]


def time_to_unix(time_str):
    dt = parse(time_str)
    timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
    return int(timestamp)


def apply_type(raw_schema, df):
    if raw_schema is not None:
        for key in raw_schema:
            if 'list:' not in raw_schema[key] and raw_schema[key] != 'datetime':
                df[key] = df[key].astype(raw_schema[key])
            if raw_schema[key] == 'datetime':
                df[key] = df[key].apply(lambda x: time_to_unix(x))

    schema = []
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
                {"type_name": 'list', "item": {"type_name": "optional", "item": second_type}}})
        else:
            schema.append({"name": col, 'type': 'string'})
    return schema


def save_table(file_to_write, path, table, schema=None, append=False):
    assert (path[-1] != '/')

    df = table.copy()
    real_schema = apply_type(schema, df)
    json_df_str = df.to_json(orient='records')
    path = path + "/" + file_to_write
    json_df = json.loads(json_df_str)
    if not yt.exists(path) or not append:
        yt.create(type="table", path=path, force=True,
                  attributes={"schema": real_schema})
    tablepath = yt.TablePath(path, append=append)
    yt.write_table(tablepath, json_df,
                   format=yt.JsonFormat(attributes={"encode_utf8": False}))


def find_tables_in_hahn_folder(path):
    tables = []
    for table in yt.search(path, node_type=["table"],
                           attributes=["account"]):
        tables.append(table)
    return tables


def get_new_type(type_v3):
    if type_v3['type_name'] == 'list':
        return f"Array(Nullable({get_new_type(type_v3['item'])}))"
    else:
        return type_v3['item'][0].upper() + type_v3['item'][1:]


def create_schema_for_grafana(yt_path, table_name, sort_col=None):
    schema = yt.get(yt_path + "/@schema")

    body = f"CREATE TABLE {table_name} (\n    "
    time_cols = set()
    columns = set()
    int_cols = set()
    for key in schema:
        name = key["name"]
        columns.add(name)
        curr_type = get_new_type(key["type_v3"])
        if curr_type == 'Uint64':
            curr_type = 'UInt64'
        if curr_type == 'Bool':
            curr_type = "UInt8"
        if curr_type == 'Yson':
            curr_type = "String"
        if curr_type == "Datetime":
            time_cols.add(name)
        if curr_type == "Int64":
            int_cols.add(name)
        if key.get('sort_order') is not None and sort_col is None:
            sort_col = name
        body = body + name + " " + curr_type + ",\n    "

    body = body[:-6]
    body += "\n)\n"
    if len(time_cols) > 0:
        col = list(time_cols)[0]
    elif len(int_cols) > 0:
        col = list(int_cols)[0]
    else:
        col = list(columns)[0]
    if sort_col is None:
        sort_col = col
    partition_col = sort_col
    if sort_col in time_cols:
        partition_col = f"toYYYYMM({sort_col})"
    body += f"ENGINE = ReplicatedMergeTree('/clickhouse/tables/{{shard}}/{table_name}', '{{replica}}')\n" \
            f"ORDER BY {sort_col}\n"
    return body


def post_grafana_sql(query):
    hosts = ['sas-tt9078df91ipro7e.db.yandex.net',
             "vla-2z4ktcci90kq2bu2.db.yandex.net"]
    auth = {
        'X-ClickHouse-User': user_name,
        'X-ClickHouse-Key': clickhouse_token,
    }
    for host in hosts:
        url = 'https://{host}:8443/?database={db}&query={query}'.format(
            host=host,
            db='cloud_analytics',
            query=query)
        r = requests.post(url=url,
                          headers=auth,
                          verify= \
                              '/usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt')
        if r.status_code == 200:
            continue
        else:
            # print(host + r.text, r)
            pass
    return


def drop_grafana_table(grafana_table_name):
    try:
        post_grafana_sql('DROP TABLE ' + grafana_table_name)
    except:
        pass


def save_table_from_yt_to_grafana(yt_path, grafana_table_name,
                                  sort_col=None, schema_table=None):
    raw_grafana_table_name = grafana_table_name + "_" + str(int(datetime.now().timestamp()))
    if schema_table is None:
        schema_table = \
            create_schema_for_grafana(yt_path, raw_grafana_table_name, sort_col=sort_col)
    # print(schema_table)
    post_grafana_sql(schema_table)

    params = {
        'clickhouse_copy_options': {
            'command': 'append',
        },
        'clickhouse_credentials': {
            'password': clickhouse_token,
            'user': user_name,
        },
        'mdb_auth': {
            'oauth_token': mdb_auth_token,
        },
        'mdb_cluster_address': {
            'cluster_id': "07bc5e8c-c4a7-4c26-b668-5a1503d858b9",
        },
        'clickhouse_copy_tool_settings_patch': {
            'clickhouse_client': {
                'per_shard_quorum': 'all',
            },
        }
    }
    task = tm.add_task(source_cluster="hahn",
                       source_table=yt_path,
                       destination_cluster='mdb-clickhouse',
                       destination_table=raw_grafana_table_name,
                       params=params,
                       sync=False)
    task_info = tm.get_task_info(task)
    while task_info['state'] in ('pending', 'running'):
        time.sleep(5)
        task_info = tm.get_task_info(task)

    if task_info['state'] != 'completed':
        raise Exception(
            'Transfer manager task failed with '
            'the following state: %s' % task_info['state'])

    drop_grafana_table(grafana_table_name)
    move_req = f"""
    RENAME TABLE {raw_grafana_table_name}
        TO {grafana_table_name} ON CLUSTER "cloud_analytics"
    """
    post_grafana_sql(move_req)
    drop_grafana_table(raw_grafana_table_name)


def get_current_date_as_str():
    return str(datetime.date(datetime.now()))


def date_to_string(date):
    return datetime.strftime(date, "%Y-%m-%d")


def date_range(end_date, freq_interval=None, start_date=None, freq='D'):
    if freq_interval is None:
        return pd.date_range(start=start_date, end=end_date, freq=freq).to_pydatetime().tolist()
    return pd.date_range(end=end_date, periods=freq_interval, freq=freq).to_pydatetime().tolist()


def get_billings_from_staff():
    staff_req = """
    SELECT
        *
    FROM "//home/cloud_analytics/import/staff/cloud_staff/cloud_staff"
    FORMAT TabSeparatedWithNames
    """
    staff = execute_query(staff_req)
    emails = np.array(staff["personal_email_all"])
    yandex_emails = np.array(staff["yandex_login"])
    all_emails = []
    for email in emails:
        all_emails += email.split(",")

    for email in yandex_emails:
        if not pd.isnull(email):
            all_emails.append(email + "@yandex.ru")
    all_emails = set(all_emails)
    final_emails = []
    for email in all_emails:
        if works_with_emails(email) is not None:
            final_emails.append(works_with_emails(email))

    ba_req = """
SELECT
    billing_account_id,
    user_settings_email as email
FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
WHERE event == 'ba_created'
FORMAT TabSeparatedWithNames
"""
    ba_df = execute_query(ba_req)
    staff_billings = ba_df[ba_df["email"].isin(set(final_emails))]
    return staff_billings


def get_wiki_table(table_path):
    url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid'
    headers = {'Authorization': f"OAuth {yt_token}"}
    url = url % table_path
    request = requests.get(url, headers=headers)
    table_json = request.json()
    columns = [column['title'] for column in table_json['data']['structure']['fields']]
    rows = [[column['raw'] for column in row] for row in table_json['data']['rows']]
    return pd.DataFrame(rows, columns=columns)


def get_wiki_table_version(table_path):
    url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid'
    headers = {'Authorization': f"OAuth {yt_token}"}
    url = url % table_path
    request = requests.get(url, headers=headers)
    table_json = request.json()
    version_res = table_json['data']['version']
    return version_res


def get_wiki_table_column_id_correspondence(table_path):
    url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid'
    headers = {'Authorization': f"OAuth {yt_token}"}
    url = url % table_path
    request = requests.get(url, headers=headers)
    table_json = request.json()
    column_id_correspondence = {column['title']: column['name'] for column in
                                table_json['data']['structure']['fields']}
    return column_id_correspondence


def get_wiki_table_row_ids(table_path):
    url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid'
    headers = {'Authorization': f"OAuth {yt_token}"}
    url = url % table_path
    request = requests.get(url, headers=headers)
    table_json = request.json()
    columns = [column['title'] for column in table_json['data']['structure']['fields']]
    rows = [[column['row_id'] for column in row][0] for row in table_json['data']['rows']]
    return rows


def chage_wiki_table(table_path, changes):
    version = get_wiki_table_version(table_path)
    url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid/change'
    url = url % table_path
    headers = {'Authorization': f"OAuth {yt_token}"}
    resp = requests.post(url, json={
        'changes': changes,
        'version': version
    }, headers=headers)
    if not resp.ok:
        raise Exception(f'Failed to post wiki page on path "{table_path}". Response: {resp.text}')


def replace_wiki_table(table_path, df_raw):
    df = df_raw.copy()
    column_id_correspondence = get_wiki_table_column_id_correspondence(table_path)
    df = df.rename(columns=column_id_correspondence)
    url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid/replace'
    url = url % table_path
    headers = {'Authorization': f"OAuth {yt_token}"}
    data = [val for val in list(df.T.to_dict().values())]
    resp = requests.post(url, json={
        'data': data
    }, headers=headers)
    if not resp.ok:
        raise Exception(f'Failed to post wiki page on path "{table_path}". Response: {resp.text}')


"""============================================================================================================"""


def __wald_statistics(n, m, p1, p2):
    sigma = np.sqrt(p1 * (1 - p1) / n + p2 * (1 - p2) / m)
    return (p1 - p2) / (sigma + 1e-5)


def __wald_body(control_group_size, conversions_control_group,
                test_group_size, conversions_test_group):
    p1 = conversions_control_group / control_group_size
    p2 = conversions_test_group / test_group_size
    statistics = __wald_statistics(control_group_size, test_group_size, p1, p2)
    return statistics, sps.norm.cdf(statistics)


def WaldTest(control_group_size, conversions_control_group,
             test_group_size, conversions_test_group, alternative="two-sided"):
    less_stat, less_pval = __wald_body(control_group_size, conversions_control_group,
                                       test_group_size, conversions_test_group)
    greater_stat, greater_pval = __wald_body(test_group_size, conversions_test_group,
                                             control_group_size, conversions_control_group)
    return_greater = False
    if alternative == "two-sided":
        if less_pval > greater_pval:
            return_greater = True
    if alternative == "greater":
        return_greater = True
    assert (alternative in ["two-sided", "greater", "less"])
    if return_greater:
        return namedtuple('WaldTestResult', ('statistic', 'pvalue'))(greater_stat,
                                                                     greater_pval)
    return namedtuple('WaldTestResult', ('statistic', 'pvalue'))(less_stat, less_pval)


"""============================================================================================================"""


## META ID

class IdGenerator:
    def __init__(self, number_of_digits: int = 9):
        self.given_id: tp.Set[int] = set()
        self.number_of_digits: int = number_of_digits

    def __generate_new_id(self) -> int:
        if len(self.given_id) == 10 ** (self.number_of_digits) - 10 ** (self.number_of_digits - 1):
            raise ValueError("Sorry, no free id")
        while True:
            current_id = np.random.randint(10 ** (self.number_of_digits - 1), 10 ** (self.number_of_digits))
            if current_id not in self.given_id:
                self.given_id.add(current_id)
                return current_id

    __call__ = __generate_new_id


class MetaInformationClass:

    def __init__(self, interested_columns=[]):
        self.dict_cloud: tp.Dict[str, int] = {}  # by cloud_id get id
        self.dict_billing: tp.Dict[str, int] = {}  # by billing_id get id
        self.dict_id_to_folders: tp.Dict[int, tp.Set[str]] = defaultdict(set)  # by meta_id get floders
        self.dict_id_to_billing: tp.Dict[int, tp.List[str]] = defaultdict(list)  # by id get get all billings
        self.dict_id_to_cloud: tp.Dict[int, tp.List[str]] = defaultdict(list)  # by id get get all clouds
        self.dict_id_to_last_billing: tp.Dict[int, str] = {}  # by id get get last billing by time in cube
        self.gen_id = IdGenerator()
        self.visited: tp.Set[str] = set()
        self.billing_to_new_billing: tp.Dict[str, str] = {}  # matching billing to last billing
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
        getattr(self, "dict_id_to_" + where_to_add)[current_id].append(billing_or_cloud)
        getattr(self, "dict_" + where_to_add)[billing_or_cloud] = current_id

    def __dfs_visit(self, billing: str, current_id: int) -> None:
        if billing in self.visited:
            return
        self.visited.add(billing)
        self.__add_info(billing, current_id, "billing")
        self.billing_to_new_billing[billing] = self.dict_id_to_last_billing[current_id]

        for cloud in self.billing_as_vertex_dict[billing]:
            self.__add_info(cloud, current_id, "cloud")
            self.dict_id_to_folders[current_id].update(self.cloud_to_folders_dict[cloud])
            for chld_billing in self.cloud_as_vertex_dict[cloud]:
                self.__dfs_visit(chld_billing, current_id)

    def __create_dict(self, where_to_add: str) -> pd.DataFrame:
        vertex_df = getattr(self, f"_MetaInformationClass__get_for_{where_to_add}_id")()
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
        meta_id = self.dict_billing[billing_id]
        return self.dict_id_to_billing[meta_id]

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
            result_dict["last_active_billing"].append(self.billing_to_new_billing[billing])
            result_dict["associated_billings"].append(
                list(set(self.dict_id_to_billing[meta_id])))
            result_dict["associated_clouds"].append(list(set(
                self.dict_id_to_cloud[meta_id])))
            result_dict["associated_folders"].append(list(self.dict_id_to_folders[meta_id]))
            for column in self.columns:
                result_dict[column].append(self.get_info_from_column(billing, column))
        result_df = pd.DataFrame.from_dict(result_dict)
        return result_df
