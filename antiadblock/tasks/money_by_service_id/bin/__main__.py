import os
import json
from datetime import timedelta, datetime as dt

import click
import requests
from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
from yt.wrapper import YtClient
from yql.api.v1.client import YqlClient
import statface_client

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import chuncker
from antiadblock.tasks.tools.yt_utils import get_available_yt_cluster
import antiadblock.tasks.tools.common_configs as configs
from antiadblock.tasks.money_by_service_id.lib.lib import Columns, get_dataframe, consume_dataframe

logger = create_logger('money_by_service_id_logger')
templates = NativeEnvironment(loader=DictLoader({
    "pageids": bytes.decode(resource.find("pageids")),
    "bschevent": bytes.decode(resource.find("bschevent")),
    "eventbad": bytes.decode(resource.find("eventbad")),
    "bschevent_only_blocks": bytes.decode(resource.find("bschevent_only_blocks")),
}))
SOLOMON_API_URL = configs.SOLOMON_PUSH_API + 'project=Antiadblock&service=chevent_stats&cluster={cluster}'


def get_last_table(yt, path):
    return os.path.join(path, sorted(yt.list(path))[-1])


@click.command()
@click.option("--test", default=False, is_flag=True, help="Will send results to Stat & Solomon testing if enabled")
@click.option("--daily_logs", default=False, is_flag=True, help="If endbled YQL request will use logs/1d istead of logs/stream/5min")
@click.option("--logs_range", nargs=2, metavar='xxxx-xx-xx*', help="Range of log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S")
@click.option("--update_pageids", default=False, is_flag=True, help="If enabled wont use cache for service_id to pageids map")
def main(test, daily_logs, logs_range, update_pageids):
    yt_token = os.getenv('YT_TOKEN')
    assert yt_token is not None
    stat_token = os.getenv('STAT_TOKEN')
    assert stat_token is not None
    solomon_token = os.getenv('SOLOMON_TOKEN')
    assert solomon_token is not None
    daily_logs = daily_logs or os.getenv('DAILY_LOGS')

    yt_cluster = get_available_yt_cluster(configs.YT_CLUSTER, os.getenv('AVAILABLE_YT_CLUSTERS'))

    stat_host = statface_client.STATFACE_BETA if test else statface_client.STATFACE_PRODUCTION
    solomon_cluster = 'push-test' if test else 'push'

    log = '1d' if daily_logs else 'stream/5min'
    table_name_fmt = configs.YT_TABLES_DAILY_FMT if daily_logs else configs.YT_TABLES_STREAM_FMT
    time_range = 2 * 24 if daily_logs else configs.HOURS_DELTA  # in hours

    if daily_logs:
        start_day = os.getenv('START_DAY')
        end_day = os.getenv('END_DAY')
    else:
        start_day, end_day = None, None

    if logs_range:
        start, end = map(lambda t: dt.strptime(t, table_name_fmt), logs_range)
        assert start <= end
    elif start_day is not None and end_day is not None:
        start = dt.strptime(start_day, table_name_fmt)
        end = dt.strptime(end_day, table_name_fmt)
        assert start <= end
    else:
        end = dt.now().replace(minute=0, second=0, microsecond=0)
        end = end.replace(hour=0) if daily_logs else end + timedelta(hours=1)
        start = end - timedelta(hours=time_range)

    yt_client = YtClient(token=yt_token, proxy=yt_cluster)
    yql_client = YqlClient(token=yt_token, db=yt_cluster)
    stat_client = statface_client.StatfaceClient(host=stat_host, oauth_token=stat_token)
    report = stat_client.get_report(configs.CHEVENT_STAT_MONEY_REPORT)

    partners_pageids_table = configs.YT_ANTIADB_PARTNERS_PAGEIDS_PATH + '/' + dt.utcnow().strftime(configs.YT_TABLES_DAILY_FMT)
    if update_pageids or not yt_client.exists(partners_pageids_table):
        logger.info(f'Trying to commit Antiadblock partners pageids table: {partners_pageids_table}')
        query = templates.get_template('pageids').render(
            file=__file__,
            partners_pageids_table=partners_pageids_table,
            service_id_to_pagename=json.dumps(configs.SERVICE_ID_TO_PAGEID_NAMES),
            service_id_to_custom_pagename=json.dumps(configs.SERVICE_ID_TO_CUSTOM_PAGEID_NAMES),
        )
        query = 'PRAGMA yt.Pool="antiadb";\n' + query
        yql_result = yql_client.query(query, syntax_version=1, title=f'PAGEIDS {__file__.split("/", 1)[1]} YQL').run().get_results()
        logger.info(yql_result)

    partners_pageids_table = get_last_table(yt_client, configs.YT_ANTIADB_PARTNERS_PAGEIDS_PATH)
    turbo_pageid_impid_table = get_last_table(yt_client, configs.YT_TURBO_PAGEID_IMPID_PATH)
    query = templates.get_template('bschevent').render(
        file=__file__, start=start, end=end, logs_scale=log, table_name_fmt=table_name_fmt, columns=Columns,
        partners_pageids_table=partners_pageids_table,
        turbo_pageid_impid_table=turbo_pageid_impid_table,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info(f'Trying to run query:\n{query}\n\n')
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')
    bschevent_yql = yql_request.run()

    query = templates.get_template('bschevent_only_blocks').render(
        file=__file__, start=start, end=end, logs_scale=log, table_name_fmt=table_name_fmt, columns=Columns,
        service_id_to_block=json.dumps(configs.SERVICE_ID_TO_CUSTOM_BLOCKID_NAMES),
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info(f'Trying to run query:\n{query}\n\n')
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')
    bschevent_only_blocks_yql = yql_request.run()

    query = templates.get_template('eventbad').render(
        file=__file__, start=start, end=end, logs_scale=log, table_name_fmt=table_name_fmt, columns=Columns,
        partners_pageids_table=partners_pageids_table,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info(f'Trying to run query:\n{query}\n\n')
    yql_request = yql_client.query(query, syntax_version=1, title=f'BAD {__file__.split("/", 1)[1]} YQL')
    eventbad_yql = yql_request.run()

    eventbad_dataframe = None
    try:
        eventbad_dataframe = eventbad_yql.full_dataframe
    except:
        logger.warning('Event bad YQL is failed')

    bschevent_only_blocks_dataframe = None
    try:
        bschevent_only_blocks_dataframe = bschevent_only_blocks_yql.full_dataframe
    except:
        logger.warning('Chevent only blocks YQL is failed')

    dataframe = get_dataframe(bschevent_yql.full_dataframe, bschevent_only_blocks_dataframe, eventbad_dataframe)
    logger.info(f'shape: {dataframe.shape}\n{dataframe.head()}\n\n')
    logger.info(f'drop all dates before {start}')
    dataframe.sort_index(level=Columns.date.name, inplace=True)
    dataframe = dataframe[start:]
    logger.info(f'shape: {dataframe.shape}\n{dataframe.head()}\n\n')

    for scale in configs.Scales:
        if scale == configs.Scales.day and not daily_logs:
            continue

        stat_data, sensors = consume_dataframe(dataframe, scale=scale)
        logger.info(f'Trying to upload results to Stat report {report} (scale {scale}), {len(stat_data)} rows of data:\n{stat_data[:10]}\n\n')
        for chunk in chuncker(stat_data, chunk_size=100000):
            upload_result = report.upload_data(scale=scale.value, data=chunk)
            logger.info(upload_result)
            logger.info(f'Response: {upload_result}\n')
        logger.info(f'Trying to upload results to Solomon (scale {scale}), {len(sensors)} rows of data:\n{sensors[:10]}\n\n')
        for chunk in chuncker(sensors):
            upload_result = requests.post(
                url=SOLOMON_API_URL.format(cluster=solomon_cluster),
                headers={'Content-Type': 'application/json', 'Authorization': 'OAuth {}'.format(solomon_token)},
                json=dict(sensors=chunk, commonLabels=dict(scale=configs.SOLOMON_SCALES_MAP[scale]))
            )
            logger.info(upload_result.text)
