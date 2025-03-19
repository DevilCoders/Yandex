# -*- coding: utf-8 -*-
"""
Greenplum module for salt
"""


from __future__ import absolute_import, print_function, unicode_literals

import json
import logging
import time
import os
import sys
import psycopg2
from collections import defaultdict
from hashlib import md5
from psycopg2.extras import LoggingConnection

try:
    from salt.utils.stringutils import to_bytes
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

LOG = logging.getLogger(__name__)
LOG_SUPPRESS_OPTIONS = (
    '-c log_statement=none -c log_min_messages=panic '
    '-c log_min_error_statement=panic -c log_min_duration_statement=-1'
)
UTILITY_OPTION = ' -c gp_session_role=utility'
# Initialize salt built-ins here (will be overwritten on lazy-import)
__salt__ = {}
__pillar__ = None


def __virtual__():
    """
    Return True always here to avoid fail on initial highstate call
    """
    return True


def get_gp_current_config():
    """
    Get current segments info
    """
    local_conn = get_connection()
    if not local_conn:
        LOG.info("Not master, skipping run")
        return {}

    conn = get_master_conn()
    if conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            SELECT content, preferred_role, hostname, datadir
            FROM pg_catalog.gp_segment_configuration
            WHERE status = 'u'
            ORDER BY content
            """
        )
        preferred_roles_map = {"p": "primary", "m": "mirror"}
        resp = {}
        for row in cursor.fetchall():
            if resp.get(row[0], False):
                resp[row[0]].update({preferred_roles_map[row[1]]: {"fqdn": row[2], "mount_point": row[3][:24]}})
            else:
                resp[row[0]] = {preferred_roles_map[row[1]]: {"fqdn": row[2], "mount_point": row[3][:24]}}

    return resp


def is_gp_preferred_role_pillar_actual():
    """
    Check that segment pillar in metadb is equal deployed configuration
    """
    current_config = get_gp_current_config()
    if not current_config:
        return {"msg": "Not a master, skipping run"}

    pillar_config = __salt__["pillar.get"]("data:greenplum:segments")

    resp = []
    for cur_seg_id, cur_seg_data in current_config.items():
        for pillar_seg_id, pillar_seg_data in pillar_config.items():
            if cur_seg_id == int(pillar_seg_id):
                cluster_id = __salt__["pillar.get"]("data:dbaas:cluster_id")
                data = {
                    "actual": False,
                    "debug_info": {
                        "pillar_seg_id": int(pillar_seg_id),
                        "cur_seg_id": cur_seg_id,
                        "pillar_seg_data": pillar_seg_data,
                        "cur_seg_data": cur_seg_data,
                    },
                    "changes": "on cluster: {} segment {} changes: {} -> {}".format(
                        __salt__["pillar.get"]("data:dbaas:cluster_id"),
                        cur_seg_id,
                        json.dumps(pillar_seg_data),
                        json.dumps(cur_seg_data),
                    ),
                    "backup": "backup: dbaas pillar get-key data -c {}".format(cluster_id),
                    "update": "update: dbaas pillar set-key data:greenplum:segments:{} '{}' -c {}".format(
                        cur_seg_id, json.dumps(cur_seg_data), cluster_id
                    ),
                    "revert": "rollback: dbaas pillar set-key data:greenplum:segments:{} '{}' -c {}".format(
                        pillar_seg_id, json.dumps(pillar_seg_data), cluster_id
                    ),
                }
                # Don't update one leg masters pillar with mirror: None
                if pillar_seg_data.get("mirror", False) is None:
                    del pillar_seg_data["mirror"]

                if cur_seg_data == pillar_seg_data:
                    data["actual"] = True
                    for k in ["changes", "backup", "update", "revert"]:
                        del data[k]
                resp.append(data)
    return resp


def get_segments_log_info():
    """
    Prepare data for using in generating push-client config: topics-logs-grpc.conf
    """
    raw_segments_pillar_data = __salt__['pillar.get']('data:greenplum:segments')
    hostname = __salt__['grains.get']('id')
    segment_name_template = "gpseg{}"  # noqa
    segment_log_path_postfix = "pg_log/greenplum-6-data.csv"
    segments = []
    resp = {}
    for segment_id in raw_segments_pillar_data:
        for role, data in raw_segments_pillar_data[segment_id].items():
            if role == "mirror" and data is None:
                LOG.info("No mirror configured for segment: {}".format(segment_id))
                continue
            if isinstance(data, dict) and data['fqdn'] == hostname:
                segment = {
                    "id": segment_id,
                    "preferred_role": role,
                    "type": "master" if int(segment_id) == -1 else "segment",
                    "name": segment_name_template.format(segment_id),
                    "fqdn": hostname,
                }
                type_path = segment["preferred_role"] if segment["type"] == "segment" else "master"
                segment["data_dir"] = "/".join(
                    [data["mount_point"], type_path, segment["name"], segment_log_path_postfix]
                )
                segments.append(segment)
    resp["segments"] = sorted(segments, key=lambda d: int(d["id"]))
    return resp


def get_map():
    """
    Prepare config for gpinitsystem
    result row format looks like this
    rc1b-sdmcdkqiehfn7mdm.mdb.cloud-preprod.yandex.net~rc1b-sdmcdkqiehfn7mdm.mdb.cloud-preprod.yandex.net~6004~/var/lib/greenplum/data1/primary/gpseg24~26~24
    """
    segments = __salt__['pillar.get']('data:greenplum:segments')
    res = {'primary': {}, 'mirror': {}, 'master': {}, 'standby': {}}
    for segment in segments:
        primary = segments[segment]['primary']['fqdn']
        if segments[segment].get('mirror', None) is None and segment == '-1':
            res['master'][primary] = [-1]
            continue
        mirror = segments[segment]['mirror']['fqdn']
        if segment == '-1':
            res['master'][primary] = [-1]
            res['standby'][mirror] = [-1]
            continue
        mirror = segments[segment]['mirror']['fqdn']
        segment = int(segment)
        if primary not in res['primary']:
            res['primary'][primary] = [segment]
        else:
            res['primary'][primary].append(segment)
        if mirror in res['mirror']:
            res['mirror'][mirror].append(segment)
        else:
            res['mirror'][mirror] = [segment]
    seg_map = {'mirror': [], 'primary': [], 'master': [], 'standby': []}
    ports = {'mirror': 7000, 'primary': 6000, 'master': 5432, 'standby': 5432}
    data = '/var/lib/greenplum/data1'
    dbid = 1
    for role in sorted(res):
        for fqdn in sorted(res[role], key=res[role].get):
            port = ports[role]
            for segment in sorted(res[role][fqdn]):
                seg_map[role].append(
                    '{fqdn}~{fqdn}~{port}~{data}/{role}/gpseg{segment}~{dbid}~{segment}'.format(
                        fqdn=fqdn,
                        dbid=dbid,
                        role=role if role != 'standby' else 'master',
                        data=data,
                        segment=segment,
                        port=port,
                    )
                )
                port += 1
                dbid += 1
    return seg_map


def get_segment_info(fqdn=None):
    res = []
    if not fqdn:
        fqdn = __salt__['grains.get']('id')

    for seg_type, segs in get_map().items():
        for seg in segs:
            if fqdn in seg:
                seg = seg.split("~")
                dic = {
                    'hostname': seg[0],
                    'address': seg[1],
                    'port': int(seg[2]),
                    'datadir': seg[3],
                    'dbid': int(seg[4]),
                    'content': int(seg[5]),
                    'preferred_role': 'm' if seg_type == 'mirror' or seg_type == 'standby' else 'p',
                }
                res.append(dic)
    return res


def get_connection(database='postgres', user='gpadmin', host='localhost', options=LOG_SUPPRESS_OPTIONS, port=5432):
    """
    Get connection with greenplum.
    """
    password = __salt__['pillar.get']('data:gpadmin_password')
    # For mvideo cluster compatibility
    if not password:
        password = __salt__['pillar.get']('data:greenplum:users:{user}:password'.format(user=user))
    LOG.info('Establishing new connection with %s (%s/%s)', host, database, user)
    try:
        conn = psycopg2.connect(
            port=port,
            dbname=database,
            user=user,
            host=host,
            password=password,
            options=options,
            connect_timeout=1,
            connection_factory=LoggingConnection,
        )
        conn.initialize(LOG)
        return conn
    except psycopg2.OperationalError:
        return None


def get_master_conn():
    """
    Get connection with master greenplum.
    """
    master_host = __salt__['pillar.get']('master_fqdn')
    if master_host:
        return get_connection(host=master_host)
    for scids, info in __salt__['pillar.get']('data:dbaas:cluster:subclusters', {}).items():
        if info['name'] == 'master_subcluster':
            for master_host in info['hosts']:
                conn = get_connection(host=master_host)
                if conn:
                    return conn


def get_segment_info_for_config_deploy():
    """
    Get list of segments data directories
    """
    hostname = __salt__['grains.get']('id')

    conn = get_master_conn()
    if conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            select datadir,content,port
            from pg_catalog.gp_segment_configuration
            where hostname = %(hostname)s
                and status = 'u'
            order by dbid
            """,
            {'hostname': hostname},
        )
        return [x for x in cursor.fetchall()]
    LOG.warn('Failed to get info about dir,content and port')
    fail = ['/tmp', 0, 0]
    return [fail]


