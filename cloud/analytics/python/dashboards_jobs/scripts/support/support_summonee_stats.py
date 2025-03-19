import gc
import json
from datetime import datetime
from datetime import timezone

import numpy as np
import pandas as pd
import requests
import sys
import time
import yt.transfer_manager.client as tm
import yt.wrapper as yt
from dateutil.parser import *
from joblib import Parallel, delayed
from startrek_client import Startrek

yt_token = sys.argv[1]
startrek_token = sys.argv[2]
clickhouse_token = sys.argv[3]
mdb_oauth_token = sys.argv[4]
user_name = sys.argv[5]

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


def raw_chyt_execute_any_query(query, request_func, columns=None):
    i = 0
    while True:
        try:
            result = request_func(query=query)
            if columns is None:
                users = pd.DataFrame(
                    [row.split('\t') for row in result[1:]], columns=result[0].split('\t'))
            else:
                users = pd.DataFrame([row.split('\t')
                                      for row in result], columns=columns)
            return users
        except Exception as err:
            print(err)
            i += 1
            if i > 5:
                print('Break Excecution')
                break


def update_automatically_types(df_raw):
    df = df_raw.copy()
    for column in df.columns:
        try:
            df[column] = df[column].apply(lambda x: json.loads(x))
            continue
        except Exception:
            pass
        try:
            df[column] = df[column].apply(
                lambda x: json.loads(x.replace("'", '"')))
            continue
        except Exception:
            pass
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
            return mail_.lower().replace('.', '-')


def time_to_unix(time_str):
    dt = parse(time_str)
    timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
    return int(timestamp)


def apply_type(raw_schema, df):
    if raw_schema is not None:
        for key in raw_schema:
            if raw_schema[key] != 'list' and raw_schema[key] != 'datetime':
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
        elif raw_schema is not None and raw_schema.get(col) is not None and raw_schema[col] == 'list':
            schema.append({"name": col, 'type_v3':
                           {"type_name": 'list', "item": {"type_name": "optional", "item": "string"}}})
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


def date_range(end_date, freq_interval=None, start_date=None, freq='D'):
    if freq_interval is None:
        return pd.date_range(start=start_date, end=end_date, freq=freq).to_pydatetime().tolist()
    return pd.date_range(end=end_date, periods=freq_interval, freq=freq).to_pydatetime().tolist()


def get_new_type(type_v3):
    if type_v3['type_name'] == 'list':
        return f"Array({get_new_type(type_v3['item'])})"
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
            f"PARTITION BY {partition_col}\n" \
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
                          verify='/usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt')
        if r.status_code == 200:
            continue
        else:
            # print(host + r.text)
            pass
    return


def drop_grafana_table(grafana_table_name):
    try:
        post_grafana_sql('DROP TABLE ' + grafana_table_name)
    except:
        pass


def save_table_from_yt_to_grafana(yt_path, grafana_table_name,
                                  sort_col=None, schema_table=None):
    raw_grafana_table_name = grafana_table_name + \
        "_" + str(int(datetime.now().timestamp()))
    if schema_table is None:
        schema_table = \
            create_schema_for_grafana(
                yt_path, raw_grafana_table_name, sort_col=sort_col)
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
            'oauth_token': mdb_oauth_token,
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
        time.sleep(1)
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


"""============================================================================================================"""

# STAFF

request = """
SELECT
    *
FROM "//home/cloud_analytics/import/staff/cloud_staff/cloud_staff"
FORMAT TabSeparatedWithNames
"""

staff = execute_query(request)
second_line = \
    'Яндекс.Облако -> Управление операционной деятельности и развития бизнеса -> Направление продаж в массовом сегменте -> Служба клиентского опыта Яндекс.Облака -> Группа ведущих инженеров поддержки Яндекс.Облака'


def get_small_name(name):
    try:
        return name.split(" -> ")[1]
    except Exception:
        return name


def get_long_name(name):
    try:
        name.split(" -> ")[1]
        return ": ".join(name.split(" -> ")[1:])
    except Exception:
        return name


staff["short_name"] = staff["group_full_path"].apply(
    lambda name: get_small_name(name))
staff["long_name"] = staff["group_full_path"].apply(
    lambda name: get_long_name(name))
staff["is_second_line"] = (staff["group_full_path"] == second_line).astype(int)

