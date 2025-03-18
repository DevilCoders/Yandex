# encoding=utf8
# Исходник бинаря для подсчета статистики Антиадблока в InApp
import os
import argparse
from datetime import timedelta, datetime as dt

from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
import yt.wrapper as yt
from yql.api.v1.client import YqlClient
import statface_client

from antiadblock.tasks.tools.common_configs import (YT_TABLES_DAILY_FMT, YT_TABLES_STREAM_FMT,
                                                    STAT_FIELDDATE_I_FMT, STAT_FIELDDATE_FMT,
                                                    STAT_INAPP_REPORT, STAT_INAPP_DETAILED_REPORT, Scales)
from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import reformat_df_fielddate

logger = create_logger('money_inapp')

templates = NativeEnvironment(loader=DictLoader({
    "inapp": bytes.decode(resource.find("inapp")),
}))

# nanpu-event-log exists only on hahn
YT_CLUSTER = 'hahn'


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for calculating antiadblock money and uploading results to Stat')
    parser.add_argument('--test',
                        action='store_true',
                        help='Will send results to Stat & Solomon testing if enabled')
    parser.add_argument('--daily_logs',
                        action='store_true',
                        help='If endbled yql request will use .../1d instead of .../30min')
    parser.add_argument('--range',
                        nargs=2,
                        metavar='xxxx-xx-xx*',
                        help='Range of log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S')

    args = parser.parse_args()
    yt_token = os.getenv('YT_TOKEN')
    assert yt_token is not None
    stat_token = os.getenv('STAT_TOKEN')
    assert stat_token is not None
    daily_logs = args.daily_logs or os.getenv('DAILY_LOGS')

    stat_host = statface_client.STATFACE_BETA if args.test else statface_client.STATFACE_PRODUCTION
    log = '1d' if daily_logs else '30min'
    table_name_fmt = YT_TABLES_DAILY_FMT if daily_logs else YT_TABLES_STREAM_FMT
    time_range = 24 if daily_logs else 6  # in hours

    if daily_logs:
        start_day = os.getenv('START_DAY')
        end_day = os.getenv('END_DAY')
    else:
        start_day, end_day = None, None

    if args.range:
        start, end = map(lambda t: dt.strptime(t, table_name_fmt), args.range)
        assert start <= end
    elif start_day is not None and end_day is not None:
        start = dt.strptime(start_day, table_name_fmt)
        end = dt.strptime(end_day, table_name_fmt)
        assert start <= end
    else:
        end = dt.now().replace(minute=0, second=0, microsecond=0)
        end = end.replace(hour=0) if daily_logs else end + timedelta(hours=1)
        start = end - timedelta(hours=time_range)

    yt_client = yt.YtClient(token=yt_token, proxy=YT_CLUSTER)
    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)
    stat_client = statface_client.StatfaceClient(host=stat_host, oauth_token=stat_token)
    report = stat_client.get_report(STAT_INAPP_REPORT)
    detailed_report = stat_client.get_report(STAT_INAPP_DETAILED_REPORT)

    query = templates.get_template('inapp').render(
        file=__file__, start=start, end=end, logs_scale=log, table_name_fmt=table_name_fmt,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')
    yql_response = yql_request.run()
    logger.info('Waiting yql request')
    df = yql_response.full_dataframe
    df = df[df.fielddate.map(lambda t: dt.strptime(t, STAT_FIELDDATE_I_FMT)) >= start]
    logger.info('Results dataframe size: ' + str(len(df)))

    def gen_stat_data(df):
        stat_data = []
        agg_df = df.groupby(['fielddate']).sum().reset_index()
        agg_df['slice'] = '\ttotal\t'
        stat_data.extend(map(lambda row: dict(row[1]), agg_df.iterrows()))

        agg_df = df.groupby(['fielddate', 'osname']).sum().reset_index()
        agg_df['slice'] = agg_df.apply(lambda row: ['total', row.osname if row.osname else 'os_empty'], axis=1)
        del agg_df['osname']
        stat_data.extend(map(lambda row: dict(row[1]), agg_df.iterrows()))

        agg_df = df.groupby(['fielddate', 'version']).sum().reset_index()
        agg_df['slice'] = agg_df.apply(lambda row: ['total', row.version if row.version else 'version_empty'], axis=1)
        del agg_df['version']
        stat_data.extend(map(lambda row: dict(row[1]), agg_df.iterrows()))

        agg_df = df.groupby(['fielddate', 'osname', 'version']).sum().reset_index()
        agg_df['slice'] = agg_df.apply(lambda row: ['total', row.osname if row.osname else 'os_empty', row.version if row.version else 'version_empty'], axis=1)
        del agg_df['osname']
        del agg_df['version']
        stat_data.extend(map(lambda row: dict(row[1]), agg_df.iterrows()))

        return stat_data

    def gen_detailed_stat_data(df):
        stat_data = []
        agg_df = df.groupby(['fielddate']).sum().reset_index()
        agg_df['slice'] = '\ttotal\t'
        stat_data.extend(map(lambda row: dict(row[1]), agg_df.iterrows()))

        agg_df = df.groupby(['fielddate', 'appid']).sum().reset_index()
        agg_df['slice'] = agg_df.apply(lambda row: ['total', row.appid], axis=1)
        del agg_df['appid']
        stat_data.extend(map(lambda row: dict(row[1]), agg_df.iterrows()))

        return stat_data

    for rep, func in [(report, gen_stat_data), (detailed_report, gen_detailed_stat_data)]:
        for scale in Scales:
            if scale == Scales.day and not daily_logs:
                continue
            logger.info(f'Trying to upload results to Stat report {rep} (scale {scale})')
            data = func(df) if scale == Scales.minute else func(reformat_df_fielddate(df, STAT_FIELDDATE_FMT[scale]))
            upload_result = rep.upload_data(scale=scale.value, data=data)
            logger.info(upload_result)
