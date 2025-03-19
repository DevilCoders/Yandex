# -*- coding: utf-8 -*-
"""
MySQL utility functions for MDB
"""

from __future__ import absolute_import, print_function, unicode_literals

import copy
import logging
import socket
import time

from contextlib import closing

try:
    from configparser import ConfigParser, RawConfigParser
except ImportError:
    from ConfigParser import ConfigParser, RawConfigParser

try:
    import six
except ImportError:
    from salt.ext import six


log = logging.getLogger(__name__)

__salt__ = {}
__pillar__ = None


def __virtual__():
    """
    We always return True here (we are always available)
    """
    return True


def resolve_allowed_hosts(hosts='%'):
    if hosts == '__cluster__':
        ret = copy.copy(__salt__['pillar.get']('data:dbaas:cluster_hosts', []))
        ret += ['localhost']
        return ret
    if hosts == 'localhost':
        return ['localhost', __salt__['grains.get']('fqdn')]
    if isinstance(hosts, str):
        return [hosts]
    return hosts


def is_replica(connection_default_file):
    """
    Returns true if current node is replica
    """
    try:
        import MySQLdb
    except ImportError:
        log.warn('Failed to check if node is replica: MySQLdb is not installed yet')
        return None
    if '__mysql_is_replica' not in __pillar__:
        try:
            with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
                cur = conn.cursor()
                cur.execute("SHOW SLAVE STATUS")
                __pillar__['__mysql_is_replica'] = len(cur.fetchall()) > 0
        except MySQLdb.OperationalError as exc:
            log.warn('Failed to check if node is replica: %s', exc)
            __pillar__['__mysql_is_replica'] = None
    return __pillar__['__mysql_is_replica']


def find_master(hosts, user, password, port=3306, timeout=60):
    """
    Find master within hosts
    (if hosts is not list we assume it is pillar path)
    """
    import MySQLdb

    if isinstance(hosts, list):
        host_list = hosts
    else:
        host_list = __salt__['pillar.get'](hosts)

    if not host_list:
        return

    hosts = host_list[:]
    my_hostname = __salt__['grains.get']('id')
    if my_hostname in hosts:
        hosts.remove(my_hostname)

    log.info("Trying to find master in %s", hosts)
    deadline = time.time() + timeout
    while time.time() < deadline:
        for host in hosts:
            conn = None
            try:
                conn = MySQLdb.connect(
                    host=host,
                    port=port,
                    user=user,
                    passwd=password,
                    db='mysql',
                    ssl={'ca': '/etc/mysql/ssl/allCAs.pem'},
                )
                cur = conn.cursor()
                cur.execute('SHOW SLAVE STATUS')
                res = cur.fetchall()
                if len(res) == 0:
                    cur.execute('SELECT @@super_read_only')
                    if int(cur.fetchone()[0]) == 0:
                        return host
            except Exception as e:
                log.warn('failed to check slave status on host %s: %s', host, e)
            finally:
                if conn:
                    conn.close()
        time.sleep(1)
    log.error('Failed to find mysql master within %s', timeout)
    return None


def get_db_variables(connection_default_file):
    import MySQLdb

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        try:
            cur = conn.cursor(MySQLdb.cursors.DictCursor)
            cur.execute("SHOW GLOBAL VARIABLES;")
            variables = cur.fetchall()
        except MySQLdb.OperationalError as exc:
            err = 'MySQL Error {0}: {1}'.format(*exc.args)
            log.error(err)
            return {}
    return {row["Variable_name"]: row["Value"] for row in variables}


