# -*- coding: utf-8 -*-

'''
MDB SQLServer states
'''

from __future__ import absolute_import, print_function, unicode_literals

import logging
import re
import subprocess
import time

try:
    import pyodbc

    HAS_ALL_IMPORTS = True
except ImportError:
    HAS_ALL_IMPORTS = False


log = logging.getLogger(__name__)


def escape_obj_name(var):
    var = var.replace("]", "]]")
    var = '[' + var + ']'
    return var


def escape_string(var):
    var = var.replace("'", "''")
    var = "'" + var + "'"
    return var


def _dict_compare(dicta, dictb):
    """
    Takes two dictionaries, compares them and returns a list of keys
     which have different values in dictb than in dicta.
    >>> a = {'a': 1, 'b': 2, 'c': 3}
    >>> b = {'a': 1, 'b': 2, 'c': 4}
    >>> _dict_compare(a,b)
    ['c']

    if no difference found then True is returned
    """
    if not isinstance(dicta, dict) or not isinstance(dictb, dict):
        raise TypeError("dictionaries expected")
    diff = []
    for k, v in dicta.items():
        if dictb[k] != v:
            diff += [k]
    if diff:
        return diff
    else:
        return []


def __virtual__():
    """
    Only load if the mdb_sqlserver module is present
    """
    return HAS_ALL_IMPORTS and 'mdb_sqlserver.get_connection' in __salt__


def query(name, sql, unless, **kwargs):
    """
    Run sql query, unless condition is true
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    cur.execute(unless)
    res = cur.fetchall()
    if len(res) != 1 or len(res[0]) != 1:
        ret['result'] = False
        ret['comment'] = '"unless" query should return one row with one column'
        return ret

    if res[0][0]:
        ret['result'] = True
        ret['comment'] = '"unless" query returned True'
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'query should be executed: {}'.format(sql)}
        return ret

    try:
        cur.execute(sql)
    except pyodbc.OperationalError as err:
        msg = 'failed to run query "{}": {}'.format(sql, err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'query executed: {}'.format(sql)}
    return ret


def hadr_endpoint_present(name, port=5022, cert_name='AG_CERT', encryption='AES', **kwargs):
    """
    Ensures existence of a database mirroring endpoint
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    sql = """SELECT
    ep.name,
    ep.state_desc,
    tep.port,
    encryption_algorithm_desc,
    cert.name as certificate_name


    FROM sys.database_mirroring_endpoints ep
    LEFT JOIN sys.certificates cert on ep.certificate_id = cert.certificate_id
    LEFT JOIN sys.server_principals p on p.sid = cert.sid
    JOIN sys.tcp_endpoints tep on tep.endpoint_id = ep.endpoint_id
    WHERE ep.name = ?
    """
    changes_str = ''

    ep_exists = False
    ep_started = False
    ep_port_ok = False
    ep_encryption_ok = False
    ep_cert_ok = False

    ep_recreate = False

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()
    ep_prop = cur.execute(sql, name).fetchall()

    if ep_prop and ep_prop[0][0] == name:
        changes_str += 'Endpoint with that name exists'
        ep_exists = True
        if ep_prop[0][1] != 'STARTED':
            changes_str += '; Endpoint is not STARTED.'
            ep_started = False
        else:
            ep_started = True
        if ep_prop[0][2] != port:
            ep_port_ok = False
            changes_str += '; Port number is wrong'
        else:
            ep_port_ok = True
        if ep_prop[0][3] != encryption:
            ep_encryption_ok = False
            changes_str += '; Encryption options mismatch'
        else:
            ep_encryption_ok = True
        if ep_prop[0][4] != cert_name:
            ep_cert_ok = False
            changes_str += '; Certificate mapping is missing'
        else:
            ep_cert_ok = True

    if not ep_exists or not ep_port_ok or not ep_encryption_ok or not ep_cert_ok:
        if ep_exists and (not ep_port_ok or not ep_encryption_ok or not ep_cert_ok):
            ep_recreate = True
        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {name: 'Endpoint needs to be created'}
            return ret
        else:
            ep_created = __salt__['mdb_sqlserver.endpoint_hadr_create'](
                name=name,
                ip='all',
                port=port,
                cert_name=cert_name,
                encryption=encryption,
                replace=ep_recreate,
                **kwargs
            )
            if ep_created:
                ret['changes'][name] = 'present'
                ret['result'] = True
                ret['comment'] = changes_str

            else:
                ret['result'] = False
                ret['comment'] = 'Endpoint creation failed'
            return ret

    if not ep_started and ep_port_ok and ep_encryption_ok and ep_cert_ok:
        changes_str += ';here we just enable endpoint'
        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {name: 'Endpoint needs to be enabled'}
            return ret
        else:
            ep_enabled = __salt__['mdb_sqlserver.endpoint_tcp_enable'](name, **kwargs)
            if ep_enabled:
                ret['changes'][name] = 'altered'
                ret['result'] = True
                ret['comment'] = changes_str
            else:
                ret['result'] = False
                ret['comment'] = 'Endpoint enabling failed'
            return ret
    return ret


