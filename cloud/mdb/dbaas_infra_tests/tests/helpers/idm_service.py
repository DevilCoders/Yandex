# pylint: disable=protected-access
"""
IDM Service helpers
"""

import requests

from tests.helpers import docker, internal_api, metadb


def get_base_url(context):
    """Get base URL for sending requests to IDM Service."""
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'idm-service01'), context.conf['projects']['idm-service']['expose']['http'])
    return 'http://{host}:{port}'.format(host=host, port=port)


def get_info(context):
    """Get info on available system roles."""
    url = '{}/info/'.format(get_base_url(context))
    return requests.get(url).json()


def get_roles(context):
    """Get info on roles of system users."""
    url = '{}/get-all-roles/'.format(get_base_url(context))
    return requests.get(url).json()


def get_clusters(context):
    """Get info of IDM clusters."""
    url = '{}/get-all-clusters/'.format(get_base_url(context))
    return requests.get(url)


def add_role(context, login, cid, role):
    """Add role to user."""
    url = '{}/add-role/'.format(get_base_url(context))
    params = {
        'login': login,
        'role': '{{"cluster": "{}", "grants": "{}"}}'.format(cid, role),
        'path': '/{}/{}'.format(cid, role),
    }
    return requests.post(url, params=params).json()


def remove_role(context, login, cid, role):
    """Remove role from user."""
    url = '{}/remove-role/'.format(get_base_url(context))
    params = {
        'login': login,
        'role': '{{"cluster": "{}", "grants": "{}"}}'.format(cid, role),
        'path': '/{}/{}'.format(cid, role),
    }
    return requests.post(url, params=params).json()


def rotate_passwords(context):
    """Rotate stale passwords of IDM users."""
    url = '{}/rotate-passwords/'.format(get_base_url(context))
    return requests.get(url)


def fetch_idm_users(context):
    """Get IDM users data."""
    query = """
    SELECT
        value->'data'->'config'->'pgusers'
    FROM
        dbaas.pillar
    WHERE
        cid = %s
    """
    with metadb._connect(context) as conn:
        with conn.cursor() as cur:
            cur.execute(query, (context.cluster['id'], ))
            users = cur.fetchone()[0] or {}
            return {login: info for login, info in users.items() if info.get('origin', 'other') == 'idm'}


def fetch_user_roles(context, login):
    """Returns a list of user's roles."""
    if context.cluster_type == 'postgresql':
        return pg_fetch_user_roles(context, login)
    if context.cluster_type == 'mysql':
        return mysql_fetch_user_roles(context, login)
    raise Exception('Unknown cluster type \'{}\''.format(context.cluster_type))


def idm_pending_task(context):
    """Get id of the task created by IDM."""
    query = """
    SELECT
        task_id
    FROM
        dbaas.worker_queue
    WHERE
        created_by = 'idm_service'
        AND end_ts IS NULL
    ORDER BY
        create_ts DESC
    LIMIT 1
    """
    with metadb._connect(context) as conn:
        with conn.cursor() as cur:
            cur.execute(query)
            res = cur.fetchone()
            return res[0] if res else None


def idm_rotate_passwords_task(context):
    """Get id of the task created by IDM for rotate passwords."""

    query = """
        SELECT task_id
        FROM
            dbaas.worker_queue
        WHERE
            created_by = 'idm_service'
            AND task_type = '{}'
        ORDER BY
            create_ts DESC
        LIMIT 1
        """.format(_get_rotate_password_task_type(context))
    with metadb._connect(context) as conn:
        with conn.cursor() as cur:
            cur.execute(query)
            res = cur.fetchone()
            return res[0] if res else None


def _get_rotate_password_task_type(context):
    if context.cluster_type == 'postgresql':
        return 'postgresql_user_modify'
    if context.cluster_type == 'mysql':
        return 'mysql_user_modify'
    raise Exception('Unknown cluster type \'{}\''.format(context.cluster_type))


def pg_fetch_user_roles(context, login):
    """Returns a list of user's roles for postgres."""
    query = """
    SELECT
        rolname
    FROM
        pg_user
        JOIN pg_auth_members ON (pg_user.usesysid = pg_auth_members.member)
        JOIN pg_roles ON (pg_roles.oid = pg_auth_members.roleid)
    WHERE
        pg_user.usename = %s
    """
    conn = internal_api.postgres_connect(
        context, cluster_config=context.cluster_config, hosts=context.hosts, master=True)
    with conn:
        with conn.cursor() as cur:
            cur.execute(query, (login, ))
            res = cur.fetchall()
            return [x[0] for x in res]


def mysql_fetch_user_roles(context, login):
    """Returns a list of user's roles for mysql."""
    query = """
    mysql -e "SELECT Proxied_user FROM mysql.proxies_priv where User = '{}' and host = '%';"
    """.format(login)
    internal_api.load_cluster_into_context(context, context.cluster['name'])
    internal_api.mysql_query(context, query="SELECT @@hostname AS host", master=True)
    master_fqdn = context.query_result['result'][0]['host']
    # container name is the same as fqdn here
    _, output = docker.run_command(master_fqdn, query, user='mysql')
    result = output.decode().split()[1:]
    return [r.replace("mdb_", "") for r in result]
