#!/skynet/python/bin/python
"""
    Script to update/show statface graphs.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import json
from collections import defaultdict

import gencfg
import requests
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.settings import SETTINGS
import utils.common.update_kpi_graphs as update_kpi_graphs

class EActions(object):
    SHOW = "show" # show single report
    LIST = "list" # list of all reports, stored in statface (with some statistics)
    UPDATEMETA = "updatemeta" # add reports which are not in statface right now
    ADDDATA = "adddata" # add data for specific graph at specific timestamp
    UPDATEDATA = "updatedata" # add data from db (for all graphs)
    ALL = [SHOW, LIST, UPDATEMETA, ADDDATA, UPDATEDATA]

def get_parser():
    parser = ArgumentParserExt(description="Manipulate statface graphs from console")
    parser.add_argument("-a", "--action", type=str, required=True,
        choices=EActions.ALL,
        help="Obligatory. Action to execute (one of <%s>)" % ",".join(EActions.ALL))
    parser.add_argument("-r", "--report-names", type=argparse_types.comma_list, default=None,
        help="Optional. Report name (for actions <%s>)" % ",".join([EActions.ADDDATA, EActions.SHOW]))
    parser.add_argument("-d", "--data", type=argparse_types.kvlist, default=None,
        help="Optional. Data to upload to report (for actions <%s>)" % EActions.ADDDATA)
    parser.add_argument("-t", "--timestamps", type=argparse_types.comma_list, default=None,
        help="Optional. Timestamp to uploaded data")
    parser.add_argument("--start-timestamp", type=int, default = None,
        help = "Optional. Start timestap for data to upload")

    return parser

def normalize(options):
    if options.start_timestamp is not None:
        options.timestamps = range(options.start_timestamp, int(time.time()), 60 * 60)

    if options.timestamps is not None:
        options.timestamps = map(int, options.timestamps)
        if len(options.timestamps) == 1:
            options.timestamp = options.timestamps[0]
    else:
        options.timestamps = [int(time.time())]
        options.timestamp = options.timestamps[0]

    if options.report_names is not None:
        if len(options.report_names) == 1:
            options.report_name = options.report_names[0]

def generate_statface_report_url(report_info):
    return "%s/%s?%s" % (SETTINGS.services.statface.http.url, report_info.name, report_info.report_cgi)

def generate_scale_string(scale, timestamp):
    if timestamp is None:
        timestamp = int(time.time())

    if scale == "d":
        return time.strftime("%Y-%m-%d", time.localtime(timestamp))
    elif scale == "h":
        return time.strftime("%Y-%m-%d %H:00:00", time.localtime(timestamp))
    elif scale == "i":
        return time.strftime("%Y-%m-%d %H:%M:00", time.localtime(timestamp))
    else:
        raise Exception("Unknown scale <%s>" % scale)

def get_statface_report_config(report_name):
    """
        Get config for report from statface (if not found, return None)
    """
    url = "%s/report/config?name=%s" % (SETTINGS.services.statface.rest.url, report_name)
    headers = {
        'StatRobotUser': SETTINGS.services.statface.rest.auth.login,
        'StatRobotPassword': SETTINGS.services.statface.rest.auth.password,
    }

    r = requests.get(url, headers=headers, verify=False)

    if r.status_code == 404:
        return None
    elif r.status_code == 200:
        return json.loads(r.text)

def add_statface_report(report_info):
    """
        Add report to statface using rest api
    """

    url = "%s/report/config" % SETTINGS.services.statface.rest.url
    headers = {
        'StatRobotUser': SETTINGS.services.statface.rest.auth.login,
        'StatRobotPassword': SETTINGS.services.statface.rest.auth.password,
    }

    signals_config = {
        "dimensions" : [
            {"fielddate": "date"},
        ],
        "measures" : map(lambda x: {x.name: x.type}, report_info.signals),
    }

    data = {
        "json_config": json.dumps({
            "user_config": signals_config,
            "title": report_info.title,
        }),
        "name": report_info.name,
        "scale": report_info.scale,
        "verbose": 1,
    }

    resp = requests.post(url, headers=headers, data=data, verify=False)

    if resp.status_code != 200:
        raise Exception("Could not add report <%s>: got status %s (text <%s>)" % (report_info.name, resp.status_code, resp.text))

    return resp

def update_point_timestamp(point_data, scale, custom_timestamp):
    """
        Add timestamp to point if do not have one
    """
    if (custom_timestamp is not None) or ('fielddata' not in point_data):
        if custom_timestamp is not None:
            timestamp = custom_timestamp
        else:
            timestamp = int(time.time())
        point_data['fielddate'] = generate_scale_string(scale, timestamp)

    return point_data

def add_statface_report_data(report_info, points_data):
    """
        Add one point to statface report

        :param report_info: object from settings.utils.manipulate_statface.reports
        :param points_data: dict with signal data
    """

    # check data fields
    config_field_names = map(lambda x: x.name, report_info.signals)
    data_field_names = filter(lambda x: x != 'fielddate', points_data[0].keys())
    if set(config_field_names) != set(data_field_names):
        raise Exception("Data fields <%s> do not match config fields <%s>" (",".join(config_field_names), ",".join(data_field_names)))

    # create request and upload data
    url = "%s/report/data" % SETTINGS.services.statface.rest.url
    headers = {
        'StatRobotUser': SETTINGS.services.statface.rest.auth.login,
        'StatRobotPassword': SETTINGS.services.statface.rest.auth.password,
    }

    data = {
        "name": report_info.name,
        "scale": report_info.scale,
        "data" : json.dumps({"values" : points_data}),
        "append_mode" : True,
    }

    resp = requests.post(url, headers=headers, data=data, verify=False)

    if resp.status_code != 200:
        print "Could not add data to report <%s>: got status %s (text <%s>)" % (report_info.name, resp.status_code, resp.text)
        raise Exception("Could not add data to report <%s>: got status %s (text <%s>)" % (report_info.name, resp.status_code, resp.text))

    return resp

def get_statface_report_data(report_info, start_date = None, end_date = None):
    headers = {
        'StatRobotUser': SETTINGS.services.statface.rest.auth.login,
        'StatRobotPassword': SETTINGS.services.statface.rest.auth.password,
    }

    base_url = "%s/%s" % (SETTINGS.services.statface.http.url,  report_info.name)
    params = "_type=json&_fg2=1&scale=%s&_fill_missing_dates=0" % report_info.scale
    if start_date is not None:
        params += "&date_min=%s" % (generate_scale_string(report_info.scale, start_date))
    if end_date is not None:
        params += "&date_max=%s" % (generate_scale_string(report_info.scale, end_date))

    url = "%s?%s" % (base_url, params)

    resp = requests.get(url, headers=headers, verify=False)

    return json.loads(resp.text)


def update_period_signals_data(options):
    """Upload to statface data on `period` type reports"""
    print "Updating period reports:"

    period_reports = [x for x in SETTINGS.kpi.reports if x.type == 'period']
    period_signals = [x.signals for x in period_reports]
    period_signals = [x.name for x in sum(period_signals, [])]

    db_values = {}
    for timestamp in options.timestamps:
        subutil_params = {
            "action": update_kpi_graphs.EActions.SHOWAT,
            "timestamp": timestamp,
            "signals": period_signals,
        }
        db_values[timestamp] = update_kpi_graphs.jsmain(subutil_params)

    for report_info in period_reports:
        if options.report_names is not None and report_info.name not in options.report_names:
            continue

        print "    Report %s:" % (report_info.name)
        points = []
        for timestamp in options.timestamps:
            point_data = {}
            for signal_info in report_info.signals:
                signal_name = signal_info.name
                signal_value = db_values[timestamp].get(signal_name, 0)
                # ====================================== UGLY FIX ========================================
                if signal_name == 'sandbox_scheduler_1307' and signal_value > 10000000:
                    signal_value = 0
                # ========================================================================================
                point_data[signal_name] = signal_value
            update_point_timestamp(point_data, report_info.scale, timestamp)
            points.append(point_data)
            print '        Point data: {}'.format(point_data)

        add_statface_report_data(report_info, points)


def update_exact_signals_data(options):
    "Updating exact reports:"

    exact_reports = [x for x in SETTINGS.kpi.reports if x.type == 'exact']

    for report in exact_reports:
        print '    Report {}:'.format(report.name)

        # get last processed timestamp
        resp = get_statface_report_data(report).get('values', [])

        if len(resp) > 0:
            ts = time.mktime(time.strptime(resp[0]['fielddate'], '%Y-%m-%d %H:%M:%S'))
        else:
            ts = int(time.time())

        subutil_params = {
            'action': update_kpi_graphs.EActions.SHOWSINCE,
            'timestamp': ts - 10000,
            'signals': [x.name for x in report.signals],
        }

        signals_data = update_kpi_graphs.jsmain(subutil_params)

        data_by_ts = defaultdict(list)
        for signal_name in signals_data:
            for ts, v in signals_data[signal_name]:
                data_by_ts[ts].append((signal_name, v))

        points_data = []
        added_timestamps = set()
        for ts in sorted(data_by_ts):
            point_data = {}
            for signal_name, v in data_by_ts[ts]:
                point_data[signal_name] = v

            update_point_timestamp(point_data, report.scale, ts)
            while point_data['fielddate'] in added_timestamps:
                ts += 1
                update_point_timestamp(point_data, report.scale, ts)
            added_timestamps.add(point_data['fielddate'])

            points_data.append(point_data)
            print '        Point data: {}'.format(point_data)

        add_statface_report_data(report, points_data)


def main(options):
    requests.packages.urllib3.disable_warnings()

    if options.action in [EActions.LIST, EActions.UPDATEMETA]:
        print "Reports:"
        for report_info in SETTINGS.kpi.reports:
            result = get_statface_report_config(report_info.name)

            print "    Report: %s" % report_info.name

            if result is not None:
                print "        status: FOUND"
                print "        url: %s" % generate_statface_report_url(report_info)
                print "        fields: %s" % " ".join(map(lambda x: x.name, report_info.signals))
            if result is None:
                if options.action == EActions.LIST:
                    print "        status: NOT FOUND"
                elif options.action == EActions.UPDATEMETA:
                    print "        status: NOT FOUND, adding ..."
                    result = add_statface_report(report_info)
                    print "        url: %s" % generate_statface_report_url(report_info)
                    print "        fields: %s" % " ".join(map(lambda x: x.name, report_info.signals))
    elif options.action == EActions.ADDDATA:
        report_infos = filter(lambda x: x.name == options.report_name, SETTINGS.kpi.reports)
        report_info = report_infos[0]
        point_data = update_point_timestamp(options.data, report_info.scale, options.timestamp)

        print "Report %s:" % report_info.name
        print "    Adding data: %s" % options.data

        return add_statface_report_data(report_info, [point_data])
    elif options.action == EActions.UPDATEDATA:
        print "Adding data from db:"
        update_period_signals_data(options)
        update_exact_signals_data(options)
    elif options.action == EActions.SHOW:
        report_infos = filter(lambda x: x.name == options.report_name, SETTINGS.kpi.reports)
        report_info = report_infos[0]
        report_signals = map(lambda x: x.name, report_info.signals)

        print "Report %s:" % report_info.name
        result = get_statface_report_data(report_info)
        for point in result["values"]:
            data = ", ".join(map(lambda x: "%s: %s" % (x, point[x]), report_signals))
            print "    %s: %s" % (point["fielddate"], data)
        return result
    else:
        raise Exception("Unknown action <%s>" % options.action)

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
