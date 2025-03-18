import os
import sys
import subprocess
import contextlib
import weakref
import pickle
import base64

import __res as resource


def share(path):
    import api.copier as ac

    return ac.Copier().create([(os.path.abspath(path), os.path.basename(path)), ]).resid()


def run_skynet(data):
    with open('/tmp/task.py', 'w') as f:
        f.write(data)

    p = subprocess.Popen(['/skynet/python/bin/python', '/tmp/task.py'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    rc = p.wait()

    if rc:
        raise Exception('shit happen: %s, %s' % (rc, err))

    return out


class Session(object):
    def __init__(self, tid, args, hosts, task):
        self._tid = tid
        self._args = args
        self._hosts = hosts
        self._task = task

    def wait(self, timeout=None):
        def iter_parts():
            data = {
                'tid': self._tid,
                'client': self._args,
                'hosts': self._hosts,
                'task': pickle.dumps(self._task),
                'timeout': timeout,
            }

            yield 'DATA = "'
            yield base64.b64encode(pickle.dumps(data))
            yield '"\n\n'
            yield resource.importer.get_source('api.cqueue.__cqudp_runner__')
            yield 'if __name__ == "__main__":\n    run()\n'

        for l in run_skynet(''.join(iter_parts())).split('\n'):
            if l:
                res = pickle.loads(base64.b64decode(l))

                try:
                    if res['res']:
                        rr, re = pickle.loads(res['res'])

                        yield res['host'], rr, re
                    else:
                        yield res['host'], None, pickle.loads(res['err'])
                except Exception as e:
                    yield res['host'], None, e

    def finish(self):
        pass


class CQueueClient(object):
    def __init__(self, cargs):
        self._t = share(sys.executable)
        self._c = cargs
        self._s = weakref.WeakSet()

    def run(self, hosts, task):
        session = Session(self._t, self._c, hosts, task)

        self._s.add(session)

        return session

    def finish(self):
        for s in self._s:
            s.finish()


@contextlib.contextmanager
def Client(*args, **kwargs):
    cargs = {
        'args': args,
        'kwargs': kwargs,
    }

    client = CQueueClient(cargs)

    try:
        yield client
    finally:
        client.finish()
