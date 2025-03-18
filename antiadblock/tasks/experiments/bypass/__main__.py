from __future__ import division
import math
import argparse
from datetime import datetime, timedelta
from os import getenv
from copy import deepcopy
from Queue import Queue
from threading import Thread

from yt.wrapper import YtClient
from yql.api.v1.client import YqlClient
import statface_client

from antiadblock.tasks.tools.common_configs import YT_TABLES_DAILY_FMT, YT_TABLES_STREAM_FMT, STAT_FIELDDATE_I_FMT, STAT_FIELDDATE_H_FMT, \
    STAT_FIELDDATE_D_FMT, YT_ANTIADB_PARTNERS_PAGEIDS_PATH
from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.experiments.lib.common import YT_CLUSTER, EXPERIMENT_START_TIME_FMT, BYPASS_STAT_REPORT_V1, BYPASS_STAT_REPORT_V2, \
    NOT_BYPASS_BYPASS_UIDS_STAT_REPORT_V1, NOT_BYPASS_BYPASS_UIDS_STAT_REPORT_V2, Experiments, \
    utc_timestamp, ExperimentParams, DEVICETYPE_MAP, DEVICENAME_MAP, get_experiment_params, DOMAIN_DATA_TMPL, SERVICE_IDS_WITH_INVERTED_SCHEMA
from antiadblock.tasks.tools.configs_api import get_configs


