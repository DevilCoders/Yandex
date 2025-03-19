"""
Internal api process run helper
"""

import os
import subprocess
import sys
import time
from random import randint
from types import SimpleNamespace

import requests
from psycopg2.extensions import parse_dsn

from . import project


def _py_api_cmd(cover: bool):
    cmd = []
    env = {}
    if cover:
        cover_binary = os.path.join(os.path.split(sys.executable)[0], 'coverage')
        cmd = [cover_binary, 'run', '-p', '--source=dbaas_internal_api']
        env['COVERAGE_PROCESS_START'] = '.coveragerc'
    else:
        cmd = [sys.executable]
        env['INTERNAL_API_DEBUG'] = '1'
    cmd += [str(project.project_root() / 'run.py')]
    return cmd, env


def _api_cmd(cover: bool):
    try:
        # pylint: disable=import-error, import-outside-toplevel
        import yatest.common

        binary = yatest.common.build_path('cloud/mdb/dbaas-internal-api-image/dbaas_internal_api_dev')
        env = {} if cover else {'INTERNAL_API_DEBUG': '1'}
        return [binary], env
    except ImportError:
        return _py_api_cmd(cover)


def _assign_port(context):
    if hasattr(context, 'internal_api_port'):
        return
    port = os.getenv('DBAAS_INTERNAL_API_RECIPE_PORT')
    if port:
        context.internal_api_port = port
        return
    context.internal_api_port = randint(1024, 65535)


def start(context: SimpleNamespace, config_path='func_tests/configs/config_auth.py', cover=True) -> None:
    """
    Start internal api in background
    """

    _assign_port(context)
    config_path = str(project.project_root() / config_path)
    metadb_dsn = parse_dsn(context.metadb_dsn)

    cmd, env = _api_cmd(cover)
    env.update(
        {
            'INTERNAL_API_PORT': str(context.internal_api_port),
            'METADB_PORT': metadb_dsn['port'],
            'METADB_HOST': metadb_dsn['host'],
            'DBAAS_INTERNAL_API_CONFIG': config_path,
            'TMP_ROOT': str(context.tmp_root),
            'LOGS_ROOT': str(context.logs_root),
        }
    )
    kwargs = {}
    devnull = open(os.devnull, 'w')
    kwargs['stderr'] = devnull
    if 'NO_CAPTURE_STDOUT' in os.environ:
        del kwargs['stderr']
    context.api_process = subprocess.Popen(cmd, env=env, **kwargs)

    deadline = time.time() + context.conf.main.start_timeout

    while time.time() < deadline:
        try:
            resp = requests.get('http://localhost:{port}/ping'.format(port=context.internal_api_port))
            resp.raise_for_status()
            return
        except Exception:
            time.sleep(0.1)

    raise RuntimeError('Internal API startup timeout')


def stop(context):
    """
    Stop internal api
    """
    if getattr(context, 'api_process', None) and not context.api_process.poll():
        context.api_process.terminate()
        context.api_process.wait()