short_group_dict = {}  # name -> short_group
long_group_dict = {}  # name -> long_group
for _, row in staff.iterrows():
    login = row["login"]
    if row["is_second_line"] == 0:
        short_group_dict[login] = row["short_name"]
        long_group_dict[login] = row["long_name"]
    else:
        short_group_dict[login] = "second line"
        long_group_dict[login] = "second line"

not_interseted_group = "Отдел по развитию бизнеса и работе с заказчиками"
second_line = "second line"


# TICKETS ANALYSIS

# Event class


class Event:
    def __init__(self, type, time, **kwargs):
        super().__init__()
        self.time = time
        if "timer starts" in type.lower():
            self.type = "timer starts"
            self.type_id = 1
        if "timer ends" in type.lower():
            self.type = "timer ends"
            self.type_id = 4
        if "status" in type.lower():
            self.type = "new status"
            self.type_id = 2
        if "призвали" in type.lower():
            self.type = "add summonee"
            self.type_id = 3
        self.args = []
        for key in kwargs:
            self.args.append(key)
            setattr(self, key, kwargs[key])

    def __repr__(self):
        ans_string = f"[event type: '{self.type}', start time: '{self.time}'"
        if len(self.args) > 0:
            added_args = ", ".join(
                [key + ": '" + str(getattr(self, key)) + "'" for key in self.args])
            return ans_string + f", {added_args}]\n"
        else:
            return ans_string + "]\n"

    def __lt__(self, other):
        if self.time == other.time:
            return self.type_id < other.type_id
        return parse(self.time) < parse(other.time)


def added_summonee(changes, log):
    answers = []
    if "robot" not in log.updatedBy.id \
            and "яндекс" not in log.updatedBy.display.lower() \
            and "yandex" not in log.updatedBy.display.lower():
        if short_group_dict.get(log.updatedBy.id) == second_line:
            current_event = Event("призвали", log.updatedAt,
                                  user_id=log.updatedBy.id,
                                  user=log.updatedBy.display)
            answers.append(current_event)

    if changes["field"].display == 'Нужен ответ пользователя':
        if changes["to"] is not None:
            for person in changes["to"]:
                current_event = Event("призвали", log.updatedAt,
                                      user_id=person.id, user=person.display)
                answers.append(current_event)
    if len(answers) > 0:
        return answers
    return None


def added_status(changes, log):
    ans_arr = []
    stop_statuses = ["Ждет выкладки", "Закрыт", "Линия 2",
                     "Требуется информация", "Частично выполнен",
                     "Подготовка документов", "Wait for Answer"]
    if changes["field"].display == 'Статус':
        curr_status = changes["to"].display
        ans_arr.append(
            Event("status changed", log.updatedAt, status=curr_status))
        if curr_status in stop_statuses:
            ans_arr.append(Event("timer ends", log.updatedAt, spent=0))
        if curr_status == "Открыт":
            ans_arr.append(Event("timer starts", log.updatedAt))
        return ans_arr
    return None


client = Startrek(useragent="robot-clanalytics-yt",
                  base_url="https://st-api.yandex-team.ru/v2/myself", token=startrek_token)


def get_ticket_name(issue):
    return issue._path.split("/")[-1]


def get_all_tickets():
    issues = client.issues.find(
        filter={'queue': 'CLOUDSUPPORT', 
                'created': {'from': '2019-01-01'}},
        order=['-created'],
        scrollType='unsorted'
    )
    ticket_names = []
    for issue in issues:
        ticket_names.append(get_ticket_name(issue))
    return ticket_names


ticket_names = get_all_tickets()


def find_update_status_time(issue, log_list):
    answer_dict = {}
    next_time = None
    answer_dict["Открыт"] = issue.createdAt
    for log in log_list:
        for changes in log.fields:
            if changes["field"].display == 'Статус':
                curr_status = changes["to"].display
                if curr_status is not None \
                        and curr_status != "Открыт" and answer_dict.get("В работе") is None:
                    answer_dict["В работе"] = log.updatedAt
        if next_time is None and answer_dict["Открыт"] != log.updatedAt:
            next_time = log.updatedAt
    try:
        parse(answer_dict["В работе"])
    except Exception:
        if next_time is None:
            next_time = datetime.utcnow().isoformat(sep=' ', timespec='milliseconds') + '+0000'
        answer_dict["В работе"] = next_time

    dt = parse(answer_dict["В работе"]) - parse(answer_dict["Открыт"])
    answer_dict["Начальный простой"] = dt.days * 24 * 60 + dt.seconds / 60
    return answer_dict


