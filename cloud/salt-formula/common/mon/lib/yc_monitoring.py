#!/usr/bin/env python3

import datetime
import ipaddress
import json
import os
import shlex
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from collections import OrderedDict
from typing import Optional

import yaml

SOLOMON_LOCAL_ENDPOINT = "http://[::1]:10050/"


class Status:
    OK = 0
    WARN = 1
    CRIT = 2

    @classmethod
    def to_string(cls, value: int):
        return _STATUS_TO_STRING[value]


_STATUS_TO_STRING = {
    Status.OK: "OK",
    Status.WARN: "WARN",
    Status.CRIT: "CRIT"
}


def load_config(path):
    with open(path) as stream:
        return yaml.safe_load(stream)


def report_status_and_exit(status: int, desc: str="", check_name: str=None):
    if check_name is None:
        base_name = os.path.basename(sys.argv[0])
        check_name = os.path.splitext(base_name)[0]
    print("PASSIVE-CHECK:{};{};{}".format(check_name, status, desc))
    sys.exit(0)


class CheckResult:
    def __init__(self, status: int, comment: Optional[str]=None):
        self.status = status
        self.comment = comment

    def report_and_exit(self):
        report_status_and_exit(self.status, self.comment)


class ComplexResult:
    def __init__(self):
        self.results = OrderedDict()

    def add_result(self, name: str, result: CheckResult):
        self.results[name] = result

    def get_status(self):
        return max(result.status for result in self.results.values())

    def get_message(self):
        def format_results_for(status: int):
            strings = []
            for name, result in self.results.items():
                if result.status == status:
                    string = "{}: {}".format(name, Status.to_string(result.status))
                    if result.comment:
                        string += " ({})".format(result.comment)
                    strings.append(string)
            return strings

        return ", ".join(format_results_for(Status.CRIT) +
                         format_results_for(Status.WARN) +
                         format_results_for(Status.OK))

    def report_and_exit(self):
        report_status_and_exit(self.get_status(), self.get_message())


def get_journalctl_logs(service: str, since: str) -> list:
    cmd = "/bin/journalctl -u {} --since {} --quiet".format(shlex.quote(service), shlex.quote(since))
    proc = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
    lines = proc.communicate()[0].decode("utf-8").split("\n")
    return [line for line in [line.rstrip() for line in lines] if line]  # Filter out empty lines


def get_file_logs(path: str, num_str: int, grep_expr: str = None) -> list:
    cmd = "tail -n {} {}".format(num_str, shlex.quote(path))
    proc = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
    lines = proc.communicate()[0].decode("utf-8").split("\n")
    if grep_expr:
        lines = [line for line in lines if grep_expr in line]
    return [line for line in [line.rstrip() for line in lines] if line]  # Filter out empty lines


