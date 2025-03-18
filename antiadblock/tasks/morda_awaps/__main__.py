# encoding=utf8
# Исходник бинаря для подсчета статистики АВАПСА на Морде
import argparse
from datetime import timedelta, datetime as dt
from os import getenv

import requests
from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
from yt.wrapper import YtClient
from yql.api.v1.client import YqlClient
import statface_client

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import chuncker, reformat_df_fielddate
import antiadblock.tasks.tools.common_configs as configs

logger = create_logger('yandex_morda_awaps_logger')
templates = NativeEnvironment(loader=DictLoader({"morda_awaps.sql": bytes.decode(resource.find("morda_awaps.sql"))}))
YT_CLUSTER = 'hahn'  # logs/awaps-log is Hahn only


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for calculating antiadblock money and uploading results to Stat')
    parser.add_argument('--test',
                        action='store_true',
                        help='Will send results to Stat-beta')
    parser.add_argument('--daily_logs',
                        action='store_true',
                        help='If endbled yql request will use logs/awaps-log/1d istead of logs/awaps-log/stream/5min')
    parser.add_argument('--range',
                        nargs=2,
                        metavar='xxxx-xx-xx*',
                        help='Range of log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S')

    args = parser.parse_args()
    yt_token = getenv('YT_TOKEN')
    assert yt_token is not None
    stat_token = getenv('STAT_TOKEN')
    assert stat_token is not None
    solomon_token = getenv('SOLOMON_TOKEN')
    assert solomon_token is not None
    stat_host = statface_client.STATFACE_BETA if args.test else statface_client.STATFACE_PRODUCTION

    daily_logs = args.daily_logs or bool(getenv('DAILY_LOGS'))
    log = '1d' if daily_logs else 'stream/5min'
    table_name_fmt = configs.YT_TABLES_DAILY_FMT if daily_logs else configs.YT_TABLES_STREAM_FMT
    time_range = 2 * 24 if daily_logs else 6  # in hours

    if args.range:
        start, end = map(lambda t: dt.strptime(t, table_name_fmt), args.range)
        assert start <= end
    else:
        end = dt.now().replace(minute=0, second=0, microsecond=0)
        end = end.replace(hour=0) if daily_logs else end + timedelta(hours=1)
        start = end - timedelta(hours=time_range)

    yt_client = YtClient(token=yt_token, proxy=YT_CLUSTER)
    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)
    stat_client = statface_client.StatfaceClient(host=stat_host, oauth_token=stat_token)

    query = templates.get_template("morda_awaps.sql").render(start=start, end=end, logs_scale=log, file=__file__, table_name_fmt=table_name_fmt)
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')
    yql_response = yql_request.run()
    logger.info('Waiting "money from awaps-log" yql request')
    df = yql_response.full_dataframe
    df = df[df.fielddate.map(lambda t: dt.strptime(t, configs.STAT_FIELDDATE_I_FMT)) >= start]
    logger.info('Results dataframe size: ' + str(len(df)))

    old_report = stat_client.get_report(configs.OLD_STAT_MONEY_REPORT)
    actions_report = stat_client.get_report(configs.MORDA_ACTIONS_REPORT)
    morda_awaps_report = stat_client.get_report(configs.MORDA_AWAPS_MONEY_REPORT)

    logger.info("Uploading data to old stat report")
    logger.info('Trying to upload results to Stat report (scale=i):')
    old_df = df.query('actionid=="0"').rename(index=str, columns={'money': 'total_money'})[['fielddate', 'aab_money', 'total_money']]
    old_df['domain'] = 'yandex_morda'
    logger.info(f'old_df:\n{old_df.iloc[:2]}')
    upload_result = old_report.upload_data(scale='i', data=old_df.to_dict('records'))
    logger.info(upload_result)

    logger.info("Uploading data to morda awaps actions stat report")
    logger.info('Trying to upload results to Stat report (scale=i):')
    actions_df = df.copy()
    del actions_df['money']
    del actions_df['aab_money']
    upload_result = actions_report.upload_data(scale='i', data=actions_df.to_dict('records'))
    logger.info(upload_result)

    logger.info('Uploading data to Solomon')
    actions_df['ts'] = df.fielddate.apply(lambda f: (dt.strptime(f, configs.STAT_FIELDDATE_I_FMT) - timedelta(hours=3)).strftime(configs.SOLOMON_TS_I_FMT))
    logger.info(f'actions_df:\n{actions_df.iloc[:2]}')
    sensors = []
    for metric in ('aab_count', 'count'):
        row_parser = lambda r: dict(ts=r.ts, value=r[metric], labels=dict(sensor=metric, action=r.actionid))
        sensors.extend(actions_df.apply(row_parser, axis=1).tolist())
    logger.info(f'We have {len(sensors)} sensors to upload, first 10:\n{sensors[:10]}')
    for chunk in chuncker(sensors):
        upload_result = requests.post(
            url=configs.SOLOMON_PUSH_API + 'project=Antiadblock&service=morda_stats&cluster=push',
            headers={'Content-Type': 'application/json', 'Authorization': 'OAuth {}'.format(solomon_token)},
            json=dict(sensors=chunk, commonLabels=dict(host='cluster', service_id='yandex_morda'))
        )
        logger.info(upload_result.text)

    logger.info("Uploading data to morda awaps money stat report")
    money_df = df.query('actionid=="0"').rename(index=str, columns={'count': 'shows',
                                                                    'aab_count': 'aab_shows',
                                                                    'non_commercial_count': 'non_commercial_shows',
                                                                    'aab_non_commercial_count': 'aab_non_commercial_shows'})
    del money_df['actionid']

    for scale in configs.Scales:
        if scale == configs.Scales.day and not daily_logs:
            continue
        logger.info(f'Trying to upload results to Stat report {morda_awaps_report} (scale {scale})')
        money_df_agg = money_df
        if scale != configs.Scales.minute:
            money_df_agg = reformat_df_fielddate(money_df, configs.STAT_FIELDDATE_FMT[scale]).groupby(['fielddate']).sum().reset_index()
        logger.info(f'money_df_agg:\n{money_df_agg.iloc[:2]}')
        upload_result = morda_awaps_report.upload_data(scale=scale.value, data=money_df_agg.to_dict('records'))
        logger.info(upload_result)
