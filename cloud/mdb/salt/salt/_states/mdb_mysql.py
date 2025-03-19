#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
MySQL additional management commands for MDB
"""

import datetime
import logging
import os

from contextlib import closing

try:
    import six
except ImportError:
    from salt.ext import six

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}


log = logging.getLogger(__name__)


def __virtual__():
    return True


def _is_datadir_empty(datadir, ignore_autocnf=False):
    for entry in os.listdir(datadir):
        if entry == 'auto.cnf' and ignore_autocnf:
            continue
        if entry in ['lost+found', '.tmp']:
            continue
        return False
    return True


def _cleanup_datadir(datadir, ignore_autocnf=False):
    for entry in os.listdir(datadir):
        if entry == 'auto.cnf' and ignore_autocnf:
            continue
        path = os.path.join(datadir, entry)
        __salt__['cmd.run']("rm -rf " + path)


def master_init(name, datadir, initfile="/etc/mysql/init.sql"):
    """
    Initialized master data directory
    """

    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    if not _is_datadir_empty(datadir):
        ret['result'] = True
        ret['comment'] = '{datadir} not empty. Skipping {name}.'.format(datadir=datadir, name=name)
        return ret

    cmds = []

    cmds.append(["rm -rf {datadir}/lost+found".format(datadir=datadir), {}])

    # initialize a new one
    cmds.append(
        [
            "/usr/sbin/mysqld --initialize --default-time-zone=SYSTEM --datadir={datadir} --init-file={initfile}".format(
                datadir=datadir, initfile=initfile
            ),
            {'runas': 'mysql', 'group': 'mysql'},
        ]
    )
    # initialize timezones, running mysql on high port
    init_timezones_script = "/usr/local/yandex/mysql_init_timezones.py"
    if os.path.exists(init_timezones_script):
        cmds.append([init_timezones_script, {'runas': 'mysql', 'group': 'mysql'}])

    if __opts__['test']:
        for cmd_opts in cmds:
            ret['comment'].append('{cmd} would be executed'.format(cmd=cmd_opts[0]))
        ret['changes'] = {name: 'pending execution'}
        ret['result'] = None
    else:
        for cmd_opts in cmds:
            cmd, opts = cmd_opts
            res = __salt__['cmd.retcode'](cmd, **opts)
            ret['comment'].append('{cmd} exited with {code}'.format(cmd=cmd, code=res))
            if res != 0:
                ret['result'] = False
                return ret
        ret['changes'] = {name: 'executed'}
        ret['result'] = True

    return ret


def replica_init(name, datadir, server=None, ssh_user='mysql', backup_threads=1):
    """
    Fetches cluster backup and prepares data directory
    """

    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    if not server:
        server = __pillar__.get('mysql-master', None)

    if not server:
        ret['result'] = False
        ret['comment'] = 'Missing master server'
        return ret

    if not _is_datadir_empty(datadir, ignore_autocnf=True):
        ret['result'] = True
        ret['comment'] = '{datadir} not empty. Skipping {name}.'.format(datadir=datadir, name=name)
        return ret

    cmds = []
    cmds.append(["rm -rf {datadir}/lost+found {datadir}/.tmp".format(datadir=datadir), {}])
    cmds.append(
        [
            (
                "ssh {ssh_user}@{server} 'xtrabackup --backup --stream=xbstream "
                " --lock-ddl --lock-ddl-timeout=3600 --parallel={backup_threads}' "
                " | xbstream --extract --directory={datadir} --parallel={backup_threads}"
            ).format(ssh_user=ssh_user, server=server, datadir=datadir, backup_threads=max(1, int(backup_threads))),
            {
                'runas': 'mysql',
                'group': 'mysql',
                'use_vt': True,
            },
        ]
    )

    cmds.append(
        [
            "xtrabackup --prepare --target-dir={datadir}".format(datadir=datadir),
            {
                'runas': 'mysql',
                'group': 'mysql',
            },
        ]
    )

    if __opts__['test']:
        for cmd_opts in cmds:
            ret['comment'].append('{cmd} whoud be executed'.format(cmd=cmd_opts[0]))
        ret['changes'] = {name: 'pending execution'}
        ret['result'] = None
    else:
        for cmd_opts in cmds:
            cmd, opts = cmd_opts
            res = __salt__['cmd.retcode'](cmd, **opts)
            ret['comment'].append('{cmd} exited with {code}'.format(cmd=cmd, code=res))
            if res != 0:
                _cleanup_datadir(datadir, ignore_autocnf=True)
                ret['result'] = False
                return ret
        ret['changes'] = {name: 'executed'}
        ret['result'] = True

    return ret


def replica_init2(
    name,
    datadir,
    connection_default_file,
    mode,
    server=None,
    ssh_user='mysql',
    backup_threads=1,
    use_memory=None,
    walg_config=None,
    backup_name=None,
    max_live_binlog_wait=0,
):
    """
    Fetches cluster backup and prepares data directory
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    if not _is_datadir_empty(datadir, ignore_autocnf=True):
        ret['result'] = True
        ret['comment'] = '{datadir} not empty. Skipping {name}.'.format(datadir=datadir, name=name)
        return ret

    cmds = []
    cmds.append(["rm -rf {datadir}/lost+found {datadir}/.tmp".format(datadir=datadir), {}])

    # for both WALG and WALG-NO-BINLOGS we fetch binlogs from s3
    if mode in ('WALG', 'WALG-NO-BINLOGS'):
        cmd = (
            "/usr/local/yandex/mysql_restore.py walg-fetch "
            "--datadir={datadir} "
            "--defaults-file={connection_default_file} "
            "--port=3308 "
            "--walg-config={walg_config} "
            "--backup-name={backup_name} "
            ""
        ).format(
            datadir=datadir,
            connection_default_file=connection_default_file,
            walg_config=walg_config,
            backup_name=backup_name,
        )
    else:
        if not server:
            server = __pillar__.get('mysql-master', None)

        if not server:
            ret['result'] = False
            ret['comment'] = 'Missing master server'
            return ret

        cmd = (
            "/usr/local/yandex/mysql_restore.py ssh-fetch "
            "--datadir={datadir} "
            "--defaults-file={connection_default_file} "
            "--server={server} "
            "--ssh-user={ssh_user} "
            "--backup_threads={backup_threads} "
            "--port=3308 "
            ""
        ).format(
            datadir=datadir,
            connection_default_file=connection_default_file,
            server=server,
            ssh_user=ssh_user,
            backup_threads=backup_threads,
        )
        if use_memory is not None:
            cmd += "--use_memory={use_memory} ".format(use_memory=use_memory)

    cmds.append(
        [
            cmd,
            {
                'runas': 'mysql',
                'group': 'mysql',
                'use_vt': True,
            },
        ]
    )

    if mode == 'WALG':
        stop_datetime = datetime.datetime.utcnow() + datetime.timedelta(seconds=max_live_binlog_wait)
        stop_datetime = stop_datetime.strftime("%Y-%m-%dT%H:%M:%SZ")
        cmds.append(
            [
                (
                    "/usr/local/yandex/mysql_restore.py apply-binlogs "
                    "--datadir={datadir} "
                    "--defaults-file={connection_default_file} "
                    "--walg-config={walg_config} "
                    "--backup-name={backup_name} "
                    "--stop-datetime={stop_datetime} "
                    "--port=3308 "
                    ""
                ).format(
                    datadir=datadir,
                    connection_default_file=connection_default_file,
                    walg_config=walg_config,
                    backup_name=backup_name,
                    stop_datetime=stop_datetime,
                ),
                {
                    'runas': 'mysql',
                    'group': 'mysql',
                },
            ]
        )

    if __opts__['test']:
        for cmd_opts in cmds:
            ret['comment'].append('{cmd} would be executed'.format(cmd=cmd_opts[0]))
        ret['changes'] = {name: 'pending execution'}
        ret['result'] = None
    else:
        for cmd_opts in cmds:
            cmd, opts = cmd_opts
            log.debug("executing {cmd} {opts}".format(cmd=cmd, opts=opts))
            res = __salt__['cmd.retcode'](cmd, **opts)
            ret['comment'].append('{cmd} exited with {code}'.format(cmd=cmd, code=res))
            if res != 0:
                _cleanup_datadir(datadir, ignore_autocnf=True)
                ret['result'] = False
                return ret
        ret['changes'] = {name: 'executed'}
        ret['result'] = True

    return ret