logger = create_logger('')
UIDS_TABLE_PATH = "//home/antiadb/experiments/__experiment_type__/__UIDS_TABLE__"
YQL_UNIQIDS = '''-- antiadblock/tasks/experiments/bypass/__main__.py
-- __MESSAGE__
$cryproxlog = "home/logfeller/logs/antiadb-cryprox-log/__LOG__";
$table_from = '__START__';
$table_to = '__END__';
$ts_is_experimental = ($ts) -> {return _EXP_START_TS_ < $ts and $ts < _EXP_END_TS_;};
$uids_table_path = "__UIDS_TABLE_PATH__";
INSERT INTO $uids_table_path
WITH TRUNCATE
select
    uniqid,
    uid_type = 'experimental' as experimental,
    uid_type = 'control' as control
from (
    select uniqid, max_by(uid_type, cnt) as uid_type from (
        select uniqid, uid_type, count(*) as cnt from range($cryproxlog, $table_from, $table_to)
        where service_id = '__SERVICE_ID__' and
              $ts_is_experimental(`timestamp`/1000) and
              `action` = 'uid_check' and
              Yson::ConvertToString(DictLookup(_rest, 'experiment_type')) = '__EXPERIMENT_TYPE__' and
              Yson::ConvertToString(DictLookup(_rest, 'uid_type')) in ('experimental', 'control')
        group by Yson::ConvertToString(DictLookup(_rest, 'uid')) as uniqid, Yson::ConvertToString(DictLookup(_rest, 'uid_type')) as uid_type
    ) group by uniqid
);
COMMIT;
'''.replace('__UIDS_TABLE_PATH__', UIDS_TABLE_PATH)
YQL_RESULTS_P1 = '''-- antiadblock/tasks/experiments/bypass/__main__.py
-- __MESSAGE__
$adlog = "logs/bs-dsp-log/__LOG__";
$table_from = '__START__';
$table_to = '__END__';
$uids_table_path = "__UIDS_TABLE_PATH__";
$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != '0';};
$time = ($ts) -> {return $ts - $ts % 600;};
$ts_is_experimental = ($ts) -> {return _EXP_START_TS_ < $ts and $ts < _EXP_END_TS_;};
$devices = __DEVICES__;
'''.replace('__UIDS_TABLE_PATH__', UIDS_TABLE_PATH)
YQL_RESULTS_P2_V1 = '''
select
    ts,
    count_if(experimental and $is_aab(adbbits) and countertype='0' and win='1') as experiment_aab_wins,
    count_if(experimental and countertype='0' and win='1') as experiment_wins,
    count_if(control and $is_aab(adbbits) and countertype='0' and win='1') as control_aab_wins,
    count_if(control and countertype='0' and win='1') as control_wins,
    count_if(experimental and $is_aab(adbbits) and countertype='1') as experiment_aab_shows,
    count_if(experimental and countertype='1') as experiment_shows,
    count_if(control and $is_aab(adbbits) and countertype='1') as control_aab_shows,
    count_if(control and countertype='1') as control_shows,
    sum(if(experimental and $is_aab(adbbits) and countertype='1', cast(price as uint64), 0))/1000000. as experiment_aab_money,
    sum(if(experimental and countertype='1', cast(price as uint64), 0))/1000000. as experiment_money,
    sum(if(control and $is_aab(adbbits) and countertype='1', cast(price as uint64), 0))/1000000. as control_aab_money,
    sum(if(control and countertype='1', cast(price as uint64), 0))/1000000. as control_money
from
    (select * from RANGE($adlog, $table_from, $table_to) where $ts_is_experimental(cast(`unixtime` as uint32)) and devicetype in $devices) as log
    join $uids_table_path as u on log.uniqid = u.uniqid
    join (select pageid from `__PAGEIDS_TABLE_PATH__` where service_id='__SERVICE_ID__') as pages on log.pageid = pages.pageid
group by $time(cast(unixtime as uint32)) as ts
'''
YQL_RESULTS_P2_V2 = '''
$uids_stats = (
    select
        uniqid, experiment,
        sum(if(countertype='1' and $is_aab(adbbits), cast(price as uint64), 0))/1000000. as aab_money,
        sum(if(countertype='1', cast(price as uint64), 0))/1000000. as money,
        count_if(countertype='1' and $is_aab(adbbits)) as aab_shows,
        count_if(countertype='1') as shows,
        count_if(countertype='0' and $is_aab(adbbits) and win='1') as aab_wins,
        count_if(countertype='0' and win='1') as wins,
        count_if(countertype='1' and $is_aab(adbbits) and dspfraudbits!='0') as aab_fraud_shows,
        count_if(countertype='1' and dspfraudbits!='0') as fraud_shows,
    from
        (select * from RANGE($adlog, $table_from, $table_to) where $ts_is_experimental(cast(`unixtime` as uint32)) and devicetype in $devices) as log
        join (select pageid from `__PAGEIDS_TABLE_PATH__` where service_id='__SERVICE_ID__') as pages on log.pageid = pages.pageid
        right join $uids_table_path as u on log.uniqid = u.uniqid
    group by u.uniqid as uniqid, u.experimental as experiment
);
select
    experiment,
    sum(aab_wins) as aab_wins,
    sum(wins) as wins,
    sum(aab_shows) as aab_shows,
    sum(shows) as shows,
    sum(aab_fraud_shows) as aab_fraud_shows,
    sum(fraud_shows) as fraud_shows,
    sum(aab_money) as aab_money,
    sum(money) as money,
    count(*) as uids_total,
    count_if(shows=0) as uids_no_shows
from $uids_stats
group by experiment
'''


