# -*- coding: utf-8 -*-
import os
import typing as t
from collections import defaultdict
import json
from datetime import datetime, timedelta
from operator import attrgetter
from enum import Enum

from queue import Queue
from threading import Thread

import pandas as pd
import numpy as np
from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
from yql.api.v1.client import YqlClient
from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.argus.bin.utils.s3_interactions import S3Uploader
from antiadblock.tasks.tools.logger import create_logger
import antiadblock.tasks.tools.common_configs as configs
from antiadblock.tasks.tools.configs_api import get_configs_from_api, post_tvm_request
from antiadblock.libs.elastic_search_client.client import SearchClient
from antiadblock.argus.tasks.logs_agregate_task.verdict_task import verdict_task


class Status(str, Enum):
    NEW = "new"
    WAIT_CONFIRMATION = "wait_confirmation"
    SUCCESS = "success"


LOG_TYPES = [
    # "balancer",
    # "cryprox",
    # "nginx",
    # "bs-event-log",
    # "bs-dsp-log",
    # "bs-hit-log",
    "elastic-count",
    "elastic-auction",
]

logger = create_logger("argus_logs_agregate_task")
templates = NativeEnvironment(loader=DictLoader(
    {
        log_type: bytes.decode(resource.find(log_type.replace("-", "_")))
        for log_type in LOG_TYPES if not log_type.startswith('elastic')
    }
))


# количество кейсов которые попадут в 1 YQL запрос
BUCKET_SIZE = 200
CASE_RUN_MAX_DURATION = timedelta(minutes=5)


def get_cases_from_configs_api() -> dict:
    api_url = "https://{}/sbs_check/results/logs".format(configs_api_host)
    return get_configs_from_api(api_url, tvm_client)


def save_agregated_logs_to_configs_api(json_data: dict) -> dict:
    api_url = "https://{}/sbs_check/results/logs".format(configs_api_host)
    post_tvm_request(api_url, tvm_client, json_data)


def is_bs_logs_type(logs_type: str) -> bool:
    return logs_type.startswith('bs-') or logs_type.startswith('elastic-')


class Case:
    def __init__(self, json_data: dict) -> None:
        self.sbs_result_id = json_data["id"]
        self.request_id = json_data["request_id"]
        self.adb_bits = json_data.get("adb_bits")
        self.start_time = datetime.strptime(json_data["start_time"], '%Y-%m-%dT%H:%M:%S') + timedelta(hours=3)
        self.logs = json.loads(json_data["logs"])
        self.logs_df = dict()

    def get_log_id(self, logs_type: str, check_status: bool = False) -> t.Optional[str]:
        if check_status and self.logs[logs_type]["status"] not in (Status.NEW, Status.WAIT_CONFIRMATION):
            return None
        return self.adb_bits if is_bs_logs_type(logs_type) else self.request_id

    def fill_log_info(self, logs_type: str, meta: dict, dataframe: pd.DataFrame) -> None:
        if meta["items"] == self.logs[logs_type]["meta"].get("items", 0):
            self.logs[logs_type]["status"] = Status.SUCCESS
        else:
            self.logs[logs_type]["status"] = Status.WAIT_CONFIRMATION
        self.logs[logs_type]["meta"] = meta
        self.logs_df[logs_type] = dataframe

    def fill_obsolete_log_info(self) -> None:
        for log_type in LOG_TYPES:
            if log_type in self.logs:
                self.logs[log_type]["status"] = Status.SUCCESS
            else:
                self.logs[log_type] = dict(status=Status.SUCCESS)


def merge_lists(list1: list, list2: list) -> list:
    if not list1 or not list2:
        return list1 or list2 or []
    pos1, pos2 = 0, 0
    result = []
    while pos1 < len(list1) and pos2 < len(list2):
        if list1[pos1] == list2[pos2]:
            result.append(list1[pos1])
            pos1 += 1
            pos2 += 1
            continue
        if int(list1[pos1]['timestamp']) < int(list2[pos2]['timestamp']):
            result.append(list1[pos1])
            pos1 += 1
        else:
            result.append(list2[pos2])
            pos2 += 1
    return result + list1[pos1:] + list2[pos2:]


