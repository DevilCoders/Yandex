# -*- coding: utf8 -*-
import argparse
from datetime import timedelta, datetime as dt
from os import getenv
from functools import partial

import pandas as pd
import requests
from retry import retry
import statface_client

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import chuncker
from antiadblock.tasks.tools.misc import reformat_df_fielddate
import antiadblock.tasks.tools.common_configs as configs

logger = create_logger('bs_dsp_money_logger')

AGG_COLS = ['fielddate', 'service_id', 'device', 'dsp']  # order is important - dsp must be last
DATA_COLS = ['aab_money', 'money']
# some usefull aggregation over service_id, that will be included in final report as a fictional partner
DF_AGG_QUERIES = {
    '_inner':  'service_id in @configs.INNER_SERVICE_IDS',
    '_outer':  'service_id not in @configs.INNER_SERVICE_IDS',
}
DF_AGG_QUERIES['_total'] = '|'.join(DF_AGG_QUERIES.values())

SOLOMON_API_URL = configs.SOLOMON_PUSH_API + 'project=Antiadb_money&service=chevent_stats&cluster={cluster}'


def get_sensors(frame):

    def parser(row, metric):
        parsed = dict(
            ts=dt.strptime(row['fielddate'], configs.STAT_FIELDDATE_I_FMT).timestamp(),
            value=row[metric],
            labels=dict(sensor=metric, service_id=row['service_id'], device=row['device']),
        )
        return parsed

    df = frame.copy(deep=True)
    del df['dsp']
    unblock = 'unblock'
    aab_money = 'aab_money'
    money = 'money'
    df = df.groupby(['fielddate', 'service_id', 'device']).sum().reset_index()
    try:
        df[unblock] = 100. * df[aab_money] / df[money]
    except Exception as e:
        logger.error(str(e))

    df.fillna(0, inplace=True)

    res = []
    try:
        res.extend(df.apply(partial(parser, metric=unblock), axis=1))
        res.extend(df.apply(partial(parser, metric=aab_money), axis=1))
        res.extend(df.apply(partial(parser, metric=money), axis=1))
    except Exception as e:
        logger.error(str(e))

    return res


