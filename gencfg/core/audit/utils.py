"""Utilities common for cpu/memory audit"""

import json

import gencfg
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen


# params for checking if enough data or not
HAVE_DATA_RATIO = 0.5


def get_capaview_report(force_load=False):
    """Get full clickhouse report from capaview service"""
    if force_load:
        get_capaview_report.cached_data = None
    if get_capaview_report.cached_data is None:
        data = retry_urlopen(3, SETTINGS.services.capaview.usage.rest.url)
        get_capaview_report.cached_data = json.loads(data)
    return get_capaview_report.cached_data
get_capaview_report.cached_data = None


def have_enough_data(group, clickhouse_result, TSuggestClass):
    """Check if we have enough data for analyze"""

    two_weeks_ratio = clickhouse_result.reports[clickhouse_result.EPeriods.TWO_WEEKS].have_data_ratio
    half_year_ratio = clickhouse_result.reports[clickhouse_result.EPeriods.HALF_YEAR].have_data_ratio

    if two_weeks_ratio > HAVE_DATA_RATIO or half_year_ratio > HAVE_DATA_RATIO:
           return True, None
    else:
        return False, TSuggestClass(group, TSuggestClass.ESuggestType.LACKDATA, 'Not enough statistics: only {:.2f}% of last two weeks, {:.2f}% of last half year'.format(
            two_weeks_ratio * 100, half_year_ratio * 100
        ))


def calculate_quantile_for_normal_group(clickhouse_result):
    """Calculate usage quantile"""

    half_year_report = clickhouse_result.reports[clickhouse_result.EPeriods.HALF_YEAR]
    two_weeks_report = clickhouse_result.reports[clickhouse_result.EPeriods.TWO_WEEKS]

    candidates = []
    if half_year_report.have_data_ratio > HAVE_DATA_RATIO:
        candidates.append(half_year_report.median99)
    if two_weeks_report.have_data_ratio > HAVE_DATA_RATIO:
        candidates.append(two_weeks_report.median95)

    return max(candidates)


class TClickhouseGroupReport(object):
    """Bunchof clickhouse reports on different timings for group"""

    class EPeriods:
        TWO_WEEKS = "14"
        HALF_YEAR = "180"
        ALL = [TWO_WEEKS, HALF_YEAR]

    __slots__ = (
            'group',  # group we work with
            'reports', # dict of (EPeriods -> TClickhouseGroupPeriodReport)
    )

    class TClickhouseGroupPeriodReport(object):
        """Clickhouse report on cpu usage (data, got from clickhouse table)"""

        __slots__ = (
            'median50',  # 50-median for cpu usage (in power units)
            'median95',  # 95-median for cpu usage (in power units)
            'median99',  # 99-median for cpu usage (in power units)
            'have_data_ratio',  # how many instances on average send data (on specified period)
        )

        def __init__(self, median50, median95, median99, have_data_ratio):
            self.median50 = median50
            self.median95 = median95
            self.median99 = median99
            self.have_data_ratio = have_data_ratio

        @staticmethod
        def from_usage_dict(d, group, signal_name, period):
            """Initialize report for dict, fetched from capaview

            :param d: dict with all data fetched from capaview
            :param group: group to get data for
            :param signal_name: signal name to extract data from (instance_cpuusage_power_units for cpu)
            :param period: period we are interested in (<14> for two weeks, <180> for half-year)
            """

            if len(group.card.audit.cpu.service_groups) == 0:
                service_groups = [group]
            else:
                service_groups = [group.parent.get_group(x) for x in group.card.audit.cpu.service_groups]

            # variuos checks on have data in dict
            usage_d = d.get('usage_stats', None)
            if usage_d is None:
                raise Exception('Key <usage_stats> not found')
            period_d = usage_d.get(period, None)
            if period_d is None:
                raise Exception('Not found usage stats for period of <{}> last days'.format(period))
            for service_group in service_groups:
                group_d = period_d.get(service_group.card.name, None)
                if group_d is None:
                    raise Exception('Not found usage stats for group <{}>'.format(group.card.name))

            # convert data for better analysis
            group_data = dict()
            for service_group in service_groups:
                group_data[service_group.card.name] = period_d[service_group.card.name][signal_name]

            # calculate signals
            icount = max(1, group_data[group.card.name]['n_instances_quant99'])
            median50 = sum(x['quantile(0.5)'] for x in group_data.itervalues()) / icount
            median95 = sum(x['quantile(0.95)'] for x in group_data.itervalues()) / icount
            median99 = sum(x['quantile(0.99)'] for x in group_data.itervalues()) / icount
            have_data_ratio = sum(x['distinct_ts_percentage'] for x in group_data.itervalues()) / float(len(service_groups))

            return TClickhouseGroupReport.TClickhouseGroupPeriodReport(median50, median95, median99, have_data_ratio)

        @staticmethod
        def from_quantile99(quantile99):
            """Initialize report from manually specified quantile99"""
            return  TClickhouseGroupReport.TClickhouseGroupPeriodReport(quantile99, quantile99, quantile99, 1.0)


    def __init__(self, group, reports):
        self.group = group
        self.reports = reports

    @staticmethod
    def from_usage_dict(d, group, signal_name):
        reports = {period: TClickhouseGroupReport.TClickhouseGroupPeriodReport.from_usage_dict(d, group, signal_name, period) for period in TClickhouseGroupReport.EPeriods.ALL}
        return TClickhouseGroupReport(group, reports)

    @staticmethod
    def from_quantile99(quantile99, group):
        reports = {period: TClickhouseGroupReport.TClickhouseGroupPeriodReport.from_quantile99(quantile99) for period in TClickhouseGroupReport.EPeriods.ALL}
        return TClickhouseGroupReport(group, reports)
