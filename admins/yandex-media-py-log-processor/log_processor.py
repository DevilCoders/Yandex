import json
import sys
import argparse
from collections import namedtuple
from datetime import datetime
import re
import math
from file_read_backwards import FileReadBackwards
from enum import Enum
from os import path

class AlertStatus(Enum):
    OK = 0
    WARNING = 1
    CRITICAL = 2


class AlertResult(object):
    def __init__(self, name, status, message=""):
        self.name = name
        self.message = "" if message is None else message
        self.status = status

    def __str__(self):
        return "{};{}\n".format(self.status.value, self.message)


max_alert_message_len = 1000
timestamp_key_name = "timestamp"
request_key_name = "request"
upstream_response_time_key_name = "upstream_response_time"
whole_line_key_name = "line"


def process_percentile_alert(conf, gfr, lfr):
    if len(lfr) is 0:
        return AlertStatus.OK, None
    warn_time = conf.warning
    crit_time = conf.critical
    percentile = conf.percentile / 100
    lfr = sorted(lfr, key=lambda record: record[upstream_response_time_key_name])
    percentile_index = max(0, math.floor(len(lfr) * percentile) - 1)
    percentile_value = lfr[percentile_index][upstream_response_time_key_name]
    if percentile_value >= crit_time:
        return AlertStatus.CRITICAL, "{}prc: {}".format(conf.percentile, percentile_value)
    if percentile_value >= warn_time:
        return AlertStatus.WARNING, "{}prc: {}".format(conf.percentile, percentile_value)
    return AlertStatus.OK, "{}prc: {}".format(conf.percentile, percentile_value)


def process_count_alert(conf, gfr, lfr):
    warn_count = conf.warning
    crit_count = conf.critical
    count = len(lfr)
    if count >= crit_count:
        return AlertStatus.CRITICAL, " {}".format(count)
    if count >= warn_count:
        return AlertStatus.WARNING, " {}".format(count)
    return AlertStatus.OK, " {}".format(count)


def process_rate_alert(conf, gfr, lfr):
    if len(gfr) is 0:
        return AlertStatus.OK, None
    warn_count = conf.warning
    crit_count = conf.critical
    rate = len(lfr) / len(gfr)
    if rate >= crit_count:
        return AlertStatus.CRITICAL, " {:.3}".format(rate)
    if rate >= warn_count:
        return AlertStatus.WARNING, " {:.3}".format(rate)
    return AlertStatus.OK, " {:.3}".format(rate)


alert_processors = {
    "percentile": process_percentile_alert,
    "count": process_count_alert,
    "rate": process_rate_alert
}


def _json_object_hook(d): return namedtuple('X', d.keys())(*d.values())


def read_settings(settings_file):
    if path.exists(settings_file):
        config = settings_file
    else:
        config = "{}/{}-default.json".format(path.dirname(settings_file),
                                             path.basename(settings_file)[:-5]
                                             )
    return json.load(open(config), object_hook=_json_object_hook)


def parse_time(record):
    return datetime.strptime(record[timestamp_key_name], '%Y-%m-%dT%H:%M:%S')


def parse_line(line):
    key_value_pairs = line.split("\t")
    dict = {}
    for key_value_pair_str in key_value_pairs:
        key_value_pair_list = key_value_pair_str.split("=", 1)
        if len(key_value_pair_list) != 2:
            continue
        if upstream_response_time_key_name == key_value_pair_list[0]:
            """
            prevent ValueError: could not convert string to float: '-'
            when timing is '-' in log line
            """
            try:
                dict[key_value_pair_list[0]] = float(key_value_pair_list[1])
            except ValueError:
                pass
            continue
        dict[key_value_pair_list[0]] = key_value_pair_list[1]
    dict[whole_line_key_name] = line
    return dict


def read_last_records(settings):
    depth_seconds = settings.depth_seconds
    with FileReadBackwards(settings.log_path, encoding="latin-1") as frb:
        first_time = None
        for line in frb:
            record = parse_line(line)
            if first_time is None:
                first_time = parse_time(record)
            curr_time = parse_time(record)
            if math.fabs((first_time - curr_time).total_seconds()) > depth_seconds:
                break
            yield record


def compile(regexp_patterns):
    compiled_regexp_patterns = []
    for pattern in regexp_patterns:
        compiled_regexp_patterns.append(re.compile(pattern))
    return compiled_regexp_patterns


def filter_records(records, filter):
    compiled_exclude_regexp_patterns = compile(filter.exclude)
    compiled_include_regexp_patterns = compile(filter.include)
    for record in records:
        excluded = False
        for exclude_pattern in compiled_exclude_regexp_patterns:
            if exclude_pattern.match(record[whole_line_key_name]):
                excluded = True
                break
        if excluded:
            continue
        for include_pattern in compiled_include_regexp_patterns:
            if include_pattern.match(record[whole_line_key_name]):
                yield record
                break


def process_alert(monitoring_settings, globally_filtered_records):
    locally_filtered = list(filter_records(
        globally_filtered_records,
        monitoring_settings.filter
    ))
    alert_type = monitoring_settings.alert_configuration.type
    if alert_type not in alert_processors.keys():
        raise RuntimeError("Specify correct alert configuration.")
    status, message = alert_processors[alert_type](
        monitoring_settings.alert_configuration,
        globally_filtered_records,
        locally_filtered
    )
    return AlertResult(monitoring_settings.name, status, message)


def aggregate(alert_results):
    if len(alert_results) is 0:
        return AlertResult("Empty", AlertStatus.OK)
    alert_results = sorted(alert_results, key=lambda x: x.status.value, reverse=True)
    aggregatd_status = alert_results[0].status
    message = ""
    for alert_result in alert_results:
        message += "{}:{}:{};".format(alert_result.status.name, alert_result.name, alert_result.message)
    return AlertResult("aggregated", aggregatd_status, message[0: min(max_alert_message_len, len(message))])


if __name__ == "__main__":

    args = argparse.ArgumentParser(
        description='nginx tslv log parser and monrun script'
        'Without args use defaults config /etc/yandex-media-py-log-processor/settings.json',
    )
    args.add_argument('-c', '--config', help='path to config file',
                            default='/etc/yandex-media-py-log-processor/settings-default.json',
                            action='store')
    args.add_argument('-o', '--one',
                            default=None,
                            help='process only one check(must be name from log_monitor_settings)',
                            action='store')
    args.add_argument('-l', '--list',
                      default=False,
                      help='list tuned checks in config and exit',
                      action='store_true')

    arguments = args.parse_args()

    settings = read_settings(arguments.config)
    last_records = list(read_last_records(settings))
    globally_filtered_records = list(filter_records(last_records, settings.global_filter))
    alert_results = []
    if arguments.list:
        for log_monitor_setting in settings.log_monitor_settings:
            print(log_monitor_setting.name)
        exit(0)

    if arguments.one is None:
        for log_monitor_setting in settings.log_monitor_settings:
            alert_result = process_alert(
                log_monitor_setting,
                globally_filtered_records
            )
            alert_results.append(alert_result)
    else:
        for log_monitor_setting in settings.log_monitor_settings:
            if log_monitor_setting.name == arguments.one:
                alert_result = process_alert(
                    log_monitor_setting,
                    globally_filtered_records
                )
                alert_results.append(alert_result)

    aggregated_result = aggregate(alert_results)
    sys.stdout.write(str(aggregated_result))
    exit(aggregated_result.status.value)
