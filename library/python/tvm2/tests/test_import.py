# coding: utf-8

from __future__ import unicode_literals

import os
import sys

import pytest


def unimport_tvm2():
    if 'tvm2' in sys.modules:
        del sys.modules['tvm2']


@pytest.mark.parametrize('use_daemon_env_var_name', (
    'TVM2_USE_DAEMON',
    'TVM2_USE_QLOUD',
))
def test_tvm2_deploy(use_daemon_env_var_name):
    os.environ[use_daemon_env_var_name] = '1'
    os.environ['DEPLOY_BOX_ID'] = 'backend'
    unimport_tvm2()
    from tvm2 import TVM2
    from tvm2.sync.daemon_tvm2 import TVM2Daemon

    assert TVM2 is TVM2Daemon

    os.environ.pop(use_daemon_env_var_name)
    os.environ.pop('DEPLOY_BOX_ID')


@pytest.mark.parametrize('use_daemon_env_var_name', (
    'TVM2_USE_DAEMON',
    'TVM2_USE_QLOUD',
))
def test_tvm2_qloud(use_daemon_env_var_name):
    os.environ[use_daemon_env_var_name] = '1'
    os.environ['QLOUD_PROJECT'] = 'tools'
    unimport_tvm2()
    from tvm2 import TVM2
    from tvm2.sync.daemon_tvm2 import TVM2Daemon

    assert TVM2 is TVM2Daemon

    os.environ.pop(use_daemon_env_var_name)
    os.environ.pop('QLOUD_PROJECT')


def test_threaded():
    unimport_tvm2()
    from tvm2 import TVM2
    from tvm2.sync.thread_tvm2 import TVM2 as TVM2Threaded

    assert TVM2 is TVM2Threaded


@pytest.mark.skipif(sys.version_info.major < 3, reason="requires python3 or higher")
def test_async():
    unimport_tvm2()
    os.environ['TVM2_USE_QLOUD'] = 'True'
    os.environ['TVM2_ASYNC'] = 'True'
    from tvm2 import TVM2
    from tvm2.aio.daemon_tvm2 import TVM2Daemon

    assert TVM2 is TVM2Daemon

    os.environ.pop('TVM2_USE_QLOUD')
    os.environ.pop('TVM2_ASYNC')
    unimport_tvm2()