def ag_present(name, host, basic=False, port=5022, try_count=5, retry_delay_s=10, **kwargs):
    """
    State ensures existence of AlwaysON AG
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    nodes = __salt__['mdb_windows.get_hosts_by_role']('sqlserver_cluster')

    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = 'SELECT COUNT(*) FROM sys.availability_groups WHERE name = ?'
    exists = cur.execute(sql, (name,)).fetchone()[0] > 0

    if exists:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should create availability group'}
        return ret

    node = __salt__['mdb_windows.shorten_hostname'](host.split('.')[0])
    if basic:
        sqls = [
            '''
                CREATE AVAILABILITY GROUP {ag_name} WITH (
                    AUTOMATED_BACKUP_PREFERENCE = PRIMARY,
                    BASIC,
                    FAILURE_CONDITION_LEVEL = 3,
                    HEALTH_CHECK_TIMEOUT = 600000,
                    DB_FAILOVER = OFF,
                    DTC_SUPPORT = NONE
                )
                FOR REPLICA ON '{node}' WITH (
                    ENDPOINT_URL = 'TCP://{host}:{port}',
                    AVAILABILITY_MODE = SYNCHRONOUS_COMMIT,
                    FAILOVER_MODE = AUTOMATIC,
                    SEEDING_MODE = MANUAL
                )
            '''.format(
                ag_name=escape_obj_name(name), node=node, host=host, port=port
            ),
            '''
                ALTER AVAILABILITY GROUP {ag_name}
                GRANT CREATE ANY DATABASE
            '''.format(
                ag_name=escape_obj_name(name)
            ),
        ]
    else:
        sqls = [
            '''
            CREATE AVAILABILITY GROUP {ag_name} WITH (
                AUTOMATED_BACKUP_PREFERENCE = PRIMARY,
                FAILURE_CONDITION_LEVEL = 3,
                HEALTH_CHECK_TIMEOUT = 600000,
                DB_FAILOVER = ON,
                DTC_SUPPORT = NONE
            )
            FOR REPLICA ON '{node}' WITH (
                ENDPOINT_URL = 'TCP://{host}:{port}',
                AVAILABILITY_MODE = SYNCHRONOUS_COMMIT,
                FAILOVER_MODE = MANUAL,
                SEEDING_MODE = MANUAL,
                SECONDARY_ROLE (
                    ALLOW_CONNECTIONS = NO
                ),
                PRIMARY_ROLE (
                    ALLOW_CONNECTIONS = ALL
                )
            )
        '''.format(
                ag_name=escape_obj_name(name), node=node, host=host, port=port
            ),
            '''
            ALTER AVAILABILITY GROUP {ag_name}
            GRANT CREATE ANY DATABASE
        '''.format(
                ag_name=escape_obj_name(name)
            ),
        ]
    err_pattern = re.compile(".*The operation encountered SQL Server error 41131 and has been rolled back.*")
    while try_count > 0:
        try:
            for sql in sqls:
                cur.execute(sql)

            ret['result'] = True
            ret['changes'] = {name: 'availability group created'}
            return ret
        except pyodbc.Error as err:
            log.exception(err)
            if err_pattern.match(err.args[1]):
                try_count = try_count - 1
                time.sleep(retry_delay_s)
                continue
            else:
                msg = 'failed to create availability group {}: {}'.format(name, err)
                log.error(msg)
                ret['result'] = False
                ret['comment'] = msg
                return ret


def ag_replica_present(
    name,
    ag_name,
    port=5022,
    availability_mode='KEEP',
    failover_mode='KEEP',
    seeding_mode='KEEP',
    secondary_allow_connections='KEEP',
    primary_allow_connections='KEEP',
    **kwargs
):
    """
    Adds replica into availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    nodes = __salt__['mdb_windows.get_hosts_by_role']('sqlserver_cluster')

    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    action = False
    if availability_mode == 'ASYNCHRONOUS_COMMIT' and failover_mode == 'AUTOMATIC':
        ret['result'] = False
        ret['comment'] = 'It is not possible to have automatic failover for asynchronous-commit replica!'
        return ret
    if availability_mode not in ('SYNCHRONOUS_COMMIT', 'ASYNCHRONOUS_COMMIT', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong availability_mode value'
        return ret
    if failover_mode not in ('MANUAL', 'AUTOMATIC', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong failover_mode value'
        return ret
    if seeding_mode not in ('MANUAL', 'AUTOMATIC', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong seeding_mode value'
        return ret
    if primary_allow_connections not in ('ALL', 'READ_WRITE', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong primary_allow_connections value'
        return ret
    if secondary_allow_connections not in ('ALL', 'READ_ONLY', 'NO', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong secondary_allow_connections value'
        return ret

    params = {
        'availability_mode': availability_mode,
        'failover_mode': failover_mode,
        'seeding_mode': seeding_mode,
        'primary_allow_connections': primary_allow_connections,
        'secondary_allow_connections': secondary_allow_connections,
    }

    param_defaults = {
        'availability_mode': 'SYNCHRONOUS_COMMIT',
        'failover_mode': 'AUTOMATIC',
        'seeding_mode': 'MANUAL',
        'primary_allow_connections': 'ALL',
        'secondary_allow_connections': 'NO',
    }

    # we shorten the name to 15 symbols if it is longer.
    node = __salt__['mdb_windows.shorten_hostname'](name.split('.')[0])
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = """SELECT
                availability_mode_desc as availability_mode,
                failover_mode_desc as failover_mode,
                primary_role_allow_connections_desc as primary_allow_connections,
                secondary_role_allow_connections_desc as secondary_allow_connections,
                seeding_mode_desc as seeding_mode
            FROM sys.availability_replicas ar
            JOIN sys.availability_groups ag
                ON ag.group_id = ar.group_id
            WHERE replica_server_name = ?
                and ag.name = ?
            """
    result = cur.execute(sql, (node, ag_name)).fetchall()
    if result:  # replica exists
        options = result[0]
        options = [v for v in options]
        columns = [column[0] for column in cur.description]
        options = dict(zip(columns, options))
        options_compare = _dict_compare(params, options)
        options_compare2 = []
        for opt in options_compare:
            if params[opt] != 'KEEP':
                options_compare2.append(opt)
        options_compare = options_compare2
        if options_compare == []:
            ret['result'] = True
            return ret
        else:
            action = 'MODIFY'
    else:
        action = 'ADD'

    if __opts__['test']:
        ret['result'] = None
        if action:
            ret['changes'] = {name: 'Action: {}'.format(action)}
        return ret

    base = '''
        ALTER AVAILABILITY GROUP {ag_name}
        {action} REPLICA ON '{node}'
        WITH '''.format(
        ag_name=escape_obj_name(ag_name), action=action, node=node
    )
    if action == 'ADD':
        for p in params.keys():
            if params[p] == 'KEEP':
                params[p] = param_defaults[p]
        sql = '''{base} (
            ENDPOINT_URL = 'TCP://{host}:{port}',
            AVAILABILITY_MODE = {availability_mode},
            FAILOVER_MODE = {failover_mode},
            SEEDING_MODE = {seeding_mode},
            SECONDARY_ROLE (
                ALLOW_CONNECTIONS = {secondary_allow_connections}
            ),
            PRIMARY_ROLE (
                ALLOW_CONNECTIONS = {primary_allow_connections}
            )
        )
        '''.format(
            base=base,
            host=name,
            port=port,
            availability_mode=params['availability_mode'],
            failover_mode=params['failover_mode'],
            seeding_mode=params['seeding_mode'],
            secondary_allow_connections=params['secondary_allow_connections'],
            primary_allow_connections=params['primary_allow_connections'],
        )
    if action == 'MODIFY':
        for p, v in params.items():
            if v == 'KEEP':
                params[p] = options[p]
        if options['availability_mode'] == 'ASYNCHRONOUS_COMMIT' and failover_mode == 'AUTOMATIC':
            sql = '''{base} (AVAILABILITY_MODE = {availability_mode})
                    {base} (FAILOVER_MODE = {failover_mode})
                    {base} (SEEDING_MODE = {seeding_mode})
                    {base} (SECONDARY_ROLE (
                    ALLOW_CONNECTIONS = {secondary_allow_connections}
                    ))
                    {base} (PRIMARY_ROLE (
                    ALLOW_CONNECTIONS = {primary_allow_connections}
                    ))
                '''.format(
                base=base,
                availability_mode=params['availability_mode'],
                failover_mode=params['failover_mode'],
                seeding_mode=params['seeding_mode'],
                secondary_allow_connections=params['secondary_allow_connections'],
                primary_allow_connections=params['primary_allow_connections'],
            )
        else:
            sql = '''
                    {base} (FAILOVER_MODE = {failover_mode})
                    {base} (AVAILABILITY_MODE = {availability_mode})
                    {base} (SEEDING_MODE = {seeding_mode})
                    {base} (SECONDARY_ROLE (
                    ALLOW_CONNECTIONS = {secondary_allow_connections}
                    ))
                    {base} (PRIMARY_ROLE (
                    ALLOW_CONNECTIONS = {primary_allow_connections}
                    ))
                '''.format(
                base=base,
                availability_mode=params['availability_mode'],
                failover_mode=params['failover_mode'],
                seeding_mode=params['seeding_mode'],
                secondary_allow_connections=params['secondary_allow_connections'],
                primary_allow_connections=params['primary_allow_connections'],
            )
        action = 'MODIFY: {opts}'.format(opts=','.join(options_compare))
    try:
        cur.execute(sql)
    except pyodbc.ProgrammingError as err:
        msg = 'failed to {} replica {} with query: {}'.format(action, name, sql)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'ACTION: {}'.format(action)}
    return ret


def join_ag(name, ag_name='AG1', timeout=600, sleep=10, **kwargs):
    """
    Joins current replica to availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    # we shorten the name to 15 symbols if it is longer.
    node = __salt__['mdb_windows.shorten_hostname'](name.split('.')[0])
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = """
            SELECT
                COUNT(*)
            FROM sys.availability_replicas ar
            JOIN sys.availability_groups ag
                    ON ag.group_id = ar.group_id
            WHERE replica_server_name = ?
                    and ag.name = ?
          """
    exists = cur.execute(sql, (node, ag_name)).fetchone()[0] > 0

    if exists:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should join availability group'}
        return ret

    sqls = [
        'ALTER AVAILABILITY GROUP {ag_name} JOIN'.format(ag_name=escape_obj_name(ag_name)),
        'ALTER AVAILABILITY GROUP {ag_name} GRANT CREATE ANY DATABASE'.format(ag_name=escape_obj_name(ag_name)),
    ]

    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            for sql in sqls:
                cur.execute(sql)
        except pyodbc.ProgrammingError as err:
            log.debug('failed to join ag: %s', err)
            time.sleep(sleep)
        else:
            ret['result'] = True
            ret['changes'] = {name: 'joined availability group {}'.format(ag_name)}
            return ret

    ret['result'] = False
    ret['comment'] = '{} failed to join availability group {} within {}s'.format(name, ag_name, timeout)
    return ret


def db_present(name, **kwargs):
    """
    State ensures database exists
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = 'SELECT COUNT(*) FROM sys.databases WHERE name = ?'
    exists = cur.execute(sql, (name,)).fetchone()[0] > 0

    if exists:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should be created'}
        return ret

    sql = """CREATE DATABASE {name} CONTAINMENT = NONE;
            ALTER AUTHORIZATION ON DATABASE::{name} TO sa;
            """.format(
        name=escape_obj_name(name)
    )
    try:
        cur.execute(sql)
    except pyodbc.OperationalError as err:
        msg = 'failed to create database {}: {}'.format(name, err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'database created'}
    return ret


def db_absent(name, **kwargs):
    """
    State ensures database does not exist
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = 'SELECT state_desc FROM sys.databases WHERE name = ?'
    cur.execute(sql, (name,))
    rows = list(cur.fetchall())

    if len(rows) == 0:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should be dropped'}
        return ret

    db_state = rows[0][0]
    sqls = []
    if db_state == 'ONLINE':
        sqls += ['ALTER DATABASE {name} SET SINGLE_USER WITH ROLLBACK IMMEDIATE'.format(name=escape_obj_name(name))]
    sqls += ['DROP DATABASE {name}'.format(name=escape_obj_name(name))]
    try:
        for sql in sqls:
            cur.execute(sql)
    except pyodbc.ProgrammingError as err:
        msg = 'failed to drop database {}: {}'.format(name, err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'database dropped'}
    return ret


def __is_db_in_ag(cursor, name, is_primary, ag_name='AG1'):
    sql = '''
        SELECT COUNT(*)
        FROM sys.databases d
        JOIN sys.dm_hadr_database_replica_states h ON d.database_id = h.database_id
        JOIN sys.availability_groups ag ON ag.group_id = h.group_id
        WHERE is_primary_replica = ? AND d.name = ? AND ag.name = ?
    '''
    is_primary = 1 if is_primary else 0
    return cursor.execute(sql, (is_primary, name, ag_name)).fetchone()[0] > 0


def db_in_ag(name, ag_name='AG1', **kwargs):
    """
    Ensures database in availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    nodes = __salt__['mdb_windows.get_hosts_by_role']('sqlserver_cluster')

    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    if __is_db_in_ag(cur, name, True, ag_name):
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should be added to availability group'}
        return ret

    sqls = [
        'BACKUP DATABASE {name} TO DISK = \'NUL\''.format(name=escape_obj_name(name)),
        'ALTER AVAILABILITY GROUP {ag_name} ADD DATABASE {name}'.format(
            ag_name=escape_obj_name(ag_name), name=escape_obj_name(name)
        ),
    ]
    try:
        for sql in sqls:
            cur.execute(sql)
            while cur.nextset():
                pass
    except pyodbc.OperationalError as err:
        msg = 'failed to add database {} to availability group: {}'.format(name, err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'added to availability group'}
    return ret


def db_not_in_ag(name, ag_name='AG1', **kwargs):
    """
    Ensures database not in availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    if not __is_db_in_ag(cur, name, True, ag_name):
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should be removed from availability group'}
        return ret

    sql = 'ALTER AVAILABILITY GROUP {ag_name} REMOVE DATABASE {name}'.format(
        ag_name=escape_obj_name(ag_name), name=escape_obj_name(name)
    )
    try:
        cur.execute(sql)
    except pyodbc.OperationalError as err:
        msg = 'failed to remove database {} to availability group: {}'.format(name, err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'removed from availability group'}
    return ret


def secondary_db_in_ag(name, ag_name='AG1', **kwargs):
    """
    Ensures secondary database in availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    nodes = __salt__['mdb_windows.get_hosts_by_role']('sqlserver_cluster')

    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    if __is_db_in_ag(cur, name, False, ag_name):
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should be added to availability group'}
        return ret

    sql = '''
        ALTER DATABASE {name} SET HADR AVAILABILITY GROUP = {ag_name}
    '''.format(
        ag_name=escape_obj_name(ag_name), name=escape_obj_name(name)
    )
    try:
        cur.execute(sql)
    except pyodbc.OperationalError as err:
        msg = 'failed to add database {} to availability group: {}'.format(name, err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'added to availability group'}
    return ret


def secondary_db_not_in_ag(name, ag_name='AG1', **kwargs):
    """
    Ensures secondary database not in availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    if not __is_db_in_ag(cur, name, False, ag_name):
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should be removed from availability group'}
        return ret

    sql = 'ALTER DATABASE {name} SET HADR OFF'.format(name=escape_obj_name(name))
    try:
        cur.execute(sql)
    except pyodbc.OperationalError as err:
        msg = 'failed to remove database {} from availability group: {}'.format(name, err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'removed from availability group'}
    return ret


def wait_started(name, timeout=60, interval=5, **kwargs):
    """
    Waits until SQLServer is ready to process SQL
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            __salt__['mdb_sqlserver.run_query']('SELECT 1', **kwargs)
        except pyodbc.Error as err:
            log.debug('SQLServer not ready yet: %s', err)
            time.sleep(interval)
        else:
            ret['result'] = True
            return ret

    ret['result'] = False
    ret['comment'] = 'SQLServer is not ready within {} seconds'.format(timeout)
    return ret


WALG_BIN = r'C:\Program Files\wal-g-sqlserver\wal-g-sqlserver.exe'


def db_backuped(name, walg_config, databases, backup_id='NEW', with_logs=True, **kwargs):
    """
    Unconditionally backups databases
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    if backup_id not in ('LATEST', 'NEW'):
        raise Exception("backup id must be LATEST or NEW")

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {db: 'should be backuped' for db in databases}
        return ret

    # actually backup databases
    backup_cmd = [WALG_BIN, '--config', walg_config, 'backup-push']
    if backup_id == 'LATEST':
        backup_cmd += ['--update-latest']
    backup_cmd += ['--databases', ','.join(databases)]
    log_backup_cmd = [WALG_BIN, '--config', walg_config, 'log-push']
    log_backup_cmd += ['--databases', ','.join(databases)]
    try:
        log.debug('backuping databases with command: ' + ' '.join(backup_cmd))
        proc = subprocess.Popen(backup_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        out, err = proc.communicate()
        if proc.returncode != 0:
            ret['result'] = False
            ret['comment'] = 'failed to perform full backup for databases {}: {}'.format(databases, err)
            return ret
        if with_logs:
            log.debug('backuping database logs with command: ' + ' '.join(log_backup_cmd))
            proc = subprocess.Popen(
                log_backup_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True
            )
            out, err = proc.communicate()
            if proc.returncode != 0:
                ret['result'] = False
                ret['comment'] = 'failed to perform a log backup for databases {}: {}'.format(databases, err)
                return ret
    except subprocess.CalledProcessError as err:
        msg = 'databases {} failed to backup: {}'.format(databases, err)
        log.error(msg)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {db: 'backuped' for db in databases}
    return ret

def db_log_backuped(name, walg_config, databases, **kwargs):
    """
    Unconditionally backups database logs
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {db: 'logs should be backuped' for db in databases}
        return ret

    # actually backup databases
    log_backup_cmd = [WALG_BIN, '--config', walg_config, 'log-push']
    log_backup_cmd += ['--databases', ','.join(databases)]
    try:
        log.debug('backuping database logs with command: ' + ' '.join(log_backup_cmd))
        proc = subprocess.Popen(
            log_backup_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True
        )
        out, err = proc.communicate()
        if proc.returncode != 0:
            ret['result'] = False
            ret['comment'] = 'failed to perform a log backup for databases {}: {}'.format(databases, err)
            return ret
    except subprocess.CalledProcessError as err:
        msg = 'databases {} logs failed to backup: {}'.format(databases, err)
        log.error(msg)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {db: 'logs backuped' for db in databases}
    return ret


def db_restored(
    name,
    walg_config,
    databases,
    from_databases=None,
    backup_id='LATEST',
    until_ts="9999-12-31T23:59:59Z",
    norecovery=False,
    timeout=600,
    sleep=10,
    roll_forward=False,
    **kwargs
):
    """
    Ensures that all required databases are restored from backup.
    If databases are not in backup or backup doesn't exists waits for it to appear withtimeout.
    Restore happens without recovery.
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    if from_databases is not None and len(from_databases) != len(databases):
        ret['result'] = False
        ret['comment'] = 'from_databases list length does not math databases list length'
        return ret
    source_map = dict(zip(databases, from_databases or databases))

    walg_cmd = [WALG_BIN, '--config', walg_config]
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = 'SELECT name, state FROM sys.databases'
    query_result = cur.execute(sql).fetchall()
    existing = set([r[0] for r in query_result])
    required = set(databases or [])
    missing = required - existing
    restoring = []
    for r in query_result:
        if r[1] == 1 and (r[0] in databases or databases == []):
            restoring.append(r[0])

    log.debug('existing: %s', existing)
    log.debug('required: %s', required)
    log.debug('missing: %s', missing)
    log.debug('source_map: %s', source_map)
    log.debug('restoring: %s', restoring)

    if not missing and not restoring:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {db: 'should be restore' for db in missing}

    if timeout > 0:
        deadline = time.time() + timeout
        # wait for backup to appear
        backup_found = False
        while time.time() < deadline:
            out = subprocess.check_output(walg_cmd + ['backup-list'])
            if not out or len(out.split('\n')) < 2:
                log.debug('backup is not found yet')
                time.sleep(sleep)
            else:
                backup_found = True
                break
        if not backup_found:
            msg = 'backup was not found within {}s'.format(timeout)
            log.error(msg)
            ret['result'] = False
            ret['comment'] = msg
            return ret
        # wait for databases to appear in backup (for "add database" screnario)
        missing_in_backup = set()
        while time.time() < deadline:
            out = subprocess.check_output(walg_cmd + ['database-list', backup_id])
            in_backup = set([db.strip().decode('utf-8') for db in out.split('\n') if db.strip()])
            missing_in_backup = set(source_map[i] for i in missing) - in_backup
            if missing_in_backup:
                log.debug('databases in %s backup %s', backup_id, in_backup)
                log.debug('databases %s are not in backup yet', missing_in_backup)
                time.sleep(sleep)
            else:
                break
        if missing_in_backup:
            msg = 'databases {} was not found in backup within {}s'.format(missing_in_backup, timeout)
            log.error(msg)
            ret['result'] = False
            ret['comment'] = msg
            return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {db: 'should be restored from backup' for db in missing}
        if restoring and roll_forward:
            for db in restoring:
                ret['changes'][db] = 'should be rolled forward'
        return ret

    # actually restore from backup
    # first try to refresh the ones that are in restoring state
    if restoring and roll_forward:
        refresh_cmd = walg_cmd + [
            'log-restore',
            '--no-recovery',
            '--since',
            backup_id,
            '--until',
            until_ts,
            '--databases',
            ','.join(restoring),
        ]
        if from_databases is not None:
            refresh_cmd += ['--from', ','.join(source_map[i] for i in restoring)]
        out = None
        try:
            log.debug('restoring database logs with command: ' + ' '.join(refresh_cmd))
            out = subprocess.check_output(refresh_cmd, stderr=subprocess.STDOUT)
            ret['result'] = True
            for db in restoring:
                ret['changes'][db] = 'rolled forward'
        except subprocess.CalledProcessError as err:
            msg = 'databases {} failed to roll forward from backup: {}'.format(restoring, err)
            if out is not None:
                log.error(out)
            log.error(msg)
            ret['result'] = False
            ret['comment'] = msg
            return ret

    if not missing:
        ret['result'] = True
        return ret

    missing = list(missing)  # to fix order
    restore_cmd = walg_cmd + ['backup-restore', '--no-recovery']
    if from_databases is not None:
        restore_cmd += ['--from', ','.join(source_map[i] for i in missing)]
    restore_cmd += ['--databases', ','.join(missing), backup_id]

    log_restore_cmd = walg_cmd + ['log-restore']
    if norecovery:
        log_restore_cmd += ['--no-recovery']
    if from_databases is not None:
        log_restore_cmd += ['--from', ','.join(source_map[i] for i in missing)]
    log_restore_cmd += ['--databases', ','.join(missing), '--since', backup_id, '--until', until_ts]
    out = None
    try:
        log.debug('restoring databases with command: ' + ' '.join(restore_cmd))
        out = subprocess.check_output(restore_cmd, stderr=subprocess.STDOUT)
        log.debug('restoring database logs with command: ' + ' '.join(log_restore_cmd))
        out = subprocess.check_output(log_restore_cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as err:
        if backup_id == 'LATEST':
            # avoid races with master when adding new database
            for db in missing:
                try:
                    cur.execute('DROP DATABASE ' + escape_obj_name(db))
                except Exception:
                    pass
        msg = 'databases {} failed to restore from backup: {}'.format(missing, err)
        if out is not None:
            log.error(out)
        log.error(msg)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    for db in missing:
        ret['changes'][db] = 'restored from backup'
    return ret


def db_recovered(name, databases, **kwargs):
    """
    Attempts to recovery all all databases in RESTORING state
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    cur.execute('SELECT name FROM sys.databases WHERE state_desc = \'RESTORING\'')
    to_recover = set(databases) & set([r[0] for r in cur.fetchall()])

    if not to_recover:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'databases {} should be recovered'.format(','.join(to_recover))}
        return ret

    for db in to_recover:
        try:
            cur.execute('RESTORE DATABASE {} WITH RECOVERY'.format(escape_obj_name(db)))
        except pyodbc.OperationalError as err:
            log.exception(err)
            ret['result'] = False
            ret['comment'] = 'Failed to recover database {}: {}'.format(db, err)
            return ret

    ret['result'] = True
    ret['changes'] = {name: 'databases {} were recovered'.format(','.join(to_recover))}
    return ret


def wait_replica_connected(name, ag_name='AG1', timeout=60, interval=5, **kwargs):
    """
    Waits until SQLServer replica gets connected to primary
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    deadline = time.time() + timeout
    sql = """
        IF (SELECT count(*) FROM sys.dm_hadr_availability_replica_states ar
                JOIN sys.availability_groups ag ON ag.group_id = ar.group_id
                WHERE is_local = 1 and connected_state_desc = 'CONNECTED' and ag.name = ?) = 0
            THROW 51000,'Replica is not connected to master yet',1
        ELSE
            SELECT 1
    """
    while time.time() < deadline:
        try:
            cur.execute(sql, ag_name)
        except pyodbc.Error as err:
            log.debug('%s', err)
            time.sleep(interval)
        else:
            ret['result'] = True
            return ret

    ret['result'] = False
    ret['comment'] = "Replica hasn't been connected to master within {} seconds".format(timeout)
    return ret


def hostname_matches(name, restart=True, **kwargs):
    """
    Fixes a mismatch of hostname and what t-sql @@SERVERNAME returns
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    hostname = __salt__['mdb_windows.get_fqdn']().split('.')[0]
    servername = __salt__['mdb_sqlserver.run_query'](
        'SELECT name FROM sys.servers WHERE name LIKE @@SERVERNAME', **kwargs
    )
    if servername:
        servername = servername[0][0]
    if servername != hostname:
        ret['changes'][name] = 'Servername'
        ret['comment'] = 'Servername needs to be changed'
        if __opts__['test']:
            ret['result'] = None
            return ret
        sql1 = """exec sp_dropserver '{0}';
        SELECT count(*) FROM sys.servers WHERE name LIKE @@SERVERNAME
            """.format(
            servername
        )
        sql2 = """
        IF NOT EXISTS (SELECT 1 FROM sys.servers WHERE name like '{0}')
            BEGIN
            exec sp_addserver '{0}', local
            END
        SELECT count(*) FROM sys.servers WHERE name LIKE '{0}'
        """.format(
            hostname
        )
        if servername:
            # there might be an edge case when @@SERVERNAME shows name
            # after it has already been dropped from sys.servers
            try:
                __salt__['mdb_sqlserver.run_query'](sql1, **kwargs)
            except pyodbc.OperationalError as err:
                msg = 'failed to run query "{}": {}'.format(sql1, err)
                log.error(msg)
                log.exception(err)
                ret['result'] = False
                ret['comment'] = msg
                return ret
        try:
            __salt__['mdb_sqlserver.run_query'](sql2, **kwargs)
        except pyodbc.OperationalError as err:
            msg = 'failed to run query "{}": {}'.format(sql2, err)
            log.error(msg)
            log.exception(err)
            ret['result'] = False
            ret['comment'] = msg
            return ret
        if restart:
            try:
                __salt__['mdb_windows.restart_service'](name='MSSQLSERVER', force=True)
            except Exception as err:
                msg = 'failed to restart MSSQLSERVER service: {}'.format(err)
                log.error(msg)
                log.exception(err)
                ret['result'] = False
                ret['comment'] = msg
                return ret
        ret['changes'][name] = 'Servername'
        ret['result'] = True
        ret['comment'] = 'Servername mismatch fixed'
        return ret
    else:
        ret['result'] = True
        ret['comment'] = 'Names match'
        return ret


def login_present(name, sid, password, server_roles=None, **kwargs):
    """
    State ensures login exists with proper password and sid
    """
    if server_roles is None:
        server_roles = []
    login_present = False
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    cur.execute('SELECT PWDCOMPARE(?, password_hash) FROM sys.sql_logins WHERE name = ?', (password, name))
    row = cur.fetchone()

    if row is None:
        sql = 'CREATE LOGIN {} WITH PASSWORD = {}, SID = {}, CHECK_POLICY = OFF'.format(
            escape_obj_name(name), escape_string(password), sid
        )
        action = 'create'
    elif not row[0]:
        sql = 'ALTER LOGIN {} WITH PASSWORD = {}'.format(escape_obj_name(name), escape_string(password))
        action = 'update'
    else:
        login_present = True

    if not login_present:
        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {name: 'login {} should be {}ed'.format(name, action)}
        else:
            try:
                cur.execute(sql)
                ret['result'] = True
                ret['changes'] = {name: 'login {} {}ed'.format(name, action)}
                login_present = True
            except pyodbc.ProgrammingError as err:
                msg = 'Failed to {} login {}: {}'.format(action, name, err)
                log.error(msg)
                log.exception(err)
                ret['result'] = False
                ret['comment'] = msg
                return ret
    if name == 'sa':
        ret['result'] = True
        return ret

    roles_present = __salt__['mdb_sqlserver.login_role_list'](name, **kwargs) or []

    roles_add = list(set(server_roles) - set(roles_present))
    roles_drop = list(set(roles_present) - set(server_roles))

    if roles_add or roles_drop:
        if not ret['changes'].get(name):
            ret['changes'][name] = 'Present'
        if roles_add:
            ret['changes'][name] += '; Roles added: ' + '; '.join(roles_add)
        if roles_drop:
            ret['changes'][name] += '; Roles dropped: ' + '; '.join(roles_drop)

        if __opts__['test']:
            ret['result'] = None
            ret['comment'] += '; Role membership for the user {0} are to be modified'.format(name)
            return ret
        roles_changed = __salt__['mdb_sqlserver.login_role_mod'](name, roles_add, roles_drop, **kwargs)
        if not roles_changed:
            ret['result'] = False
            ret['comment'] = 'Roles modification failed. Refer to minion log for details.'
            return ret

    ret['result'] = True
    return ret


def login_absent(name, **kwargs):
    """
    State ensures login absent
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()
    cur.execute('SELECT name FROM sys.syslogins WHERE name = ?', (name,))

    if len(cur.fetchall()) == 0:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'login {} should be dropped'.format(name)}
        return ret
    sql =  '''
        DECLARE @sql NVARCHAR(MAX) = ''
        SELECT @sql += N'KILL ' + CONVERT(VARCHAR(11), session_id) + N';'
        FROM sys.dm_exec_sessions
        WHERE
            login_name = {login_string_name}
            AND session_id <> @@SPID

        EXEC sys.sp_executesql @sql;
        DROP LOGIN {login_name}
    '''.format(login_name = escape_obj_name(name), login_string_name = escape_string(name))
    try:

        cur.execute(sql)
        ret['result'] = True
        ret['changes'] = {name: 'login {} dropped'.format(name)}
    except pyodbc.ProgrammingError as err:
        msg = 'Failed to drop login {}: {}; query was {}'.format(name, err, sql)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
    return ret


def wait_all_replica_dbs_joined(name, ag_name='AG1', timeout=300, sleep=5, **kwargs):
    """
    Waits specific number of seconds for all replica databases to get connected
    """
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}

    replicas = __salt__['pillar.get']('data:dbaas:shard_hosts')

    if len(replicas) == 1:
        ret['result'] = True
        ret['comment'] = 'Standalone instance, no wait required'
        return ret

    replicas2 = []
    for rep in replicas:
        if not __salt__['mdb_windows.is_witness'](rep):
            replicas2 += [rep]

    replicas = replicas2

    replicas = [rep.split('.')[0] for rep in replicas]

    sql = """
            SELECT DISTINCT r.replica_server_name
              FROM sys.dm_hadr_database_replica_cluster_states rs
              JOIN sys.availability_replicas r ON rs.replica_id = r.replica_id
              JOIN sys.availability_groups ag ON ag.group_id = r.group_id
             WHERE is_database_joined = 1 AND ag.name = ?
            """
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            reps = set([r[0] for r in cur.execute(sql, ag_name).fetchall()])
            if set(replicas) == set(reps):
                ret['result'] = True
                return ret
            else:
                time.sleep(sleep)
        except pyodbc.ProgrammingError as err:
            log.debug('Query execution failed: %s', err)
    ret['result'] = False
    ret['comment'] = '{} not all replica databases are joined after {}s'.format(name, timeout)
    return ret


def wait_db_listed_in_ag(name, timeout=300, sleep=5, **kwargs):
    """
    State is used on replica to wait until a database
    is added to an availability group definition on primary replica.
    This allows to avoid race condition during cluster restore operation
    when replica may try to add a restored copy of database to an availability group
    before the primary replica has completed it's restoration and added it to and AG definition
    """
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}

    deadline = time.time() + timeout
    sql = """
            SELECT COUNT(*) from sys.availability_databases_cluster
                 WHERE database_name = {0}""".format(
        escape_string(name)
    )
    while time.time() < deadline:
        try:
            db_ok = __salt__['mdb_sqlserver.run_query'](sql, **kwargs)
            if db_ok[0][0] == 1:
                ret['result'] = True
                return ret
            else:
                time.sleep(sleep)
        except pyodbc.Error as err:
            log.debug('Failure during check if a database is added to AG on master: %s', err)
    ret['result'] = False
    ret['comment'] = "Database has not been added to AG on master within {} seconds".format(timeout)
    return ret


def sqlcollation_matches(name, sa_password, sqlserver_version, **kwargs):
    """
    state ensures that SQL Server Collation matched the one that user chosen
    After change we'll have a completely new master database
    which means that all the logins and configuration needs to be applied again
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    sql_collation_query = "SELECT CAST(SERVERPROPERTY('COLLATION') as NVARCHAR(50))"
    current_collation = __salt__['mdb_sqlserver.run_query'](sql_collation_query, **kwargs)
    current_collation = current_collation[0][0]

    vers = {
        "2016sp2ent": '130',
        "2016sp2std": '130',
        "2017ent": '140',
        "2017std": '140',
        "2019ent": '150',
        "2019std": '150',
    }

    versname = {
        '2016sp2ent': 'SQLServer2016',
        '2016sp2std': 'SQLServer2016',
        '2017ent': 'SQL2017',
        '2017std': 'SQL2017',
        '2019ent': 'SQL2019',
        '2019std': 'SQL2019',
    }

    sql_databases_query = """SELECT count(name)
                              FROM sys.databases
                              WHERE database_id > 4
                              """
    sql_ag_query = """SELECT count(name)
                       FROM sys.availability_groups
                       """

    setup_exe = r"C:\Program Files\Microsoft SQL Server\{vers}\Setup Bootstrap\{versname}\setup.exe".format(
        vers=vers[sqlserver_version], versname=versname[sqlserver_version]
    )

    if current_collation == name:
        ret['result'] = True
        ret['Comment'] = 'Server collation is correct'
        return ret

    databases_present = __salt__['mdb_sqlserver.run_query'](sql_databases_query, **kwargs)
    databases_present = databases_present[0][0] != 0
    ag_found = __salt__['mdb_sqlserver.run_query'](sql_ag_query, **kwargs)
    ag_found = ag_found[0][0] != 0

    if ag_found or databases_present:
        ret[
            'comment'
        ] = 'SQL Collation needs to be changed. {} -> {}, but we cannot do this as server hosts databases or AG. '.format(
            current_collation, name
        )
        ret['result'] = False
        return ret

    ret['changes'][name] = 'SQL Collation needs to be changed. {} -> {}'.format(current_collation, name)
    if __opts__['test']:
        ret['result'] = None
        return ret
    try:
        out = subprocess.check_output(
            [
                setup_exe,
                '/Q',
                '/ACTION=REBUILDDATABASE',
                '/INSTANCENAME=MSSQLSERVER',
                "/SQLSYSADMINACCOUNTS=Administrators",
                '/SAPWD={0}'.format(sa_password),
                '/SQLCOLLATION={0}'.format(name),
            ],
            stderr=subprocess.STDOUT,
            universal_newlines=True,
        )
        ret['comment'] = out
    except subprocess.CalledProcessError as err:
        msg = 'Failed to change server collation: {0}; output: {1}'.format(err, err.output)
        log.error(msg)
        ret['changes'] = {}
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'][name] = 'set'
    return ret


def query_file_run(name, unless, **kwargs):
    """
    Run sql query file, unless condition is true
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    cur.execute(unless)
    res = cur.fetchall()
    if len(res) != 1 or len(res[0]) != 1:
        ret['result'] = False
        ret['comment'] = '"unless" query should return one row with one column'
        return ret

    if res[0][0]:
        ret['result'] = True
        ret['comment'] = '"unless" query returned True'
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'query file that should be executed: {}'.format(name)}
        return ret

    try:
        ok, out, err = __salt__['mdb_sqlserver.run_sql_file'](filename=name, **kwargs)
        if err:
            ret['result'] = False
            ret['comment'] = 'failed to run query file "{}": {}'.format(name, err)
            return ret
    except subprocess.CalledProcessError as e:
        msg = 'failed to run query "{}": {}'.format(name, e)
        log.error(msg)
        log.exception(e)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'query file executed: {}'.format(name)}
    return ret


def server_role_present(name, permissions=None, **kwargs):
    """
    State ensures that the given server-level role exists with defined set of privileges.
    """
    if permissions is None:
        permissions = []

    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    role_present = False
    role_present = name in [v[0] for v in __salt__['mdb_sqlserver.server_role_list'](**kwargs)]

    if not role_present:
        if __opts__['test']:
            ret['changes'][name] = 'Role {0} needs to be created.'.format(name)
            ret['result'] = None
        else:
            role_present = __salt__['mdb_sqlserver.server_role_create'](name, **kwargs)
            if not role_present:
                ret['result'] = False
                ret['comment'] = "Server role {0} couldn't be created.".format(name)
                return ret
            ret['comment'] = 'Server role {0} created'
            ret['changes'][name] = 'Created'

    permissions_present = __salt__['mdb_sqlserver.server_role_permissions_list'](name, **kwargs)
    permissions_add = list(set(permissions) - set(permissions_present))
    permissions_drop = list(set(permissions_present) - set(permissions))

    if permissions_add or permissions_drop:
        if not ret['changes'].get(name) == 'Created':
            ret['changes'][name] = 'Present'
        if permissions_add:
            ret['changes'][name] += '; Permissions added: ' + ';'.join(permissions_add)
        if permissions_drop:
            ret['changes'][name] += '; Permissions dropped: ' + ';'.join(permissions_drop)

        if __opts__['test']:
            ret['result'] = None
            ret['comment'] += '; permissions for the role {0} are to be modified'.format(name)
            return ret

        permissions_changed = __salt__['mdb_sqlserver.server_role_permissions_mod'](
            name, permissions_add, permissions_drop, **kwargs
        )
        if not permissions_changed:
            ret['result'] = False
            ret['comment'] = 'Permissions modification failed. Refer to minion log for details.'
            return ret

    ret['result'] = True
    return ret


def backup_exported(name, walg_config, walg_ext_config, databases, backup_id='LATEST', **kwargs):
    """
    Exports backup to the external client's storage.
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    export_cmd = [WALG_BIN, '--config', walg_config, '--external-config', walg_ext_config, 'backup-export']
    for dbname, prefix in databases.items():
        export_cmd += ['-d', dbname + '=' + prefix]
    export_cmd += [backup_id]
    try:
        log.debug('exporting backup with command: ' + ' '.join(export_cmd))
        proc = subprocess.Popen(export_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        out, err = proc.communicate()
        if proc.returncode != 0:
            ret['result'] = False
            ret['comment'] = 'failed to export backup {} for databases {}: {}'.format(backup_id, databases.keys(), err)
            return ret
    except subprocess.CalledProcessError as err:
        msg = 'failed to export backup {} for databases {}: {}'.format(backup_id, databases.keys(), err)
        log.error(msg)
        ret['result'] = False
        ret['comment'] = msg
        return ret
    ret['result'] = True
    ret['changes'] = {db: 'exported' for db in databases}
    return ret


def backup_imported(name, walg_config, walg_ext_config, databases, **kwargs):
    """
    Imports backup from the external client's storage
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    import_cmd = [WALG_BIN, '--config', walg_config, '--external-config', walg_ext_config, 'backup-import']
    for dbname, filelist in databases.items():
        import_cmd += ['-d', dbname + '=' + ','.join(filelist)]
    try:
        log.debug('importing backup with command: ' + ' '.join(import_cmd))
        proc = subprocess.Popen(import_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        out, err = proc.communicate()
        if proc.returncode != 0:
            ret['result'] = False
            ret['comment'] = 'failed to import backup for databases {}: {}'.format(databases.keys(), err)
            return ret
    except subprocess.CalledProcessError as err:
        msg = 'failed to import backup for databases {}: {}'.format(databases.keys(), err)
        log.error(msg)
        ret['result'] = False
        ret['comment'] = msg
        return ret
    ret['result'] = True
    ret['changes'] = {db: 'imported' for db in databases}
    return ret


def ag_absent(name, **kwargs):
    """
    Ensures that named availability group is not present
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = 'SELECT COUNT(name) FROM sys.availability_groups WHERE name = ?'
    exists = cur.execute(sql, (name,)).fetchone()[0] > 0

    if not exists:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'Availability Group {} should be removed'.format(name)}
        return ret
    try:
        cur.execute('DROP AVAILABILITY GROUP {}'.format(escape_obj_name(name)))
    except pyodbc.OperationalError as err:
        log.exception(err)
        ret['result'] = False
        ret['comment'] = 'Failed to drop Availability Group {}: {}'.format(name, err)
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'Availability Group {} has been remode'.format(name)}
    return ret


def dbs_in_ags(name, databases, **kwargs):
    """
    Ensures databases in availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    nodes = __salt__['mdb_windows.get_hosts_by_role']('sqlserver_cluster')

    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()
    for db, ag_name in databases.items():
        if __is_db_in_ag(cur, db, True, ag_name):
            continue

        if __opts__['test']:
            ret['result'] = None
            ret['changes'][db] = 'should be added to availability group'
        else:
            sqls = [
                'BACKUP DATABASE {db} TO DISK = \'NUL\''.format(db=escape_obj_name(db)),
                'ALTER AVAILABILITY GROUP {ag_name} ADD DATABASE {db}'.format(
                    ag_name=escape_obj_name(ag_name), db=escape_obj_name(db)
                ),
            ]
            try:
                for sql in sqls:
                    cur.execute(sql)
                    while cur.nextset():
                        pass
                    ret['changes'][db] = 'added to availability group'
            except pyodbc.OperationalError as err:
                msg = 'failed to add database {} to availability group: {}'.format(db, err)
                log.error(msg)
                log.exception(err)
                ret['result'] = False
                ret['comment'] = msg
                return ret

    ret['result'] = True
    return ret


def dbs_present(name, databases, **kwargs):
    """
    State ensures databases exist
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()
    if not isinstance(databases, list):
        databases = [databases]
    sql_get_dbnames = 'SELECT name FROM sys.databases WHERE database_id > 4'
    dbs_got_dbnames = set([r[0] for r in cur.execute(sql_get_dbnames).fetchall()])

    dbs_absent = set(databases) - dbs_got_dbnames

    if not dbs_absent:
        ret['result'] = True
        return ret
    for db in dbs_absent:
        ret['changes'][db] = "should be created"
    if __opts__['test']:
        ret['result'] = None
        return ret

    for db in dbs_absent:
        sql = """CREATE DATABASE {name} CONTAINMENT = NONE;
                ALTER AUTHORIZATION ON DATABASE::{name} TO sa;
                """.format(
            name=escape_obj_name(db)
        )
        try:
            cur.execute(sql)
        except pyodbc.OperationalError as err:
            msg = 'failed to create database {}: {}'.format(db, err)
            log.error(msg)
            log.exception(err)
            ret['result'] = False
            ret['comment'] = msg
            return ret
        ret['changes'][db] = "database created"

    ret['result'] = True
    return ret


def wait_synchronized(name, timeout_ms=20000, interval_ms=10000, **kwargs):
    """
    Waits until SQLServer cluster is synchronized
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    deadline = time.time() + timeout_ms / 1000
    while time.time() < deadline:
        try:
            rows = __salt__['mdb_sqlserver.run_query'](
                'SELECT database_id, synchronization_state_desc FROM sys.dm_hadr_database_replica_states', **kwargs
            )
            for row in rows:
                database_id = row[0]
                synchronization_state_desc = row[1]
                if synchronization_state_desc != 'SYNCHRONIZED':
                    log.debug(
                        'SQLServer not synchronized yet: database %s is in state %s',
                        database_id,
                        synchronization_state_desc,
                    )
                    time.sleep(interval_ms / 1000)
                    continue
            ret['result'] = True
            return ret
        except pyodbc.Error as err:
            log.debug('SQLServer not ready yet: %s', err)
            time.sleep(interval_ms / 1000)

    ret['result'] = False
    ret['comment'] = 'SQLServer is not SYNCHRONIZED within {} seconds'.format(timeout_ms / 1000)
    return ret


def ensure_master(name, ag=['AG1'], **kwargs):
    """
    State ensures this host is master (will do swtichover if needed)
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = (
        'SELECT ags.name, rs.role_desc '
        'FROM sys.dm_hadr_availability_replica_states rs '
        'JOIN sys.availability_groups ags '
        'ON rs.group_id = ags.group_id '
        'WHERE is_local = 1'
    )
    cur.execute(sql)
    rows = list(cur.fetchall())

    if len(rows) == 0:
        ret['result'] = False
        ret['comment'] = "No role_desc fetched for this host"
        return ret

    ag_replicas = []
    for row in rows:
        if row[1] != 'PRIMARY':
            ag_replicas.append(row[0])

    if len(ag_replicas) == 0:
        ret['result'] = True
        ret['comment'] = "Already PRIMARY"
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should run switchover for ' + ', '.join(ag_replicas)}
        return ret

    sqls = []
    for ag in ag_replicas:
        sqls += ['ALTER AVAILABILITY GROUP {} FAILOVER'.format(escape_obj_name(ag))]
    try:
        for sql in sqls:
            cur.execute(sql)
    except pyodbc.ProgrammingError as err:
        msg = 'failed to perform switchover: {}'.format(err)
        log.error(msg)
        log.exception(err)
        ret['result'] = False
        ret['comment'] = msg
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'switchover is done'}
    return ret


def all_replicas_present(
    name,
    ags=[],
    port=5022,
    nodes=[],
    availability_mode='KEEP',
    failover_mode='KEEP',
    seeding_mode='KEEP',
    secondary_allow_connections='KEEP',
    primary_allow_connections='KEEP',
    **kwargs
):
    """
    Ensures that all the replicas for all the databses for all the AGs are present.
    ags paramete is expected to be a dictionary {db_name: ag_name}
    for Enterprise edition it is expected to be one ag for all the databases
    and for Standard edition it is required to be one database per one ag.
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    all_nodes = __salt__['mdb_windows.get_hosts_by_role']('sqlserver_cluster')

    if len(all_nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    if availability_mode == 'ASYNCHRONOUS_COMMIT' and failover_mode == 'AUTOMATIC':
        ret['result'] = False
        ret['comment'] = 'It is not possible to have automatic failover for asynchronous-commit replica!'
        return ret
    if availability_mode not in ('SYNCHRONOUS_COMMIT', 'ASYNCHRONOUS_COMMIT', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong availability_mode value'
        return ret
    if failover_mode not in ('MANUAL', 'AUTOMATIC', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong failover_mode value'
        return ret
    if seeding_mode not in ('MANUAL', 'AUTOMATIC', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong seeding_mode value'
        return ret
    if primary_allow_connections not in ('ALL', 'READ_WRITE', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong primary_allow_connections value'
        return ret
    if secondary_allow_connections not in ('ALL', 'READ_ONLY', 'NO', 'KEEP'):
        ret['result'] = False
        ret['comment'] = 'Wrong secondary_allow_connections value'
        return ret

    params = {
        'availability_mode': availability_mode,
        'failover_mode': failover_mode,
        'seeding_mode': seeding_mode,
        'primary_allow_connections': primary_allow_connections,
        'secondary_allow_connections': secondary_allow_connections,
    }

    param_defaults = {
        'availability_mode': 'SYNCHRONOUS_COMMIT',
        'failover_mode': 'AUTOMATIC',
        'seeding_mode': 'MANUAL',
        'primary_allow_connections': 'ALL',
        'secondary_allow_connections': 'NO',
    }

    nodes = [n.split('.')[0] for n in nodes]
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    ags_params = {}
    for ag in ags:
        ags_params[ag] = params.copy()

    desired_state = {}
    for node in nodes:
        desired_state[node] = ags_params
    try:
        actual_state = __salt__['mdb_sqlserver.get_ag_replicas_properties'](**kwargs)
    except Exception as e:
        msg = "error during loading replicas configuration: {}".format(e)
        ret['result'] = False
        return ret

    replicas_absent = {}
    replicas_to_modify = {}

    for h in nodes:
        for ag in desired_state[h]:
            if not actual_state.get(h, ''):
                replicas_absent[(h, ag)] = desired_state[h][ag]
            else:
                if not actual_state[h].get(ag, ''):
                    replicas_absent[(h, ag)] = desired_state[h][ag]
                else:
                    temp_compare = _dict_compare(desired_state[h][ag], actual_state[h][ag])
                    options_compare = []
                    for opt in temp_compare:
                        if desired_state[h][ag][opt] != 'KEEP':
                            options_compare.append(opt)
                    if options_compare:
                        replicas_to_modify[(h, ag)] = desired_state[h][ag]

    if replicas_absent == {} and replicas_to_modify == {}:
        ret['result'] = True
        return ret

    if replicas_absent:
        ret['changes']['To be added'] = replicas_absent
    if replicas_to_modify:
        ret['changes']['To be altered'] = replicas_to_modify

    if __opts__['test']:
        ret['result'] = None
        return ret

    sqls = []

    for (host, ag), opts in replicas_absent.items():
        for p, o in opts.items():
            if o == 'KEEP':
                opts[p] = param_defaults[p]
        sql = '''
        ALTER AVAILABILITY GROUP {ag}
        ADD REPLICA ON '{host}'
        WITH  (
            ENDPOINT_URL = 'TCP://{host}:{port}',
            AVAILABILITY_MODE = {availability_mode},
            FAILOVER_MODE = {failover_mode},
            SEEDING_MODE = {seeding_mode},
            SECONDARY_ROLE (
                ALLOW_CONNECTIONS = {secondary_allow_connections}
            ),
            PRIMARY_ROLE (
                ALLOW_CONNECTIONS = {primary_allow_connections}
            )
        )
        '''.format(
            host=host,
            ag=escape_obj_name(ag),
            port=port,
            availability_mode=opts['availability_mode'],
            failover_mode=opts['failover_mode'],
            seeding_mode=opts['seeding_mode'],
            secondary_allow_connections=opts['secondary_allow_connections'],
            primary_allow_connections=opts['primary_allow_connections'],
        )
        sqls += [sql]
    for k, opts in replicas_to_modify.items():
        host = k[0]
        ag = k[1]
        for p, v in opts.items():
            if v == 'KEEP':
                opts[p] = actual_state[host][ag][p]
        if actual_state[host][ag]['availability_mode'] == 'ASYNCHRONOUS_COMMIT' and failover_mode == 'AUTOMATIC':
            sql = '''ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (AVAILABILITY_MODE = {availability_mode})
                        ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (FAILOVER_MODE = {failover_mode})
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (SEEDING_MODE = {seeding_mode})
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (SECONDARY_ROLE (
                            ALLOW_CONNECTIONS = {secondary_allow_connections}
                        ))
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (PRIMARY_ROLE (
                            LLOW_CONNECTIONS = {primary_allow_connections}
                        ))
                '''.format(
                ag=escape_obj_name(ag),
                host=host,
                availability_mode=opts['availability_mode'],
                failover_mode=opts['failover_mode'],
                seeding_mode=opts['seeding_mode'],
                secondary_allow_connections=opts['secondary_allow_connections'],
                primary_allow_connections=opts['primary_allow_connections'],
            )
        else:
            sql = '''
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (FAILOVER_MODE = {failover_mode})
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (AVAILABILITY_MODE = {availability_mode})
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (SEEDING_MODE = {seeding_mode})
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                        (SECONDARY_ROLE (
                            ALLOW_CONNECTIONS = {secondary_allow_connections}
                        ))
                    ALTER AVAILABILITY GROUP {ag}
                    MODIFY REPLICA ON '{host}' WITH
                            (PRIMARY_ROLE (
                            ALLOW_CONNECTIONS = {primary_allow_connections}
                        ))
                '''.format(
                ag=escape_obj_name(ag),
                host=host,
                availability_mode=opts['availability_mode'],
                failover_mode=opts['failover_mode'],
                seeding_mode=opts['seeding_mode'],
                secondary_allow_connections=opts['secondary_allow_connections'],
                primary_allow_connections=opts['primary_allow_connections'],
            )
        sqls += [sql]
    for sql in sqls:
        try:
            cur.execute(sql)
        except pyodbc.Error as err:
            msg = 'all_replicas_present: failed to run query: {}; Error: {}'.format(sql, err)
            log.error(msg)
            log.exception(err)
            ret['result'] = False
            ret['comment'] = msg
            return ret

    ret['result'] = True
    if replicas_absent:
        ret['changes']['replicas added'] = replicas_absent.keys()
    if replicas_to_modify:
        ret['changes']['replicas altered'] = replicas_to_modify.keys()
    return ret


def all_ags_present(name, host, ags=[], basic=False, port=5022, try_count=5, retry_delay_s=10, **kwargs):
    """
    State ensures existence of AlwaysON AG
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    nodes = __salt__['mdb_windows.get_hosts_by_role']('sqlserver_cluster')

    if len(nodes) == 1:
        ret['result'] = True
        ret['comment'] = 'standalone instance'
        return ret

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = 'SELECT name FROM sys.availability_groups'
    ags_present = [row[0] for row in cur.execute(sql).fetchall()]
    ags_missing = list(set(ags) - set(ags_present))

    if not ags_missing:
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {ag: 'should create availability group' for ag in ags_missing}
        return ret

    node = __salt__['mdb_windows.shorten_hostname'](host.split('.')[0])
    commands = {}
    for ag in ags_missing:
        if basic:
            sqls = [
                '''
                    CREATE AVAILABILITY GROUP {ag_name} WITH (
                        AUTOMATED_BACKUP_PREFERENCE = PRIMARY,
                        BASIC,
                        FAILURE_CONDITION_LEVEL = 3,
                        HEALTH_CHECK_TIMEOUT = 600000,
                        DB_FAILOVER = OFF,
                        DTC_SUPPORT = NONE
                    )
                    FOR REPLICA ON '{node}' WITH (
                        ENDPOINT_URL = 'TCP://{host}:{port}',
                        AVAILABILITY_MODE = SYNCHRONOUS_COMMIT,
                        FAILOVER_MODE = AUTOMATIC,
                        SEEDING_MODE = MANUAL
                    )
                '''.format(
                    ag_name=escape_obj_name(ag), node=node, host=host, port=port
                ),
                '''
                    ALTER AVAILABILITY GROUP {ag_name}
                    GRANT CREATE ANY DATABASE
                '''.format(
                    ag_name=escape_obj_name(ag)
                ),
            ]
        else:
            sqls = [
                '''
                CREATE AVAILABILITY GROUP {ag_name} WITH (
                    AUTOMATED_BACKUP_PREFERENCE = PRIMARY,
                    FAILURE_CONDITION_LEVEL = 3,
                    HEALTH_CHECK_TIMEOUT = 600000,
                    DB_FAILOVER = ON,
                    DTC_SUPPORT = NONE
                )
                FOR REPLICA ON '{node}' WITH (
                    ENDPOINT_URL = 'TCP://{host}:{port}',
                    AVAILABILITY_MODE = SYNCHRONOUS_COMMIT,
                    FAILOVER_MODE = MANUAL,
                    SEEDING_MODE = MANUAL,
                    SECONDARY_ROLE (
                        ALLOW_CONNECTIONS = NO
                    ),
                    PRIMARY_ROLE (
                        ALLOW_CONNECTIONS = ALL
                    )
                )
            '''.format(
                    ag_name=escape_obj_name(ag), node=node, host=host, port=port
                ),
                '''
                ALTER AVAILABILITY GROUP {ag_name}
                GRANT CREATE ANY DATABASE
            '''.format(
                    ag_name=escape_obj_name(ag)
                ),
            ]
        commands[ag] = sqls
    err_pattern = re.compile(".*The operation encountered SQL Server error 41131 and has been rolled back.*")
    for ag, sqls in commands.items():
        ag_created = False
        tries_left = try_count
        while tries_left > 0 and not ag_created:
            try:
                for sql in sqls:
                    cur.execute(sql)
                ag_created = True
                ret['changes'][ag] = 'availability group created'
            except pyodbc.Error as err:
                log.exception(err)
                if err_pattern.match(err.args[1]):
                    tries_left = tries_left - 1
                    time.sleep(retry_delay_s)
                    continue
                else:
                    msg = 'failed to create availability group {}: {}'.format(ag, err)
                    log.error(msg)
                    ret['result'] = False
                    ret['comment'] = msg
                    return ret

    ret['result'] = True
    return ret


def wait_all_replica_dbs_joined_ags(name, nodes=[], ags=['AG1'], timeout=300, sleep=5, **kwargs):
    """
    Waits specific number of seconds for all replica databases in specific groups to get connected
    """
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}

    replicas = [n.split('.')[0] for n in nodes]

    if len(replicas) == 1:
        ret['result'] = True
        ret['comment'] = 'Standalone instance, no wait required'
        return ret

    sql = """
            SELECT DISTINCT r.replica_server_name
              FROM sys.dm_hadr_database_replica_cluster_states rs
              JOIN sys.availability_replicas r ON rs.replica_id = r.replica_id
              JOIN sys.availability_groups ag ON ag.group_id = r.group_id
             WHERE is_database_joined = 1 AND ag.name = ?
            """
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    for ag in ags:
        connected = False
        deadline = time.time() + timeout
        while time.time() < deadline and not connected:
            connected = False
            try:
                reps = set([r[0] for r in cur.execute(sql, ag).fetchall()])
                if set(replicas) == set(reps):
                    connected = True
                else:
                    time.sleep(sleep)
            except pyodbc.ProgrammingError as err:
                log.debug('Query execution failed: %s', err)
        if not connected:
            ret['result'] = False
            ret['comment'] = '{} not all replica databases are joined after {}s'.format(name, timeout)
            return ret
    ret['result'] = True
    ret['comment'] = "All replica databases connected"
    return ret