def set_master_writable(name, **connection_args):
    """
    Sets read_only to OFF on master server in order to create users and so on
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    sql = "SHOW SLAVE STATUS"
    res = __salt__['mysql.query']('mysql', sql, **connection_args)
    if not res:
        ret['result'] = False
        ret['comment'] = 'Failed to execute: {sql}'.format(sql=sql)
        return ret
    if res['results']:
        ret['result'] = True
        ret['comment'] = 'Not needed on slave'
        return ret

    sql = 'SELECT @@read_only'
    res = __salt__['mysql.query']('mysql', sql, **connection_args)
    if not res or not res['results']:
        ret['result'] = False
        ret['comment'] = 'Failed to execute: {sql}'.format(sql=sql)
        return ret
    if int(res['results'][0][0]) == 0:
        ret['result'] = True
        ret['comment'] = 'Master already writable'
        return ret

    sql = 'SET GLOBAL read_only = 0'
    ret['changes'] = {'read_only': {'old': '1', 'new': '0'}}
    if __opts__['test']:
        ret['result'] = None
        ret['comment'] = 'read_only should be set to 0'
        return ret
    res = __salt__['mysql.query']('mysql', sql, **connection_args)
    if not res:
        ret['result'] = False
        ret['comment'] = 'Failed to execute: {sql}'.format(sql=sql)
        return ret
    ret['result'] = True
    ret['comment'] = 'master set writable'
    return ret


def set_gtid_purged_from_snapshot(name, datadir, **connection_args):
    """
    Updates GTID_PURGED values to be consistent with xtrabackup snapshot.
    It will prevent slave to apply transactions already received with snapshot.
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }
    # xtrabackup_binlog_info created by xtrabackup
    binlog_info_path = os.path.join(datadir, 'xtrabackup_binlog_info')
    if not os.path.exists(binlog_info_path):
        ret['result'] = True
        ret['comment'] = ('{path} does not exists.' 'No need in updating GTID_EXECUTED.').format(path=binlog_info_path)
        return ret
    snapshot_gtid_executed = ''
    try:
        with open(binlog_info_path, 'r') as fhndl:
            # gtid set may be splitted across multiple lines
            line = "".join(l.strip() for l in fhndl.readlines())
            snapshot_gtid_executed = line.split('\t')[2].strip()
    except Exception:
        ret['result'] = False
        ret['comment'] = ('{path} is malformed. ' 'Aborting setting @@GLOBAL.GTID_PURGED').format(path=binlog_info_path)
        return ret
    sql = (
        "SELECT @@GLOBAL.GTID_EXECUTED, "
        "GTID_SUBSET('{snapshot_gitd}', @@GLOBAL.GTID_EXECUTED), "
        "GTID_SUBSET(@@GLOBAL.GTID_EXECUTED, '{snapshot_gitd}')"
    ).format(snapshot_gitd=snapshot_gtid_executed)
    res = __salt__['mysql.query']('mysql', sql, **connection_args)
    if not res and res.get('results'):
        ret['result'] = False
        ret['comment'] = "Failed to execute: {sql}".format(sql=sql)
        return ret
    gtid_executed, is_superset, is_subset = res['results'][0][0], int(res['results'][0][1]), int(res['results'][0][2])
    log.debug(
        'GTID_EXECUTED \'%s\' xtrabackup_binlog_info GTID_EXECUTED: \'%s\'', gtid_executed, snapshot_gtid_executed
    )
    if is_superset:
        ret['result'] = True
        return ret
    if not is_subset:
        ret['changes'] = {
            name: 'MySQL has incorrect GTID_EXECUTED. ' 'It does NOT include GTID_EXECUTED from xtrabackup_binlog_info',
        }
        ret['result'] = False
        return ret
    sqls = [
        "RESET MASTER",
        "SET @@GLOBAL.GTID_PURGED='{snapshot_gitd}'".format(snapshot_gitd=snapshot_gtid_executed),
    ]
    if __opts__['test']:
        for sql in sqls:
            ret['comment'].append('{sql} whould be executed'.format(sql=sql))
        ret['changes'] = {
            name: 'GTID_PURGED would be set to: ' + snapshot_gtid_executed,
        }
        ret['result'] = None
    else:
        for sql in sqls:
            res = __salt__['mysql.query']('mysql', sql, **connection_args)
            if not res:
                ret['result'] = False
                ret['comment'] = "Failed to execute: {sql}".format(sql=sql)
                return ret
        ret['changes'] = {
            name: 'GTID_PURGED was set to: ' + snapshot_gtid_executed,
        }
        ret['result'] = True
    return ret