def merge_json_data(old_data: dict, new_data: dict) -> dict:
    if not old_data:
        return new_data

    result = {log_type: defaultdict(list) for log_type in LOG_TYPES}

    for log_type in LOG_TYPES:
        request_ids = set(new_data[log_type].keys()) | set(old_data[log_type].keys())
        for request_id in request_ids:
            result[log_type][request_id] = merge_lists(old_data[log_type].get(request_id),
                                                       new_data[log_type].get(request_id))
    return result


def upload_to_s3(filename: str, json_data: dict) -> str:
    old_data = None
    if not os.getenv("LOCAL_RUN"):
        old_data = json.loads(s3_client.get_object(filename, default="{}"))
    merged = merge_json_data(old_data, json_data)
    with open(filename, "w") as f:
        json.dump(merged, f, sort_keys=True)
    if not os.getenv("LOCAL_RUN"):
        return s3_client.upload_file(filename, rewrite=True)  # return url
    return filename


def get_query_predicate(log_type: str, request_ids: list[str]) -> str:
    """
    :param log_type:
    :param request_ids:
    :return:
    `request_id` LIKE "a"
    OR `request_id` LIKE "b"
    OR `request_id` LIKE "c"
    """
    if is_bs_logs_type(log_type):
        return ", ".join('"{}"'.format(int(x) >> 1) for x in request_ids)
    name = {
        "cryprox": "`request_id`",
        "nginx": "`request_id`",
        "balancer": "`x_aab_requestid`",
    }
    return "\n\tOR ".join([name[log_type] + " LIKE \"{}\"".format(request_id.replace("-{cnt}", "%")) for request_id in request_ids])


def get_stream_table_name(datetime_obj: datetime) -> datetime:
    return datetime_obj.replace(minute=datetime_obj.minute - datetime_obj.minute % 5, second=0, microsecond=0)


def get_list_of_table_names(time_points: list[datetime]) -> list[str]:
    table_names = set([get_stream_table_name(item) for item in time_points])
    results = set()
    for item in table_names:
        results.add(item.strftime(configs.YT_TABLES_STREAM_FMT))
        results.add((item + timedelta(minutes=5)).strftime(configs.YT_TABLES_STREAM_FMT))
    return sorted(list(results))


def get_dataframe_from_logs(log_type: str, bucket) -> tuple[pd.DataFrame, str]:
    if not log_type.startswith('elastic-'):
        return get_dataframe_from_yt(log_type, bucket)
    elastic_client = SearchClient()
    range_dt = {
        "from": (bucket[0][1] - timedelta(hours=3)).strftime(configs.STARTREK_DATETIME_FMT),
        "to": (bucket[-1][1] + timedelta(minutes=5) - timedelta(hours=3)).strftime(configs.STARTREK_DATETIME_FMT),
    }
    request_tag = "RTB_COUNT" if log_type == 'elastic-count' else "RTB_AUCTION"
    results = elastic_client.get(
        range_dt=range_dt,
        match_phrases=[
            dict(action="argus_grep_logs"),
            dict(request_tags=request_tag),
        ],
        match_fields=[dict(adbbits=int(item[0]) >> 1) for item in bucket],
    )
    df = pd.DataFrame(columns=['log_id', 'adbbits', 'action', '@timestamp'])
    df = df.append(pd.DataFrame(results))
    df["log_id"] = df["adbbits"].map(lambda x: str(x << 1))
    df["timestamp"] = df["@timestamp"].map(lambda x: int((datetime.strptime(x[:19], "%Y-%m-%dT%H:%M:%S")
                                                          + timedelta(hours=3)).strftime("%s")))
    df = df.drop(columns=['@timestamp', 'adbbits', 'action'])
    return df, log_type


