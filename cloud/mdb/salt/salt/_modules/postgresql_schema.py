#!/usr/bin/env python

import logging
import os
import json
from salt.exceptions import CommandExecutionError

log = logging.getLogger(__name__)
CACHE = {'path': '/opt/yandex/pgmigrate/bin/pgmigrate'}


def _check_path():
    return os.path.isfile(CACHE['path']) and os.access(CACHE['path'], os.X_OK)


def __virtual__():
    new_ok = _check_path()
    if new_ok:
        return True

    CACHE['path'] = '/usr/local/yandex/pgmigrate/pgmigrate.py'
    return _check_path()


def info(base, dbname, baseline=None, dryrun=False, target=None, runas='postgres', conn=None, session=None):
    cmd = CACHE['path'][:]
    cmd += " -d " + os.path.join(base, dbname)
    if conn:
        cmd += " -c " + conn
    if baseline:
        cmd += " -b " + str(baseline)
    if dryrun:
        cmd += " -n "
    if target:
        cmd += " -t " + str(target)
    if session:
        cmd += " -s " + session
    cmd += " info"

    result = __salt__['cmd.run_all'](cmd, cwd='/', runas=runas, group=runas)

    if result['retcode'] != 0:
        raise CommandExecutionError(cmd + '\nstdout:\n' + result['stdout'] + '\nstderr:\n' + result['stderr'])

    stdout = json.loads(result['stdout'])

    return stdout


def migrate(
    base,
    dbname,
    target,
    callbacks={},
    baseline=None,
    termination_interval=0.1,
    dryrun=False,
    runas='postgres',
    conn=None,
    session=None,
):
    cmd = CACHE['path'][:]
    cmd += " -d " + os.path.join(base, dbname)
    cmd += " -t " + str(target)
    if conn:
        cmd += " -c " + conn
    if session:
        cmd += " -s " + session
    if baseline:
        cmd += " -b " + str(baseline)
    if dryrun:
        cmd += " -n "
    if termination_interval:
        cmd += " -l " + str(termination_interval)
    if callbacks:
        cblist = []
        for cbt in callbacks:
            for callback in callbacks[cbt]:
                cblist.append(cbt + ':' + callback)
        cmd += " -a " + ','.join(cblist)
    cmd += " migrate"

    result = __salt__['cmd.run_all'](cmd, cwd='/', runas=runas, group=runas)

    if result['retcode'] != 0:
        raise CommandExecutionError(cmd + '\nstdout:\n' + result['stdout'] + '\nstderr:\n' + result['stderr'])

    return {'result': True, 'steps': result['stdout'].split('\n')}