def get_segment_info_for_config_deploy_v2():
    """
    Get list of segments data directories
    """
    cache_file = '/var/cache/salt/mdb_greenplum_get_segment_info_for_config_deploy.cache'
    hostname = __salt__['grains.get']('id')
    column_names = ['datadir', 'content', 'port', 'role', 'preferred_role', 'hostname']
    fallback_data = [dict(zip(column_names, ['/tmp', 0, 0, '', '', '']))]
    conn = get_master_conn()
    response_data = []
    if conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            select datadir, content, port, role, preferred_role, hostname
            from pg_catalog.gp_segment_configuration
            where hostname = %(hostname)s
                and status = 'u'
            order by dbid
            """,
            {'hostname': hostname},
        )
        response_data = [dict(zip(column_names, row)) for row in cursor.fetchall()]
        with open(cache_file, 'w') as cache:
            json.dump(response_data, cache)
    if not response_data:
        LOG.warn('Failed to connect to database, try returning cached data')
        try:
            with open(cache_file, 'r') as cache:
                try:
                    response_data = json.load(cache)
                    LOG.warn('Cache file {} parsed, returning data'.format(cache_file))
                except json.decoder.JSONDecodeError:
                    LOG.warn('Failed parse cache file, setting data to fallback data {}'.format(fallback_data))
                    response_data = fallback_data
        except IOError:
            LOG.warn('No valid cache file found, setting data to fallback data {}'.format(fallback_data))
            response_data = fallback_data
    return response_data


def render_walg_restore_config():
    segments_layout = __salt__['pillar.get']('data:greenplum:segments')
    primary_port_base = __salt__['pillar.get']('data:gp_init:port_base', 6000)
    mirror_port_base = __salt__['pillar.get']('data:gp_init:mirror_port_base', 7000)
    seg_prefix = __salt__['pillar.get']('data:gp_init:seg_prefix', 'gpseg')
    master_dir = __salt__['pillar.get']('data:gp_master_directory', '/var/lib/greenplum/data1')
    data_folders = __salt__['pillar.get']('data:gp_data_folders', ['/var/lib/greenplum/data1'])
    # currently we don't support multiple segment data folders for restore
    segment_dir = data_folders[0]
    master_port = __salt__['pillar.get']('data:gp_init:master_port', 5432)

    primary_configs = {}
    mirror_configs = {}
    n_used_primary_ports = defaultdict(int)  # useful for port number assignment
    n_used_mirror_ports = defaultdict(int)

    for content_id, segment_data in segments_layout.items():
        # primary setup
        fqdn = segment_data['primary']['fqdn']
        if content_id == "-1":
            primary_configs[content_id] = _format_restore_config_entry(
                fqdn, master_port, master_dir, seg_prefix, content_id, "master"
            )
        else:
            port = primary_port_base + n_used_primary_ports[fqdn]
            n_used_primary_ports[fqdn] += 1
            primary_configs[content_id] = _format_restore_config_entry(
                fqdn, port, segment_dir, seg_prefix, content_id, "primary"
            )

        # mirror setup (if exists)
        if 'mirror' not in segment_data or segment_data['mirror'] is None:
            continue

        fqdn = segment_data['mirror']['fqdn']
        if content_id == "-1":
            mirror_configs[content_id] = _format_restore_config_entry(
                fqdn, master_port, master_dir, seg_prefix, content_id, "master"
            )
        else:
            port = mirror_port_base + n_used_mirror_ports[fqdn]
            n_used_mirror_ports[fqdn] += 1
            mirror_configs[content_id] = _format_restore_config_entry(
                fqdn, port, segment_dir, seg_prefix, content_id, "mirror"
            )

    return json.dumps({"segments": primary_configs, "mirrors": mirror_configs}, indent=4)


def _format_restore_config_entry(fqdn, port, segment_dir, seg_prefix, content_id, folder):
    return {'hostname': fqdn, 'port': port, 'data_dir': os.path.join(segment_dir, folder, seg_prefix + content_id)}


def get_active_master():
    """
    Get fqdn of active_master
    """
    for scids, info in __salt__['pillar.get']('data:dbaas:cluster:subclusters', {}).items():
        if info['name'] == 'master_subcluster':
            for master_host in info['hosts']:
                conn = get_connection(host=master_host)
                if conn:
                    return master_host
    LOG.warn('Failed to get info about fqdn of active_master')
    fail = ''
    return fail


def get_master_replica_status():
    """
    Get list of segments data directories
    """
    hostname = __salt__['pillar.get']('standby_master_fqdn')

    conn = get_master_conn()
    if conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            select status
            from pg_catalog.gp_segment_configuration
            where hostname = %(hostname)s
            order by dbid
            """,
            {'hostname': hostname},
        )
        for row in cursor.fetchall():
            return row[0]
        return 'n'
    LOG.warn('Failed to get info about dir,content and port')
    fail = ''
    return fail