def get_exp_stat(exp):
    description = str(exp)
    if exp.experiment_type == Experiments.BYPASS:
        report1 = stat_client.get_report(BYPASS_STAT_REPORT_V1)
        report2 = stat_client.get_report(BYPASS_STAT_REPORT_V2)
    else:
        report1 = stat_client.get_report(NOT_BYPASS_BYPASS_UIDS_STAT_REPORT_V1)
        report2 = stat_client.get_report(NOT_BYPASS_BYPASS_UIDS_STAT_REPORT_V2)

    try:
        logger.info('Calculating experiment stats:\n' + description)
        experiment_start_dt = exp.experiment_start
        if not daily_logs and now - experiment_start_dt > timedelta(days=1):
            logger.info('Skipping experiment with stats:\n{}\nMore than 1 day since experiment start'.format(description))
            return exp, None
        experiment_end_dt = experiment_start_dt + timedelta(hours=exp.duration)
        start = experiment_start_dt.replace(minute=experiment_start_dt.minute // 5 * 5, second=0, microsecond=0)
        end = start + timedelta(hours=exp.duration, minutes=5)
        uids_table_name = '{}/{}'.format(exp.service_id, experiment_start_dt.strftime(EXPERIMENT_START_TIME_FMT))
        logger.info('YQL to create table with experiment and control proxied uids')
        query = 'PRAGMA yt.Pool="antiadb";\n' + YQL_UNIQIDS
        yql_request = yql_client.query(query
                                       .replace('__MESSAGE__', description)
                                       .replace('__LOG__', log)
                                       .replace('__UIDS_TABLE__', uids_table_name)
                                       .replace('__EXPERIMENT_TYPE__', exp.experiment_type.name)
                                       .replace('__experiment_type__', exp.experiment_type.name.lower())
                                       .replace('__SERVICE_ID__', exp.service_id)
                                       .replace('_EXP_START_TS_', str(utc_timestamp(experiment_start_dt)))
                                       .replace('_EXP_END_TS_', str(utc_timestamp(experiment_end_dt)))
                                       .replace('__START__', (start + timedelta(hours=3)).strftime(table_name_fmt))
                                       .replace('__END__', (end + timedelta(hours=3)).strftime(table_name_fmt)),
                                       syntax_version=1)
        yql_result = yql_request.run().get_results()
        logger.info(str(yql_result))
        logger.info('Yql request to calculate experiment results (v1)')
        devicetypes = tuple(sum([DEVICETYPE_MAP[d] for d in exp.device], []))
        query = 'PRAGMA yt.Pool="antiadb";\n' + YQL_RESULTS_P1 + YQL_RESULTS_P2_V1
        yql_request = yql_client.query(query
                                       .replace('__MESSAGE__', description)
                                       .replace('__LOG__', log)
                                       .replace('__UIDS_TABLE__', uids_table_name)
                                       .replace('__EXPERIMENT_TYPE__', exp.experiment_type.name)
                                       .replace('__experiment_type__', exp.experiment_type.name.lower())
                                       .replace('__SERVICE_ID__', exp.service_id)
                                       .replace('__PAGEIDS_TABLE_PATH__', partners_pageids_table)
                                       .replace('_EXP_START_TS_', str(utc_timestamp(experiment_start_dt)))
                                       .replace('_EXP_END_TS_', str(utc_timestamp(experiment_end_dt)))
                                       .replace('__START__', (start + timedelta(hours=3)).strftime(table_name_fmt))
                                       .replace('__END__', (end + timedelta(hours=3)).strftime(table_name_fmt))
                                       .replace('__DEVICES__', str(devicetypes)),
                                       syntax_version=1)
        yql_response = yql_request.run()
        df = yql_response.full_dataframe
        df['fielddate'] = df.ts.apply(lambda t: datetime.fromtimestamp(t).strftime(STAT_FIELDDATE_I_FMT))
        if len(exp.device) == 1:
            report_service_id = "{}_{}".format(exp.service_id, DEVICENAME_MAP[exp.device[0]])
        else:
            report_service_id = exp.service_id
        df['service_id'] = report_service_id
        del df['ts']
        logger.info('Got result: {}'.format(df.shape))
        logger.info('Uploading results to Stat')
        logger.info('Trying to upload results to Stat report (scale=i):')
        upload_result = report1.upload_data(scale='i', data=map(lambda row: dict(row[1]), df.iterrows()))
        logger.info(upload_result)

        logger.info('Trying to upload results to Stat report (scale=h):')
        df_hourly = df.copy()
        df_hourly.fielddate = df_hourly.fielddate.map(lambda t: datetime.strptime(t, STAT_FIELDDATE_I_FMT).strftime(STAT_FIELDDATE_H_FMT))
        upload_result = report1.upload_data(
            scale='h',
            data=map(lambda row: dict(row[1]), df_hourly.groupby(['fielddate', 'service_id']).sum().reset_index().iterrows())
        )
        logger.info(upload_result)

        if daily_logs:
            logger.info('Trying to upload results to Stat report (scale=d):')
            df_daily = df.copy()
            df_daily.fielddate = df_daily.fielddate.map(lambda t: datetime.strptime(t, STAT_FIELDDATE_I_FMT).strftime(STAT_FIELDDATE_D_FMT))
            upload_result = report1.upload_data(
                scale='d',
                data=map(lambda row: dict(row[1]), df_daily.groupby(['fielddate', 'service_id']).sum().reset_index().iterrows())
            )
            logger.info(upload_result)

        logger.info('Yql request to calculate aggregated experiment results (v2)')
        query = 'PRAGMA yt.Pool="antiadb";\n' + YQL_RESULTS_P1 + YQL_RESULTS_P2_V2
        yql_request = yql_client.query(query
                                       .replace('__MESSAGE__', description)
                                       .replace('__LOG__', log)
                                       .replace('__UIDS_TABLE__', uids_table_name)
                                       .replace('__EXPERIMENT_TYPE__', exp.experiment_type.name)
                                       .replace('__experiment_type__', exp.experiment_type.name.lower())
                                       .replace('__SERVICE_ID__', exp.service_id)
                                       .replace('__PAGEIDS_TABLE_PATH__', partners_pageids_table)
                                       .replace('_EXP_START_TS_', str(utc_timestamp(experiment_start_dt)))
                                       .replace('_EXP_END_TS_', str(utc_timestamp(experiment_end_dt)))
                                       .replace('__START__', (start + timedelta(hours=3)).strftime(table_name_fmt))
                                       .replace('__END__', (end + timedelta(hours=3)).strftime(table_name_fmt))
                                       .replace('__DEVICES__', str(devicetypes)),
                                       syntax_version=1)
        yql_response = yql_request.run()
        df = yql_response.full_dataframe

        logger.info('Trying to upload results to Stat report (scale=i):')
        prefix_key = 'experiment'
        stat_data = {}
        for _, row in df.iterrows():
            raw = dict(row)
            prefix = prefix_key if raw[prefix_key] else 'control'
            raw.pop(prefix_key)
            stat_data.update({'_'.join([prefix, k]): v for k, v in raw.items()})
        result = deepcopy(stat_data)
        stat_data.update(dict(
            fielddate=experiment_start_dt.strftime(STAT_FIELDDATE_I_FMT),
            percent=exp.percent,
            service_id=report_service_id,
            duration=exp.duration,
            device=', '.join([DEVICENAME_MAP[d] for d in sorted(exp.device)]),
        ))
        logger.info('Calculated statistics:')
        logger.info(stat_data)
        upload_result = report2.upload_data(scale='i', data=stat_data)
        logger.info(upload_result)
    except Exception as e:
        logger.error(e)
        logger.error('Failed to make yql request and upload result for params\n' + description, exc_info=True)
        return exp, None
    return exp, result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for antiadblock bypass experiment: '
                                                 'can collect experiment and control uiniqids and calculate after experiment stats')
    parser.add_argument('--test',
                        action='store_true',
                        help='Will send results to Stat testing')
    parser.add_argument('--daily_logs',
                        action='store_true',
                        help='If endbled yql request will use logs/bs-dsp-log/1d istead of logs/bs-dsp-log/stream/5min')
    parser.add_argument('--service_id',
                        metavar='SERVICE_ID',
                        help='Expects Antiadblock service_id')
    parser.add_argument('--start',
                        metavar='xxxx-x-xTx:x:x',
                        help='Date experiment was started')
    parser.add_argument('--percent',
                        metavar='INT',
                        help='Experimental uids percent')
    parser.add_argument('--duration',
                        metavar='INT',
                        help='Experiment duration')
    parser.add_argument('--experiment_type', metavar='INT', help='1 (BYPASS) or 3 (NOT_BYPASS_BYPASS_UIDS)')

    args = parser.parse_args()
    yt_token = getenv('YT_TOKEN')
    stat_token = getenv('STAT_TOKEN')
    stat_host = statface_client.STATFACE_BETA if args.test else statface_client.STATFACE_PRODUCTION
    daily_logs = args.daily_logs or getenv('DAILY_LOGS')
    log = '1d' if daily_logs else 'stream/5min'
    table_name_fmt = YT_TABLES_DAILY_FMT if daily_logs else YT_TABLES_STREAM_FMT
    # experiment params:
    service_id = args.service_id or getenv('SERVICE_ID')
    percent = args.percent or getenv('PERCENT')
    experiment_start = args.start or getenv('START')
    duration = args.duration or getenv('DURATION')
    if duration is not None:
        duration = int(duration)
    experiment_type = Experiments(int(getenv('EXPERIMENT_TYPE', 1))) if not args.experiment_type else Experiments(args.experiment_type)

    delta_days = int(getenv('DELTA_DAYS', '7'))

    if service_id is not None and percent is not None and experiment_start is not None and duration is not None:  # if all parameters set - don't use configs_api
        logger.info('All parameters set - don`t use configs_api')
        params = [ExperimentParams(service_id, percent, experiment_start, duration, [0, 1], experiment_type)]
    else:
        logger.info('Using configs_api to get active experiments parameters')
        configs_api_host = getenv("CONFIGS_API_HOST", 'api.aabadmin.yandex.ru')
        tvm_id = int(getenv('TVM_ID', '2002631'))  # use SANDBOX monitoring tvm_id as default
        configsapi_tvm_id = int(getenv('CONFIGSAPI_TVM_ID', '2000629'))
        tvm_secret = getenv('TVM_SECRET')
        logger.info('Trying get configs from: {}'.format(configs_api_host))
        configs = get_configs(tvm_id, tvm_secret, configsapi_tvm_id, configs_api_host)
        params = get_experiment_params(configs, experiment_types=(1, 3), delta_days=delta_days)

    yt_client = YtClient(token=yt_token, proxy=YT_CLUSTER)
    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)
    stat_client = statface_client.StatfaceClient(host=stat_host, oauth_token=stat_token)

    partners_pageids_table = YT_ANTIADB_PARTNERS_PAGEIDS_PATH + '/' + sorted(yt_client.list(YT_ANTIADB_PARTNERS_PAGEIDS_PATH))[-1]
    now = datetime.utcnow()
    stat_data_agg_by_domain = {
        Experiments.BYPASS: deepcopy(DOMAIN_DATA_TMPL),
        Experiments.NOT_BYPASS_BYPASS_UIDS: deepcopy(DOMAIN_DATA_TMPL),
    }

    num_threads = int(getenv('NUMBER_THREADS', '4'))
    checks_results_queue = Queue()
    for i in range(int(math.ceil(len(params) / num_threads))):
        threads_list = []
        for exp in params[i*num_threads:(i+1)*num_threads]:
            t = Thread(target=lambda q, e: q.put(get_exp_stat(e)),
                       args=(checks_results_queue, exp),
                       name='{}-{}'.format(exp.service_id, DEVICENAME_MAP[exp.device[0]]))
            t.start()
            threads_list.append(t)

        for thread in threads_list:
            logger.info('Wait thread {}...'.format(thread.name))
            thread.join()

    while not checks_results_queue.empty():
        exp, stat_data = checks_results_queue.get()
        if stat_data is None:
            continue

        domain = "YA_DOMAIN" if "yandex" in exp.service_id else "NON_YA_DOMAIN"
        if len(exp.device) == 1:
            report_service_id = "{}_{}".format(exp.service_id, DEVICENAME_MAP[exp.device[0]])
            domain = "{}_{}".format(domain, DEVICENAME_MAP[exp.device[0]])
        else:
            report_service_id = exp.service_id

        def aggregate(stat_data, day):
            for k, v in stat_data.iteritems():
                stat_data_agg_by_domain[exp.experiment_type][domain][day][k] += v

        if exp.service_id not in SERVICE_IDS_WITH_INVERTED_SCHEMA or exp.experiment_type == Experiments.NOT_BYPASS_BYPASS_UIDS:
            experiment_start_dt = exp.experiment_start
            aggregate(stat_data, experiment_start_dt.replace(hour=0, minute=0, second=0, microsecond=0).strftime(STAT_FIELDDATE_I_FMT))

    # upload agg stat
    for exp_type, domain_data in stat_data_agg_by_domain.iteritems():
        if exp_type == Experiments.BYPASS:
            report = stat_client.get_report(BYPASS_STAT_REPORT_V2)
        else:
            report = stat_client.get_report(NOT_BYPASS_BYPASS_UIDS_STAT_REPORT_V2)
        for domain, days in domain_data.iteritems():
            for day, data in days.iteritems():
                data.update({"fielddate": day, "service_id": domain})
                logger.info('Calculated agg statistics ({}, {}):'.format(domain, day))
                logger.info(data)
                upload_result = report.upload_data(scale='i', data=data)
                logger.info(upload_result)
