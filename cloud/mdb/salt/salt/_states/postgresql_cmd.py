#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re

try:
    # Import salt module, but not in arcadia tests
    from salt.ext import six
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}
__grains__ = {}


def __virtual__():
    if 'postgres.psql_exec' in __salt__:
        return 'postgresql_cmd'

    return False


def mod_run_check(onlyif, unless, **kwargs):
    """
    Execute the onlyif and unless logic.
    Return a result dict if:
    * onlyif failed (onlyif != 0)
    * unless succeeded (unless == 0)
    else return True
    """
    # never use VT for onlyif/unless executions because this will lead
    # to quote problems
    cmd_kwargs = {
        'ignore_retcode': True,
        'python_shell': True,
    }
    if onlyif is not None:
        if isinstance(onlyif, six.string_types):
            composed = __salt__['postgres.psql_compose'](onlyif, **kwargs)
            cmd = __salt__['cmd.retcode'](composed, **cmd_kwargs)
            if cmd != 0:
                return {
                    'comment': 'onlyif condition is false',
                    'skip_watch': True,
                    'result': True,
                }
        elif isinstance(onlyif, list):
            for entry in onlyif:
                composed = __salt__['postgres.psql_compose'](entry, **kwargs)
                cmd = __salt__['cmd.retcode'](composed, **cmd_kwargs)
                if cmd != 0:
                    return {
                        'comment': 'onlyif condition is false: {0}'.format(entry),
                        'skip_watch': True,
                        'result': True,
                    }
        elif not isinstance(onlyif, six.string_types):
            if not onlyif:
                return {
                    'comment': 'onlyif condition is false',
                    'skip_watch': True,
                    'result': True,
                }

    if unless is not None:
        if isinstance(unless, six.string_types):
            composed = __salt__['postgres.psql_compose'](unless, **kwargs)
            cmd = __salt__['cmd.retcode'](composed, **cmd_kwargs)
            if cmd == 0:
                return {
                    'comment': 'unless condition is true',
                    'skip_watch': True,
                    'result': True,
                }
        elif isinstance(unless, list):
            cmd = []
            for entry in unless:
                composed = __salt__['postgres.psql_compose'](entry, **kwargs)
                cmd.append(__salt__['cmd.retcode'](composed, **cmd_kwargs))
            if all([c == 0 for c in cmd]):
                return {
                    'comment': 'unless condition is true',
                    'skip_watch': True,
                    'result': True,
                }
        elif not isinstance(unless, six.string_types):
            if unless:
                return {
                    'comment': 'unless condition is true',
                    'skip_watch': True,
                    'result': True,
                }

    # No reason to stop, return True
    return True


def check_psql(user=None, host=None, port=None, maintenance_db=None, password=None, runas=None):
    code, _, _ = __salt__['postgres.psql_exec'](
        'SELECT 1', user=user, host=host, port=port, maintenance_db=maintenance_db, password=password, runas=runas
    )
    return code == 0


def psql_exec(
    name, user=None, host=None, port=None, maintenance_db=None, password=None, runas=None, unless=None, onlyif=None
):
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    cret = mod_run_check(
        onlyif, unless, user=user, host=host, port=port, maintenance_db=maintenance_db, password=password, runas=runas
    )

    if __opts__['test'] and cret is True:
        if check_psql(user=user, host=host, port=port, maintenance_db=maintenance_db, password=password, runas=runas):
            ret['comment'] = 'PostgreSQL command "{cmd}" would have been executed'.format(cmd=name)
        else:
            ret['result'] = False
            ret['comment'] = 'PostgreSQL is dead'
        return ret

    if isinstance(cret, dict):
        ret.update(cret)
        return ret

    code, stdout, stderr = __salt__['postgres.psql_exec'](
        name, user=user, host=host, port=port, maintenance_db=maintenance_db, password=password, runas=runas
    )

    ret['result'] = code == 0

    ret['changes']['psql_exec'] = name

    ret['comment'] = 'stdout: {stdout}\nstderr: {stderr}'.format(stdout=stdout, stderr=stderr)

    return ret