def setup_replication(
    name,
    user,
    password,
    server=None,
    master_port=3306,
    master_heartbeat_period=2,
    master_connect_retry=10,
    master_retry_count=0,
    master_ssl=False,
    master_ssl_ca=None,
    **connection_args
):
    """
    Starts replication on slave server.
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }
    if not server:
        server = __pillar__.get('mysql-master', None)

    if not server:
        ret['result'] = False
        ret['comment'] = 'Missing master server'
    if not user:
        ret['result'] = False
        ret['comment'] = 'Missing user'
    if not password:
        ret['result'] = False
        ret['comment'] = 'Missing password'
    if ret['result'] is False:
        return ret

    res = __salt__['mysql.get_slave_status'](**connection_args)
    if res and res['Slave_IO_Running'] == 'Yes' and res['Slave_SQL_Running'] == 'Yes' and res['Master_Host'] == server:
        ret['result'] = True
        ret['comment'] = 'Replication already set up and running'
        return ret

    sqls = []

    sqls += ["STOP SLAVE"]
    sqls += ["RESET SLAVE"]
    sqls += [
        """
            CHANGE MASTER TO
                MASTER_HOST='{master_host}',
                MASTER_PORT={master_port},
                MASTER_USER='{master_user}',
                MASTER_PASSWORD='{master_pass}',
                MASTER_AUTO_POSITION=1,
                MASTER_SSL={master_ssl},
                MASTER_SSL_CA='{master_ssl_ca}',
                MASTER_SSL_VERIFY_SERVER_CERT={master_ssl_verify},
                MASTER_HEARTBEAT_PERIOD={master_heartbeat_period},
                MASTER_CONNECT_RETRY={master_connect_retry},
                MASTER_RETRY_COUNT={master_retry_count}
        """.format(
            master_host=server,
            master_port=master_port,
            master_user=user,
            master_pass=password,
            master_heartbeat_period=master_heartbeat_period,
            master_connect_retry=master_connect_retry,
            master_retry_count=master_retry_count,
            master_ssl=(1 if master_ssl else 0),
            master_ssl_ca=(master_ssl_ca or ''),
            master_ssl_verify=(1 if master_ssl else 0),
        ),
    ]
    sqls += ["START SLAVE"]

    if __opts__['test']:
        for sql in sqls:
            ret['comment'].append('{sql} would be executed'.format(sql=sql))
        ret['changes'] = {name: 'pending execution'}
        ret['result'] = None
    else:
        for sql in sqls:
            res = __salt__['mysql.query']('mysql', sql, **connection_args)
            if not res:
                ret['result'] = False
                ret['comment'] = "Failed to execute: {sql}".format(sql=sql)
                return ret
        ret['changes'] = {name: 'executed'}
        ret['result'] = True

    return ret


def set_master(name, user, password, hosts='data:dbaas:shard_hosts', port=3306, timeout=60):
    """
    Find master within hosts and set `mysql-master` in pillar
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    master = __salt__['mdb_mysql.find_master'](
        hosts=hosts,
        user=user,
        port=port,
        password=password,
        timeout=timeout,
    )

    if master:
        ret['result'] = True
        ret['comment'] = 'Found master at {name}'.format(name=master)
        __pillar__['mysql-master'] = master
    else:
        ret['result'] = False
        ret['comment'] = 'Unable to find master'

    return ret


CONNECTION_DEFAULT_FILE = '/home/mysql/.my.cnf'

SYSTEM_USERS = ('admin', 'monitor', 'repl')

SYSTEM_DATABASES = ('mysql', 'performance_schema', 'sys')

# functions required for sys views
SYS_VIEW_FUNCS = ('format_bytes', 'format_time', 'format_statement', 'format_path', 'sys_get_config')

LIMIT_2_QUERY = {
    'MAX_QUERIES_PER_HOUR': 'max_questions',
    'MAX_UPDATES_PER_HOUR': 'max_updates',
    'MAX_CONNECTIONS_PER_HOUR': 'max_connections',
    'MAX_USER_CONNECTIONS': 'max_user_connections',
}

LIMITS = list(LIMIT_2_QUERY.keys())