def wait_secondary_database_joined(name, nodes, databases, timeout=300, sleep=5, **kwargs):
    """
    Waits specific number of seconds for all secondary databases to get connected
    """
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}

    replicas = [ n.split('.')[0] for n in nodes ]

    if len(replicas) == 1:
        ret['result'] = True
        ret['comment'] = 'Standalone instance, no wait required'
        return ret

    sql = """
        SELECT DISTINCT database_name, r.replica_server_name
          FROM sys.dm_hadr_database_replica_cluster_states rs
          JOIN sys.availability_replicas r ON rs.replica_id = r.replica_id
         WHERE is_database_joined = 1 AND database_name IN ({})
    """.format(', '.join(escape_string(d) for d in databases))

    required = set((d, r) for d in databases for r in replicas)

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    deadline = time.time() + timeout
    connected = set()
    while time.time() < deadline:
        try:
            rows = cur.execute(sql).fetchall()
            connected = set((r[0], r[1]) for r in rows)
            if required.issubset(connected):
                break
            time.sleep(sleep)
        except pyodbc.ProgrammingError as err:
            log.debug('Query execution failed: %s', err)
    if not required.issubset(connected):
        ret['result'] = False
        ret['comment'] = 'secondary databases {} not joined within {}s'.format(required-connected, timeout)
        return ret
    ret['result'] = True
    ret['comment'] = "All replica databases connected"
    return ret