def check_segment_replica_status():
    """
    Get list of segments data directories
    """
    hostname = __salt__['pillar.get']('segment_fqdn')

    ret = {'changes': {}, 'result': False, 'comment': ''}

    conn = get_master_conn()
    if conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            select status, content
            from pg_catalog.gp_segment_configuration
            where status != 'u'
                and hostname = %(hostname)s
            """,
            {'hostname': hostname},
        )
        for row in cursor.fetchall():
            ret['comment'] = str(row[0]) + " " + str(row[1])
            return ret
        ret['result'] = True
        return ret
    LOG.warn('Failed to get info about host health')
    ret['comment'] = 'Failed to get info about host health'
    return ret


def check_segment_replicas_up_status():
    """
    Cgeck segments in UP status
    """

    ret = {'changes': {}, 'result': False, 'comment': ''}

    conn = get_master_conn()
    if conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            select status, content, hostname
            from pg_catalog.gp_segment_configuration
            where status != 'u' and content != -1;
            """,
            {},
        )
        for row in cursor.fetchall():
            ret['comment'] = str(row[0]) + " " + str(row[1]) + " " + str(row[2])
            return ret
        ret['result'] = True
        return ret
    LOG.warn('Failed to get info about host health')
    ret['comment'] = 'Failed to get info about host health'
    return ret


