"""
Recipe for dbaas-internal-api
"""
import errno
import logging
import os
import subprocess
import time

import yatest.common
from yatest.common import network
from library.python.testing.recipe import set_env

# we 'save' PortManager,
# cause it 'holds' acquired port
pm = network.PortManager()


def start(_):
    """
    Start api
    """
    cmd = yatest.common.build_path('cloud/mdb/dbaas-internal-api-image/dbaas_internal_api_dev')

    tmp_root = os.path.join(yatest.common.ram_drive_path() or yatest.common.work_path(), 'tmp')
    logs_root = os.path.join(yatest.common.output_ram_drive_path() or yatest.common.output_path(), 'logs')
    os.makedirs(tmp_root, exist_ok=True)
    os.makedirs(logs_root, exist_ok=True)

    api_port = str(pm.get_port())
    env = os.environ.copy()
    env['INTERNAL_API_DEBUG'] = '1'
    env['INTERNAL_API_PORT'] = api_port
    env['TMP_ROOT'] = tmp_root
    env['LOGS_ROOT'] = logs_root
    env['DBAAS_INTERNAL_API_CONFIG'] = yatest.common.source_path(
        'cloud/mdb/dbaas-internal-api-image/func_tests/configs/config_auth.py'
    )
    env['METADB_HOST'] = os.environ.get('METADB_POSTGRESQL_RECIPE_HOST')
    env['METADB_PORT'] = os.environ.get('METADB_POSTGRESQL_RECIPE_PORT')

    api_process = subprocess.Popen(
        cmd,
        env=env,
    )

    with open('dbaas_internal_api.pid', 'w') as out:
        out.write(str(api_process.pid))

    set_env('DBAAS_INTERNAL_API_RECIPE_PORT', api_port)
    set_env('DBAAS_INTERNAL_API_RECIPE_TMP_ROOT', tmp_root)
    set_env('DBAAS_INTERNAL_API_RECIPE_LOGS_ROOT', logs_root)


def _is_running(pid):
    try:
        os.kill(pid, 0)
    except OSError as err:
        if err.errno == errno.ESRCH:
            return False
    return True


def stop(_):
    """
    Stop running api
    """
    if not os.path.exists('dbaas_internal_api.pid'):
        return

    with open('dbaas_internal_api.pid') as inp:
        pid = int(inp.read())

    if _is_running(pid):
        os.kill(pid, 15)

    timeout = 10
    for _ in range(timeout):
        if not _is_running(pid):
            break
        time.sleep(1)

    if _is_running(pid):
        logging.error('dbaas-internal-api is still running after %s seconds', timeout)
        os.kill(pid, 9)