def join_ags(name, ags=['AG1'], timeout=600, sleep=10, **kwargs):
    """
    Joins current replica to availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    # we shorten the name to 15 symbols if it is longer.
    node = __salt__['mdb_windows.shorten_hostname'](name.split('.')[0])
    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = """
            SELECT
                ag.name
            FROM sys.availability_replicas ar
            JOIN sys.availability_groups ag
                    ON ag.group_id = ar.group_id
			WHERE replica_server_name = @@servername
          """
    ags_joined = [row[0] for row in cur.execute(sql).fetchall()]
    ags_to_join = set(ags) - set(ags_joined)

    if not ags_to_join:
        ret['result'] = True
        return ret

    for ag in ags_to_join:
        ret['changes'][ag] = "AG should be joined"

    if __opts__['test']:
        ret['result'] = None
        return ret

    for ag in ags_to_join:
        joined = False
        deadline = time.time() + timeout
        while time.time() < deadline and not joined:
            joined = False
            sqls = [
                'ALTER AVAILABILITY GROUP {ag_name} JOIN'.format(ag_name=escape_obj_name(ag)),
                'ALTER AVAILABILITY GROUP {ag_name} GRANT CREATE ANY DATABASE'.format(ag_name=escape_obj_name(ag)),
            ]
            try:
                for sql in sqls:
                    cur.execute(sql)
                ret['changes'][ag] = "joined"
                joined = True
            except pyodbc.ProgrammingError as err:
                log.error('failed to join ag: %s', err)
                time.sleep(sleep)
        if not joined:
            ret['result'] = False
            ret['comment'] = '{} not AGs joined after {}s'.format(name, timeout)
            return ret
    ret['result'] = True
    ret['comment'] = "All AGs joined"
    return ret


def all_secondary_dbs_in_ags(name, dbs=[], edition='standard', **kwargs):
    """
    Ensures secondary database in availability group
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    sql = """SELECT d.name
        FROM sys.databases d
        JOIN sys.dm_hadr_database_replica_states h ON d.database_id = h.database_id
        JOIN sys.availability_groups ag ON ag.group_id = h.group_id
        WHERE is_primary_replica = 0"""
    dbs_included = [row[0] for row in cur.execute(sql).fetchall()]
    dbs_missing = list(set(dbs) - set(dbs_included))

    if not dbs_missing:
        ret['result'] = True
        ret['comment'] = 'All dbs are joined to ags'
        return ret

    for db in dbs_missing:
        ret['changes'][db] = 'should be added to availability group'

    if __opts__['test']:
        ret['result'] = None
        return ret

    for db in dbs_missing:
        if edition == 'standard':
            ag_name = db
        else:
            ag_name = 'AG1'
        sql = '''
            ALTER DATABASE {db} SET HADR AVAILABILITY GROUP = {ag_name}
        '''.format(
            ag_name=escape_obj_name(ag_name), db=escape_obj_name(db)
        )
        try:
            cur.execute(sql)
            ret['changes'][db] = 'added to availability group'
        except pyodbc.OperationalError as err:
            msg = 'failed to add database {} to availability group: {}'.format(name, err)
            log.error(msg)
            log.exception(err)
            ret['result'] = False
            ret['changes'][db] = 'failed'
            ret['comment'] = msg
            return ret

    ret['result'] = True
    return ret