def check_segment_is_in_dead_status():
    """
    Get list of segments data directories
    """
    hostname = __salt__['pillar.get']('segment_fqdn')
    conn = get_master_conn()
    ret = {'changes': {}, 'result': False, 'comment': ''}
    i = 0
    while i < 100:
        i = i + 1
        if conn:
            cursor = conn.cursor()
            cursor.execute(
                """
                select case when exists(select 1 from gp_segment_configuration
                    where   hostname = %(hostname)s
                            and status = 'u') then 0 else 1 end all_dead;
                """,
                {'hostname': hostname},
            )
            for row in cursor.fetchall():
                if row[0] == 1:
                    ret['result'] = True
                    return ret
        time.sleep(5)
    LOG.warn('Failed to get info about host health')
    ret['comment'] = 'Failed to get info about host health'
    return ret


def check_master_replica_is_alive():
    """
    Get check replica is alive
    """
    conn = get_master_conn()
    ret = {'changes': {}, 'result': False, 'comment': ''}
    if conn:
        cursor = conn.cursor()
        cursor.execute(
            """
           select case
                    when
                        exists(
                                select 1
                                    from pg_stat_replication
                                    where state = 'streaming' and usename = 'gpadmin' and sync_state = 'sync'
                            )
                        and exists(select hostname from gp_segment_configuration where role = 'm' and content = -1)
                        and exists(
                                select * from gp_stat_replication where gp_segment_id = -1 and state = 'streaming' and sync_state = 'sync' and sync_error = 'none'
                            )
                        then 1 else 0 end;
            """,
            {},
        )
        for row in cursor.fetchall():
            if row[0] == 1:
                ret['result'] = True
                return ret
            else:
                ret['comment'] = 'Master-replica host is not healthy'
    LOG.warn('Failed to get info about master host health')
    ret['comment'] = 'Failed to get info about master host health'
    return ret


