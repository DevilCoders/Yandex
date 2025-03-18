import re
import subprocess
import dateutil.parser

import requests

from sandbox.projects.common.gencfg import task as gencfg_task


_LineWithCommitNumber = re.compile('^r\d+ \| ')


def _get_commit_and_date(line):
    line = line.split(' | ')
    commit, date = line[0], line[2]
    commit = commit.lstrip('r')
    date = date.partition('(')[0].strip()
    date = dateutil.parser.parse(date)
    return commit, date


def get_dates(commits_cnt, svn_gencfg_path):
    args = ['svn', 'log', '-l', str(commits_cnt), '-v', svn_gencfg_path]
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    stdout = popen.communicate()[0]

    result = {}
    for line in stdout.split('\n'):
        if _LineWithCommitNumber.match(line):
            commit, date = _get_commit_and_date(line)
            result[commit] = date

    return result


def load_commits(oldest_commit='0'):
    for rec in gencfg_task.get_topology_mongo()['commits'].find(
        {'commit': {'$gt': oldest_commit}},
        {'commit': 1, 'test_passed': 1},
        sort=[('commit', 1)]
    ):
        yield rec


def osol_function():
    pass


class SolomonClient(object):
    _push_url = 'http://solomon.yandex.net/push/json'
    _get_url = 'http://api.solomon.search.yandex.net/data-api/get'
    _who = 'robot-gencfg'

    def __init__(self, project, cluster, service):
        self._project = project
        self._cluster = cluster
        self._service = service

        self._sensors = []

    def add_sensor(self, sensor_name, value, ts_or_datetime):
        self._sensors.append({
            "labels": {
                "sensor": sensor_name,
            },
            "ts": ts_or_datetime,
            "value": value,
        })

    def flush(self):
        data = {
            "commonLabels": {
                "project": self._project,
                "cluster": self._cluster,
                "service": self._service,
            },
            "sensors": self._sensors
        }
        requests.post(self._push_url, json=data)
        self._sensors = []

    def load(self, sensor, points_cnt, interval='365d'):
        params = {
            "project": self._project,
            "cluster": self._cluster,
            "service": self._service,
            "l.sensor": sensor,
            "b": interval,
            "points": points_cnt,
            'who': self._who,
        }
        sensors = requests.get(self._get_url, params=params, timeout=30).json()['sensors']
        values = sensors.pop()['values']

        result = {}
        for rec in values:
            ts, value = rec['ts'], rec['value']
            result[dateutil.parser.parse(ts)] = value
        return result


client = SolomonClient(project='gencfg', cluster='monitoring', service='health')