def get_systemctl_status(service_name):
    args = ["systemctl", "show", service_name]
    s_info = subprocess.Popen(args, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    try:
        out, err = s_info.communicate()
    except Exception as e:
        report_status_and_exit(Status.CRIT, "Could not get status for {} ({})".format(service_name, e))
    params = {}
    for line in out.decode().split("\n"):
        if not line:
            continue
        name, val = line.strip().split("=", 1)
        params[name] = val

    if params.get("LoadError"):
        report_status_and_exit(Status.CRIT,
                               "Got error when loading {}: {}".format(service_name, params["LoadError"]))
    return params


def get_service_active_since(service_attrs):
    active_ts = service_attrs.get("ActiveEnterTimestamp", "")
    try:
        return datetime.datetime.strptime(active_ts, "%a %Y-%m-%d %H:%M:%S %Z")
    except ValueError as e:
        report_status_and_exit(
            Status.CRIT,
            "Could not parse ActiveEnter date: {}".format(e))


def check_service_running(service: str, min_uptime: int=None, result: ComplexResult=None) -> ComplexResult:
    if result is None:
        result = ComplexResult()
    attrs = get_systemctl_status(service)

    if (attrs["ActiveState"] != "active" and
            attrs["SubState"] != "running"):
        result.add_result(service, CheckResult(
                          Status.CRIT,
                          "Service {} is {} ({})".format(service, attrs["ActiveState"], attrs["SubState"])))
        return result

    active_dt = get_service_active_since(attrs)
    delta = datetime.datetime.now() - active_dt
    seconds = int(delta.total_seconds())
    msg = "Service '{}' started {} sec ago".format(service, seconds)

    if min_uptime and seconds < min_uptime:
        result.add_result(service, CheckResult(Status.CRIT, msg))
        return result

    result.add_result(service, CheckResult(Status.OK, msg))
    return result


def _ping(target):
    ip_ver = ipaddress.ip_address(target).version
    command = '/bin/ping' if ip_ver == 4 else '/bin/ping6'
    return subprocess.Popen([command, '-c1', '-W1', '-q', target],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def ping_hosts(hosts_file, solomon_metric, solomon_url_path="pings"):
    cr = ComplexResult()

    try:
        with open(hosts_file) as f:
            conf = yaml.safe_load(f)
    except (IOError, ValueError):
        cr.add_result("ping", CheckResult(
            Status.WARN, "Could not parse list of hosts to ping from {}".format(hosts_file)))
        return cr

    pings = []
    for host, ip in conf["hosts"].items():
        pings.append((host, ip, _ping(ip)))

    result = {
        'fails': [],
        'successes': [],
    }
    for host, ip, ping in pings:
        out, _ = ping.communicate()
        out = out.decode('utf-8')
        if ping.returncode:
            result['fails'].append((host, ip, out, ping.returncode))
        else:
            result['successes'].append((host, ip, out, ping.returncode))

    try:
        _report_pings_to_solomon(results=result['successes'] + result['fails'],
                                 solomon_metric=solomon_metric,
                                 solomon_url_path=solomon_url_path)
    except Exception as e:
        cr.add_result("solomon", CheckResult(Status.WARN, "Could not report to local solomon: {!r}".format(e)))

    fails = ','.join([record[0] + ': ' + record[1] for record in result['fails']])
    if result['successes'] and not result['fails']:
        cr.add_result("ping", CheckResult(Status.OK, "All hosts available"))
    elif result['successes'] and result['fails']:
        cr.add_result("ping", CheckResult(Status.WARN, "Can't ping hosts ({})".format(fails)))
    else:
        cr.add_result("ping", CheckResult(Status.CRIT, "Can't ping all hosts ({})".format(fails)))

    return cr


def form_sensor(value, ts=None, kind="GAUGE", **labels):
    if ts is not None:
        ts = int(time.time())
    sensor = {
        "kind": kind,
        "labels": labels,
        "timeseries": [{
            "ts": ts,
            "value": value
        }]
    }
    return sensor


def _parse_rtt(output):
    # rtt min/avg/max/mdev = 0.037/0.037/0.037/0.000 ms
    for line in output.split("\n"):
        if line.startswith("rtt"):
            stats = line.split('=')[1].strip()
            value = stats.split('/')[1]  # Taking average
            return float(value)
    return -1


def _report_pings_to_solomon(results, solomon_metric, solomon_url_path):
    sensors = []
    now = int(time.time())
    for host, ip, out, ret_code in results:
        if ret_code:
            value = -1
        else:
            value = _parse_rtt(out)
        sensor = form_sensor(value, ts=now,
                             metric=solomon_metric + "_rtt",
                             peer=host,
                             peer_ip=ip,
                             return_code=str(ret_code))
        sensors.append(sensor)
    report = {"sensors": sensors}
    data = json.dumps(report).encode('utf8')
    url = urllib.parse.urljoin(SOLOMON_LOCAL_ENDPOINT, solomon_url_path)
    req = urllib.request.Request(url, data=data,
                                 headers={'Content-Type': 'application/json'})
    urllib.request.urlopen(req)