def get_ic_proxy_adresses_config():
    """
    Get gp_interconnect_proxy_addresses
    """

    conn = get_master_conn()
    if conn:
        cursor = conn.cursor()
        cursor.execute('SELECT dbid, content, address, port-1488 FROM gp_segment_configuration ORDER BY 1')
        res = []
        for seg in cursor.fetchall():
            res.append('{}:{}:{}:{}'.format(seg[0], seg[1], seg[2], seg[3]))
        return ','.join(res)
    LOG.warn('Failed to get info gp_segment_configuration')
    return None


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


def assign_role_to_user(name, role, admin_option, cursor):
    """
    Grant role to user
    """
    if admin_option:
        query = cursor.mogrify('GRANT %(role)s TO %(name)s WITH ADMIN OPTION', {'role': role, 'name': name})
    else:
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


def make_with_query(query_start, with_options):
    """
    Format several groups of options using WITH statement
    """
    if with_options:
        filtered = filter(lambda option: option not in ('RESOURCE_GROUP', 'RESOURCE_QUEUE'), with_options)
        return '{query_start} WITH {formatted}'.format(query_start=query_start, formatted=' '.join(filtered))
    return query_start


def format_encrypted_password(cursor, _, encrypted_password):
    """
    Password formatter
    """
    return ensure_text(cursor.mogrify('ENCRYPTED PASSWORD %(password)s', {'password': encrypted_password}))


def list_users(cursor):
    """
    Get list of users in greenplum
    """
    cursor.execute("SELECT rolname FROM pg_roles")
    return [x[0] for x in cursor.fetchall()]


def cmd_obj_escape(query, obj_name):
    """
    Escape user/database name in query
    """
    obj_name = ensure_text(obj_name)
    query = ensure_text(query)
    return query.replace("'{name}'".format(name=obj_name), '"{name}"'.format(name=obj_name.replace('"', '\\"')))