def get_issue_main_info(issue):
    answer_dict = {}
    answer_dict["pay type"] = issue.pay
    components_array = []
    for component in issue.components:
        components_array.append(component.display)
    answer_dict["components"] = components_array
    return answer_dict


def get_time_information(time_dict, issue, events):
    curr_start_time = None
    delta = 0
    answer_dict = {}
    deltas = []
    timer_events = []
    for ind, event in enumerate(events):
        if 'timer' in event.type:
            if len(timer_events) > 0 and \
                    timer_events[-1].type == event.type and \
                    event.type == 'timer starts':
                continue
            timer_events.append(event)

        if event.type =='new status':
            if event.status == 'Закрыт':
                is_closed = 1
            else:
                is_closed = 0
        
    answer_dict["is_closed"] = is_closed

    for ind, event in enumerate(timer_events):
        if event.type == 'timer ends' and \
                (ind + 1 == len(timer_events) or timer_events[ind + 1].type != 'timer ends'):
            # последний закрывающий таймер
            dt = (parse(event.time) - parse(curr_start_time))
            delta = dt.days * 24 * 60 + dt.seconds / 60
            deltas.append(delta)
            event.spent = delta
            if len(deltas) == 1:
                event.spent -= time_dict["Начальный простой"]
        if event.type == 'timer starts':
            curr_start_time = event.time
    if (len(deltas)==0):
        dt_now = datetime.utcnow().isoformat(sep=' ', timespec='milliseconds') + '+0000'
        dt = (parse(dt_now) - parse(curr_start_time))
        delta = dt.days * 24 * 60 + dt.seconds / 60
        deltas.append(delta)

    deltas[0] -= time_dict["Начальный простой"]
    answer_dict["all_active_time"] = sum(deltas)
    answer_dict["avg_active_time"] = np.mean(deltas)
    answer_dict["std_active_time"] = np.std(deltas)
    answer_dict["number_of_timer_stops"] = len(deltas) - 1
    answer_dict["percentile_25"], \
        answer_dict["percentile_50"], \
        answer_dict["percentile_75"] = np.percentile(
            deltas, [25, 50, 75], interpolation='midpoint').tolist()
    # print(timer_events)
    return answer_dict


def create_event_line(log_list, time_dict):
    events = []
    event_functions = [added_summonee, added_status]
    for log in log_list:
        for changes in log.fields:
            for func in event_functions:
                event = func(changes, log)
                if event is not None:
                    events += event
    events.append(Event("timer starts", time_dict["Открыт"]))
    return sorted(events)


def get_summonee_dict(time_dict, events):
    curr_last_time = time_dict["В работе"]
    delta = 0
    summonee_dict = {}
    is_started = False
    summonee_dict["help was needed"] = 0
    for event in events:
        if event.type == 'new status' and event.status == "В работе":
            is_started = True
        if event.type == 'timer ends' and is_started:
            delta += event.spent
        if event.type == 'timer starts' and is_started:
            curr_last_time = event.time

        if event.type == "add summonee" \
                and short_group_dict.get(event.user_id) is not None and \
                short_group_dict[event.user_id] != not_interseted_group:

            if summonee_dict.get("first summonee time") is None:
                dt = parse(event.time) - parse(curr_last_time)
                last_delta = dt.days * 24 * 60 + dt.seconds / 60
                delta += last_delta
                summonee_dict["help was needed"] = 1
                summonee_dict["first summonee time"] = delta
                summonee_dict["summonee short groups"] = set()
                summonee_dict["summonee long groups"] = set()
                # обнулили все
            try:
                summonee_dict["summonee short groups"].add(
                    short_group_dict[event.user_id])
                summonee_dict["summonee long groups"].add(
                    long_group_dict[event.user_id])
                if short_group_dict[event.user_id] != second_line:
                    summonee_dict["help from not second line"] = 1

            except Exception:
                pass
    return summonee_dict