def wait_all_replicas_connected(name, dbs, edition='standard', timeout=60, interval=5, **kwargs):
    """
    Waits until SQLServer replica gets connected to primary
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()
    if edition == 'standard':
        ags = dbs
    else:
        ags = ['AG1']
    deadline = time.time() + timeout
    sql_get_replicas = """
            SELECT ag.name
                FROM sys.dm_hadr_availability_replica_states ar
                    JOIN sys.availability_groups ag
                        ON ag.group_id = ar.group_id
                WHERE is_local = 1
                    AND connected_state_desc = 'CONNECTED'
                    """
    while time.time() < deadline:
        try:
            ags_connected = [row[0] for row in cur.execute(sql_get_replicas).fetchall()]
            ags_missing = set(ags) - set(ags_connected)
            if not ags_missing:
                ret['result'] = True
                return ret
            time.sleep(interval)
        except pyodbc.Error as err:
            msg = 'failed to check if replica is connected to all the AGs: {}'.format(err)
            log.error(msg)
            log.exception(err)
            ret['result'] = False
            ret['comment'] = msg
            return ret

    ret['result'] = False
    ret['comment'] = "Replica hasn't been connected to master within {} seconds".format(timeout)
    return ret


def wait_all_dbs_listed_in_ag(name, dbs, timeout=300, sleep=5, **kwargs):
    """
    State is used on replica to wait until a database
    is added to an availability group definition on primary replica.
    This allows to avoid race condition during cluster restore operation
    when replica may try to add a restored copy of database to an availability group
    before the primary replica has completed it's restoration and added it to and AG definition
    """
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}

    conn = __salt__['mdb_sqlserver.get_connection'](**kwargs)
    cur = conn.cursor()

    deadline = time.time() + timeout
    sql = "SELECT database_name from sys.availability_databases_cluster"
    while time.time() < deadline:
        try:
            dbs_present = [row[0] for row in cur.execute(sql).fetchall()]
            dbs_missing = list(set(dbs) - set(dbs_present))
            if not dbs_missing:
                ret['result'] = True
                return ret
            else:
                time.sleep(sleep)
        except pyodbc.Error as err:
            log.debug('Failure during check if a database is added to AG on master: %s', err)
    ret['result'] = False
    ret['comment'] = "Some databases have not been added to AG on a primary replica within {} seconds".format(timeout)
    return ret