def get_md5_encrypted_password(user, password):
    """
    Get postgresql-style md5-encrypted password
    """
    return 'md5{hash}'.format(
        hash=md5(to_bytes(password, encoding='UTF-8') + to_bytes(user, encoding='UTF-8')).hexdigest()
    )


def format_role_flag(_, flag_name, value):
    """
    Role flag formatter
    """
    return '{no}{flag}'.format(flag=flag_name.upper(), no='' if value else 'NO')


def format_resource_group(cursor, _, resource_group_name):
    """
    Resource group formatter
    """
    return ensure_text(cursor.mogrify('RESOURCE GROUP {resource_group}'.format(resource_group=resource_group_name)))


def format_resource_queue(cursor, _, resource_queue_name):
    """
    Resource queue formatter
    """
    return ensure_text(cursor.mogrify('RESOURCE QUEUE {resource_queue}'.format(resource_queue=resource_queue_name)))


def format_with_role_options(options, cursor):
    """
    Role options formatter
    """
    formatters = [
        (flag, format_role_flag)
        for flag in [
            'superuser',
            'createdb',
            'createrole',
            'login',
            'createexttable(type=\'readable\')',
            'createexttable(type=\'writable\')',
            'createexttable(protocol=\'http\')',
            'resource_group',
        ]
    ]
    formatters += [('encrypted_password', format_encrypted_password)]
    if not options.get('resource_group'):
        formatters += [('resource_queue', format_resource_queue)]
    else:
        formatters += [('resource_group', format_resource_group)]
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


def add_extension(name, cursor, version=None, test=False):
    """
    Install extension in database
    """
    if version:
        query = cursor.mogrify(
            'CREATE EXTENSION IF NOT EXISTS %(name)s ' 'WITH VERSION %(version)s',
            {'name': name, 'version': str(version)},
        )
    else:
        query = cursor.mogrify('CREATE EXTENSION IF NOT EXISTS %(name)s', {'name': name})
    query = cmd_obj_escape(query, name)
    if not test:
        cursor.execute(query)
    return query


def add_protocol(name, cursor, write_func=None, read_func=None, trusted=False, test=False):
    """
    Install protocol in database
    """
    query = cursor.mogrify(
        'CREATE '
        + ('TRUSTED ' if trusted else '')
        + 'PROTOCOL %(name)s ('
        + ('writefunc=%(write_func)s' if write_func is not None else '')
        + (',' if None not in (write_func, read_func) else '')
        + ('readfunc=%(read_func)s' if read_func is not None else '')
        + ')',
        {'name': name, 'write_func': write_func, 'read_func': read_func},
    )
    query = cmd_obj_escape(query, name)
    if not test:
        cursor.execute(query)
    return query


def _rolconfig_to_dict(rolconfig):
    if rolconfig is None:
        return {}
    ret = {}
    for pair in rolconfig:
        key, value = pair.split('=', 1)
        ret[key] = value
    return ret


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
            r.rolcreaterextgpfd,
            r.rolcreaterexthttp,
            r.rolcreatewextgpfd,
            rgc.groupname,
            s.passwd
        FROM
            pg_roles r
            LEFT JOIN pg_shadow s ON (s.usename = r.rolname)
            LEFT JOIN gp_toolkit.gp_resgroup_config rgc ON (rgc.groupid = r.rolresgroup)
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
        'createexttable(type=\'readable\')': user[8],
        'createexttable(protocol=\'http\')': user[9],
        'createexttable(type=\'writable\')': user[10],
        'resource_group': user[11],
        'encrypted_password': user[12],
    }


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


def list_resource_groups(cursor):
    """
    Get list of users in greenplum
    """
    cursor.execute("SELECT groupname FROM gp_toolkit.gp_resgroup_config")
    return [x[0] for x in cursor.fetchall()]


def create_resource_group(group, cursor):
    """
    Creates resource group
    """
    query = cursor.mogrify('CREATE RESOURCE GROUP %(name)s WITH (cpu_rate_limit=1)', {'name': group})
    query = cmd_obj_escape(query, group)
    cursor.execute(query)
    return query


