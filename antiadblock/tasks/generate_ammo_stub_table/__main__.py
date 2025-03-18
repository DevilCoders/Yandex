# coding=utf-8
# утилита для подготовки таблички для ШМ из логов NGINX и CRYPROX

import os
import httplib
import cPickle as pickle
from datetime import timedelta, datetime as dt

import tornado  # noqa
import tornado.httputil  # noqa

import yt.wrapper
from library.python.yt.ttl import set_ttl

from antiadblock.tasks.tools.common_configs import YT_TABLES_DAILY_FMT
from antiadblock.tasks.tools.logger import create_logger

logger = create_logger('generate_ammo_stub_logger')

YT_CLUSTER = 'hahn'  # logs is Hahn only
DIRECTORY = '//home/antiadb/ammo_stubs'
X_AAB_REQUEST_ID = 'X-Aab-Requestid'
EXCLUDED_HEADERS = ["transfer-encoding", "content-encoding", "content-length"]


@yt.wrapper.with_context
def logs_reducer(key, input_row_iterator, context):
    data = {}
    http_version = "1.0"
    for input_row in input_row_iterator:
        if context.table_index == 0:
            # логи NGINX //home/logfeller/logs/antiadb-nginx-request-log/1d/...
            # 4xx|5xx ошибки отбрасываем (мешают вычислить capacity в ШМ)
            if input_row['response_code'] >= 400:
                continue
            data['unixtime'] = int(input_row['timestamp'] // 1000)
            data['GraphID'] = input_row['request_id']
            data[X_AAB_REQUEST_ID] = input_row['request_id']
            data['HttpStatus'] = input_row['response_code']
            data['Request'] = input_row['request']
            http_version = input_row['request'].split('\r\n', 1)[0][-3:]
            assert http_version in ("1.0", "1.1", "2.0")
            response = "HTTP/{} {} {}\r\n".format(http_version, input_row['response_code'], httplib.responses.get(input_row['response_code'], "Unknown"))
            if input_row['is_bamboozled'] == "true":
                data['Tag'] = "bamboozled"
                data['Handler'] = "nginx"
                response += "Content-Length: 0\r\n\r\n"
            elif input_row['is_accel_redirect'] == "1":
                if input_row["response_headers"]:
                    response += "\r\n".join(["{}: {}".format(k, v) for k, v in input_row["response_headers"].items() if k.lower() not in EXCLUDED_HEADERS])
                    response += "\r\n"
                if input_row["response_body"]:
                    response += "\r\n" + input_row["response_body"]
                response += "\r\n"
                data['Tag'] = "accel_redirect"
                data['Handler'] = "nginx"
            else:
                response = ""
            if response:
                data['Response'] = response

        elif context.table_index == 1:
            # Логи CRYPROX //home/logfeller/logs/antiadb-cryprox-response-log/1d/...
            if input_row['request_id']:
                code, reason = pickle.loads(input_row["status_code"])
                body, h = pickle.loads(input_row["response"])
                headers = "\r\n".join(["{}: {}".format(k, v) for k, v in h.items() if k.lower() not in EXCLUDED_HEADERS])
                data['Response'] = "HTTP/{} {} {}\r\n{}\r\n\r\n{}\r\n".format(http_version, code, reason, headers, body)
                data['Handler'] = "cryprox"
                data['Tag'] = "cryprox"
        else:
            # такого индекса быть не может
            raise RuntimeError("Unknown table index")
    if data.get(X_AAB_REQUEST_ID):
        yield data


def load_to_func_mapper(input_row):
    if input_row['Handler'] == 'cryprox':
        yield input_row


if __name__ == "__main__":
    yt_token = os.getenv('YT_TOKEN')

    date = dt.now().replace(hour=0, minute=0, second=0, microsecond=0)
    date = date - timedelta(days=1)

    yt_client = yt.wrapper.YtClient(token=yt_token, proxy=YT_CLUSTER)

    ammo_stubs_cryprox_load_table = "{}/ammo_stubs_cryprox_load_table".format(DIRECTORY)
    ammo_stubs_cryprox_func_table = "{}/ammo_stubs_cryprox_func_table".format(DIRECTORY)
    is_func_stub = {ammo_stubs_cryprox_load_table: False, ammo_stubs_cryprox_func_table: True}
    request_table = "//home/logfeller/logs/antiadb-nginx-request-log/1d/{}".format(date.strftime(YT_TABLES_DAILY_FMT))
    response_table = "//home/logfeller/logs/antiadb-cryprox-response-log/1d/{}".format(date.strftime(YT_TABLES_DAILY_FMT))
    if not yt_client.exists(request_table) or not yt_client.exists(response_table):
        raise Exception('Table with logs is not exists')
    # схема для таблички взята здесь
    # https://yt.yandex-team.ru/hahn/navigation?path=//home/yabs-cs-sandbox/gen_sandbox_stub/example/table&
    # поле GraphNode пока никак не используется
    schema = [
        {"name": "unixtime", "type": "uint64"},
        {"name": "GraphID", "type": "string"},
        {"name": "GraphNode", "type": "string"},
        {"name": "HttpStatus", "type": "uint16"},
        {"name": "Request", "type": "string"},
        {"name": "Tag", "type": "string"},
        {"name": "Handler", "type": "string"},
        {"name": "Response", "type": "string"},
        {"name": X_AAB_REQUEST_ID, "type": "string"},
    ]
    for path_to_table in (ammo_stubs_cryprox_load_table, ammo_stubs_cryprox_func_table):
        yt_client.create("table", path=path_to_table, force=True, attributes={"schema": schema, "dynamic": False})
        yt_client.set_attribute(path_to_table, "request_key_headers", [X_AAB_REQUEST_ID])
        yt_client.set_attribute(path_to_table, "cachedaemon_key_headers", [X_AAB_REQUEST_ID])
        set_ttl(yt_client, path_to_table, days=7)

    yt_client.run_reduce(
        logs_reducer,
        source_table=[request_table, response_table],
        destination_table=ammo_stubs_cryprox_load_table,
        reduce_by=["request_id"],
        job_io={"table_writer": {"max_row_weight": 64 * 2**20}},  # 64Mb

    )
    logger.info("Output table: https://yt.yandex-team.ru/{}/#page=navigation&offsetMode=row&path={}".format(YT_CLUSTER, ammo_stubs_cryprox_load_table))

    yt_client.run_map(
        load_to_func_mapper,
        source_table=ammo_stubs_cryprox_load_table,
        destination_table=ammo_stubs_cryprox_func_table,
        job_io={"table_writer": {"max_row_weight": 64 * 2 ** 20}},  # 64Mb
    )
    logger.info("Output table: https://yt.yandex-team.ru/{}/#page=navigation&offsetMode=row&path={}".format(YT_CLUSTER, ammo_stubs_cryprox_func_table))
