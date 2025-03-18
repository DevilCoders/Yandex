# coding=utf-8

import enum
from datetime import datetime, timedelta
from collections import namedtuple, defaultdict


class Experiments(enum.Enum):
    NONE = 0
    BYPASS = 1
    FORCECRY = 2
    NOT_BYPASS_BYPASS_UIDS = 3


YT_CLUSTER = 'hahn'
EXPERIMENT_START_TIME_FMT = '%Y-%m-%dT%H:%M:%S'
BYPASS_STAT_REPORT_V1 = 'AntiAdblock/experiment_bypass'
BYPASS_STAT_REPORT_V2 = 'AntiAdblock/experiment_bypass_agg'
FORCECRY_STAT_REPORT_V1 = 'AntiAdblock/experiment_forcecry'
FORCECRY_STAT_REPORT_V2 = 'AntiAdblock/experiment_forcecry_agg'
NOT_BYPASS_BYPASS_UIDS_STAT_REPORT_V1 = 'AntiAdblock/not_bypass_bypass_uids'
NOT_BYPASS_BYPASS_UIDS_STAT_REPORT_V2 = 'AntiAdblock/not_bypass_bypass_uids_agg'

ExperimentParams = namedtuple('ExperimentParams', ['service_id', 'percent', 'experiment_start', 'duration', 'device', 'experiment_type'])

DEVICENAME_MAP = {
    0: 'DESKTOP',
    1: 'MOBILE',
}
# https://wiki.yandex-team.ru/JandeksKontekst/PartnerCode/deviceType/
DEVICETYPE_MAP = {
    0: map(str, range(5, 12)),
    1: map(str, range(5)),
}


DOMAIN_DATA_TMPL = {
    "YA_DOMAIN": defaultdict(lambda: defaultdict(float)),
    "NON_YA_DOMAIN": defaultdict(lambda: defaultdict(float)),
    "YA_DOMAIN_DESKTOP": defaultdict(lambda: defaultdict(float)),
    "NON_YA_DOMAIN_DESKTOP": defaultdict(lambda: defaultdict(float)),
    "YA_DOMAIN_MOBILE": defaultdict(lambda: defaultdict(float)),
    "NON_YA_DOMAIN_MOBILE": defaultdict(lambda: defaultdict(float)),
}

SERVICE_IDS_WITH_INVERTED_SCHEMA = {"autoru"}


def utc_timestamp(utc_dt_obj):
    return int((utc_dt_obj - datetime(1970, 1, 1)).total_seconds())


def get_experiment_params(configs, experiment_types, delta_days=7):
    params = []
    # будем собирать эксперименты за последние полные delta_days
    end_period = datetime.utcnow().replace(hour=0, minute=0, second=0, microsecond=0)
    start_period = end_period - timedelta(days=delta_days)
    for service_id, config in configs.items():
        config = config.get('config', {})
        experiments = config.get('EXPERIMENTS', {})
        for exp in experiments:
            if exp.get('EXPERIMENT_TYPE') not in experiment_types or exp.get('EXPERIMENT_START') is None or exp.get('EXPERIMENT_PERCENT') is None or exp.get('EXPERIMENT_DURATION') is None:
                continue
            start = datetime.strptime(exp["EXPERIMENT_START"], EXPERIMENT_START_TIME_FMT)
            experiment_days = exp.get('EXPERIMENT_DAYS', [])
            if not experiment_days:
                # эксперимент без повторов
                if start_period <= start < end_period:
                    params.append(ExperimentParams(
                        service_id,
                        percent=exp['EXPERIMENT_PERCENT'],
                        experiment_start=start,
                        duration=exp['EXPERIMENT_DURATION'],
                        device=exp.get('EXPERIMENT_DEVICE', [0, 1]),
                        experiment_type=Experiments(exp.get('EXPERIMENT_TYPE')),
                    ))
            else:
                for weekday in experiment_days:
                    delta = (weekday - start_period.weekday()) % 7
                    tmp = start_period + timedelta(days=delta)
                    start = start.replace(year=tmp.year, month=tmp.month, day=tmp.day)
                    if start_period <= start < end_period:
                        params.append(ExperimentParams(
                            service_id,
                            percent=exp['EXPERIMENT_PERCENT'],
                            experiment_start=start,
                            duration=exp['EXPERIMENT_DURATION'],
                            device=exp.get('EXPERIMENT_DEVICE', [0, 1]),
                            experiment_type=Experiments(exp.get('EXPERIMENT_TYPE')),
                        ))

    return params
