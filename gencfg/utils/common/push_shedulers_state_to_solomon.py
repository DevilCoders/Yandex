#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import json
import logging

import config
import gencfg
from core.argparse.parser import ArgumentParserExt

from utils.common.manipulate_sandbox_schedulers import get_scheduler_lastok
import requests


class SolomonPushClient():
    def __init__(self, project, cluster, service):
        self.url = "https://solomon.yandex.net/api/v2/push?project={project}&cluster={cluster}&service={service}".format(
            project=project,
            cluster=cluster,
            service=service
        )
        self.token = config.get_default_oauth()
        self.metrics = []
        self.try_count = 3

    def add(self, data_point):
        metric = {
            'labels': {'sensor': data_point.name},
            'ts': data_point.ts,
            'value': data_point.value
        }
        self.metrics.append(metric)

    def flush(self):
        headers = {
            'Content-Type': 'application/json',
            'Authorization': 'OAuth ' + self.token,
        }
        solomon_json = {
            'sensors': self.metrics,
        }

        for i in range(self.try_count):
            try:
                response = requests.post(
                    self.url,
                    data=json.dumps(solomon_json, separators=(',', ':')),
                    headers=headers,
                )
                response.raise_for_status()
                break
            except requests.exceptions.HTTPError:
                if i == self.try_count - 1:
                    raise


class TSchedulerDataPoint():
    def __init__(self, signal_name, scheduler_id):
        self.name = signal_name
        self.ts = time.time()
        self._scheduler_id = scheduler_id

        self.value = None

    def collect_delay(self):
        self.value = sandbox_scheduler_delay(self._scheduler_id)

    @property
    def is_ready(self):
        return self.value is not None

def get_parser():
    parser = ArgumentParserExt("""Push scheduler's state to solomon""")

    parser.add_argument('-p', '--project', type=str, help='solomon project')
    parser.add_argument('-c', '--cluster', type=str, help='solomon cluster')
    parser.add_argument('-s', '--service', type=str, help='solomon service')
    parser.add_argument('-w', '--wait', type=int, default=0,
                        help='wait X seconds between schedulers')
    parser.add_argument('-r', '--retries', type=int, default=1,
                        help='retry each scheduler X times in case of error')

    return parser


def sandbox_scheduler_delay(scheduler_id):
    """Get delay for specified scheduler"""
    headers = {
        'Content-Type': 'application/json',
        'Authorization': 'OAuth {}'.format(config.get_default_oauth())
    }

    lastok = get_scheduler_lastok(scheduler_id, headers=headers)
    if lastok is None:  # could happen if no task finished yet
        lastok = 0

    return time.time() - lastok


SCHEDULERS = [
    ("Update ipv4/ipv6 addrs", 924),
    ("Sync with bot", 926),
    ("Move free hosts to reserved group", 927),
    ("Update invnum/dc/queue/switch/rack/vlan for all hosts", 931),
    ("Rename hosts based on invnum information", 932),
    ("Updated hosts info from hb", 981),
    ("Create files with and of all gencfg groups in order to export this to cauth.", 1307),
    ("Cycle unworking machines (move working machines from ALL_UNWORKING group)", 1512),
    ("Remove fired group owners/watchers", 2074),
    ("Update groups with monitoring ports ready", 4227),
    ("Update dns cache of virtual machines", 6096),
    ("Update ipv4tunnels for groups with internet tunnels required", 6104),
    ("Export mtn hosts dns cache to mongo (db table)", 7335),
    ("Sync slbs with racktables", 7991),
    ("Sync abc groups", 9498),
    ("Sync staff groups", 9499),
    ("Remove pending ips from ipv4tunnels (RX-526)", 9963),
    ("Update botmem/botdisk/botssd", 17375),
    ("Update nanny services cache", 22935),
    ("Dump groups data to YT", 43625),
    ("Upload GenCfg groups aggregation keys to YT", 22016),
    ("Dump group hbf info", 21008),
    ("Release gencfg tags", 14727),
    ("Send email notifications about build new tag", 13057),
    ("Sync project_id and macros info with RackTables", 12989),
    ("Find requests in mongo and start relevant task", 10315),
    ("Check commits and update db", 8675),
    ("Sheduler for gencfg solomon charts", 8343),
    ("Push state of the shedulers to solomon", 43774)
]


def main(options):
    updaters = []
    client = SolomonPushClient(
        project=options.project,
        cluster=options.cluster,
        service=options.service
    )

    for scheduler_name, scheduler_id in SCHEDULERS:
        data_point = TSchedulerDataPoint(scheduler_name, scheduler_id)
        for iteration in range(options.retries):
            try:
                data_point.collect_delay()
                logging.warning('Have got value for scheduler %s: %s',
                                scheduler_id, data_point.value)
                break
            except Exception as e:
                logging.warning('Could not get value for %s scheduler, retry',
                                scheduler_id)
                time.sleep(options.wait * iteration)
        if data_point.is_ready:
            client.add(data_point)
        else:
            logging.error('Could not get value for %s scheduler finaly! Skip.',
                          scheduler_id)
        if (scheduler_name, scheduler_id) != SCHEDULERS[-1]:
            time.sleep(options.wait)

    client.flush()

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    assert options.retries > 0, 'Retries must be more then 0'

    main(options)