# https://dev.mysql.com/doc/refman/8.0/en/privileges-provided.html#privileges-provided-summary
# this table may contain grants not existing in particular MySQL version, it's fine
GRANT2FIELD = {
    'ALTER': 'Alter_priv',
    'ALTER ROUTINE': 'Alter_routine_priv',
    'CREATE': 'Create_priv',
    'CREATE ROLE': 'Create_role_priv',
    'CREATE ROUTINE': 'Create_routine_priv',
    'CREATE TABLESPACE': 'Create_tablespace_priv',
    'CREATE TEMPORARY TABLES': 'Create_tmp_table_priv',
    'CREATE USER': 'Create_user_priv',
    'CREATE VIEW': 'Create_view_priv',
    'DELETE': 'Delete_priv',
    'DROP': 'Drop_priv',
    'DROP ROLE': 'Drop_role_priv',
    'EVENT': 'Event_priv',
    'EXECUTE': 'Execute_priv',
    'FILE': 'File_priv',
    'GRANT OPTION': 'Grant_priv',
    'INDEX': 'Index_priv',
    'INSERT': 'Insert_priv',
    'LOCK TABLES': 'Lock_tables_priv',
    'PROCESS': 'Process_priv',
    'REFERENCES': 'References_priv',
    'RELOAD': 'Reload_priv',
    'REPLICATION CLIENT': 'Repl_client_priv',
    'REPLICATION SLAVE': 'Repl_slave_priv',
    'SELECT': 'Select_priv',
    'SHOW DATABASES': 'Show_db_priv',
    'SHOW VIEW': 'Show_view_priv',
    'SHUTDOWN': 'Shutdown_priv',
    'SUPER': 'Super_priv',
    'TRIGGER': 'Trigger_priv',
    'UPDATE': 'Update_priv',
}


FIELD2GRANT = {v: k for k, v in GRANT2FIELD.items()}

# List of grants defined by plugins.
# Can't figure out full list, so just hardcode all needed here
# https://dev.mysql.com/doc/refman/8.0/en/privileges-provided.html#priv_application-password-admin
DYNAMIC_PRIVILEGES = [
    'APPLICATION_PASSWORD_ADMIN',
    'BACKUP_ADMIN',
    'BINLOG_ADMIN',
    'BINLOG_ENCRYPTION_ADMIN',
    'CONNECTION_ADMIN',
    'ENCRYPTION_KEY_ADMIN',
    'GROUP_REPLICATION_ADMIN',
    'PERSIST_RO_VARIABLES_ADMIN',
    'REPLICATION_SLAVE_ADMIN',
    'RESOURCE_GROUP_ADMIN',
    'RESOURCE_GROUP_USER',
    'ROLE_ADMIN',
    'SERVICE_CONNECTION_ADMIN',
    'SESSION_VARIABLES_ADMIN',
    'SET_USER_ID',
    'SYSTEM_VARIABLES_ADMIN',
    'XA_RECOVER_ADMIN',
]