def list_databases(cursor):
    """
    Get databases in greenplum
    """
    cursor.execute("SELECT datname, datallowconn FROM pg_database WHERE datname <> 'template0'")
    return [{'datname': x[0], 'datallowconn': x[1]} for x in cursor.fetchall()]


def get_database_options(cursor, database):
    """
    Get databases options which was setted via ALTER DATABASE
    """
    result = {}
    cursor.execute(
        'SELECT unnest(setconfig) FROM pg_db_role_setting s JOIN pg_database d ON d.oid = s.setdatabase WHERE d.datname = %(name)s',
        {'name': database},
    )
    for row in cursor.fetchall():
        option, value = row[0].split('=', 1)  # maxsplit=1
        result[option] = value
    return result


def database_set_option(cursor, database, key, value):
    """
    Set database option with ALTER DATABASE
    """
    query = cursor.mogrify(
        'ALTER DATABASE %(name)s SET %(key)s TO %(value)s', {'name': database, 'key': key, 'value': value}
    )
    query = cmd_obj_escape(query, database)
    query = cmd_obj_escape(query, key)
    cursor.execute(query)


def create_database(database, owner, cursor, lc_ctype='C', lc_collate='C'):
    """
    Create database in postgres
    """
    query = cursor.mogrify(
        'CREATE DATABASE %(database)s WITH '
        'OWNER %(owner)s '
        'LC_CTYPE %(lc_ctype)s '
        'LC_COLLATE %(lc_collate)s '
        'TEMPLATE template0',
        {
            'database': database,
            'owner': owner,
            'lc_ctype': lc_ctype,
            'lc_collate': lc_collate,
        },
    )
    query = cmd_obj_escape(query, database)
    query = cmd_obj_escape(query, owner)
    cursor.execute(query)


def reload_segment(port=5432, content_id=None, host='localhost', test=False):
    if content_id == -1:
        options = LOG_SUPPRESS_OPTIONS
    else:
        options = LOG_SUPPRESS_OPTIONS + UTILITY_OPTION

    ret = {'changes': {}, 'result': True, 'comment': 'Reload segment'}
    if test:
        ret['changes']['reload'] = 'Would reload segment on {}:{} with content_id {}'.format(host, port, content_id)
        return ret
    try:
        conn = psycopg2.connect(
            port=port, dbname='postgres', user='gpadmin', host=host, options=options, connect_timeout=1
        )
        cursor = conn.cursor()
        cursor.execute('select pg_reload_conf()')
        ret['changes']['reload'] = 'Reloaded segment on {}:{} with content_id {}'.format(host, port, content_id)
        return ret
    except psycopg2.OperationalError as e:
        msg = str(e)
        if msg.find('the database system is in recovery mode') != -1:
            ret['comment'] = 'Mirror, nothing to reload'
            return ret
    ret['result'] = False
    LOG.error('Failed to reload segment on %s:%s with content_id %s', host, port, content_id)
    ret['comment'] = 'Failed to reload segment'
    return ret


def update_extension(name, cursor, version, test=False):
    """
    Update extension in database
    """
    query = cursor.mogrify('ALTER EXTENSION %(name)s UPDATE TO %(version)s', {'name': name, 'version': str(version)})
    query = cmd_obj_escape(query, name)
    if not test:
        cursor.execute(query)
    return query


def get_master_fqdns():
    """
    Get fqdns of master subcluster hosts
    """
    for _, info in __salt__['pillar.get']('data:dbaas:cluster:subclusters', {}).items():
        if info['name'] == 'master_subcluster':
            return info['hosts'].keys()


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


def list_all_databases():
    conn = get_master_conn()
    conn.autocommit = True
    cursor = conn.cursor()
    dbs = [x["datname"] for x in __salt__["mdb_greenplum.list_databases"](cursor)]
    return dbs
