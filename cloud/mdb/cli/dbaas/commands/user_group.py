import hashlib

from click import argument, group, option, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import JsonParamType, ListParamType
from cloud.mdb.cli.dbaas.internal.intapi import perform_operation_rest
from cloud.mdb.cli.dbaas.internal.intapi import rest_request
from cloud.mdb.cli.dbaas.internal.metadb.cluster import (
    cluster_lock,
    get_cluster,
    get_cluster_type,
    get_cluster_type_by_id,
)
from cloud.mdb.cli.dbaas.internal.metadb.pillar import update_pillar_key
from cloud.mdb.cli.dbaas.internal.metadb.subcluster import get_subcluster
from cloud.mdb.cli.dbaas.internal.metadb.task import create_task
from cloud.mdb.cli.dbaas.internal.utils import encrypt, gen_random_string


ZK_ACL_USER_SUPER = 'super'
ZK_ACL_USER_CLICKHOUSE = 'clickhouse'
ZK_ACL_USER_BACKUP = 'backup'

MDB_BACKUP_ADMIN_USER = 'mdb_backup_admin'


@group('user')
def user_group():
    """User management commands."""
    pass


@user_group.command('list')
@argument('cluster_id', metavar='CLUSTER')
@pass_context
def list_users_command(ctx, cluster_id):
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    response = rest_request(ctx, 'GET', cluster_type, f'clusters/{cluster_id}/users')
    print_response(ctx, response)


@user_group.command('create')
@argument('cluster_id', metavar='CLUSTER')
@argument('user_name', metavar='USER')
@pass_context
def create_user_command(ctx, cluster_id, user_name):
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(
        ctx,
        'POST',
        cluster_type,
        f'clusters/{cluster_id}/users',
        data={
            'userSpec': {
                'name': user_name,
                'password': '12345678',
            },
        },
    )


@user_group.command('patch')
@argument('cluster_id', metavar='CLUSTER')
@argument('user_name', metavar='USER')
@argument('data', type=JsonParamType())
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def patch_user_command(ctx, cluster_id, user_name, data, hidden):
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(
        ctx, 'PATCH', cluster_type, f'clusters/{cluster_id}/users/{user_name}', hidden=hidden, data=data
    )


@user_group.command('batch-create')
@argument('cluster_id', metavar='CLUSTER')
@argument('user-specs', type=JsonParamType())
@option('--metadb-only', is_flag=True)
@pass_context
def batch_create_user_command(ctx, cluster_id, user_specs, metadb_only):
    """
    Create ClickHouse users.
    """
    cluster = get_cluster(ctx, cluster_id)

    cluster_type = get_cluster_type(cluster)
    if cluster_type not in ['clickhouse']:
        ctx.fail(f'The command is not supported for {cluster_type} clusters')

    with cluster_lock(ctx, cluster_id):
        ch_subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='clickhouse_cluster')

        users = ch_subcluster['pillar']['data']['clickhouse']['users'] or {}
        new_users = {}
        for user_spec in user_specs:
            name = user_spec['name']
            if name in users:
                ctx.fail(f'The user {name} already exists')

            password = user_spec['password']
            hash = hashlib.sha256(password.encode('utf-8')).hexdigest()
            user_data = {
                'password': encrypt(ctx, password),
                'hash': encrypt(ctx, hash),
            }
            if 'permissions' in user_spec:
                user_data['databases'] = [p['database_name'] for p in user_spec['permissions']]

            new_users[name] = user_data

        users.update(new_users)
        update_pillar_key(ctx, subcluster_id=ch_subcluster['id'], key='data,clickhouse,users', value=users)

        if not metadb_only:
            create_task(
                ctx,
                cluster_id=cluster_id,
                task_type='clickhouse_cluster_modify',
                operation_type='clickhouse_cluster_modify',
                wait=True,
            )


@user_group.command('delete')
@argument('cluster_id', metavar='CLUSTER')
@argument('user_name', metavar='USER')
@pass_context
def delete_user_command(ctx, cluster_id, user_name):
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(ctx, 'DELETE', cluster_type, f'clusters/{cluster_id}/user/{user_name}')


@user_group.command('zk-ch-create')
@argument('cluster_ids', metavar='CLUSTERS', type=ListParamType())
@pass_context
def create_zk_ch_users_command(ctx, cluster_ids):
    """
    Create ZooKeeper users in ClickHouse clusters.
    """
    for cluster_id in cluster_ids:
        cluster = get_cluster(ctx, cluster_id)

        cluster_type = get_cluster_type(cluster)
        if cluster_type not in ['clickhouse']:
            ctx.fail(f'The command is not supported for {cluster_type} clusters')

        with cluster_lock(ctx, cluster_id):
            ch_subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='clickhouse_cluster')
            ch_users = ch_subcluster['pillar']['data']['clickhouse'].get('zk_users', {})
            if _ensure_user(ctx, ch_users, ZK_ACL_USER_CLICKHOUSE, 32) > 0:
                update_pillar_key(
                    ctx,
                    subcluster_id=ch_subcluster['id'],
                    key='data,clickhouse,zk_users',
                    value=ch_users,
                )

            zk_subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='zk')
            zk_users = zk_subcluster['pillar']['data']['zk'].get('users', {})
            if (
                _ensure_user(ctx, zk_users, ZK_ACL_USER_SUPER, 128)
                + _ensure_user(ctx, zk_users, ZK_ACL_USER_BACKUP, 32)
                > 0
            ):
                update_pillar_key(ctx, subcluster_id=zk_subcluster['id'], key='data,zk,users', value=zk_users)


def _ensure_user(ctx, users, name, password_len):
    if name in users and 'password' in users[name]:
        return 0

    password = gen_random_string(password_len)
    users[name] = {
        'password': encrypt(ctx, password),
    }
    return 1


@user_group.command('system-ch-create')
@argument('cluster_ids', metavar='CLUSTERS', type=ListParamType())
@pass_context
def create_system_ch_users_command(ctx, cluster_ids):
    """
    Create system users in ClickHouse clusters (only 'mdb_backup_admin' now).
    """
    for cluster_id in cluster_ids:
        cluster = get_cluster(ctx, cluster_id)

        cluster_type = get_cluster_type(cluster)
        if cluster_type not in ['clickhouse']:
            ctx.fail(f'The command is not supported for {cluster_type} clusters')

        with cluster_lock(ctx, cluster_id):
            ch_subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='clickhouse_cluster')
            system_users = ch_subcluster['pillar']['data']['clickhouse'].get('system_users', {}) or {}
            if _ensure_system_user(ctx, system_users, MDB_BACKUP_ADMIN_USER, 128) > 0:
                update_pillar_key(
                    ctx,
                    subcluster_id=ch_subcluster['id'],
                    key='data,clickhouse,system_users',
                    value=system_users,
                )


def _ensure_system_user(ctx, users, name, password_len):
    if name in users and 'password' in users[name] and 'hash' in users[name]:
        return 0

    password = gen_random_string(password_len)
    hash = hashlib.sha256(password.encode('utf-8')).hexdigest()
    users[name] = {
        'password': encrypt(ctx, password),
        'hash': encrypt(ctx, hash),
    }
    return 1