# TODO: move to mdb_mysql_users after MDB-17686
def ensure_grants(name, connection_default_file, target_user=None, target_database=None):
    users, _, idm_system_users, _ = __salt__['mdb_mysql_users.get_users'](filter=target_user)
    for k, v in idm_system_users.items():
        users[k] = v
    databases = ['*'] + list(SYSTEM_DATABASES) + __salt__['pillar.get']('data:mysql:databases', [])

    ret = {
        'name': name,
        'result': True,
        'comment': [],
        'changes': {},
    }

    all_global_grants = []
    all_db_grants = []
    desired_grants = {}
    real_grants = {}
    changes = []

    # preload MySQLdb driver
    __salt__['mysql.query']('mysql', 'SELECT 1', connection_default_file=connection_default_file)
    import MySQLdb

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        cur.execute('SELECT VERSION() AS version')
        version = cur.fetchone()['version'].split('-')[0]
        version = tuple(int(i) for i in version.split('.'))

        # identify all possible grants
        cur.execute(
            '''
            SELECT COLUMN_NAME
            FROM information_schema.COLUMNS
            WHERE TABLE_SCHEMA = 'mysql'
              AND TABLE_NAME = 'user'
              AND COLUMN_NAME LIKE '%_priv'
        '''
        )
        for row in cur.fetchall():
            if row['COLUMN_NAME'] in FIELD2GRANT:
                all_global_grants.append(FIELD2GRANT[row['COLUMN_NAME']])
        if version >= (8, 0):
            all_global_grants += DYNAMIC_PRIVILEGES
        if version >= (8, 0, 16):
            all_global_grants += ['SYSTEM_USER', 'TABLE_ENCRYPTION_ADMIN']
        if version >= (8, 0, 23):
            all_global_grants += ['FLUSH_TABLES']

        cur.execute(
            '''
            SELECT COLUMN_NAME
            FROM information_schema.COLUMNS
            WHERE TABLE_SCHEMA = 'mysql'
              AND TABLE_NAME = 'db'
              AND COLUMN_NAME LIKE '%_priv'
        '''
        )
        for row in cur.fetchall():
            if row['COLUMN_NAME'] in FIELD2GRANT:
                all_db_grants.append(FIELD2GRANT[row['COLUMN_NAME']])

        # prepare user grants from pillar
        for user, props in users.items():
            if target_user and user != target_user:
                continue
            full_user = (user, '%')
            desired_grants.setdefault(full_user, {})
            for db in databases:
                all_grants = all_global_grants if db == '*' else all_db_grants
                if target_database and db != target_database:
                    continue
                grants = props.get('dbs', {}).get(db, [])
                if 'ALL' in grants or 'ALL PRIVILEGES' in grants:
                    grants = all_grants
                grants = set(grants) & set(all_grants)
                # common grants hack
                if grants:
                    grants.add('GRANT OPTION')
                desired_grants[full_user][db] = grants
            # give it as default
            if '*' not in desired_grants[full_user]:
                desired_grants[full_user]['*'] = set()
            desired_grants[full_user]['*'].add('REPLICATION CLIENT')
            # PROCESS priv also give select to query mysql and perf_schema databases
            if 'PROCESS' in desired_grants[full_user]['*']:
                for db in SYSTEM_DATABASES:
                    desired_grants[full_user].setdefault(db, set())
                    desired_grants[full_user][db].add('SELECT')

        _append_system_users(desired_grants, all_global_grants, version, target_user)

        # load actual user grants from database
        cur.execute('SELECT * FROM mysql.db')
        for row in cur.fetchall():
            if target_user and row['User'] != target_user:
                continue
            if target_database and row['Db'] != target_database:
                continue
            full_user = (row['User'], row['Host'])
            db = row['Db']
            grants = []
            for field, grant in FIELD2GRANT.items():
                if row.get(field, 'N') == 'Y':
                    grants.append(grant)
            real_grants.setdefault(full_user, {})
            real_grants[full_user][db] = set(grants)

        cur.execute('SELECT * FROM mysql.user')
        for row in cur.fetchall():
            if target_user and row['User'] != target_user:
                continue
            full_user = (row['User'], row['Host'])
            grants = []
            for field, grant in FIELD2GRANT.items():
                if row.get(field, 'N') == 'Y':
                    grants.append(grant)
            real_grants.setdefault(full_user, {})
            real_grants[full_user]['*'] = set(grants)
        if version >= (8, 0):
            cur.execute('SELECT USER, HOST, PRIV FROM mysql.global_grants')
            for row in cur.fetchall():
                if target_user and row['USER'] != target_user:
                    continue
                full_user = (row['USER'], row['HOST'])
                real_grants.setdefault(full_user, {})
                real_grants[full_user]['*'].add(row['PRIV'])

        sys_func_grants = {}
        cur.execute(
            '''
            SELECT * FROM mysql.procs_priv
            WHERE Db = 'sys'
              AND Routine_type = 'FUNCTION'
              AND FIND_IN_SET(Proc_priv, 'Execute')
        '''
        )
        for row in cur.fetchall():
            if target_user and row['User'] != target_user:
                continue
            full_user = (row['User'], row['Host'])
            sys_func_grants.setdefault(full_user, set())
            sys_func_grants[full_user].add(row['Routine_name'])

        # calculate required changes
        for full_user in desired_grants:
            for db in databases:
                if target_database and db != target_database:
                    continue
                db_quoted = db if db == '*' else '`' + db + '`'
                d_grants = desired_grants.get(full_user, {}).get(db, set())
                r_grants = real_grants.get(full_user, {}).get(db, set())
                for grant in d_grants - r_grants:
                    if grant == 'GRANT OPTION':
                        changes.append(
                            [
                                'GRANT USAGE ON {0}.* TO %(user)s@%(host)s WITH GRANT OPTION'.format(db_quoted),
                                {'user': full_user[0], 'host': full_user[1]},
                            ]
                        )
                    else:
                        changes.append(
                            [
                                'GRANT {0} ON {1}.* TO %(user)s@%(host)s'.format(grant, db_quoted),
                                {'user': full_user[0], 'host': full_user[1]},
                            ]
                        )
                for grant in r_grants - d_grants:
                    changes.append(
                        [
                            'REVOKE {0} ON {1}.* FROM %(user)s@%(host)s'.format(grant, db_quoted),
                            {'user': full_user[0], 'host': full_user[1]},
                        ]
                    )
            # PROCESS priv also give select to query mysql and perf_schema databases
            if full_user[0] not in SYSTEM_USERS:
                for func in SYS_VIEW_FUNCS:
                    d_grants = desired_grants.get(full_user, {}).get('*', set())
                    sf_grants = sys_func_grants.get(full_user, set())
                    if 'PROCESS' in d_grants and func not in sf_grants:
                        changes.append(
                            [
                                'GRANT EXECUTE ON FUNCTION sys.{0} TO %(user)s@%(host)s'.format(func),
                                {'user': full_user[0], 'host': full_user[1]},
                            ]
                        )
                    elif 'PROCESS' not in d_grants and func in sf_grants:
                        changes.append(
                            [
                                'REVOKE EXECUTE ON FUNCTION sys.{0} FROM %(user)s@%(host)s'.format(func),
                                {'user': full_user[0], 'host': full_user[1]},
                            ]
                        )

        if not changes:
            ret['result'] = True
            ret['changes'] = {}
            ret['comment'] = ''
            return ret

        # apply the changes
        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {name: 'grants have to be updated'}
            ret['comment'] = '\n'.join(c[0] % c[1] for c in changes)
        else:
            errors = []
            for sql, args in changes:
                try:
                    log.debug('Doing query: {0} args: {1} '.format(sql, repr(args)))
                    cur.execute(sql, args)
                except MySQLdb.OperationalError as exc:
                    err = 'MySQL Error {0}: {1}'.format(*exc.args)
                    log.error(err)
                    errors.append(err)
            if errors:
                ret['result'] = False
                ret['changes'] = {name: "failed to update some grants"}
                ret['comment'] = '\n'.join(errors)
            else:
                ret['result'] = True
                ret['changes'] = {name: 'grants were updated'}
                ret['comment'] = '\n'.join(c[0] % c[1] for c in changes)

    return ret


# TODO: move to mdb_mysql_users after MDB-17686
def user_present(name, host, password, connection_default_file, connection_limits=None, auth_plugin=None):
    """
    Ensure user exists with proper password and options
    """
    full_name = '{}@{}'.format(name, host)

    ret = {
        'name': full_name,
        'result': None,
        'comment': 'user exists',
        'changes': {},
    }

    import MySQLdb

    password = _ensure_str(password)

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        user = _try_get_user(cur, name, host)

        resource_opts, should_flush_privileges = _get_resource_opts(user, connection_limits)

        sql = None
        args = {}
        action = ''

        cur.execute("select @@global.default_authentication_plugin as plugin;")
        default_auth_plugin = cur.fetchone()['plugin']

        auth_opts, auth_string, alter_required = _get_auth_options(user, password, auth_plugin, default_auth_plugin)

        if user is None:
            sql = 'CREATE USER %(user)s@%(host)s IDENTIFIED {auth_opts} AS %(auth_string)s {resource_opts}'
            args = {'user': name, 'host': host, 'auth_string': auth_string}
            action = 'created'
        elif not __salt__['mdb_mysql.check_user_password'](name, password) or alter_required:
            sql = 'ALTER USER %(user)s@%(host)s IDENTIFIED {auth_opts} AS %(auth_string)s {resource_opts}'
            args = {'user': name, 'host': host, 'auth_string': auth_string}
            action = 'updated'
        elif resource_opts:
            sql = 'ALTER USER %(user)s@%(host)s {resource_opts}'
            args = {'user': name, 'host': host}
            action = 'updated'
        else:
            ret['result'] = True
            return ret

        sql = sql.format(auth_opts=auth_opts, resource_opts=resource_opts)
        sql_comment = sql.replace('%(user)s', name).replace('%(host)s', host).replace('%(password)s', '********')

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {full_name: 'have to be ' + action}
            ret['comment'] = sql_comment
        else:
            try:
                cur.execute(sql, args)
                if should_flush_privileges:
                    sql_comment = sql_comment + '; FLUSH PRIVILEGES;'
                    cur.execute('FLUSH PRIVILEGES')

                ret['result'] = True
                ret['changes'] = {full_name: action}
                ret['comment'] = sql_comment
            except MySQLdb.OperationalError as exc:
                log.error('Failed to execute: ' + sql_comment)
                log.exception(exc)
                ret['result'] = False
                ret['comment'] = 'SQL: {}\nError: {} {}'.format(sql_comment, exc.args[0], exc.args[1])

    return ret


