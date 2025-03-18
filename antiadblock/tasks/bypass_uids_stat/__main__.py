import os
import argparse
from datetime import timedelta, datetime as dt

import requests

from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.misc import chuncker
from antiadblock.tasks.tools.configs_api import get_configs
import antiadblock.tasks.tools.common_configs as configs


logger = create_logger("bypass_uids_stat_logger")
SOLOMON_API_URL = configs.SOLOMON_PUSH_API + "project=Antiadblock&service=users_validation&cluster={cluster}"
SOLOMON_TS_FMT = configs.SOLOMON_TS_H_FMT
YT_CLUSTER = "hahn"

YQL_QUERY = """-- antiadblock/tasks/bypass_uids_stat/__main__.py
$cryprox_logs_path = "logs/antiadb-cryprox-log/__LOG_FOLDER__";
$table_from = "__START__";
$table_to = "__END__";
$to_dict = __TO_DICT_FUNCTION__;
$format = DateTime::Format("__SOLOMON_TS_FMT__");
$time = ($ts) -> {return $format(DateTime::FromSeconds(cast($ts/1000 as uint32)));};
$uniqids = (
    SELECT DISTINCT
        service_id, $time(`timestamp`) as ts,
        Yson::ConvertToString(DictLookup($to_dict(_rest), "uid")) as uid,
        Yson::ConvertToBool(DictLookup($to_dict(_rest), "is_mobile")) as is_mobile,
        Yson::ConvertToString(DictLookup($to_dict(_rest), "uid_type")) as uid_type
    FROM RANGE($cryprox_logs_path, $table_from, $table_to)
    where action="uid_check" and service_id in (__SERVICE_IDS__)
);

SELECT
    service_id,
    ts,
    is_mobile,
    COUNT_IF(uid_type = "bypass_by_uid") as cnt_bypass,
    COUNT(*) as cnt
FROM $uniqids
GROUP BY service_id, ts, is_mobile
ORDER by ts;
""".replace("__SOLOMON_TS_FMT__", SOLOMON_TS_FMT)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for checking if yabro antiadblock users have adblock (results pushed to Solomon)')
    parser.add_argument('--test',
                        action='store_true',
                        help='Will send results to Solomon testing if enabled')
    parser.add_argument('--daily_logs',
                        action='store_true',
                        help='If endbled yql request will use logs/antiadb-nginx-log/1d istead of logs/antiadb-nginx-log/stream/5min')
    parser.add_argument('--cryprox_range',
                        nargs=2,
                        metavar='xxxx-xx-xx*',
                        help='Range of antiadb-nginx-log files in yql request. Y-m-d format if --daily_logs else Y-m-dTH:M:S')

    args = parser.parse_args()
    yt_token = os.getenv('YT_TOKEN')
    assert yt_token is not None
    solomon_token = os.getenv('SOLOMON_TOKEN')
    assert solomon_token is not None
    solomon_cluster = 'push-test' if args.test else 'push'

    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)
    is_daily = args.daily_logs or bool(os.getenv('DAILY_LOGS')) or False
    log = '1d' if is_daily else 'stream/5min'
    to_dict_function = '($x) -> { return $x; }' if is_daily else 'Yson::ConvertToDict'
    table_name_fmt = '%Y-%m-%d' if is_daily else '%Y-%m-%dT%H:%M:00'
    time_range = 2 * 24 if is_daily else 3  # in hours

    if is_daily:
        start_day = os.getenv('START_DAY')
        end_day = os.getenv('END_DAY')
    else:
        start_day, end_day = None, None

    if args.cryprox_range:
        cryprox_start, cryprox_end = map(lambda t: dt.strptime(t, table_name_fmt), args.cryprox_range)
        if cryprox_start > cryprox_end:
            raise Exception('Incorrect "--cryprox_range" argument')
    elif start_day is not None and end_day is not None:
        cryprox_start = dt.strptime(start_day, table_name_fmt)
        cryprox_end = dt.strptime(end_day, table_name_fmt)
        assert cryprox_start <= cryprox_end
    else:
        if is_daily:
            cryprox_end = dt.now().replace(hour=0, minute=0, second=0, microsecond=0)
        else:
            cryprox_end = dt.now().replace(minute=0, second=0, microsecond=0) + timedelta(hours=1)  # to make precise aggregation by hour
        cryprox_start = cryprox_end - timedelta(hours=time_range)

    configs_api_host = os.getenv("CONFIGS_API_HOST", 'api.aabadmin.yandex.ru')
    tvm_id = int(os.getenv('TVM_ID', '2002631'))  # use SANDBOX monitoring tvm_id as default
    configsapi_tvm_id = int(os.getenv('CONFIGSAPI_TVM_ID', '2000629'))
    tvm_secret = os.getenv('TVM_SECRET')
    logger.info('Trying get configs from: {}'.format(configs_api_host))
    configs = get_configs(tvm_id, tvm_secret, configsapi_tvm_id, configs_api_host, hierarchical=True)

    service_ids = set()

    for key, config in configs.items():
        service_id = key.split("::")[0] if "::" in key else key
        if "test" in service_id:
            continue
        if config.get("config", {}).get("BYPASS_BY_UIDS", False):
            service_ids.add(service_id)

    service_ids = ", ".join(f'"{service_id}"' for service_id in sorted(service_ids))
    logger.info(f"Service with enabled BYPASS_BY_UIDS: {service_ids}")

    query = 'PRAGMA yt.Pool="antiadb";\n' + YQL_QUERY
    yql_request = yql_client.query(
        query
            .replace('__START__', cryprox_start.strftime(table_name_fmt))
            .replace('__END__', cryprox_end.strftime(table_name_fmt))
            .replace('__TO_DICT_FUNCTION__', to_dict_function)
            .replace('__LOG_FOLDER__', log)
            .replace('__SERVICE_IDS__', service_ids),
        syntax_version=1,
        title=f'{__file__.split("/", 1)[1]} YQL',
    )

    yql_response = yql_request.run()
    logger.info('Waiting "bypass uids stat" yql request')
    df = yql_response.full_dataframe
    df = df[df.ts.map(lambda t: dt.strptime(t, SOLOMON_TS_FMT)) >= cryprox_start - timedelta(hours=3)]
    logger.info('Results dataframe size: ' + str(len(df)))

    logger.info('Trying to upload results to Solomon:')
    sensors = []

    def make_sensors(dataframe):
        return dataframe.apply(
            lambda row: dict(ts=row.ts,
                             value=int(100 * row.cnt_bypass / row.cnt),
                             labels=dict(sensor="bypass_by_uid",
                                         service_id=row.service_id,
                                         device="smartphone" if row.is_mobile else "pc"),
                             ),
            axis=1).tolist()

    def extend_sensors(sensor_df):
        sensors.extend(make_sensors(sensor_df))

    def extend_aggregated_sensor(sensor_df, service_id):
        df_agg = sensor_df.groupby(['ts', 'is_mobile']).sum().reset_index()
        df_agg['service_id'] = service_id
        extend_sensors(df_agg)

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
