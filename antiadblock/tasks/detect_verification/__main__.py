import argparse
from datetime import timedelta, datetime as dt
from os import getenv
import requests

from yt.wrapper import YtClient
from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import chuncker
from antiadblock.tasks.detect_verification.config import UIDS_STATE_TABLE_PATH, SOLOMON_API_URL, SOLOMON_TS_FMT
from antiadblock.tasks.detect_verification.uids import update_uids_state


logger = create_logger('detect_verification_logger')


YQL_QUERY = '''-- antiadblock/tasks/detect_verification/__main__.py
$cryprox_logs_path = "logs/antiadb-nginx-log/" || "__LOG_FOLDER__";
$uids_state_path = "__UID_TABLE__";
$to_dict = __TO_DICT_FUNCTION__;
$format = DateTime::Format('__SOLOMON_TS_FMT__');
$time = ($ts) -> {return $format(DateTime::FromSeconds(cast($ts/1000 as uint32)));};
$cryprox = (
    SELECT uid, service_id, ts, ab, device, count(*) as cnt
    FROM (
        select if("yandexuid" in $to_dict(_rest),
                  Yson::ConvertToString(DictLookup($to_dict(_rest), "yandexuid")),
                  Antiadb::DecryptCrookie(Yson::ConvertToString(DictLookup($to_dict(_rest), "crookie")))
                ) as uid,
                if(bamboozled is not null, Yson::ConvertToString(DictLookup(Yson::ConvertToDict(bamboozled), 'ab')), '-') as ab,
                service_id, $time(`timestamp`) as ts,
                if("device" in $to_dict(_rest), Yson::ConvertToString(DictLookup($to_dict(_rest), "device")), 'unknown') as device
        from RANGE($cryprox_logs_path, "__CRYPROX_START__", "__CRYPROX_END__")
        WHERE user_browser_name like "yandex%" and ("yandexuid" in $to_dict(_rest) or "crookie" in $to_dict(_rest))
    )
    group by service_id, ts, uid, ab, device
);
select
    ts, service_id, ab, device,
    count_if(adb) as confirmed,
    count_if(service_id is not null and
             adb is not null and
             not adb) as false_detect,
    count_if(adb is null) as no_info,
    sum(if(adb, cnt, 0)) as confirmed_requests,
    sum(if(service_id is not null and
           adb is not null and
           not adb, cnt, 0)) as false_detect_requests,
    sum(if(adb is null, cnt, 0)) as no_info_requests
from
    (SELECT * from $cryprox where LENGTH(uid) >= 11 and LENGTH(uid) <= 64) as cryprox left join
    $uids_state_path as state
    using (uid)
group by cryprox.service_id as service_id, cryprox.ts as ts, cryprox.ab as ab, cryprox.device as device
'''.replace('__SOLOMON_TS_FMT__', SOLOMON_TS_FMT)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for checking if yabro antiadblock users have adblock (results pushed to Solomon)')
    parser.add_argument('--test',
                        action='store_true',
                        help='Will send results to Solomon testing if enabled')
    parser.add_argument('--update_uids_state',
                        action='store_true',
                        help='If enabled, updates dynamic table with uniqid adblock state')
    parser.add_argument('--barnavig_range',
                        nargs=2,
                        metavar='xxxx-x-x',
                        help='Range of bar-navig-log files in yql request (Y-m-d format)')
    parser.add_argument('--uids_state_table',
                        metavar='TABLENAME',
                        help='Use it if you dont want use default dynamic table with uniqid adblock state')

    parser.add_argument('--daily_logs',
                        action='store_true',
                        help='If endbled yql request will use logs/antiadb-nginx-log/1d istead of logs/antiadb-nginx-log/stream/5min')
    parser.add_argument('--dont_check_cryprox_uids',
                        action='store_true',
                        help='If you want just update uniqid state from bar-navig-log')
    parser.add_argument('--cryprox_range',
                        nargs=2,
                        metavar='xxxx-xx-xx*',
                        help='Range of antiadb-nginx-log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S')

    args = parser.parse_args()
    yt_token = getenv('YT_TOKEN')
    assert yt_token is not None
    solomon_token = getenv('SOLOMON_TOKEN')
    assert solomon_token is not None
    solomon_cluster = 'push-test' if args.test else 'push'
    yt_cluster = 'hahn'
    yt_client = YtClient(token=yt_token, proxy=yt_cluster)
    yql_client = YqlClient(token=yt_token, db=yt_cluster)

    uids_table = UIDS_STATE_TABLE_PATH + (args.uids_state_table or 'default')
    if args.barnavig_range:
        barnavig_start, barnavig_end = map(lambda t: dt.strptime(t, '%Y-%m-%d'), args.barnavig_range)
        if barnavig_start > barnavig_end:
            raise Exception('Incorrect "--barnavig_range" argument')
    else:
        barnavig_start, barnavig_end = dt.now() - timedelta(days=1), dt.now()

    if args.update_uids_state or bool(getenv('UPDATE_UIDS_STATE')):
        logger.info('Updating table with uids state. PATH={}'.format(uids_table))
        update_uids_state(yql_client=yql_client, yt_client=yt_client, table_path=uids_table, start=barnavig_start, end=barnavig_end)
    else:
        if not yt_client.exists(uids_table):
            raise Exception('There is no table with uids state. PATH={}'.format(uids_table))

    if args.dont_check_cryprox_uids:
        exit(0)

    is_daily = args.daily_logs or bool(getenv('DAILY_LOGS')) or False
    log = '1d' if is_daily else 'stream/5min'
    to_dict_function = '($x) -> { return $x; }' if is_daily else 'Yson::ConvertToDict'
    table_name_fmt = '%Y-%m-%d' if is_daily else '%Y-%m-%dT%H:%M:00'
    time_range = 2 * 24 if is_daily else 3  # in hours

    if args.cryprox_range:
        cryprox_start, cryprox_end = map(lambda t: dt.strptime(t, table_name_fmt), args.cryprox_range)
        if cryprox_start > cryprox_end:
            raise Exception('Incorrect "--cryprox_range" argument')
    else:
        if is_daily:
            cryprox_end = dt.now().replace(hour=0, minute=0, second=0, microsecond=0)
        else:
            cryprox_end = dt.now().replace(minute=0, second=0, microsecond=0) + timedelta(hours=1)  # to make precise aggregation by hour
        cryprox_start = cryprox_end - timedelta(hours=time_range)

    query = 'PRAGMA yt.Pool="antiadb";\n' + YQL_QUERY
    yql_request = yql_client.query(query
                                   .replace('__CRYPROX_START__', cryprox_start.strftime(table_name_fmt))
                                   .replace('__CRYPROX_END__', cryprox_end.strftime(table_name_fmt))
                                   .replace('__TO_DICT_FUNCTION__', to_dict_function)
                                   .replace('__UID_TABLE__', uids_table.lstrip('/'))
                                   .replace('__LOG_FOLDER__', log), syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')
    yql_response = yql_request.run()
    logger.info('Waiting "detect_verification" yql request')
    df = yql_response.full_dataframe
    df = df[df.ts.map(lambda t: dt.strptime(t, SOLOMON_TS_FMT)) >= cryprox_start - timedelta(hours=3)]
    logger.info('Results dataframe size: ' + str(len(df)))

    logger.info('Trying to upload results to Solomon:')
    sensors = []

    def make_sensors(dataframe, label, sensor):
        return dataframe.apply(
            lambda row: dict(ts=row.ts,
                             value=row[label],
                             labels=dict(sensor=sensor,
                                         service_id=row.service_id,
                                         validation=label,
                                         device=row.device,
                                         **{} if row.ab == '-' else {'ab': row.ab}),
                             ),
            axis=1).tolist()

    def extend_sensors(sensor_df):
        sensors.extend(make_sensors(sensor_df, label=l, sensor='uniqids'))
        sensors.extend(make_sensors(sensor_df, label=l + '_requests', sensor='requests'))

    def extend_aggregated_sensor(sensor_df, service_id):
        df_agg = sensor_df.groupby(['ts', 'ab', 'device']).sum().reset_index()
        df_agg['service_id'] = service_id
        extend_sensors(df_agg)

    for l in ('confirmed', 'false_detect', 'no_info'):
        extend_sensors(df)
        # aggregate sensors
        extend_aggregated_sensor(df, 'ALL')
        extend_aggregated_sensor(df[df['service_id'].str.contains('yandex')], 'YA_DOMAIN')
        extend_aggregated_sensor(df[~df['service_id'].str.contains('yandex')], 'NON_YA_DOMAIN')

    for chunk in chuncker(sensors):
        upload_result = requests.post(
            url=SOLOMON_API_URL.format(cluster=solomon_cluster),
            headers={'Content-Type': 'application/json', 'Authorization': 'OAuth {}'.format(solomon_token)},
            json=dict(sensors=chunk, commonLabels=dict(host='cluster'))
        )
        logger.info(upload_result.text)
