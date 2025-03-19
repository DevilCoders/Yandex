#!/usr/bin/python

from mastermind.service import ReconnectableService
import urllib2
import msgpack
import json
import os
from pprint import pprint

try:
    from cocaine.asio.exceptions import ConnectionError as ServiceConnectionError, DisconnectionError
except ImportError:
    from cocaine.exceptions import ServiceConnectionError, DisconnectionError

def http_req(request, t_out=2):
    result = ''
    try:
        result = http_req_try(request, t_out=2)
        code = 200
    except urllib2.HTTPError, e:
        code = e.code
    except urllib2.URLError, e:
        code = 408
    except:
        code = 409
    return {'code': code, 'result': result}


def http_req_try(request, t_out=2):
    http_answer = urllib2.urlopen(request, timeout=t_out)
    return http_answer


def detectEnvironment():
    if os.path.isfile('/etc/yandex/environment.type'):
        f = open('/etc/yandex/environment.type', 'r')
        environment = f.read().split('\n')[0]
        f.close()
    else:
        environment = 'unknown'
    return environment

def getMastermindHosts(environment, baseUrl='https://c.yandex-team.ru/api-cached/groups2hosts/', fields=['fqdn']):
    if environment in ['production', 'prestable']:
        group = 'elliptics-cloud'
    else:
        group = 'elliptics-test-cloud'
    h = http_req(baseUrl + group + '?fields=' +
                 ','.join(fields) + '&format=json')
    if h['code'] == 200:
        info = json.load(h['result'])
    return [x['fqdn'] for x in info]

def MastermindSession(mm_hosts):
    try:
        s = ReconnectableService(
            "mastermind2.26",
            addresses=",".join(mm_hosts),
            delay=10,
            delay_exp=2.0,
            max_delay=3600,
            attempts=20,
        )
        if not hasattr(s, 'run_sync'):
            def run_sync(self, method, data):
                return self.enqueue(method, data).get()
            ReconnectableService.run_sync = run_sync
        return s
    except:
        l.error("Error connect to mastermind")
        sys.exit()

def couple_list(client):
    try:
        data = client.run_sync('get_flow_stats', msgpack.packb(None))
        status_list = ['open_couples','closed_couples','bad_couples','broken_couples','frozen_couples', 'uncoupled_groups']
        stat = data['dc']
        for dc in stat:
            for s in status_list:
                state = s.split('_')[0]
                print "{0}.{1} {2}".format(dc, state, stat[dc][s])
    except Exception as e:
        l.error("Error running get_flow_stats: " + e.what())
        sys.exit()
        
if __name__ == "__main__":
    environment = detectEnvironment()
    cl = MastermindSession(['localhost'])
    if cl is None:
        exit(1)

    couple_list(cl)