def get_dataframe_from_yt(log_type: str, bucket):
    """
    :param log_type:
    :param bucket: list of tuple (request_id, start_time)
    :return: dataframe of yql_request
    """
    request_ids = [item[0] for item in bucket]
    time_points = [item[1] for item in bucket]

    query = templates.get_template(log_type).render(
        list_of_tables=get_list_of_table_names(time_points),
        table_name_fmt=configs.YT_TABLES_STREAM_FMT,
        query_predicate=get_query_predicate(log_type, request_ids)
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    yql_request = yql_client.query(query, syntax_version=1, title="logs_agregate_task:{} YQL".format(log_type)).run()
    return yql_request.dataframe, log_type


def get_buckets_per_log_type_from_cases(cases: list[Case], log_type: str) -> t.Iterable[list[tuple[str, datetime]]]:
    bucket: list[tuple[str, datetime]] = []
    for case in cases:
        request_id = case.get_log_id(log_type, check_status=True)
        if request_id is None:
            continue
        bucket.append((request_id, case.start_time))

        if len(bucket) == BUCKET_SIZE or case == cases[-1]:
            yield bucket
            bucket = []


def get_duration_meta_dict(df_duration: pd.DataFrame) -> dict:
    """
    using only in get_*logs_type*_meta
    :param df_duration: Series
    :return: dict of meta about duration
    """
    durations = {
        "duration_mean": df_duration.mean(),
        "duration_std": df_duration.std(),
        "duration_p95": df_duration.quantile(0.95),
        "duration_p99": df_duration.quantile(0.99),
    }
    for item in durations:
        if np.isnan(durations[item]):
            durations[item] = None
    return durations


def get_balancer_meta(df_logs: dict) -> dict:
    results = get_duration_meta_dict(df_logs["duration"].map(lambda x: float(x.split("s")[0])))
    results.update({
        "items": df_logs.shape[0],
    })
    return results


def get_cryprox_meta(df_logs):
    results = get_duration_meta_dict(df_logs["duration"])
    results.update({
        "items": df_logs.shape[0],
        "actions": df_logs.action.value_counts().to_dict(),
        "http_codes": df_logs.http_code.value_counts().to_dict(),
    })
    return results


def get_nginx_meta(df_logs: dict) -> dict:
    results = get_duration_meta_dict(df_logs.duration)
    results.update({
        "items": df_logs.shape[0],
        "methods": df_logs.method.value_counts().to_dict(),
        "response_codes": df_logs.response_code.value_counts().to_dict(),
    })
    return results


def get_bs_meta(df_logs: dict) -> dict:
    return {
        "items": df_logs.shape[0],
    }


def fill_logs_info(dataframe: pd.DataFrame, log_type: str, log_id_to_case: dict):
    get_meta_callback = {
        "balancer": get_balancer_meta,
        "cryprox": get_cryprox_meta,
        "nginx": get_nginx_meta,
        "bs-dsp-log": get_bs_meta,
        "bs-hit-log": get_bs_meta,
        "bs-event-log": get_bs_meta,
        "elastic-count": get_bs_meta,
        "elastic-auction": get_bs_meta,
    }

    log_ids = list(dataframe.log_id.unique())
    for log_id in log_ids:
        filter_df = dataframe[dataframe.log_id == log_id]
        if filter_df.shape[0] == 0:
            continue
        meta = get_meta_callback[log_type](filter_df)
        offset_log_id = log_id
        if log_type.startswith('bs-'):
            offset_log_id = str((int(log_id) >> 1) << 1)
        log_id_to_case[offset_log_id].fill_log_info(log_type, meta, filter_df)


def result_agregator() -> None:
    while not checks_results_queue.empty():
        dataframe, log_type = checks_results_queue.get()
        if dataframe is None:
            continue

        if not is_bs_logs_type(log_type):  # add log_id
            dataframe["log_id"] = dataframe["request_id"].map(lambda x: x.split("-")[0] + "-{cnt}")

        fill_logs_info(dataframe,
                       log_type,
                       adb_bits_to_case if is_bs_logs_type(log_type) else request_id_to_case)


def prepare_logs_to_s3(cases: list[Case]) -> dict:
    """
    :return:
    {
        *log_type*: {
            *request_id*: [ "logs" ] # sorted by timestamp
        },
    }
    """
    results = {log_type: defaultdict(list) for log_type in LOG_TYPES}
    for case_obj in cases:
        request_id = case_obj.request_id
        for log_type in LOG_TYPES:
            df = case_obj.logs_df.get(log_type)
            if df is None:
                continue
            results[log_type][request_id].extend(df.to_dict(orient='records'))
            results[log_type][request_id].sort(key=lambda x: x['timestamp'])
    return results


def upload_logs_to_s3(sandbox_id_to_cases: dict[str, list[Case]]) -> dict:
    sandbox_id_to_logs_url = {}
    for sandbox_id in sandbox_id_to_cases:
        filename = "log_{}.json".format(sandbox_id)
        json_data = prepare_logs_to_s3(sandbox_id_to_cases[sandbox_id])
        url = upload_to_s3(filename, json_data)
        sandbox_id_to_logs_url[sandbox_id] = url
        logger.info("Upload to s3 {}".format(url))
    return sandbox_id_to_logs_url


def main() -> None:
    logger.info("Start logs agregate task")
    cases: list[Case] = []
    obsolete_cases: list[Case] = []
    sandbox_id_to_cases: dict[str, list[Case]] = defaultdict(list)

    for json_case in get_cases_from_configs_api():
        case_obj = Case(json_case)
        request_id_to_case[case_obj.request_id] = case_obj
        adb_bits_to_case[case_obj.adb_bits] = case_obj
        sandbox_id_to_cases[case_obj.sbs_result_id].append(case_obj)
        if (datetime.now() + timedelta(hours=3)) - case_obj.start_time > timedelta(hours=OBSOLETE_HOURS):
            obsolete_cases.append(case_obj)
        else:
            cases.append(case_obj)
    cases.sort(key=attrgetter("start_time"))
    threads_list: list[Thread] = []

    for log_type in LOG_TYPES:
        for num_of_bucket, bucket in enumerate(get_buckets_per_log_type_from_cases(cases, log_type)):
            t = Thread(target=lambda q, args: q.put(get_dataframe_from_logs(*args)),
                       args=(checks_results_queue, (log_type, bucket)),
                       name='{}-{}'.format(log_type, num_of_bucket))
            t.start()
            threads_list.append(t)

    for thread in threads_list:
        logger.info('Wait thread {}...'.format(thread.name))
        thread.join()

    result_agregator()

    sandbox_id_to_logs_url = upload_logs_to_s3(sandbox_id_to_cases)

    for case_obj in obsolete_cases:
        case_obj.fill_obsolete_log_info()

    logs_info = defaultdict(list)
    for sandbox_id in sandbox_id_to_cases:
        for case_obj in sandbox_id_to_cases[sandbox_id]:
            for log_type in case_obj.logs:
                case_obj.logs[log_type]["url"] = sandbox_id_to_logs_url[sandbox_id]
            logs_info[sandbox_id].append({
                "request_id": case_obj.request_id,
                "logs": case_obj.logs
            })

    if os.getenv("LOCAL_RUN"):
        with open("results.json", "w") as f:
            json.dump(logs_info, f, indent=4)
    else:
        for sandbox_id, data in logs_info.items():
            logger.info(f"Save agregated logs for {sandbox_id}")
            save_agregated_logs_to_configs_api({sandbox_id: data})


if __name__ == "__main__":
    TVM_ID = int(os.getenv("TVM_ID", "2002631"))
    TVM_SECRET = os.getenv("TVM_SECRET")
    OBSOLETE_HOURS = int(os.getenv("OBSOLETE_HOURS", "6"))

    CONFIGSAPI_TEST_TVM_CLIENT_ID = 2000627
    CONFIGSAPI_PROD_TVM_CLIENT_ID = 2000629

    yql_token = os.getenv("YT_TOKEN")
    yql_client = YqlClient(token=yql_token, db="hahn")

    if os.getenv("ENV_TYPE", "PRODUCTION") == "PRODUCTION":
        configs_api_tvm_id = CONFIGSAPI_PROD_TVM_CLIENT_ID
        configs_api_host = "api.aabadmin.yandex.ru"
    elif os.getenv("ENV_TYPE") == "TESTING":  # TODO DELETE
        configs_api_tvm_id = CONFIGSAPI_TEST_TVM_CLIENT_ID
        configs_api_host = "api-aabadmin44.n.yandex-team.ru"
    else:
        configs_api_tvm_id = CONFIGSAPI_TEST_TVM_CLIENT_ID
        configs_api_host = "preprod.aabadmin.yandex.ru"

    logger.info("ConfigsApiUrl {}".format(configs_api_host))
    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configs_api_tvm_id),
    )

    tvm_client = TvmClient(tvm_settings)
    if not os.getenv("LOCAL_RUN"):
        s3_client = S3Uploader(logger)
    else:
        s3_client = None

    request_id_to_case = {}
    adb_bits_to_case = {}
    checks_results_queue = Queue()
    main()
    try:
        verdict_task(configs_api_host, tvm_client, logger)
    except Exception as e:
        logger.info("Exception")
        logger.info(e)
        logger.error(e, exc_info=True)
