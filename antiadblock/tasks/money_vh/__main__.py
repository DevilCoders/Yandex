import argparse
from datetime import timedelta, datetime as dt
from os import getenv

from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
from yt.wrapper import YtClient
from yql.api.v1.client import YqlClient
import statface_client

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import reformat_df_fielddate
from antiadblock.tasks.tools.yt_utils import get_available_yt_cluster
import antiadblock.tasks.tools.common_configs as configs

logger = create_logger('money_vh_logger')

YT_CLUSTER = 'hahn'
YT_VH_MAP_PATH = 'home/antiadb/monitorings/vh_map/'
templates = NativeEnvironment(loader=DictLoader({
    "vh": bytes.decode(resource.find("vh")),
    "vh_map": bytes.decode(resource.find("vh_map")),
}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for calculating antiadblock money and uploading results to Stat & Solomon')
    parser.add_argument('--test',
                        action='store_true',
                        help='Will send results to Stat & Solomon testing if enabled')
    parser.add_argument('--daily_logs',
                        action='store_true',
                        help='If endbled yql request will use logs/bs-chevent-log/1d istead of logs/bs-chevent-log/stream/5min')
    parser.add_argument('--update_vh_map',
                        action='store_true',
                        help='If enabled wont use cache for vh banners to impid/pageid map')
    parser.add_argument('--range',
                        nargs=2,
                        metavar='xxxx-xx-xx*',
                        help='Range of log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S')

    args = parser.parse_args()
    yt_token = getenv('YT_TOKEN')
    assert yt_token is not None
    stat_token = getenv('STAT_TOKEN')
    assert stat_token is not None
    stat_host = statface_client.STATFACE_BETA if args.test else statface_client.STATFACE_PRODUCTION

    yt_cluster = get_available_yt_cluster(YT_CLUSTER, getenv('AVAILABLE_YT_CLUSTERS'))

    daily_logs = args.daily_logs or getenv('DAILY_LOGS')
    log = '1d' if daily_logs else 'stream/5min'
    table_name_fmt = configs.YT_TABLES_DAILY_FMT if daily_logs else configs.YT_TABLES_STREAM_FMT
    time_range = 24 if daily_logs else configs.HOURS_DELTA  # in hours

    if daily_logs:
        start_day = getenv('START_DAY')
        end_day = getenv('END_DAY')
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

    yt_client = YtClient(token=yt_token, proxy=yt_cluster)
    yql_client = YqlClient(token=yt_token, db=yt_cluster)
    stat_client = statface_client.StatfaceClient(host=stat_host, oauth_token=stat_token)

    vh_map_table = '//' + YT_VH_MAP_PATH + dt.utcnow().strftime(configs.YT_TABLES_DAILY_FMT)
    if args.update_vh_map or not yt_client.exists(vh_map_table):
        logger.info(f'Trying to commit Antiadblock vh-map table: {vh_map_table}')
        query = templates.get_template('vh_map').render(file=__file__, vh_map_path=YT_VH_MAP_PATH)
        query = 'PRAGMA yt.Pool="antiadb";\n' + query
        yql_result = yql_client.query(query, syntax_version=1, title=f'VHMAP {__file__.split("/", 1)[1]} YQL').run().get_results()
        logger.info(yql_result)

    query = templates.get_template('vh').render(
        file=__file__, start=start, end=end, logs_scale=log, table_name_fmt=table_name_fmt,
        vh_map_path=YT_VH_MAP_PATH
    )
    query = 'PRAGMA yt.Pool="antiadb";\nPRAGMA yt.AutoMerge="disabled";\n' + query
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')
    yql_response = yql_request.run()
    logger.info('Waiting "money_vh" yql request')
    df = yql_response.full_dataframe
    df = df[df.ts.apply(lambda t: start <= dt.fromtimestamp(t) <= end.replace(hour=23, minute=50))]
    df['fielddate'] = df.ts.apply(lambda t: dt.fromtimestamp(t).strftime(configs.STAT_FIELDDATE_I_FMT))
    del df['ts']
    logger.info(f'Results dataframe {df.shape}:\n{df.head()}\n\n')

    report = stat_client.get_report(configs.STAT_VH_REPORT)
    detailed_report = stat_client.get_report(configs.STAT_DETAILED_VH_REPORT)

    def gen_stat_data(frame):
        stat_data = []
        for aggregation in (['fielddate', 'category_id'],
                            ['fielddate', 'category_id', 'device'],
                            ['fielddate'],
                            ['fielddate', 'device']):
            agg = frame.groupby(aggregation).sum().reset_index()
            if 'category_id' in aggregation:
                agg['category_id'] = agg.category_id.apply(str)
            else:
                agg['category_id'] = '_total'
            if 'device' not in aggregation:
                agg['device'] = 'any'
            stat_data.extend(map(lambda row: dict(row[1]), agg.iterrows()))
        return stat_data

    def gen_detailed_stat_data(frame):
        stat_data = []
        logger.info('gen_detailed_stat_data total')
        agg = frame.groupby(['fielddate']).sum().reset_index()
        agg['slice'] = '\ttotal\t'
        stat_data.extend(map(lambda row: dict(row[1]), agg.iterrows()))

        for field in ('category_id', 'device'):
            logger.info(f'gen_detailed_stat_data total by {field}')
            agg = frame.groupby(['fielddate', field]).sum().reset_index()
            agg['slice'] = agg.apply(lambda row: ['total', field, row[field]], axis=1)
            del agg[field]
            stat_data.extend(map(lambda row: dict(row[1]), agg.iterrows()))
        return stat_data

    for rep, func in [(report, gen_stat_data), (detailed_report, gen_detailed_stat_data)]:
        for scale in configs.Scales:
            if scale == configs.Scales.day and not daily_logs:
                continue
            logger.info(f'Trying to upload results to Stat report {rep} (scale {scale})')
            data = func(df) if scale == configs.Scales.minute else func(reformat_df_fielddate(df, configs.STAT_FIELDDATE_FMT[scale]))
            upload_result = rep.upload_data(scale=scale.value, data=data)
            logger.info(upload_result)
