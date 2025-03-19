#!/usr/bin/env python

import json
import re
from subprocess import check_output
from tornado import gen, ioloop
from cocaine.services import Service
from cocaine.tools.actions import mql

TIMEOUT = 5

try:
    cocaine_conf = json.loads(open('/etc/cocaine/cocaine.conf', 'r').read())
    query = mql.compile_query('contains(name(), logging.timer[emit])')

    @gen.coroutine
    def GetRuntimeMetrics():
        ms = Service('metrics', endpoints=[['localhost', cocaine_conf['network']['pinned']['locator']]], timeout=TIMEOUT)
        channel = yield ms.fetch('json', query)
        metrics = yield channel.rx.get(timeout=TIMEOUT)
        print 'cocaine_runtime.logging.emit_count %d' % metrics['logging']['timer[emit]']['count']

    ioloop.IOLoop.current().run_sync(lambda: [GetRuntimeMetrics()], timeout=TIMEOUT)
except Exception:
    pass


try:
    processes = check_output(['pgrep', '-a', 'push-client']).split('\n')
    push_client_config_re = re.compile(r'\s+(-c\s*\S+)')
    push_client_file_re = re.compile(r'\s+(--files=\S+)')
    identity = []
    for p in processes:
        search = re.search(push_client_config_re, p)
        if search is not None:
            identity.append(search.group(1))
        else:
            search = re.search(push_client_file_re, p)
            if search is not None:
                identity.append(search.group(1))
    uniq = sorted(list(set(identity)))

    lag = 0
    for identity in uniq:
        args = ['push-client', '--status', '--json']
        args.extend(identity.split(' '))
        output = check_output(args)
        status = json.loads(output)
        for elem in status:
            lag = lag + elem['lag']
    print 'push_client.lag_in_bytes %d' % lag
except Exception:
    pass
