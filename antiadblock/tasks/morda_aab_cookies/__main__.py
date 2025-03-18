import requests
from os import getenv
from enum import Enum
from datetime import timedelta, datetime as dt

import click
from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment
import pandas as pd

from library.python import resource
from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import chuncker
import antiadblock.tasks.tools.common_configs as configs
from antiadblock.tasks.tools.configs_api import get_configs


logger = create_logger('')
templates = NativeEnvironment(loader=DictLoader({"cookies.sql": bytes.decode(resource.find("cookies.sql"))}))
MORDA_ACCESS_LOGS = 'logs/morda-access-log'
SOLOMON_API_URL = configs.SOLOMON_PUSH_API + 'project=Antiadblock&service=morda_aab_cookies&cluster={cluster}'
Cookies = Enum('Cookies', names=['bltsr', 'cookie_of_the_day', 'not_adb'])


def get_whitelist_cookies(service_id, tvm_secret):
    configs_api_host = getenv("CONFIGS_API_HOST", 'api.aabadmin.yandex.ru')
    tvm_id = int(getenv('TVM_ID', '2002631'))  # "Sandbox monitoring" tvm_id as default
    configsapi_tvm_id = int(getenv('CONFIGSAPI_TVM_ID', '2000629'))

    logger.info(f'Trying get configs from: {configs_api_host}')
    configs = get_configs(tvm_id, tvm_secret, configsapi_tvm_id, configs_api_host)
    return configs[service_id]['config']['WHITELIST_COOKIES']


@click.command()
@click.option("--test", default=False, is_flag=True, help="Use Solomon claster push-test for uploading results")
@click.option("--daily_logs", default=False, is_flag=True, help="If endbled YQL request will use logs/1d istead of logs/stream/5min")
@click.option("--logs_range", nargs=2, metavar='xxxx-xx-xx*', help="Range of log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S")
def main(test, daily_logs, logs_range):
    yt_token = getenv('YT_TOKEN')
    assert yt_token is not None
    solomon_token = getenv('SOLOMON_TOKEN')
    assert solomon_token is not None
    tvm_secret = getenv('TVM_SECRET')
    assert tvm_secret is not None
    daily_logs = daily_logs or bool(getenv('DAILY_LOGS'))

    logs_scale = '1d' if daily_logs else 'stream/5min'
    table_name_fmt = configs.YT_TABLES_DAILY_FMT if daily_logs else configs.YT_TABLES_STREAM_FMT

    if logs_range:
        start, end = map(lambda t: dt.strptime(t, table_name_fmt), logs_range)
        assert start <= end
    else:
        end = dt.now().replace(minute=0, second=0, microsecond=0)
        end = end.replace(hour=0) if daily_logs else end + timedelta(hours=1)
        start = end - timedelta(hours=2 * 24 if daily_logs else 3)

    cookies = get_whitelist_cookies('yandex_morda', tvm_secret)

    yql_client = YqlClient(token=yt_token, db=configs.YT_CLUSTER)
    query = templates.get_template("cookies.sql").render(
        start=start,
        end=end,
        logs_base_path=MORDA_ACCESS_LOGS,
        logs_scale=logs_scale,
        table_name_fmt=table_name_fmt,
        cookies=cookies,
        not_adb_name=Cookies.not_adb.name,
        file=__file__,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info(f'trying to run query:\n{query}')
    yql_response = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL').run()
    results = []
    for table in yql_response.get_results():
        df = table.full_dataframe
        df.loc[:, 'time'] = pd.to_datetime(df['time']) - timedelta(hours=3)
        df['ts'] = df.time.apply(lambda f: f.strftime(configs.SOLOMON_TS_H_FMT))
        results.append(df)
    logger.info(f'got from yql:\n{results}')
    cotd_vs_bltsr_df, flatten_by_cotd_df = results

    sensors = []
    metric_types = ('users', 'requests')
    for metric in metric_types:
        cotd_vs_bltsr_df[f'{Cookies.cookie_of_the_day.name}_{metric}'] = cotd_vs_bltsr_df[f'adb_{metric}'] - cotd_vs_bltsr_df[f'{Cookies.bltsr.name}_{metric}']
        cotd_vs_bltsr_df[f'{Cookies.not_adb.name}_{metric}'] = cotd_vs_bltsr_df[metric] - cotd_vs_bltsr_df[f'adb_{metric}']
        for c in Cookies:
            row_parser = lambda row: dict(ts=row.ts, value=row[f'{c.name}_{metric}'], labels=dict(sensor=f'{metric}_cookie_of_the_day_vs_bltsr', device=row.device, cookie=c.name))
            sensors.extend(cotd_vs_bltsr_df.apply(row_parser, axis=1).tolist())

        row_parser = lambda row: dict(ts=row.ts, value=row[metric], labels=dict(sensor=f'{metric}_by_cookie', device=row.device, cookie=row.cookie))
        sensors.extend(flatten_by_cotd_df.apply(row_parser, axis=1).tolist())

    for chunk in chuncker(sensors):
        upload_result = requests.post(
            url=SOLOMON_API_URL.format(cluster='push-test' if test else 'push'),
            headers={'Content-Type': 'application/json', 'Authorization': 'OAuth {}'.format(solomon_token)},
            json={'sensors': chunk}
        )
        logger.info(upload_result.text)
