# -*- coding: utf-8 -*-
"""
PostgreSQL module for salt
"""

from __future__ import absolute_import, print_function, unicode_literals

import logging
from contextlib import closing
from hashlib import md5
import sys

try:
    from salt.exceptions import CommandExecutionError
    from salt.utils.stringutils import to_bytes
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

    class CommandExecutionError(RuntimeError):
        pass


try:
    import six
except ImportError:
    from salt.ext import six


LOG = logging.getLogger(__name__)
LOG_SUPPRESS_OPTIONS = (
    '-c log_statement=none -c log_min_messages=panic '
    '-c log_min_error_statement=panic -c log_min_duration_statement=-1 '
    '-c pg_hint_plan.enable_hint=off '
)
READWRITE_OPTIONS = '-c default_transaction_read_only=off '
CONNECT_OPTIONS = LOG_SUPPRESS_OPTIONS + READWRITE_OPTIONS

# Initialize salt built-ins here (will be overwritten on lazy-import)
__salt__ = {}
__pillar__ = None


def __virtual__():
    """
    Return True always here to avoid fail on initial highstate call
    """
    return True


def ensure_text(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to six.text_type.
    For Python 2:
      - `unicode` -> `unicode`
      - `str` -> `unicode`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    """
    if sys.version_info[0] == 2:
        binary_type = str
        text_type = unicode  # noqa
    else:
        text_type = str
        binary_type = bytes
    if isinstance(s, binary_type):
        return s.decode(encoding, errors)
    elif isinstance(s, text_type):
        return s
    else:
        raise TypeError("not expecting type '%s'" % type(s))


def get_connection(database='postgres', user='postgres', host='localhost'):
    """
    Get connection with postresql.
    """
    import psycopg2
    from psycopg2.extras import LoggingConnection

    password = __salt__['pillar.get']('data:config:pgusers:{user}:password'.format(user=user))
    LOG.info('Establishing new connection with %s (%s/%s)', host, database, user)
    conn = psycopg2.connect(
        dbname=database,
        user=user,
        host=host,
        password=password,
        options=CONNECT_OPTIONS,
        connect_timeout=1,
        connection_factory=LoggingConnection,
    )
    conn.initialize(LOG)
    return conn


def is_in_recovery(cursor):
    """
    Return true if we are in recovery (= replica)
    """
    cursor.execute('SELECT pg_is_in_recovery()')
    return cursor.fetchone()[0]


def is_replica():
    """
    Return true if we are in recovery (= replica)
    """
    try:
        import psycopg2
    except ImportError:
        LOG.warn('Failed to check if node is replica: psycopg2 is not installed yet')
        return None
    if '__pg_is_replica' not in __pillar__:
        try:
            with closing(get_connection()) as conn:
                with conn as txn:
                    cursor = txn.cursor()
                    __pillar__['__pg_is_replica'] = is_in_recovery(cursor)
        except psycopg2.OperationalError as exc:
            LOG.warn('Failed to check if node is replica: %s', exc)
            __pillar__['__pg_is_replica'] = None
    return __pillar__['__pg_is_replica']


def drop_replication_slot(slot_name, cursor):
    """
    Drop replication slot helper, pg_sleep need for prevent 'replication slot is active'
    """
    cursor.execute(
        """
        SELECT
            pg_terminate_backend(active_pid),
            pg_sleep(0.1),
            pg_drop_replication_slot(slot_name)
        FROM pg_replication_slots
        WHERE slot_name = %(slot_name)s
        """,
        {'slot_name': slot_name},
    )


def create_physical_replication_slot(slot_name, cursor):
    """
    Create physical replication slot helper
    """
    cursor.execute('SELECT pg_create_physical_replication_slot(%(slot_name)s)', {'slot_name': slot_name})


def get_required_replication_slots(cursor):
    """
    Get list of slot_names for HA cluster nodes.
    Returns empty set on standby nodes.
    """
    cursor.execute('SELECT pg_is_in_recovery()')
    (is_replica,) = cursor.fetchone()
    if is_replica:
        return set()
    our_hostname = __salt__['grains.get']('id')
    hosts = __salt__['pillar.get']('data:cluster_nodes:ha', [])
    ret = set()
    for host in hosts:
        if host != our_hostname:
            ret.add(host.replace('-', '_').replace('.', '_'))
    return ret


def list_physical_replication_slots(cursor):
    """
    Get list of existing replication slots
    """
    cursor.execute(
        "SELECT slot_name FROM pg_replication_slots WHERE slot_type = 'physical' AND slot_name !~ 'pg_basebackup'"
    )
    return [x[0] for x in cursor.fetchall()]


def _rolconfig_to_dict(rolconfig):
    if rolconfig is None:
        return {}
    ret = {}
    for pair in rolconfig:
        key, value = pair.split('=', 1)
        ret[key] = value
    return ret


def get_md5_encrypted_password(user, password):
    """
    Get postgresql-style md5-encrypted password
    """
    return 'md5{hash}'.format(
        hash=md5(to_bytes(password, encoding='UTF-8') + to_bytes(user, encoding='UTF-8')).hexdigest()
    )


def list_users(cursor):
    """
    Get list of users in postgres
    """
    cursor.execute("SELECT rolname FROM pg_roles WHERE rolname !~ '^pg_'")
    return [x[0] for x in cursor.fetchall()]


def get_user(user_name, cursor):
    """
    Return basic user info from database
    """
    cursor.execute(
        """
        SELECT
            r.rolname,
            r.rolsuper,
            r.rolcreaterole,
            r.rolcreatedb,
            r.rolcanlogin,
            r.rolreplication,
            r.rolconnlimit,
            r.rolconfig,
            s.passwd
        FROM
            pg_roles r
            LEFT JOIN pg_shadow s ON (s.usename = r.rolname)
        WHERE r.rolname = %(rolname)s
        """,
        {'rolname': user_name},
    )
    res = cursor.fetchall()
    if not res:
        return None
    user = res[0]
    return {
        'name': user[0],
        'superuser': user[1],
        'createrole': user[2],
        'createdb': user[3],
        'login': user[4],
        'replication': user[5],
        'conn_limit': user[6],
        'settings': _rolconfig_to_dict(user[7]),
        'encrypted_password': user[8],
    }


def cmd_obj_escape(query, obj_name):
    """
    Escape user/database name in query
    """
    obj_name = ensure_text(obj_name)
    query = ensure_text(query)
    return query.replace("'{name}'".format(name=obj_name), '"{name}"'.format(name=obj_name.replace('"', '\\"')))


def make_with_query(query_start, with_options):
    """
    Format several groups of options using WITH statement
    """
    if with_options:
        with_options = [ensure_text(c) for c in with_options]
        return '{query_start} WITH {formatted}'.format(query_start=query_start, formatted=' '.join(with_options))
    return query_start


def format_encrypted_password(cursor, _, encrypted_password):
    """
    Password formatter
    """
    return ensure_text(cursor.mogrify('ENCRYPTED PASSWORD %(password)s', {'password': encrypted_password}))


def format_connection_limit(_cursor, _flag_name, conn_limit):
    """
    Connection limit formatter
    """
    return 'CONNECTION LIMIT {conn_limit}'.format(conn_limit=conn_limit)


def format_role_flag(_, flag_name, value):
    """
    Role flag formatter
    """
    return '{no}{flag}'.format(flag=flag_name.upper(), no='' if value else 'NO')


def format_with_role_options(options, cursor):
    """
    Role options formatter
    """
    formatters = [(flag, format_role_flag) for flag in ['superuser', 'createdb', 'createrole', 'login', 'replication']]
    formatters += [('conn_limit', format_connection_limit), ('encrypted_password', format_encrypted_password)]
    ret = []
    for flag, formatter in formatters:
        if flag in options:
            ret.append(formatter(cursor, flag, options[flag]))
    return ret


def create_user(name, data, cursor):
    """
    Create user from pillar-style spec
    """
    query = make_with_query(
        cmd_obj_escape(cursor.mogrify('CREATE ROLE %(name)s', {'name': name}), name),
        format_with_role_options(data, cursor),
    )
    cursor.execute(query)
    for key, value in data.get('settings', {}).items():
        query = cmd_obj_escape(cursor.mogrify('ALTER ROLE %(name)s', {'name': name}), name)
        query += ' SET {key} TO {value}'.format(
            key=key, value=ensure_text(cursor.mogrify('%(value)s', {'value': value}))
        )
        cursor.execute(query)


def modify_user(name, changes, cursor):
    """
    Modify user with pillar-style diff-spec
    """
    changed_options = format_with_role_options(changes, cursor)
    if changed_options:
        query = make_with_query(
            cmd_obj_escape(cursor.mogrify('ALTER ROLE %(name)s', {'name': name}), name), changed_options
        )
        cursor.execute(query)
    for key, value in changes.get('settings', {}).items():
        query = cmd_obj_escape(cursor.mogrify('ALTER ROLE %(name)s', {'name': name}), name)
        query += ' SET {key} TO {value}'.format(
            key=key, value=ensure_text(cursor.mogrify('%(value)s', {'value': value}))
        )
        cursor.execute(query)
    for key in changes.get('settings_removals', []):
        query = cmd_obj_escape(cursor.mogrify('ALTER ROLE %(name)s', {'name': name}), name)
        query += ' RESET {key}'.format(key=key)
        cursor.execute(query)


def terminate_user_connections(name, cursor):
    """
    Run pg_terminate_backend on all user connections
    """
    cursor.execute('SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE usename = %(name)s', {'name': name})


def _get_shdepend_objects(name, cursor):
    cursor.execute(
        """
        SELECT classid::regclass::text cl,
               array_agg(objid)
        FROM pg_catalog.pg_shdepend s
                 JOIN pg_catalog.pg_database d ON (d.oid = s.dbid)
        WHERE refclassid = 'pg_authid'::regclass
          AND datname = pg_catalog.current_database()
          AND refobjid = (SELECT r.oid FROM pg_catalog.pg_roles r WHERE rolname = %(name)s)
        GROUP BY classid;
        """,
        {'name': name},
    )
    return cursor.fetchall()


def _get_relation_acl(name, object_ids, cursor):
    cursor.execute(
        """
        WITH RECURSIVE privs AS (
            SELECT table_schema,
                   table_name,
                   privilege_type,
                   grantor,
                   grantee,
                   is_grantable,
                   1 AS level
            FROM information_schema.table_privileges
            WHERE grantee = %(name)s
              AND grantee != grantor
              AND (table_schema, table_name) IN
                (SELECT nspname,
                  relname
                FROM pg_catalog.pg_class c
                  JOIN pg_catalog.pg_namespace ns ON (c.relnamespace = ns.oid)
                WHERE c.oid = ANY(%(object_ids)s))
            UNION ALL
            SELECT tp.table_schema,
                tp.table_name,
                tp.privilege_type,
                tp.grantor,
                tp.grantee,
                tp.is_grantable, privs.level + 1 AS level
            FROM information_schema.table_privileges tp
                JOIN privs
            ON (tp.grantor = privs.grantee AND tp.privilege_type = privs.privilege_type
                AND tp.table_name = privs.table_name AND tp.table_schema = privs.table_schema)
        )
        SELECT DISTINCT *
        FROM privs
        WHERE table_schema !~ '^pg_temp'
        ORDER BY level;
        """,
        {'name': name, 'object_ids': object_ids},
    )
    res = {}
    for x in cursor.fetchall():
        level = x[6]
        if level not in res:
            res[level] = []
        res[level].append(
            {
                'table_schema': x[0],
                'table_name': x[1],
                'privilege_type': x[2],
                'grantor': x[3],
                'grantee': x[4],
                'is_grantable': x[5],
            }
        )
    return res


def revoke_acl_user(name, cursor):
    try:
        from psycopg2.extensions import AsIs, quote_ident
    except ImportError:
        LOG.warn('Failed to revoke acl, psycopg2 is not installed yet')
        return None
    acl_obj = _get_shdepend_objects(name, cursor)
    cursor.execute('SELECT user')
    root = cursor.fetchone()[0]
    for object_class, object_id in acl_obj:
        if object_class == 'pg_class':
            acls = _get_relation_acl(name, object_id, cursor)
            levels = sorted(acls.keys())
            for level in reversed(levels):
                for acl in acls[level]:
                    auth_query = cursor.mogrify(
                        'SET SESSION AUTHORIZATION %(grantor)s', {'grantor': AsIs(quote_ident(acl['grantor'], cursor))}
                    )
                    revoke_query = cursor.mogrify(
                        'REVOKE %(privilege_type)s ON %(ns)s.%(rel)s FROM %(grantee)s',
                        {
                            'privilege_type': AsIs(acl['privilege_type']),
                            'ns': AsIs(quote_ident(acl['table_schema'], cursor)),
                            'rel': AsIs(quote_ident(acl['table_name'], cursor)),
                            'grantee': AsIs(quote_ident(acl['grantee'], cursor)),
                        },
                    )
                    cursor.execute(auth_query)
                    cursor.execute(revoke_query)
            for level in levels:
                acl = acls[level]
                for i in range(len(acl)):
                    if acl[i]['grantee'] == name:
                        continue
                    if acl[i]['grantor'] == name:
                        acl[i]['grantor'] = acls[level - 1][i]['grantor']
                    auth_query = cursor.mogrify(
                        'SET SESSION AUTHORIZATION %(grantor)s',
                        {'grantor': AsIs(quote_ident(acl[i]['grantor'], cursor))},
                    )
                    grant_query_str = 'GRANT %(privilege_type)s ON %(ns)s.%(rel)s TO %(grantee)s'
                    if acl[i]['is_grantable'] == 'YES':
                        grant_query_str += " WITH GRANT OPTION"
                    grant_query = cursor.mogrify(
                        grant_query_str,
                        {
                            'privilege_type': AsIs(acl[i]['privilege_type']),
                            'ns': AsIs(quote_ident(acl[i]['table_schema'], cursor)),
                            'rel': AsIs(quote_ident(acl[i]['table_name'], cursor)),
                            'grantee': AsIs(quote_ident(acl[i]['grantee'], cursor)),
                        },
                    )

                    cursor.execute(auth_query)
                    cursor.execute(grant_query)
    cursor.execute('SET SESSION AUTHORIZATION %(root)s', {'root': root})


def reassign_doomed_user_objects(name, owner, cursor):
    """
    Reassign all owned by name to owner and drop all privileges
    """
    query = cursor.mogrify('REASSIGN OWNED BY %(doomed)s TO %(owner)s', {'doomed': name, 'owner': owner})
    query = cmd_obj_escape(query, name)
    query = cmd_obj_escape(query, owner)
    cursor.execute(query)
    query = cmd_obj_escape(cursor.mogrify('DROP OWNED BY %(doomed)s', {'doomed': name}), name)
    cursor.execute(query)


def drop_user(name, cursor):
    """
    Drop user (all privileges should be dropped and all connections shold be closed before calling this function)
    """
    query = cmd_obj_escape(cursor.mogrify('DROP ROLE %(doomed)s', {'doomed': name}), name)
    cursor.execute(query)


def get_user_role_membership(name, cursor):
    """
    Get list of roles for user
    """
    cursor.execute(
        """
        SELECT
            b.rolname
        FROM
            pg_catalog.pg_auth_members m
            JOIN pg_catalog.pg_roles b ON (m.roleid = b.oid)
        WHERE
            m.member IN (SELECT r.oid FROM pg_catalog.pg_roles r WHERE rolname = %(name)s)
        """,
        {'name': name},
    )
    return [x[0] for x in cursor.fetchall()]


def assign_role_to_user(name, role, cursor):
    """
    Grant role to user
    """
    query = cursor.mogrify('GRANT %(role)s TO %(name)s', {'role': role, 'name': name})
    query = cmd_obj_escape(query, name)
    query = cmd_obj_escape(query, role)
    cursor.execute(query)


def revoke_role_from_user(name, role, cursor):
    """
    Revoke role from user
    """
    query = cursor.mogrify('REVOKE %(role)s FROM %(name)s', {'role': role, 'name': name})
    query = cmd_obj_escape(query, name)
    query = cmd_obj_escape(query, role)
    cursor.execute(query)


def _expand_database_acl(acl_elem):
    """
    Return grantee if grantee is able to connect to database
    (That is all we need in current scope)
    """
    grantee, acl = acl_elem.split('=', 1)
    if grantee == '':
        grantee = 'public'
    if grantee.startswith('"') and grantee.endswith('"'):
        grantee = grantee[1:-1]
    grants = acl.split('/')[0]
    if 'c' in grants:
        return grantee


def get_database_acl(name, cursor):
    """
    Get database acl in dict form
    """
    cursor.execute('SELECT unnest(datacl) FROM pg_database WHERE datname = %(name)s', {'name': name})
    datacl = cursor.fetchall()
    # NULL in datacl means that public can connect (and public is empty string)
    if not datacl:
        datacl = ['=Tc/postgres']
    else:
        datacl = [x[0] for x in datacl]
    ret = set()
    for element in datacl:
        expanded = _expand_database_acl(element)
        if expanded:
            ret.add(expanded)
    return ret


def grant_database_connect(database, user, cursor):
    """
    Grant connect to database to user
    """
    query = cursor.mogrify('GRANT connect ON DATABASE %(database)s TO %(user)s', {'database': database, 'user': user})
    query = cmd_obj_escape(query, database)
    query = cmd_obj_escape(query, user)
    cursor.execute(query)


def revoke_database_connect(database, user, cursor):
    """
    Revoke connect to database from user
    """
    query = cursor.mogrify(
        'REVOKE connect ON DATABASE %(database)s FROM %(user)s',
        {
            'database': database,
            'user': user,
        },
    )
    query = cmd_obj_escape(query, database)
    query = cmd_obj_escape(query, user)
    cursor.execute(query)


def list_databases(cursor):
    """
    Get databases in postgres
    """
    cursor.execute("SELECT datname, datallowconn FROM pg_database WHERE datname <> 'template0'")
    return [{'datname': x[0], 'datallowconn': x[1]} for x in cursor.fetchall()]


def get_databases_with_owners(cursor):
    """
    Return list of tupes (dbname, datallowconn, owner)
    """
    cursor.execute(
        """
        SELECT
            d.datname,
            d.datallowconn,
            r.rolname
        FROM
            pg_database d
            JOIN pg_roles r ON (r.oid = d.datdba)
        WHERE
            datname <> 'template0'
        """
    )
    return [(x[0], x[1], x[2]) for x in cursor.fetchall()]


def create_database(database, owner, cursor, lc_ctype='C', lc_collate='C', template='template0'):
    """
    Create database in postgres
    """
    query = cursor.mogrify(
        'CREATE DATABASE %(database)s WITH '
        'OWNER %(owner)s '
        'LC_CTYPE %(lc_ctype)s '
        'LC_COLLATE %(lc_collate)s '
        'TEMPLATE %(template)s',
        {
            'database': database,
            'owner': owner,
            'lc_ctype': lc_ctype,
            'lc_collate': lc_collate,
            'template': template,
        },
    )
    query = cmd_obj_escape(query, database)
    query = cmd_obj_escape(query, owner)

    cursor.execute("SELECT datallowconn FROM pg_database WHERE datname =  %(template)s", {'template': template})
    allow_connections = int(cursor.fetchone()[0])

    if allow_connections:
        change_allow_connections(cursor, template, False)

    terminate_connections = cursor.mogrify(
        'SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = %(template)s;',
        {'template': template},
    )
    cursor.execute(terminate_connections)

    cursor.execute(query)
    if allow_connections:
        change_allow_connections(cursor, template, True)


def change_allow_connections(cursor, dbname, value):
    query = cursor.mogrify('ALTER DATABASE %(dbname)s ALLOW_CONNECTIONS %(value)s;', {'dbname': dbname, 'value': value})
    query = cmd_obj_escape(query, dbname)
    cursor.execute(query)


def terminate_database_connections(name, cursor):
    """
    Run pg_terminate_backend on all database connections
    """
    cursor.execute('SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = %(name)s', {'name': name})


def drop_database_subscriptions(cursor):
    """
    Drop database related subscriptions
    """
    cursor.execute(
        """
        SELECT
            subname
        FROM
            pg_subscription s
            JOIN pg_database d ON (s.subdbid = d.oid)
        WHERE
            d.datname = current_database()
        """
    )
    subnames = [x[0] for x in cursor.fetchall()]
    for sub in subnames:
        query = cmd_obj_escape(cursor.mogrify('ALTER SUBSCRIPTION %(sub)s DISABLE', {'sub': sub}), sub)
        cursor.execute(query)
        query = cmd_obj_escape(cursor.mogrify('ALTER SUBSCRIPTION %(sub)s SET (slot_name = NONE)', {'sub': sub}), sub)
        cursor.execute(query)
        query = cmd_obj_escape(cursor.mogrify('DROP SUBSCRIPTION %(sub)s', {'sub': sub}), sub)
        cursor.execute(query)


def drop_database(database, cursor):
    """
    Drop database in postgres
    """
    query = cmd_obj_escape(cursor.mogrify('DROP DATABASE %(database)s', {'database': database}), database)
    cursor.execute(query)


def list_extensions(cursor):
    """
    Get list of extensions in database
    """
    cursor.execute('SELECT extname FROM pg_extension')
    return [x[0] for x in cursor.fetchall()]


def add_extension(name, cursor, version=None, test=False):
    """
    Install extension in database
    """
    if version:
        query = cursor.mogrify(
            'CREATE EXTENSION IF NOT EXISTS %(name)s ' 'WITH VERSION %(version)s CASCADE',
            {'name': name, 'version': str(version)},
        )
    else:
        query = cursor.mogrify('CREATE EXTENSION IF NOT EXISTS %(name)s CASCADE', {'name': name})
    query = cmd_obj_escape(query, name)
    if not test:
        cursor.execute(query)
    return query


def drop_extension(name, cursor, cascade=False, test=False):
    """
    Drop extension in database
    """
    query = cursor.mogrify('DROP EXTENSION %(name)s' + (' CASCADE' if cascade else ''), {'name': name})
    query = cmd_obj_escape(query, name)
    if not test:
        cursor.execute(query)
    return query


def update_extension(name, cursor, version, test=False):
    """
    Update extension in database
    """
    query = cursor.mogrify('ALTER EXTENSION %(name)s UPDATE TO %(version)s', {'name': name, 'version': str(version)})
    query = cmd_obj_escape(query, name)
    if not test:
        cursor.execute(query)
    return query


def generate_ssh_keys(*args, **kwargs):
    from cryptography.hazmat.primitives import serialization as crypto_serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.hazmat.backends import default_backend as crypto_default_backend
    import os

    key = rsa.generate_private_key(backend=crypto_default_backend(), public_exponent=65537, key_size=2048)

    private_key = key.private_bytes(
        crypto_serialization.Encoding.PEM, crypto_serialization.PrivateFormat.PKCS8, crypto_serialization.NoEncryption()
    )
    private_key = ensure_str(private_key).replace(' PRIVATE KEY', ' RSA PRIVATE KEY')

    with open("/root/.ssh/id_rsa", 'wb') as content_file:
        content_file.write(ensure_binary(private_key))
        os.chmod("/root/.ssh/id_rsa", 0o600)

    public_key = key.public_key().public_bytes(
        crypto_serialization.Encoding.OpenSSH, crypto_serialization.PublicFormat.OpenSSH
    )
    return ensure_str(public_key) + ' %s' % os.uname()[1]


def upgrade(target_version, pillar=None, **kwargs):
    import json
    import subprocess

    if pillar:
        cluster_node_addrs = pillar.get('cluster_node_addrs', {})
        if cluster_node_addrs:
            with open('/tmp/cluster_node_addrs.conf', 'w') as f:
                json.dump(cluster_node_addrs, f)

    cmd = "flock -n /tmp/pg_upgrade_cluster.lock /usr/local/yandex/pg_upgrade_cluster.py {target_version}".format(
        target_version=target_version,
    )
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if stdout is not None:
        stdout = ensure_str(stdout)
    if stderr is not None:
        stderr = ensure_str(stderr)
    retcode = proc.returncode

    return {
        'is_rolled_back': retcode in {3, 4},
        'is_upgraded': True if retcode == 0 else False,
        'user_exposed_error': stdout.strip() if retcode == 3 else '',
        'stdout': stdout if retcode != 3 else '',
        'stderr': stderr,
    }


def run_backup(
    user_backup=False,
    backup_id=None,
    from_backup_id=None,
    full_backup=False,
    delta_from_previous_backup=True,
    from_backup_name=None,
):
    """
    Run backup (should be run as postgres user)
    """
    import setproctitle

    # set the process title so mdb_ping_salt_master
    # won't restart the salt-minion during the backup creation (MDB-17206)
    curr_title = setproctitle.getproctitle()
    setproctitle.setproctitle(curr_title + " run-backup")

    use_walg = __salt__['pillar.get']('data:use_walg', True)
    use_backup_api = __salt__['pillar.get']('data:backup:use_backup_service', False)
    if not use_walg:
        raise CommandExecutionError('use_walg is set to False, aborting the backup')

    args = ["--skip-delete-old", "--skip-election-in-zk"]

    if user_backup:
        args.append("--user-backup")

    if backup_id:
        args.append("--id %s" % backup_id)

    if from_backup_id:
        args.append("--delta-from-id %s" % from_backup_id)

    if full_backup:
        args.append("--full")

    if use_backup_api:
        rc, stdout, stderr = _run_backup_backup_service(args, backup_id, from_backup_name)
    else:
        rc, stdout, stderr = _run_backup_legacy(args, delta_from_previous_backup)

    if rc != 0:
        raise CommandExecutionError("backup failed: rc %d, stdout '%s', stderr '%s'" % (rc, stdout, stderr))

    return {'result': True, 'out': 'stdout: %s, stderr: %s' % (stdout, stderr)}


def _run_backup_legacy(args, delta_from_previous_backup):
    """
    Run backup (Backup API is disabled)
    """
    import subprocess

    if delta_from_previous_backup:
        args.append("--delta-from-previous-backup")

    cmd = (
        "sudo -u postgres flock -o /tmp/pg_walg_backup_push.lock /usr/local/yandex/pg_walg_backup_push.py "
        + " ".join(args)
    )

    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if stdout is not None:
        stdout = ensure_str(stdout)
    if stderr is not None:
        stderr = ensure_str(stderr)

    return proc.returncode, stdout, stderr


def _run_backup_backup_service(args, backup_id, from_backup_name):
    """
    Run backup (Backup API is enabled)
    """
    import subprocess

    if from_backup_name:
        args.append("--delta-from-name %s" % from_backup_name)

    # check if backup with the specified backup id already exists
    check_exists = (
        "/usr/bin/wal-g backup-list --config=/etc/wal-g/wal-g.yaml --json --detail "
        "| grep -q %s && echo 'backup with id %s already exists'" % (backup_id, backup_id)
    )

    do_backup = (
        "sudo -u postgres "
        "flock -o /tmp/pg_walg_backup_push.lock "
        "/usr/local/yandex/pg_walg_backup_push.py " + " ".join(args)
    )

    cmd = "%s || %s" % (check_exists, do_backup)

    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if stdout is not None:
        stdout = ensure_str(stdout)
    if stderr is not None:
        stderr = ensure_str(stderr)

    return proc.returncode, stdout, stderr


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


def ensure_binary(s, encoding='utf-8', errors='strict'):
    """Coerce **s** to six.binary_type.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> encoded to `bytes`
      - `bytes` -> `bytes`
    NOTE: The function equals to six.ensure_binary that is not present in the salt version of six module.
    """
    if isinstance(s, six.binary_type):
        return s
    if isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    raise TypeError("not expecting type '%s'" % type(s))
