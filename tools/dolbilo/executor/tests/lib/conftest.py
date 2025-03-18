import socket
import os
import logging
import json
import dateutil.parser
import time

import requests
import pytest

from yatest import common
from yatest.common import network, path, process

log = logging.getLogger(os.path.basename(__file__))


def is_tcp_open(hostport, timeout=0.5):
    try:
        socket.create_connection(hostport, timeout=timeout)
        return True
    except Exception:
        return False


class _Balancer(object):
    def __init__(self):
        self.port_manager = network.PortManager()
        self.port = self.port_manager.get_port(8080)
        self.logdir = os.getcwd()
        self.admin_port = self.port_manager.get_port(8081)
        self.unistat_port = self.port_manager.get_port(8082)
        binary = common.binary_path('balancer/daemons/balancer/balancer')
        conf = common.source_path('tools/dolbilo/executor/tests/balancer.lua')
        open(os.path.join(self.logdir, 'access.log'), 'w')  # truncate
        self.proc = process.execute([
            binary,
            '-V', 'admin_port=%d' % self.admin_port,
            '-V', 'data_port=%d' % self.port,
            '-V', 'logdir=%s' % self.logdir,
            '-V', 'unistat_port=%s' % self.unistat_port,
            conf], wait=False)
        process.wait_for(lambda: is_tcp_open(('localhost', self.admin_port)), 5)

    @staticmethod
    def accesslog_to_ts(line):
        # 127.0.0.1:52744	2016-08-09T17:03:33.825317+0300	"GET /ping HTTP/1.1"	0.000063s	""	"localhost"	 [errordocument succ 404]  # noqa
        _, ts, _ = line.split('\t', 2)
        ts = dateutil.parser.isoparse(ts)
        return int(time.mktime(ts.timetuple())) * 1000000 + ts.microsecond

    def accesslog_us(self):
        requests.get('http://localhost:%d/admin?action=reopenlog' % self.admin_port)  # flush logs
        with open(os.path.join(self.logdir, 'access.log'), 'r+') as fd:
            return sorted(map(self.accesslog_to_ts, fd))  # sorted due to buffering

    def status(self):
        return self._parse_status(requests.get('http://localhost:%d/unistat' % self.unistat_port).content)

    @staticmethod
    def _parse_status(out):
        stats = dict(json.loads(out))
        return {
            'accepts': stats['worker-connections_count_summ'],
            'requests': stats['http-main-connection_close_reqs_summ'] + stats['http-main-connection_keep_alive_reqs_summ'],
        }

    def close(self):
        log.info('stopping balancer at port %d', self.port)
        self.proc.kill()  # it waits on its own


@pytest.fixture
def balancer(request):
    n = _Balancer()
    request.addfinalizer(n.close)
    return n


@pytest.fixture(scope='session')
def planner():
    return common.binary_path('tools/dolbilo/planner/d-planner')


@pytest.fixture
def executor():
    return common.binary_path('tools/dolbilo/executor/d-executor')


@pytest.fixture
def single_ping_plan():
    return common.source_path('tools/dolbilo/executor/tests/single-ping.plan')


@pytest.fixture
def huge_plan():
    return './RESOURCE'


@pytest.fixture(scope='session')
def exact_ms():
    # real-world timings from https://st.yandex-team.ru/SEPE-15853
    return [
        0, 4614, 5042, 27404, 46865, 47252, 104828, 105149, 124728,
        125107, 134117, 134458, 136381, 136660, 187841, 188097, 190780, 191173,
        200916, 201258, 254194, 254487, 288074, 288442]


@pytest.fixture(scope='session')
def exact_stpd(exact_ms):
    ping = ('GET /stpd HTTP/1.1\x0d\x0a'
            'Host: localhost\x0d\x0a'
            'Connection: Close\x0d\x0a'
            '\x0d\x0a')
    fname = path.get_unique_file_path(common.output_path(), 'SEPE-15853.stpd')
    with open(fname, 'w') as fd:
        for t in exact_ms:
            fd.write('%d %d\n' % (len(ping), t))
            fd.write(ping)
            fd.write('\n')
    return fname


@pytest.fixture(scope='session')
def exact_plan(exact_stpd, planner):
    fname = path.get_unique_file_path(common.output_path(), 'SEPE-15853.plan')
    process.execute([planner, '-t', 'phantom', '-l', exact_stpd, '-d', 'stpd', '-o', fname])
    return fname
