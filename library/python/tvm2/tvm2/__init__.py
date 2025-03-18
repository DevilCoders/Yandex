# coding: utf-8

import os
import importlib


TVM2_USE_QLOUD = os.environ.get('TVM2_USE_QLOUD')
TVM2_USE_DAEMON = os.environ.get('TVM2_USE_DAEMON') or TVM2_USE_QLOUD
TVM2_ASYNC = os.environ.get('TVM2_ASYNC')


def import_module(path):
    path = '.{}.{}'.format(
        'aio' if TVM2_ASYNC else 'sync',
        path,
    )
    return importlib.import_module(path, package=__name__)


if TVM2_USE_DAEMON:
    TVM2 = import_module('daemon_tvm2').TVM2Daemon

else:
    TVM2 = import_module('thread_tvm2').TVM2
    TVM2Qloud = import_module('daemon_tvm2').TVM2Daemon
    TVM2Deploy = import_module('daemon_tvm2').TVM2Daemon