def psql_file(
    name, user=None, host=None, port=None, maintenance_db=None, password=None, runas=None, unless=None, onlyif=None
):
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    cret = mod_run_check(
        onlyif, unless, user=user, host=host, port=port, maintenance_db=maintenance_db, password=password, runas=runas
    )

    if __opts__['test'] and cret is True:
        ret['comment'] = 'PostgreSQL file "{file_name}" ' 'would have been executed'.format(file_name=name)
        return ret

    if isinstance(cret, dict):
        ret.update(cret)
        return ret

    code, stdout, stderr = __salt__['postgres.psql_file'](
        name, user=user, host=host, port=port, maintenance_db=maintenance_db, password=password, runas=runas
    )

    ret['result'] = code == 0

    ret['changes']['psql_file'] = name

    ret['comment'] = 'stdout: {stdout}\nstderr: {stderr}'.format(stdout=stdout, stderr=stderr)

    return ret


def set_master(name, hosts='data:dbaas:shard_hosts', user=None, port=None, password=None, timeout=60):
    """
    Find master within hosts and set `pg-master` in pillar
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    master = __salt__['postgres.find_master'](hosts, user, port, password, timeout)

    if master:
        ret['result'] = True
        ret['comment'] = 'Found master at {name}'.format(name=master)
        __pillar__['pg-master'] = master
    else:
        ret['result'] = False
        ret['comment'] = 'Unable to find master'

    return ret


def populate_recovery_conf(
    name, application_name=None, use_replication_slots=True, recovery_min_apply_delay=None, use_restore_command=True
):
    """
    Create recovery.conf
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    # replication source or master
    master = __salt__['pillar.get']('pg-master', __salt__['pillar.get']('data:pgsync:replication_source'))

    if os.path.exists(name):
        with open(name) as inp:
            content = inp.readlines()

        current_recovery_min_apply_delay = None
        need_change_recovery_conf = False
        primary_conninfo_present = False
        primary_slot_name_present = False

        for line in content:
            if '/etc/wal-g/envdir' in line:
                need_change_recovery_conf = True
            match = re.match("recovery_min_apply_delay = '(.*?)'", line)
            if match:
                current_recovery_min_apply_delay = match.groups()[0]

            if line.startswith('primary_conninfo = '):
                primary_conninfo_present = True
                if master not in line or application_name not in line:
                    need_change_recovery_conf = True

            if line.startswith('primary_slot_name = '):
                primary_slot_name_present = True

        if recovery_min_apply_delay != current_recovery_min_apply_delay:
            need_change_recovery_conf = True

        if not primary_conninfo_present:
            need_change_recovery_conf = True

        if use_replication_slots != primary_slot_name_present:
            need_change_recovery_conf = True

        if not need_change_recovery_conf:
            ret['result'] = True
            ret['comment'] = 'recovery.conf already present'
            return ret

    cmd_kwargs = {
        'ignore_retcode': True,
        'python_shell': True,
        'runas': 'postgres',
        'group': 'postgres',
    }

    cmd = ' '.join(
        [
            '/usr/local/yandex/populate_recovery_conf.py',
            '-p',
            name,
            '-s' if not use_replication_slots else '',
            '-e' if not use_restore_command else '',
            '-d {recovery_min_apply_delay}'.format(recovery_min_apply_delay=recovery_min_apply_delay)
            if recovery_min_apply_delay is not None
            else '',
            master,
        ]
    )

    if __opts__['test']:
        ret['comment'] = '{cmd} would be executed'.format(cmd=cmd)
    else:
        res = __salt__['cmd.retcode'](cmd, **cmd_kwargs)
        if res == 0:
            ret['result'] = True
            ret['changes'] = {name: 'created'}
        else:
            ret['result'] = False
            ret['comment'] = '{cmd} exited with {code}'.format(cmd=cmd, code=res)

    return ret