@retry(tries=3, delay=1, backoff=3, exceptions=(requests.exceptions.ConnectionError, requests.exceptions.HTTPError), logger=logger)
def upload_chunk_to_solomon(chunk, solomon_cluster, solomon_token):
    upload_result = requests.post(
        url=SOLOMON_API_URL.format(cluster=solomon_cluster),
        headers={'Content-Type': 'application/json', 'Authorization': 'OAuth {}'.format(solomon_token)},
        json=dict(sensors=chunk, commonLabels=dict(scale=configs.SOLOMON_SCALES_MAP[scale]))
    )
    return upload_result.text


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for calculating antiadblock money and uploading results to Stat')
    parser.add_argument('--test',
                        action='store_true',
                        help='Will send results to Stat-beta')
    parser.add_argument('--daily_logs',
                        action='store_true',
                        help='If endbled yql request will use logs/bs-chevent-log/1d istead of logs/bs-chevent-log/stream/5min')
    parser.add_argument('--range',
                        nargs=2,
                        metavar='xxxx-xx-xx*',
                        help='Range of log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S')

    args = parser.parse_args()
    stat_token = getenv('STAT_TOKEN')
    assert stat_token is not None
    solomon_token = getenv('SOLOMON_TOKEN')
    assert solomon_token is not None

    daily_logs = args.daily_logs or bool(getenv('DAILY_LOGS'))
    table_name_fmt = configs.YT_TABLES_DAILY_FMT if daily_logs else configs.YT_TABLES_STREAM_FMT
    time_range = 2 * 24 if daily_logs else configs.HOURS_DELTA  # in hours

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

    stat_client_download = statface_client.StatfaceClient(host=statface_client.STATFACE_PRODUCTION, oauth_token=stat_token)
    stat_client_upload = statface_client.StatfaceClient(
        host=statface_client.STATFACE_BETA if args.test else statface_client.STATFACE_PRODUCTION,
        oauth_token=stat_token
    )
    solomon_cluster = 'push-test' if args.test else 'push'

    united_report = stat_client_upload.get_report(configs.UNITED_STAT_MONEY_REPORT)

    # reports with data we need to create united money report
    chevent_report = stat_client_download.get_report(configs.CHEVENT_STAT_MONEY_REPORT)
    turbo_report = stat_client_download.get_report(configs.STAT_TURBO_REPORT)
    player_report = stat_client_download.get_report(configs.STAT_DETAILED_VH_REPORT)
    games_report = stat_client_download.get_report(configs.STAT_GAMES_REPORT)
    inapp_report = stat_client_download.get_report(configs.STAT_INAPP_REPORT)

    logger.info('Downloading data from chevent Stat report:')
    chevent_df = pd.DataFrame.from_records(
        chevent_report.download_data(
            scale='i',
            date_min=start.strftime(configs.STAT_FIELDDATE_I_FMT),
            date_max=(end.replace(hour=23, minute=50, second=0, microsecond=0) if daily_logs else end).strftime(configs.STAT_FIELDDATE_I_FMT),
            service_id__lvl=2,
        )
    )
    logger.info('Downloaded dataframe size: ' + str(len(chevent_df)))
    if len(chevent_df) > 0:
        chevent_df = chevent_df[chevent_df.service_id.apply(lambda _: '\tdevice\t' in _)]  # filter stats grouped by service_id & device
        # splitting service_id column from "\tzen.yandex.ru\tdevice\tmobile\t" to two: service_id="zen.yandex.ru", device="mobile"
        splitted_col = chevent_df.service_id.apply(lambda x: x.strip().split('\t'))
        chevent_df.loc[:, 'service_id'], chevent_df.loc[:, 'device'], chevent_df.loc[:, 'dsp'] = splitted_col.apply(lambda x: x[0]), splitted_col.apply(lambda x: x[2]), 'direct'
        chevent_df = chevent_df[AGG_COLS + DATA_COLS].query('service_id != "TOTAL"')
        logger.info('chevent_df')
        logger.info(chevent_df.iloc[:2])

    def prepare_service_df(service_report, service_id, df_query, dsp='direct', device='desktop'):
        """
        функция для выкачивания данных только из тех отчетов, в которых даннные по одному service_id
        :param service_report:
        :param service_id:
        :param df_query:
        :param dsp:
        :param device:
        :return:
        """
        logger.info('Downloading data from {} Stat report:'.format(service_id))
        service_df = pd.DataFrame.from_records(
            service_report.download_data(
                scale='i',
                date_min=start.strftime(configs.STAT_FIELDDATE_I_FMT),
                date_max=(end.replace(hour=23, minute=50, second=0, microsecond=0) if daily_logs else end).strftime(configs.STAT_FIELDDATE_I_FMT),
            )
        )
        logger.info('Downloaded dataframe size: ' + str(len(service_df)))
        if len(service_df) > 0:
            service_df['service_id'], service_df['dsp'], service_df['device'] = service_id, dsp, device
            if df_query:
                service_df = service_df.query(df_query)
            service_df = service_df[AGG_COLS + DATA_COLS]
        return service_df

    yandex_player_dfs = []
    yandex_turbo_dfs = []
    yandex_games_dfs = []
    yandex_adlibsdk_dfs = []
    for d in ('mobile', 'desktop'):
        logger.info(f'yandex_player_df {d}')
        yandex_player_dfs.append(prepare_service_df(player_report, "yandex_player", df_query=f'slice == "\ttotal\tdevice\t{d}\t"', device=d))
        logger.info(yandex_player_dfs[-1].iloc[:2])
        logger.info(f'yandex_turbo_df {d}')
        yandex_turbo_dfs.append(prepare_service_df(turbo_report, "turbo.yandex.ru", df_query=f'slice == "\ttotal\t{d}\t"', device=d))
        logger.info(yandex_turbo_dfs[-1].iloc[:2])
        logger.info(f'yandex_games_df {d}')
        yandex_games_dfs.append(prepare_service_df(games_report, "games.yandex.ru", df_query=f'slice == "\ttotal\t{d}\t"', device=d))
        logger.info(yandex_games_dfs[-1].iloc[:2])
    for d in ('android', ):
        logger.info(f'yandex_adlibsdk_df {d}')
        yandex_adlibsdk_dfs.append(prepare_service_df(inapp_report, service_id="yandex.adlibsdk", df_query=f'slice == "\ttotal\t{d}\t"', dsp="inapp", device=d))
        logger.info(yandex_adlibsdk_dfs[-1].iloc[:2])

    data = []
    # https://st.yandex-team.ru/ANTIADB-2262 таска MONEY_BY_SERVICE_ID может упасть и в статовском отчетет по chevent не будет нужных данных
    if len(chevent_df):
        data.append(chevent_df)

    service_ids_max_dates = pd.concat(
        map(lambda df: df[['service_id', 'fielddate']].groupby(['service_id']).max().reset_index(), data)
    ).groupby(['service_id']).min().reset_index()
    date_max = dict(zip(service_ids_max_dates['service_id'], service_ids_max_dates['fielddate']))

    def cut_above_date_max(df):
        df['date_max'] = df['service_id'].apply(lambda service_id: date_max[service_id])
        df = df[df['fielddate'] <= df['date_max']]
        return df

    data = list(map(cut_above_date_max, data))
    data.extend(yandex_turbo_dfs)
    data.extend(yandex_games_dfs)
    data.extend(yandex_player_dfs)
    data.extend(yandex_adlibsdk_dfs)
    united_df = pd.concat(data, sort=True).drop(columns=['date_max'])
    logger.info('united_df')
    logger.info(united_df.iloc[:2])
    logger.info("Uploading data to united Stat report")

    def gen_united_report_stat_data(frame):
        stat_data = []
        for field in ('dsp', 'device'):
            logger.info(f'group united_df by {field}')
            agg = frame.groupby(['fielddate', 'service_id', field]).sum().reset_index()
            agg.service_id = agg.apply(lambda row: [row.service_id, field, row[field]], axis=1)
            del agg[field]
            stat_data.extend(map(lambda row: dict(row[1]), agg.iterrows()))
            for name, query in DF_AGG_QUERIES.items():
                logger.info(f'group united_df by aggregate {name} {field}')
                agg = frame.query(query).groupby(['fielddate', field]).sum().reset_index()
                agg['service_id'] = agg.apply(lambda row: [name, field, row[field]], axis=1)
                del agg[field]
                stat_data.extend(map(lambda row: dict(row[1]), agg.iterrows()))

        agg = frame.groupby(['fielddate', 'service_id']).sum().reset_index()
        agg.service_id = agg.apply(lambda row: [row.service_id], axis=1)
        stat_data.extend(map(lambda row: dict(row[1]), agg.iterrows()))
        for name, query in DF_AGG_QUERIES.items():
            logger.info(f'group united_df aggregate {name}')
            agg = frame.query(query).groupby(['fielddate']).sum().reset_index()
            agg['service_id'] = agg.apply(lambda row: [name], axis=1)
            stat_data.extend(map(lambda row: dict(row[1]), agg.iterrows()))
        return stat_data

    for scale in configs.Scales:
        if scale == configs.Scales.day and not daily_logs:
            continue
        logger.info(f'Trying to upload results to Stat report {united_report} (scale {scale})')
        df = united_df if scale == configs.Scales.minute else reformat_df_fielddate(united_df, configs.STAT_FIELDDATE_FMT[scale])
        data = gen_united_report_stat_data(df)
        upload_result = united_report.upload_data(scale=scale.value, data=data)
        logger.info(upload_result)
        # do not repush data to Solomon for minute scale from daily log
        if scale == configs.Scales.minute and daily_logs:
            continue
        sensors = get_sensors(df)
        logger.info(f'Trying to upload results to Solomon (scale {scale}), {len(sensors)} rows of data:\n{sensors[:10]}\n\n')
        for chunk in chuncker(sensors):
            upload_result = upload_chunk_to_solomon(chunk, solomon_cluster, solomon_token)
            logger.info(upload_result)