# TODO: move to mdb_mysql_users after MDB-17686
def idm_system_user_present(name):
    """
    Ensure idm system user exists
    """
    full_name = "{}@'%'".format(name)

    ret = {
        'name': full_name,
        'result': None,
        'comment': 'user exists',
        'changes': {},
    }

    import MySQLdb

    with closing(MySQLdb.connect(read_default_file=CONNECTION_DEFAULT_FILE, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        user = _try_get_user(cur, name, '%')

        if user:
            ret['result'] = True
            return ret

        sql = "CREATE USER '{user}'@'%' IDENTIFIED WITH mysql_no_login".format(user=name)

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {full_name: 'have to be created'}
            ret['comment'] = sql
            return ret

        try:
            cur.execute(sql)

            ret['result'] = True
            ret['changes'] = {full_name: 'created'}
            ret['comment'] = sql
        except MySQLdb.OperationalError as exc:
            log.error('Failed to execute: ' + sql)
            log.exception(exc)
            ret['result'] = False
            ret['comment'] = 'SQL: {}\nError: {} {}'.format(sql, exc.args[0], exc.args[1])

    return ret


# TODO: move to mdb_mysql_users after MDB-17686
def idm_user_present(name, password, connection_limits=None, auth_plugin=None):
    """
    Ensure idm user exists with proper password and options
    """
    host = '%'
    full_name = '{}@{}'.format(name, host)

    ret = {
        'name': full_name,
        'result': None,
        'comment': 'user exists',
        'changes': {},
    }

    import MySQLdb

    password = _ensure_str(password)

    with closing(MySQLdb.connect(read_default_file=CONNECTION_DEFAULT_FILE, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        user = _try_get_user(cur, name, host)

        resource_opts, should_flush_privileges = _get_resource_opts(user, connection_limits)

        sql = None
        args = {}
        action = ''

        auth_opts, auth_string, alter_required = _get_auth_options(user, password, auth_plugin, 'sha256_password')

        if user and user['account_locked'] == 'Y':
            ret['comment'] = 'account locked'
            ret['result'] = True
            return ret

        if user is None:
            sql = 'CREATE USER %(user)s@%(host)s IDENTIFIED {auth_opts} AS %(auth_string)s {resource_opts}'
            args = {'user': name, 'host': host, 'auth_string': auth_string}
            action = 'created'
        elif not __salt__['mdb_mysql.check_user_password'](name, password) or alter_required:
            sql = 'ALTER USER %(user)s@%(host)s IDENTIFIED {auth_opts} AS %(auth_string)s {resource_opts}'
            args = {'user': name, 'host': host, 'auth_string': auth_string}
            action = 'updated'
        elif resource_opts:
            sql = 'ALTER USER %(user)s@%(host)s {resource_opts}'
            args = {'user': name, 'host': host}
            action = 'updated'
        else:
            ret['result'] = True
            return ret

        sql = sql.format(auth_opts=auth_opts, resource_opts=resource_opts)
        sql_comment = sql.replace('%(user)s', name).replace('%(host)s', host).replace('%(password)s', '********')

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {full_name: 'have to be ' + action}
            ret['comment'] = sql_comment
        else:
            try:
                cur.execute(sql, args)
                if should_flush_privileges:
                    sql_comment = sql_comment + '; FLUSH PRIVILEGES;'
                    cur.execute('FLUSH PRIVILEGES')

                ret['result'] = True
                ret['changes'] = {full_name: action}
                ret['comment'] = sql_comment
            except MySQLdb.OperationalError as exc:
                log.error('Failed to execute: ' + sql_comment)
                log.exception(exc)
                ret['result'] = False
                ret['comment'] = 'SQL: {}\nError: {} {}'.format(sql_comment, exc.args[0], exc.args[1])

    return ret


# TODO: move to mdb_mysql_users after MDB-17686
def user_absent(name, host, connection_default_file):
    """
    Ensure user not exists. Terminates it's connections
    """
    ret = {
        'name': name,
        'result': True,
        'comment': [],
        'changes': {},
    }

    user = '{}@{}'.format(name, host)

    import MySQLdb

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)
        cur.execute('SELECT User, Host FROM mysql.user')
        users = ['{}@{}'.format(row['User'], row['Host']) for row in cur.fetchall()]

        if user not in users:
            return ret

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {user: 'have to be dropped'}
            return ret

        err = None
        try:
            # droping user is not blocked by active connections
            cur.execute('DROP USER {}@{}'.format(_quote_name(name), _quote_name(host)))
            # but we have to terminate them manually
            __salt__['mdb_mysql.kill_connections'](cur, user=name, host=host)
        except MySQLdb.OperationalError as exc:
            err = 'Failed to drop user {0}: due to {1}: "{2}"'.format(user, exc.args[0], exc.args[1])
            log.error(err)
        if err:
            ret['result'] = False
            ret['comment'] = err
        else:
            ret['result'] = True
            ret['changes'] = {user: 'dropped'}
        return ret


def _change_settings(cur, changes):
    import MySQLdb

    CODE_READ_ONLY_VARIABLE_ERROR = 1238
    mutual_exclusion = {
        'audit_log_exclude_accounts': {
            'reset': ['audit_log_include_accounts'],
            'value': None,  # pass None instead of 'NULL' (that will be interpreted as string)
        },
        'audit_log_include_accounts': {'reset': ['audit_log_exclude_accounts'], 'value': None},
        'audit_log_exclude_databases': {'reset': ['audit_log_include_databases'], 'value': None},
        'audit_log_include_databases': {'reset': ['audit_log_exclude_databases'], 'value': None},
    }
    errors = []
    for db_name in changes:
        db_value, mycnf_value = changes[db_name]
        try:
            # handle mutual exclusive settings:
            if db_name in mutual_exclusion:
                config = mutual_exclusion.get(db_name, {})
                reset = config.get('reset', [])
                value = config.get('value', None)
                for setting in reset:
                    sql = "SET GLOBAL {0} = %s".format(setting)
                    try:
                        log.info("Running: {0}".format(sql))
                        cur.execute(sql, [value])
                    except MySQLdb.OperationalError as exc:
                        log.warning(exc)

            sql = "SET GLOBAL {0} = %s".format(db_name)
            cur.execute(sql, [mycnf_value])
        except MySQLdb.OperationalError as exc:
            err = 'MySQL Error {0}: "{1}" - while setting {2} from "{3}" to "{4}"'.format(
                exc.args[0], exc.args[1], db_name, db_value, mycnf_value
            )
            if exc.args[0] == CODE_READ_ONLY_VARIABLE_ERROR:
                log.warning(err)
            else:
                log.error(err)
                errors.append(err)
    return errors


def _shadow_settings(variables, trigger, target):
    """
    This function copies config setting from 'trigger'-setting
    to all undefined target-settings
    """
    target_value = None
    for variable, value in variables:
        if variable == trigger:
            target_value = value
            break
    if target_value is None:
        return variables
    defined_variables = [v[0] for v in variables]
    for variable in target:
        if variable in defined_variables:
            continue  # there is value in config - don't replace it
        variables.append((variable, target_value))
    return variables


def ensure_settings(name, connection_default_file, mycnf_file, is_replica=False):
    import MySQLdb

    ret = {
        'name': name,
        'result': True,
        'comment': [],
        'changes': {},
    }
    changes = {}
    errors = []
    slave_affecting_settings = {
        'slave_parallel_workers',
        'slave_parallel_type',
        'slave_preserve_commit_order',
    }
    black_list = {
        'log_bin',
        'super_read_only',
        'read_only',
        'offline_mode',
        'rpl_semi_sync_master_enabled',
        'rpl_semi_sync_slave_enabled',
    }

    db_variables = __salt__['mdb_mysql.get_db_variables'](connection_default_file=connection_default_file)
    innodb_buffer_pool_instances = int(db_variables['innodb_buffer_pool_instances'])
    innodb_buffer_pool_chunk_size = int(db_variables['innodb_buffer_pool_chunk_size'])
    calculate_db_value = __salt__['mdb_mysql.get_calculator_db_values'](
        innodb_buffer_pool_instances=innodb_buffer_pool_instances,
        innodb_buffer_pool_chunk_size=innodb_buffer_pool_chunk_size,
    )
    mycnf_variables = __salt__['mdb_mysql.get_variables_from_file'](filename=mycnf_file)

    # some settings triggers changes in other settings:
    mycnf_variables = _shadow_settings(mycnf_variables, 'collation_server', ['collation_connection'])
    mycnf_variables = _shadow_settings(
        mycnf_variables,
        'character_set_server',
        ['character_set_client', 'character_set_connection', 'character_set_results'],
    )

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        for variable, mycnf_value in mycnf_variables:
            if variable in black_list:
                continue
            if variable in db_variables:
                db_value = db_variables.get(variable)
            else:
                log.debug("Can't find {0} in db".format(variable))
                continue

            new_value = calculate_db_value(variable, mycnf_value, db_value)
            if new_value is None:
                continue  # nothing changed - skip variable
            changes[variable] = (db_value, new_value)

        changes_str = '\n'.join("{0}: {1} -> {2}".format(key, changes[key][0], changes[key][1]) for key in changes)
        if __opts__['test']:
            if changes:
                ret['result'] = None
                ret['changes'] = {name: 'variables have to be updated'}
                ret['comment'] = changes_str
            else:
                ret['result'] = True
                ret['changes'] = {}
                ret['comment'] = ''
        else:
            # some settings require stopping/restarting sql slave thread on replica
            if is_replica:
                sa_changes = {k: changes.pop(k) for k in slave_affecting_settings if k in changes}
                if sa_changes:
                    try:
                        cur.execute('STOP SLAVE SQL_THREAD')
                        errors += _change_settings(cur, sa_changes)
                    except MySQLdb.OperationalError as exc:
                        err = 'MySQL Error {0}: "{1}" - while stoping slave sql_thread'.format(exc.args[0], exc.args[1])
                        log.error(err)
                        errors.append(err)
                    try:
                        cur.execute('START SLAVE SQL_THREAD')
                    except MySQLdb.OperationalError as exc:
                        err = 'MySQL Error {0}: "{1}" - while starting slave sql_thread'.format(
                            exc.args[0], exc.args[1]
                        )
                        log.error(err)
                        errors.append(err)
            # apply all others settings
            errors += _change_settings(cur, changes)

            if errors:
                ret['result'] = False
                ret['changes'] = {name: "failed to update some variables"}
                ret['comment'] = '\n'.join(errors)
            elif changes:
                ret['result'] = True
                ret['changes'] = {name: 'variables were updated'}
                ret['comment'] = changes_str
            else:
                ret['result'] = True
                ret['changes'] = {}
                ret['comment'] = ''
    return ret


def _quote_name(name):
    if not name:
        return name
    return '`' + name.replace('`', '``') + '`'


def database_present(name, connection_default_file, lower_case_table_names=0):
    """
    Ensure database exists
    """
    ret = {
        'name': name,
        'result': True,
        'comment': [],
        'changes': {},
    }

    import MySQLdb

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        if _database_exists(cur, name, lower_case_table_names):
            return ret

        name = _transform_db_name(name, lower_case_table_names)

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {name: 'have to be created'}
            return ret

        err = None
        try:
            cur.execute('CREATE DATABASE {}'.format(_quote_name(name)))
        except MySQLdb.OperationalError as exc:
            err = 'Failed to drop database {0}: due to {1}'.format(name, exc)
            log.error(err)
        if err:
            ret['result'] = False
            ret['comment'] = err
        else:
            ret['result'] = True
            ret['changes'] = {name: 'created'}
        return ret


def database_absent(name, connection_default_file, lower_case_table_names=0, timeout=120):
    """
    Ensure database not exists. Terminates connections to this database.
    Known bug: can't find cross-database table usages:
    If connection were made to db1, but holds tables in db.*, we can't find and drop it.
    """
    ret = {
        'name': name,
        'result': True,
        'comment': [],
        'changes': {},
    }

    import MySQLdb

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        if not _database_exists(cur, name, lower_case_table_names):
            return ret

        name = _transform_db_name(name, lower_case_table_names)

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {name: 'have to be dropped'}
            return ret

        err = None
        try:
            # running transactions may prevent database from dropping,
            # need to terminate them in advance
            __salt__['mdb_mysql.kill_connections'](cur, db=name, attempts=10)
            cur.execute('SET SESSION lock_wait_timeout = {}'.format(timeout))
            cur.execute('DROP DATABASE {}'.format(_quote_name(name)))
        except MySQLdb.OperationalError as exc:
            err = 'Failed to drop database {0}: due to {1}'.format(name, exc)
            log.error(err)
        if err:
            ret['result'] = False
            ret['comment'] = err
        else:
            ret['result'] = True
            ret['changes'] = {name: 'dropped'}
        return ret


def _database_exists(cur, name, lower_case_table_names):
    """
    Checks if database exists
    """
    cur.execute('SHOW DATABASES')

    db_names = [row['Database'] for row in cur.fetchall()]

    db_name = _transform_db_name(name, lower_case_table_names)
    db_names = _transform_db_names(db_names, lower_case_table_names)

    return db_name in db_names


def _transform_db_name(name, lower_case_table_names):
    return name.lower() if lower_case_table_names else name


def _transform_db_names(db_names, lower_case_table_names):
    if lower_case_table_names:
        return [db.lower() for db in db_names]

    return db_names


def _try_get_user(cur, name, host):
    cur.execute('SELECT * FROM mysql.user WHERE User = %s AND Host = %s', (name, host))
    return cur.fetchone()


def _get_auth_options(user, password, auth_plugin, default_auth_plugin):
    user_plugin = default_auth_plugin
    alter_required = False
    if user:
        # current user plugin
        user_plugin = user['plugin']
    if auth_plugin:
        # plugin from user request
        user_plugin = auth_plugin

        current_plugin = user['plugin'] if user else ''
        alter_required = current_plugin != auth_plugin

    auth_opts = 'WITH ' + user_plugin
    auth_string = __salt__['mysql_hashes.get_auth_string'](password, user_plugin)

    return auth_opts, auth_string, alter_required


def _get_resource_opts(user, connection_limits):
    resource_opts = []
    should_flush_privileges = False
    if connection_limits is not None:
        for limit in LIMITS:
            desired_value = connection_limits.get(limit, 0)
            current_value = user[LIMIT_2_QUERY[limit]] if user else 0
            if desired_value != current_value:
                resource_opts.append('{} {}'.format(limit, int(desired_value)))
                # MDB-14502: there is a bug in MySQL before 5.7.30 and 8.0.20 -> we should also execute
                #            `FLUSH PRIVILEGES` statement. It is safe to remove this workaround after
                #            minor versions bump
                should_flush_privileges = True

    if resource_opts:
        resource_opts = 'WITH ' + ' '.join(resource_opts)
    else:
        resource_opts = ''

    return resource_opts, should_flush_privileges


def _append_system_users(desired_grants, all_global_grants, version, target_user):
    filtered_system_users = []
    if not target_user:
        filtered_system_users = SYSTEM_USERS
    else:
        for u in SYSTEM_USERS:
            if target_user == u:
                filtered_system_users.append(u)

    hosts = __salt__['mdb_mysql.resolve_allowed_hosts']('__cluster__')
    for host in hosts:
        for user in filtered_system_users:
            full_user = (user, host)
            desired_grants.setdefault(full_user, {})
            desired_grants[full_user]['*'] = set()
            desired_grants[full_user]['performance_schema'] = set()
            desired_grants[full_user]['mysql'] = set()
            desired_grants[full_user]['sys'] = set()

            # default grants
            desired_grants[full_user]['*'].add('REPLICATION CLIENT')
            if version >= (8, 0):
                desired_grants[full_user]['*'].add('SERVICE_CONNECTION_ADMIN')

        if 'admin' in filtered_system_users:
            desired_grants[('admin', host)]['*'].update(all_global_grants)

        if 'monitor' in filtered_system_users:
            desired_grants[('monitor', host)]['*'].add('PROCESS')
            desired_grants[('monitor', host)]['performance_schema'].add('SELECT')
            desired_grants[('monitor', host)]['mysql'].add('SELECT')
            desired_grants[('monitor', host)]['sys'].add('SELECT')

        if 'repl' in filtered_system_users:
            desired_grants[('repl', host)]['*'].add('REPLICATION SLAVE')


def _ensure_str(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to `str`.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    NOTE: The function equals to six.ensure_str that is not present in the salt version of six module.
    """
    if type(s) is str:
        return s
    if six.PY2 and isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    elif six.PY3 and isinstance(s, six.binary_type):
        return s.decode(encoding, errors)
    elif not isinstance(s, (six.text_type, six.binary_type)):
        raise TypeError("not expecting type '%s'" % type(s))
    return s