def create_replication_slot(name, user=None, port=None, password=None, control_path=None, force=False):
    """
    Create replication slot on master
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    if control_path and os.path.exists(control_path) and not force:
        ret['result'] = True
        ret['comment'] = '{path} exists. Skipping slot creation'.format(path=control_path)

        return ret

    master = __salt__['pillar.get']('pg-master', __salt__['pillar.get']('data:pgsync:replication_source'))

    query_result = __salt__['postgres.psql_query'](
        query=('SELECT slot_name FROM pg_replication_slots ' "WHERE slot_name = '{name}'").format(name=name),
        host=master,
        user=user,
        port=port,
        password=password,
        maintenance_db='postgres',
    )

    if query_result:
        ret['result'] = True
        ret['comment'] = 'Replication slot {name} exists'.format(name=name)
    elif __opts__['test']:
        ret['changes'] = {
            '{host}.replication_slot.{name}'.format(host=master, name=name): 'pending create',
        }
        ret['comment'] = 'Replication slot {name} would be created'.format(name=name)
    else:
        create_result = __salt__['postgres.psql_query'](
            query=('SELECT pg_create_physical_' "replication_slot('{name}')").format(name=name),
            host=master,
            user=user,
            port=port,
            password=password,
            maintenance_db='postgres',
        )

        if create_result:
            ret['changes'] = {
                '{host}.replication_slot.{name}'.format(host=master, name=name): 'created',
            }
            ret['result'] = True
            ret['comment'] = 'Replication slot {name} created on {host}'.format(
                name=name,
                host=__salt__['pillar.get']('pg-master', __salt__['pillar.get']('data:pgsync:replication_source')),
            )
        else:
            ret['result'] = False
            ret['comment'] = 'Unable to create replication slot {name} on {host}'.format(
                name=name,
                host=__salt__['pillar.get']('pg-master', __salt__['pillar.get']('data:pgsync:replication_source')),
            )

    return ret


def replica_init(name, method, version_major_num=None, server=None):
    """
    Fetch initialized postgresql cluster data directory
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    control_path = os.path.join(name, 'global/pg_control')
    if os.path.exists(control_path):
        ret['result'] = True
        ret['comment'] = '{path} exists. Skipping pg-init'.format(path=control_path)

        return ret

    cmd_kwargs = {
        'runas': 'postgres',
        'group': 'postgres',
    }

    if method == 'barman':
        cmd = (
            'ssh -oStrictHostKeyChecking=no barman-wal-restore@{server} '
            '/usr/bin/sudo -u robot-pgbarman '
            '/usr/local/yandex/barman_restore_last_backup.py '
            '-r {host} -d {path}'
        ).format(server=server, host=__grains__['id'], path=name)
    elif method == 'wal-g':
        cmd = 'wal-g backup-fetch {path} LATEST --config /etc/wal-g/wal-g.yaml --turbo'.format(path=name)
    elif method == 'basebackup':
        master = __salt__['pillar.get']('pg-master', __salt__['pillar.get']('data:pgsync:replication_source'))
        cmd = (
            'pg_basebackup --pgdata={path} --checkpoint=fast '
            '--wal-method=none --dbname="host={master} '
            'port=5432 dbname=postgres user=repl"'
        ).format(path=name, master=master)
    else:
        ret['result'] = False
        ret['comment'] = 'Unknown method {method}'.format(method=method)
        return ret

    if __opts__['test']:
        ret['changes'] = {'pg-init.{path}'.format(path=name): 'pending execution'}
        ret['comment'] = '{cmd} would be executed'.format(cmd=cmd)
    else:
        res = __salt__['cmd.retcode'](cmd, **cmd_kwargs)
        if res != 0:
            ret['result'] = False
            ret['comment'] = '{cmd} exited with {code}'.format(cmd=cmd, code=res)
        else:
            ret['changes'] = {'pg-init.{path}'.format(path=name): 'executed'}
            ret['comment'] = '{cmd} exited with {code}'.format(cmd=cmd, code=res)
            ret['result'] = True

    return ret


def master_init(name, version, pwfile=None):
    """
    Initialize new postgresql cluster
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    control_path = os.path.join(name, 'global/pg_control')
    if os.path.exists(control_path):
        ret['result'] = True
        ret['comment'] = '{path} exists. Skipping pg-init'.format(path=control_path)

        return ret

    command = 'pg_createcluster --port=5432 {version} data'.format(version=version)
    if pwfile:
        command += ' -- --pwfile=' + str(pwfile)

    if __opts__['test']:
        ret['changes'] = {'pg-init.{path}'.format(path=name): 'pending execution'}
        ret['comment'] = '{command} would be executed'.format(command=command)
    else:
        res = __salt__['cmd.retcode'](command)
        if res != 0:
            ret['result'] = False
            ret['comment'] = '{command} exited with {code}'.format(command=command, code=res)
        else:
            ret['changes'] = {'pg-init.{path}'.format(path=name): 'executed'}
            ret['comment'] = '{cmd} exited with {code}'.format(cmd=command, code=res)
            ret['result'] = True

    return ret