def get_variables_from_file(filename, depth=0):
    section = 'mysqld'
    config = RawConfigParser(allow_no_value=True) if six.PY2 else RawConfigParser(allow_no_value=True, strict=False)
    config.read(filename)
    variables = list(config.items(section))
    for i in range(len(variables)):
        variable, value = variables[i]
        variable = variable.replace('-', '_').strip()
        # !include handling
        if variable.startswith('!include'):
            if depth > 5:
                raise Exception('We have reached depth %s, we cant go deeper' % depth)
            include_path = variable[8:].strip()
            included_vars = get_variables_from_file(include_path, depth=depth + 1)
            variables.extend(included_vars)
            continue
        if variable.startswith('loose_'):
            variable = variable.replace('loose_', '', 1)
        if variable == 'default_time_zone':
            variable = 'time_zone'
        value = value.strip()
        if value and (value[0] == value[-1] == '"' or value[0] == value[-1] == "'"):
            value = value[1:-1]
        variables[i] = (variable, value)
    return variables


def get_calculator_db_values(innodb_buffer_pool_instances, innodb_buffer_pool_chunk_size):
    def try_parse(value):
        result = value
        try:
            result = float(value)
            result = int(value)
        except Exception:
            pass
        return result

    def prepare_size_variable(size):
        if not size[-1].isdigit() and size[:-1].isdigit() and size[-1].upper() in "KMGT":
            suffix_to_factor = {
                'k': 2**10,
                'K': 2**10,
                'm': 2**20,
                'M': 2**20,
                'g': 2**30,
                'G': 2**30,
                't': 2**40,
                'T': 2**40,
            }
            factor = suffix_to_factor[size[-1]]
            return str(int(size[:-1]) * factor)
        return size

    def floor_to_kb(size):
        size = int(size)
        kb = 2**10
        return str(size // kb * kb)

    def ceil_innodb_buffer_pool_size(pool_size):
        # https://dev.mysql.com/doc/refman/5.7/en/innodb-buffer-pool-resize.html
        new_pool_size = int(pool_size)
        mb = 2**20
        instances = innodb_buffer_pool_instances
        chunk_size = innodb_buffer_pool_chunk_size
        # innodb_buffer_pool_size = _k * innodb_buffer_pool_instances * innodb_buffer_pool_chunk_size
        _k = new_pool_size // instances // chunk_size
        if new_pool_size >= _k * instances * chunk_size + mb:
            return str((_k + 1) * instances * chunk_size)
        return str(_k * instances * chunk_size)

    string_to_bool = {'ON': 1, '1': 1, 'True': 1, 'true': 1, 'OFF': 0, '0': 0, 'False': 0, 'false': 0}

    def calculate_db_value(variable, mycnf_value, db_value):
        if mycnf_value is not None and mycnf_value[:-1].isdigit() and mycnf_value[-1].upper() in "KMGT":
            mycnf_value = prepare_size_variable(mycnf_value)

        hacked_mycnf_value = mycnf_value
        hacked_db_value = db_value

        if mycnf_value is None and db_value in string_to_bool:
            hacked_mycnf_value = 1
            hacked_db_value = string_to_bool[db_value]
        elif variable == 'innodb_buffer_pool_size':
            hacked_mycnf_value = ceil_innodb_buffer_pool_size(mycnf_value)
            mycnf_value = hacked_mycnf_value
        elif variable == 'max_allowed_packet':
            hacked_mycnf_value = floor_to_kb(mycnf_value)
        elif mycnf_value in string_to_bool and db_value in string_to_bool:
            hacked_mycnf_value = string_to_bool[mycnf_value]
            hacked_db_value = string_to_bool[db_value]
        elif variable == "sql_mode":
            hacked_db_value = frozenset(db_value.split(','))
            hacked_mycnf_value = frozenset(mycnf_value.split(','))
        elif len(db_value) > 0 and db_value[-1] == '/' and mycnf_value[-1] != '/':
            hacked_mycnf_value += '/'

        if try_parse(hacked_mycnf_value) != try_parse(hacked_db_value):
            return try_parse(mycnf_value)
        return None

    return calculate_db_value


def check_user_password(name, password, ssl_ca='/etc/mysql/ssl/allCAs.pem'):
    import MySQLdb

    conn = None
    try:
        conn = MySQLdb.connect(
            user=name, host=socket.getfqdn(), port=3306, password=ensure_str(password), ssl={'ca': ssl_ca}
        )
        conn.ping()
        return True
    except MySQLdb.OperationalError as exc:
        log.debug('password check for %s failed: %s %s', name, exc.args[0], exc.args[1])
        return False
    except MySQLdb.InterfaceError as exc:
        log.debug('password check for %s failed: %s %s', name, exc.args[0], exc.args[1])
        return False
    finally:
        if conn is not None:
            conn.close()


def get_major_num(major_human_meta):
    major, middle = [int(_) for _ in major_human_meta.split('.')]
    return major * 100 + middle


def check_upgrade(target_version, defaults_file='/root/.my.cnf', config_path='/etc/mysql/my.cnf', **kwargs):
    import subprocess
    import json

    # More details in https://st.yandex-team.ru/MDB-13800
    MDB_CHECKS = [
        'oldTemporalCheck',
        'mysqlSchemaCheck',
        'foreignKeyLengthCheck',
        # yes, there is a typo in JSON output
        # proof: https://github.com/mysql/mysql-shell/blob/8.0.26/modules/util/upgrade_check.cc#L599
        'enumSetElementLenghtCheck',
        'partitionedTablesInSharedTablespaceCheck',
        'circularDirectoryCheck',
        'removedFunctionsCheck',
        'groupByAscCheck',
        'removedSysLogVars',
        'schemaInconsistencyCheck',
        'ftsTablenameCheck',
        'engineMixupCheck',
        'oldGeometryCheck',
        'checkTableOutput',
    ]

    config = ConfigParser() if six.PY2 else ConfigParser(strict=False)
    config.read(defaults_file)

    cmd = "mysqlsh  -- util check-for-server-upgrade \
        {{ --user={user} --password=\"{password}\" --host={host} --port={port} }} \
        --output-format=JSON \
        --config-path={mysql_config_path} \
        --target-version=\"{target_version}\"".format(
        user=config.get('client', 'user'),
        password=config.get('client', 'password'),
        host=config.get('client', 'host'),
        port=config.get('client', 'port'),
        mysql_config_path=config_path,
        target_version=target_version,  # version should be like '8.0.17'
    )
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, err = proc.communicate()

    if proc.returncode != 0:
        log.error("mysqlsh stderr: %s", err)
        return {
            'result': False,  # trigger deploy-api to retry this operation
            'is_upgrade_safe': False,
            'comment': "upgrade-checker return non-zero result",
        }

    out = json.loads(output)

    result = []
    for check in out['checksPerformed']:
        if check['id'] in MDB_CHECKS and len(check['detectedProblems']) != 0:
            description = check.get('description', check.get('title', ''))
            objects = [x['dbObject'] for x in check['detectedProblems']]
            result.append(description + " [" + ','.join(objects) + "]")

    return {
        'is_upgrade_safe': len(result) == 0,
        'comment': '\n'.join(result),
    }


def ensure_str(s, encoding='utf-8', errors='strict'):
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


def kill_connections(cur, db=None, user=None, host=None, attempts=3):
    """
    Kills all connections of specified user or to specified database
    """
    import MySQLdb

    if (user is None) == (db is None):
        raise Exception("either user or db should be specified, not both")

    query = 'SELECT id FROM information_schema.processlist WHERE true'
    args = []
    if db is not None:
        query += ' AND db = %s'
        args.append(db)
    if user is not None:
        query += ' AND user = %s'
        args.append(user)
        if host is not None and host != '%':
            query += ' AND host = %s'
            args.append(host)
    args = tuple(args)

    conn_ids = []
    for i in range(attempts):
        cur.execute(query, args)
        conn_ids = [r['id'] for r in cur.fetchall()]
        if not conn_ids:
            return True
        log.warning('Killing connections for %s: %s', ' '.join(args), ' '.join(str(c) for c in conn_ids))
        for conn_id in conn_ids:
            try:
                cur.execute('KILL {}'.format(conn_id))
            except MySQLdb.OperationalError as err:
                log.warning('Got error %s killing connection %d', err.args, conn_id)
        time.sleep(0.5 * (i + 1))

    log.error('Connections %s are still alive', conn_ids)
    return False