def get_support_names(log_list):
    support_names = set()
    second_line_names = set()
    for log in log_list:
        for changes in log.fields:
            if "robot" not in log.updatedBy.id \
                    and "яндекс" not in log.updatedBy.display.lower() \
                    and "yandex" not in log.updatedBy.display.lower():
                try:
                    if short_group_dict[log.updatedBy.id] == not_interseted_group:
                        support_names.add(log.updatedBy.id)
                    if short_group_dict[log.updatedBy.id] == second_line:
                        second_line_names.add(log.updatedBy.id)
                except Exception:
                    pass
    help_from_support = 0
    if len(second_line_names) > 0:
        help_from_support = 1
    return {"support": support_names, "second line": second_line_names,
            "support number": len(support_names), "second line number": len(second_line_names),
            "help from second line": help_from_support}


def get_all_ticket_info(ticket_name):
    try:
        issue = client.issues[ticket_name]
        log_list = list(issue.changelog.get_all(sort='asc'))
        time_dict = find_update_status_time(issue, log_list)
        main_ticket_info = get_issue_main_info(issue)
        events = create_event_line(log_list, time_dict)
        delta_info = get_time_information(time_dict, issue, events)

        support_dict = get_support_names(log_list)
        summonee_dict = get_summonee_dict(time_dict, events)
        answer_dict = {"ticket_name": ticket_name,
                       **main_ticket_info,
                       **delta_info,
                       **time_dict,
                       **summonee_dict,
                       **support_dict}
        # print(events)
        return answer_dict
    except: # Exception:
        print(ticket_name)
        raise


data = Parallel(n_jobs=-1)(delayed(get_all_ticket_info)(ticket)
                           for ticket in ticket_names)


gc.collect()

ticket_df = pd.DataFrame(data)
ticket_df["support"] = ticket_df["support"].apply(lambda x: sorted(list(x)))
ticket_df["components"] = ticket_df["components"].apply(
    lambda x: sorted(list(x)))
ticket_df["second line"] = ticket_df["second line"].apply(
    lambda x: sorted(list(x)))
ticket_df["summonee short groups"] = \
    ticket_df["summonee short groups"].apply(
        lambda x: sorted(list(x)) if not pd.isnull(x) else [])
ticket_df["summonee long groups"] = \
    ticket_df["summonee long groups"].apply(
        lambda x: sorted(list(x)) if not pd.isnull(x) else [])
ticket_df["components"] = ticket_df["components"].apply(lambda x: sorted(x) if len(x) > 0
                                                        else ["no_component"])
ticket_df["summonee short groups"] = \
    ticket_df["summonee short groups"].apply(lambda x: sorted(x) if len(x) > 0
                                             else ["no_summonee"])


def list_digest(strings):
    import hashlib
    import struct
    hash = hashlib.sha1()
    for s in strings:
        hash.update(struct.pack("I", len(s)))
        hash.update(s.encode('utf-8'))
    return hash.hexdigest()


ticket_df["components_hashed"] = ticket_df["components"].apply(
    lambda x: list_digest(x))
ticket_df["summonee_short_groups_hashed"] = \
    ticket_df["summonee short groups"].apply(lambda x: list_digest(x))


def update_column(col):
    if "Открыт" == col:
        return "ticket_opened_time"
    if "В работе" == col:
        return "ticket_in_work_time"
    if "Начальный простой" == col:
        return "initial_not_working_time"
    return "_".join(col.split())


ticket_df.columns = [update_column(x) for x in ticket_df.columns]
ticket_df.replace(np.nan, 0, inplace=True)
ticket_df["pay_type"].replace(0, "no_type", inplace=True)
save_table("ticket_info",
           "//home/cloud_analytics/support_tables",
           ticket_df, schema={"components": 'list', "support": "list",
                              "second_line": 'list',
                              "summonee_short_groups": "list",
                              "summonee_long_groups": "list",
                              "ticket_opened_time": "datetime",
                              "ticket_in_work_time": "datetime"})

save_table_from_yt_to_grafana("//home/cloud_analytics/support_tables/ticket_info",
                              "cloud_analytics.support_ticket_information",
                              sort_col="ticket_opened_time")

with open('output.json', 'w') as f:
    json.dump({"table_path" : '//home/cloud_analytics/support_tables/ticket_info' }, f)