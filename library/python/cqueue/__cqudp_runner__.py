import os
import sys
import pickle
import base64
import getpass
import subprocess

import api.copier
import api.cqueue


def get_network_class(network):
    if network == 'fastbone':
        return api.copier.Network.Fastbone

    if network == 'backbone':
        return api.copier.Network.Backbone

    return api.copier.Network.Auto


def download_impl(torrentid, where, network=None, timeout=None):
    try:
        os.makedirs(where)
    except OSError:
        pass

    net_class = get_network_class(network)
    h = api.copier.Copier().handle(torrentid)
    l = h.list(network=net_class).wait().files()

    try:
        h.get(dest=where, user=True, network=net_class).wait(timeout)
    except Exception as e:
        for file_info in l:
            try:
                path = os.path.join(where, file_info['name'])
                os.unlink(path)
            except:
                pass

        raise e

    return os.path.join(where, l[0]['name'])


class Runner(object):
    def __init__(self, task, tid):
        self._tid = tid
        self._task = task

    def __call__(self):
        os.environ['Y_PYTHON_ENTRY_POINT'] = 'api.cqueue.__cqudp_entry__'
        where = (getpass.getuser() + '_' + self._tid).replace(':', '_')

        return subprocess.check_output([download_impl(self._tid, where), base64.b64encode(self._task)], shell=False)


def run():
    args = pickle.loads(base64.b64decode(DATA))  # noqa
    cargs = args['client']

    with api.cqueue.Client(*cargs['args'], **cargs['kwargs']) as c:
        for host, res, err in c.run(args['hosts'], Runner(args['task'], args['tid'])).wait(timeout=args['timeout']):
            v = {
                'host': host,
                'res': res,
                'err': pickle.dumps(Exception(str(err))),
            }

            sys.stdout.write(base64.b64encode(pickle.dumps(v)))
            sys.stdout.write('\n')

    sys.stdout.flush()
