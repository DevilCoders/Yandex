from collections import defaultdict
from copy import deepcopy

import jmespath
from click import argument, ClickException, group, option, pass_context

from cloud.mdb.cli.common.formatting import print_diff, print_header, print_response, format_timestamp
from cloud.mdb.cli.common.parameters import JsonParamType, ListParamType
from cloud.mdb.cli.common.utils import diff_objects
from cloud.mdb.cli.dbaas.internal.common import get_key
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.metadb.cluster import cluster_lock, get_cluster, get_cluster_type, get_clusters
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import PillarNotFound
from cloud.mdb.cli.dbaas.internal.metadb.host import get_hosts
from cloud.mdb.cli.dbaas.internal.metadb.pillar import (
    delete_pillar_key,
    get_pillar,
    get_pillar_history,
    get_pillar_keys,
    PILLAR_KEY_SEPARATORS,
    update_pillar_key,
    create_pillar,
    delete_pillar,
)
from cloud.mdb.cli.dbaas.internal.metadb.shard import get_shards
from cloud.mdb.cli.dbaas.internal.metadb.subcluster import get_subclusters
from cloud.mdb.cli.dbaas.internal.metadb.task import create_finished_task
from cloud.mdb.cli.dbaas.internal.utils import decrypt, encrypt


@group('pillar')
def pillar_group():
    """Pillar management commands."""
    pass


@pillar_group.command('get')
@option('-c', '--cluster', 'cluster_id')
@option('--sc', '--subcluster', 'subcluster_id')
@option('-S', '--shard', 'shard_id')
@option('-H', '--host')
@option('--cluster-type', type=ClusterType())
@option('--target', 'target_id')
@option('--rev', '--revision', 'revision', type=int)
@option(
    '--jsonpath',
    'jsonpath_expression',
    help='JSONPath expression to apply to the retrieved pillar using jsonb_path_query_array PostgreSQL function.',
)
@option(
    '--jmespath',
    'jmespath_expression',
    help='JMESPath expression to apply to the retrieved pillar (http://jmespath.org).',
)
@option('--decrypt', 'decrypt_result', is_flag=True, help='Decrypt retrieving value.')
@argument('untyped_id', metavar='[OBJECT]', required=False)
@pass_context
def get_pillar_command(
    ctx,
    untyped_id,
    cluster_id,
    subcluster_id,
    shard_id,
    host,
    cluster_type,
    target_id,
    revision,
    jsonpath_expression,
    jmespath_expression,
    decrypt_result,
):
    """Get pillar."""
    option_count = sum(bool(v) for v in (untyped_id, cluster_id, subcluster_id, shard_id, host, cluster_type))
    if option_count == 0:
        ctx.fail('One of --cluster, --subcluster, --shard, --host or --cluster-type option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --cluster, --subcluster, --shard, --host or --cluster-type option can be specified.')

    pillar = get_pillar(
        ctx,
        untyped_id=untyped_id,
        cluster_type=cluster_type,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=host,
        target_id=target_id,
        revision=revision,
        jsonpath_expression=jsonpath_expression,
    )

    if jmespath_expression:
        pillar = jmespath.search(jmespath_expression, pillar)

    if decrypt_result:
        pillar = _decrypt(ctx, pillar)

    print_response(ctx, pillar)


@pillar_group.command('get-key')
@argument('untyped_id', metavar='[OBJECT]', required=False)
@argument('key', metavar='KEY', required=False)
@option('--cluster-type', type=ClusterType())
@option('-c', '--cluster', '--clusters', 'cluster_ids', type=ListParamType())
@option('--sc', '--subcluster', '--subclusters', 'subcluster_ids', type=ListParamType())
@option('-S', '--shard', '--shards', 'shard_ids', type=ListParamType())
@option('-H', '--host', '--hosts', 'hostnames', type=ListParamType())
@option('--rev', '--revision', 'revision', type=int)
@option('--decrypt', 'decrypt_result', is_flag=True, help='Decrypt retrieving value.')
@pass_context
def get_pillar_key_command(
    ctx, untyped_id, cluster_type, cluster_ids, subcluster_ids, shard_ids, hostnames, key, revision, decrypt_result
):
    """Get pillar key value.

    The key can be specified using Salt or PostgreSQL syntax (e.g. "a1:b2" or "a1,b2"). It's also supported globing
    for key elements (e.g."a1:*:b2").
    """
    # Handling of positional arguments. Valid syntax: "OBJECT KEY" or "KEY".
    if key is None:
        if untyped_id is None:
            ctx.fail('Missing argument "KEY".')
        else:
            key = untyped_id
            untyped_id = None

    option_count = sum(bool(v) for v in (untyped_id, cluster_ids, subcluster_ids, shard_ids, hostnames, cluster_type))
    if option_count == 0:
        ctx.fail('One of --cluster, --subcluster, --shard, --host or --cluster-type option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --cluster, --subcluster, --shard, --host or --cluster-type option can be specified.')

    result = []

    if untyped_id:
        result = get_pillar_keys(ctx, key=key, untyped_id=untyped_id, revision=revision)

    elif cluster_type:
        result = get_pillar_keys(ctx, key=key, cluster_type=cluster_type, revision=revision)

    elif cluster_ids:
        clusters = get_clusters(ctx, cluster_ids=cluster_ids)
        if not clusters:
            raise ClickException('No clusters found.')

        for cluster in clusters:
            result.extend(get_pillar_keys(ctx, key=key, cluster_id=cluster['id'], revision=revision))

    elif subcluster_ids:
        subclusters = get_subclusters(ctx, subcluster_ids=subcluster_ids)
        if not subclusters:
            raise ClickException('No subclusters found.')

        for subcluster in subclusters:
            result.extend(get_pillar_keys(ctx, key=key, subcluster_id=subcluster['id'], revision=revision))

    elif shard_ids:
        shards = get_shards(ctx, shard_ids=shard_ids)
        if not shards:
            raise ClickException('No shards found.')

        for shard in shards:
            result.extend(get_pillar_keys(ctx, key=key, shard_id=shard['id'], revision=revision))

    elif hostnames:
        hosts = get_hosts(ctx, hostnames=hostnames)
        if not hosts:
            raise ClickException('No hosts found.')

        for host in hosts:
            result.extend(get_pillar_keys(ctx, key=key, host=host['public_fqdn'], revision=revision))

    for value in result:
        if decrypt_result:
            value = _decrypt(ctx, value)
        print_response(ctx, value)


@pillar_group.command('create')
@argument('value', type=JsonParamType())
@option('-c', '--cluster', '--clusters', 'cluster_ids', type=ListParamType())
@option('--sc', '--subcluster', '--subclusters', 'subcluster_ids', type=ListParamType())
@option('-S', '--shard', '--shards', 'shard_ids', type=ListParamType())
@option('-H', '--host', '--hosts', 'hostnames', type=ListParamType())
@option(
    '--rollback-protection/--no-rollback-protection',
    'rollback_protection',
    default=True,
    help='Create finished task to protect pillar changes from unintentional rollback. '
    'By default, the protection is enabled.',
)
@pass_context
def create_pillar_command(ctx, cluster_ids, subcluster_ids, shard_ids, hostnames, value, rollback_protection):
    """Create pillar."""
    option_count = sum(bool(v) for v in (cluster_ids, subcluster_ids, shard_ids, hostnames))
    if option_count == 0:
        ctx.fail('One of --cluster, --subcluster, --shard or --host option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --cluster, --subcluster, --shard or --host option can be specified.')

    if cluster_ids:
        clusters = get_clusters(ctx, cluster_ids=cluster_ids)
        if not clusters:
            raise ClickException('No clusters found.')

        for cluster in clusters:
            with cluster_lock(ctx, cluster['id']):
                create_pillar(ctx, value, cluster_id=cluster['id'])
                print(f'Pillar for cluster {cluster["id"]} created')
                if rollback_protection:
                    _create_finished_task(ctx, cluster, 'Created cluster pillar.')

    elif subcluster_ids:
        subclusters = get_subclusters(ctx, subcluster_ids=subcluster_ids)
        if not subclusters:
            raise ClickException('No subclusters found.')

        for subcluster in subclusters:
            with cluster_lock(ctx, subcluster['cluster_id']):
                cluster = get_cluster(ctx, subcluster['cluster_id'])
                create_pillar(ctx, value, subcluster_id=subcluster['id'])
                print(f'Pillar for subcluster {subcluster["id"]} created')
                if rollback_protection:
                    _create_finished_task(ctx, cluster, 'Created subcluster pillar.')

    elif shard_ids:
        shards = get_shards(ctx, shard_ids=shard_ids)
        if not shards:
            raise ClickException('No shards found.')

        cluster_shards_map = defaultdict(list)
        for shard in shards:
            cluster_shards_map[shard['cluster_id']].append(shard)

        for cluster_id, shards in cluster_shards_map.items():
            with cluster_lock(ctx, cluster_id):
                cluster = get_cluster(ctx, cluster_id)
                for shard in shards:
                    create_pillar(ctx, value, shard_id=shard['id'])
                    print(f'Pillar for shard {shard["id"]} created')
                if rollback_protection:
                    _create_finished_task(ctx, cluster, 'Created shard pillar(s).')

    elif hostnames:
        hosts = get_hosts(ctx, hostnames=hostnames)
        if not hosts:
            raise ClickException('No hosts found.')

        cluster_hosts_map = defaultdict(list)
        for host in hosts:
            cluster_hosts_map[host['cluster_id']].append(host)

        for cluster_id, hosts in cluster_hosts_map.items():
            with cluster_lock(ctx, cluster_id):
                cluster = get_cluster(ctx, cluster_id)
                for host in hosts:
                    create_pillar(ctx, value, host=host['public_fqdn'])
                    print(f'Pillar for host {host["public_fqdn"]} created')
                if rollback_protection:
                    _create_finished_task(ctx, cluster, 'Created host pillar(s).')


@pillar_group.command('set-key')
@argument('key')
@argument('value', type=JsonParamType())
@option('-c', '--cluster', '--clusters', 'cluster_ids', type=ListParamType())
@option('--sc', '--subcluster', '--subclusters', 'subcluster_ids', type=ListParamType())
@option('--with-shards', is_flag=True)
@option('-S', '--shard', '--shards', 'shard_ids', type=ListParamType())
@option('-H', '--host', '--hosts', 'hostnames', type=ListParamType())
@option('--encrypt', 'encrypt_value', is_flag=True, help='Encrypt the specified value.')
@option(
    '--rollback-protection/--no-rollback-protection',
    'rollback_protection',
    default=True,
    help='Create finished task to protect pillar changes from unintentional rollback. '
    'By default, the protection is enabled.',
)
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def set_pillar_key_command(
    ctx,
    cluster_ids,
    subcluster_ids,
    with_shards,
    shard_ids,
    hostnames,
    key,
    value,
    encrypt_value,
    rollback_protection,
    force,
):
    """Set pillar key.

    The key can be specified using Salt or PostgreSQL syntax (e.g. "a1:b2" or "a1,b2"). It's also supported globing
    for key elements (e.g."a1:*:b2").
    """
    option_count = sum(bool(v) for v in (cluster_ids, subcluster_ids, shard_ids, hostnames))
    if option_count == 0:
        ctx.fail('One of --cluster, --subcluster, --shard or --host option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --cluster, --subcluster, --shard or --host option can be specified.')

    if with_shards and not subcluster_ids:
        ctx.fail('Option --with-shards can be specified only together with --subcluster.')

    if encrypt_value:
        value = _encrypt(ctx, value)

    if cluster_ids:
        clusters = get_clusters(ctx, cluster_ids=cluster_ids)
        if not clusters:
            raise ClickException('No clusters found.')

        for cluster in clusters:
            with cluster_lock(ctx, cluster['id']):
                update_pillar_key(ctx, key, value, cluster_id=cluster['id'], force=force)
                print(f'Key "{key}" was updated in pillar of cluster {cluster["id"]}')

                comment = f"Updated key '{key}' in cluster pillar."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)

    elif subcluster_ids:
        subclusters = get_subclusters(ctx, subcluster_ids=subcluster_ids)
        if not subclusters:
            raise ClickException('No subclusters found.')

        for subcluster in subclusters:
            with cluster_lock(ctx, subcluster['cluster_id']):
                cluster = get_cluster(ctx, subcluster['cluster_id'])
                update_pillar_key(ctx, key, value, subcluster_id=subcluster['id'], force=force)
                print(f'Key "{key}" was updated in pillar of subcluster {subcluster["id"]}')

                if with_shards:
                    for shard in get_shards(ctx, subcluster_id=subcluster['id']):
                        update_pillar_key(ctx, key, value, shard_id=shard['id'], force=force)
                        print(f'Key "{key}" was updated in pillar of shard {shard["id"]}')

                comment = f"Updated key '{key}' in subcluster pillar."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)

    elif shard_ids:
        shards = get_shards(ctx, shard_ids=shard_ids)
        if not shards:
            raise ClickException('No shards found.')

        cluster_shards_map = defaultdict(list)
        for shard in shards:
            cluster_shards_map[shard['cluster_id']].append(shard)

        for cluster_id, shards in cluster_shards_map.items():
            with cluster_lock(ctx, cluster_id):
                cluster = get_cluster(ctx, cluster_id)
                for shard in shards:
                    update_pillar_key(ctx, key, value, shard_id=shard['id'], force=force)
                    print(f'Key "{key}" was updated in pillar of shard {shard["id"]}')

                comment = f"Updated key '{key}' in shard pillar(s)."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)

    elif hostnames:
        hosts = get_hosts(ctx, hostnames=hostnames)
        if not hosts:
            raise ClickException('No hosts found.')

        cluster_hosts_map = defaultdict(list)
        for host in hosts:
            cluster_hosts_map[host['cluster_id']].append(host)

        for cluster_id, hosts in cluster_hosts_map.items():
            with cluster_lock(ctx, cluster_id):
                cluster = get_cluster(ctx, cluster_id)
                for host in hosts:
                    update_pillar_key(ctx, key, value, host=host['public_fqdn'], force=force)
                    print(f'Key "{key}" was updated in pillar of host {host["public_fqdn"]}')

                comment = f"Updated key '{key}' in host pillar(s)."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)


@pillar_group.command('encrypt-key')
@argument('key')
@option('-c', '--cluster', '--clusters', 'cluster_ids', required=True, type=ListParamType())
@option(
    '--rollback-protection/--no-rollback-protection',
    'rollback_protection',
    default=True,
    help='Create finished task to protect pillar changes from unintentional rollback. '
    'By default, the protection is enabled.',
)
@pass_context
def encrypt_pillar_key_command(ctx, cluster_ids, key, rollback_protection):
    """Encrypt pillar key.

    The key can be specified using Salt or PostgreSQL syntax (e.g. "a1:b2" or "a1,b2"). It's also supported globing
    for key elements (e.g."a1:*:b2").
    """
    clusters = get_clusters(ctx, cluster_ids=cluster_ids)
    if not clusters:
        raise ClickException('No clusters found.')

    for cluster in clusters:
        with cluster_lock(ctx, cluster['id']):
            for current_value in get_pillar_keys(ctx, key=key, cluster_id=cluster['id']):
                _current_value = deepcopy(current_value)
                enc_value = _encrypt(ctx, current_value)
                if _current_value == enc_value:
                    print(f'Key "{key}" was already encrypted in pillar of cluster {cluster["id"]}, nothing done')
                    continue
                update_pillar_key(ctx, key, enc_value, cluster_id=cluster['id'], force=True)
                print(f'Key "{key}" was encrypted in pillar of cluster {cluster["id"]}')

                comment = f"Encrypted key '{key}' in cluster pillar."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)


def _encrypt(ctx, node):
    if not (isinstance(node, dict) or isinstance(node, str)):
        raise ClickException('Only strings are expected now, got {}'.format(node))
    if isinstance(node, dict) and node.get("encryption_version") is not None:
        # already cyphered
        return node
    if isinstance(node, str):
        # non-cyphered
        return encrypt(ctx, node)
    for key, value in node.items():
        node[key] = _encrypt(ctx, value)
    return node


@pillar_group.command('decrypt-key')
@argument('key')
@option('-c', '--cluster', '--clusters', 'cluster_ids', required=True, type=ListParamType())
@option(
    '--rollback-protection/--no-rollback-protection',
    'rollback_protection',
    default=True,
    help='Create finished task to protect pillar changes from unintentional rollback. '
    'By default, the protection is enabled.',
)
@pass_context
def decrypt_pillar_key_command(ctx, cluster_ids, key, rollback_protection):
    """Decrypt pillar key.

    The key can be specified using Salt or PostgreSQL syntax (e.g. "a1:b2" or "a1,b2"). It's also supported globing
    for key elements (e.g."a1:*:b2").
    """
    clusters = get_clusters(ctx, cluster_ids=cluster_ids)
    if not clusters:
        raise ClickException('No clusters found.')

    for cluster in clusters:
        with cluster_lock(ctx, cluster['id']):
            for current_value in get_pillar_keys(ctx, key=key, cluster_id=cluster['id']):
                _current_value = deepcopy(current_value)
                dec_value = _decrypt(ctx, current_value)
                if _current_value == dec_value:
                    print(f'Key "{key}" was already decrypted in pillar of cluster {cluster["id"]}, nothing done')
                    continue
                update_pillar_key(ctx, key, dec_value, cluster_id=cluster['id'], force=True)
                print(f'Key "{key}" was decrypted in pillar of cluster {cluster["id"]}')

                comment = f"Decrypted key '{key}' in cluster pillar."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)


def _decrypt(ctx, value):
    """
    Recursively decrypt the passed in value. Non-encrypted items are preserved intact.
    """
    if isinstance(value, dict):
        if value.get("encryption_version") is not None:
            return decrypt(ctx, value)

        return {i_key: _decrypt(ctx, i_value) for i_key, i_value in value.items()}

    if isinstance(value, list):
        return [_decrypt(ctx, i_value) for i_value in value]

    return value


@pillar_group.command('delete')
@option('--sc', '--subcluster', '--subclusters', 'subcluster_ids', type=ListParamType())
@option('-S', '--shard', '--shards', 'shard_ids', type=ListParamType())
@option('-H', '--host', '--hosts', 'hostnames', type=ListParamType())
@option(
    '--rollback-protection/--no-rollback-protection',
    'rollback_protection',
    default=True,
    help='Create finished task to protect pillar changes from unintentional rollback. '
    'By default, the protection is enabled.',
)
@pass_context
def delete_pillar_command(ctx, subcluster_ids, shard_ids, hostnames, rollback_protection):
    """delete pillar."""
    option_count = sum(bool(v) for v in (subcluster_ids, shard_ids, hostnames))
    if option_count == 0:
        ctx.fail('One of --subcluster, --shard or --host option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --subcluster, --shard or --host option can be specified.')

    if subcluster_ids:
        subclusters = get_subclusters(ctx, subcluster_ids=subcluster_ids)
        if not subclusters:
            raise ClickException('No subclusters found.')

        for subcluster in subclusters:
            with cluster_lock(ctx, subcluster['cluster_id']):
                cluster = get_cluster(ctx, subcluster['cluster_id'])
                delete_pillar(ctx, subcluster_id=subcluster['id'])
                print(f'Pillar for subcluster {subcluster["id"]} deleted')
                if rollback_protection:
                    _create_finished_task(ctx, cluster, 'Deleted subcluster pillar.')

    elif shard_ids:
        shards = get_shards(ctx, shard_ids=shard_ids)
        if not shards:
            raise ClickException('No shards found.')

        cluster_shards_map = defaultdict(list)
        for shard in shards:
            cluster_shards_map[shard['cluster_id']].append(shard)

        for cluster_id, shards in cluster_shards_map.items():
            with cluster_lock(ctx, cluster_id):
                cluster = get_cluster(ctx, cluster_id)
                for shard in shards:
                    delete_pillar(ctx, shard_id=shard['id'])
                    print(f'Pillar for shard {shard["id"]} deleted')
                if rollback_protection:
                    _create_finished_task(ctx, cluster, 'Deleted shard pillar(s).')

    elif hostnames:
        hosts = get_hosts(ctx, hostnames=hostnames)
        if not hosts:
            raise ClickException('No hosts found.')

        cluster_hosts_map = defaultdict(list)
        for host in hosts:
            cluster_hosts_map[host['cluster_id']].append(host)

        for cluster_id, hosts in cluster_hosts_map.items():
            with cluster_lock(ctx, cluster_id):
                cluster = get_cluster(ctx, cluster_id)
                for host in hosts:
                    delete_pillar(ctx, host=host['public_fqdn'])
                    print(f'Pillar for host {host["public_fqdn"]} deleted')
                if rollback_protection:
                    _create_finished_task(ctx, cluster, 'Deleted host pillar(s).')


@pillar_group.command('delete-key')
@argument('key')
@option('-c', '--cluster', '--clusters', 'cluster_ids', type=ListParamType())
@option('--sc', '--subcluster', '--subclusters', 'subcluster_ids', type=ListParamType())
@option('--with-shards', is_flag=True)
@option('-S', '--shard', '--shards', 'shard_ids', type=ListParamType())
@option('-H', '--host', '--hosts', 'hostnames', type=ListParamType())
@option('--ignore-missing', is_flag=True, help='Ignore missing pillars.')
@option(
    '--rollback-protection/--no-rollback-protection',
    'rollback_protection',
    default=True,
    help='Create finished task to protect pillar changes from unintentional rollback. '
    'By default, the protection is enabled.',
)
@pass_context
def delete_pillar_key_command(
    ctx, key, cluster_ids, subcluster_ids, with_shards, shard_ids, hostnames, ignore_missing, rollback_protection
):
    """Delete pillar key.

    The key can be specified using Salt or PostgreSQL syntax (e.g. "a1:b2" or "a1,b2"). It's also supported globing
    for key elements (e.g."a1:*:b2").
    """
    option_count = sum(bool(v) for v in (cluster_ids, subcluster_ids, shard_ids, hostnames))
    if option_count == 0:
        ctx.fail('One of --cluster, --subcluster, --shard or --host option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --cluster, --subcluster, --shard or --host option can be specified.')

    if with_shards and not subcluster_ids:
        ctx.fail('Option --with-shards can be specified only together with --subcluster.')

    if cluster_ids:
        clusters = get_clusters(ctx, cluster_ids=cluster_ids)
        if not clusters:
            raise ClickException('No clusters found.')

        for cluster in clusters:
            try:
                with cluster_lock(ctx, cluster['id']):
                    delete_pillar_key(ctx, key, cluster_id=cluster['id'])
                    print(f'Key "{key}" was deleted in pillar of cluster {cluster["id"]}')

                    comment = f"Deleted key '{key}' from cluster pillar."
                    if rollback_protection:
                        _create_finished_task(ctx, cluster, comment)
            except PillarNotFound:
                if not ignore_missing:
                    raise

    elif subcluster_ids:
        subclusters = get_subclusters(ctx, subcluster_ids=subcluster_ids)
        if not subclusters:
            raise ClickException('No subclusters found.')

        for subcluster in subclusters:
            try:
                with cluster_lock(ctx, subcluster['cluster_id']):
                    cluster = get_cluster(ctx, subcluster['cluster_id'])
                    delete_pillar_key(ctx, key, subcluster_id=subcluster['id'])
                    print(f'Key "{key}" was deleted in pillar of subcluster {subcluster["id"]}')

                    if with_shards:
                        for shard in get_shards(ctx, subcluster_id=subcluster['id']):
                            delete_pillar_key(ctx, key, shard_id=shard['id'])
                            print(f'Key "{key}" was deleted in pillar of shard {shard["id"]}')

                    comment = f"Deleted key '{key}' from subcluster pillar."
                    if rollback_protection:
                        _create_finished_task(ctx, cluster, comment)
            except PillarNotFound:
                if not ignore_missing:
                    raise

    elif shard_ids:
        shards = get_shards(ctx, shard_ids=shard_ids)
        if not shards:
            raise ClickException('No shards found.')

        cluster_shards_map = defaultdict(list)
        for shard in shards:
            cluster_shards_map[shard['cluster_id']].append(shard)

        for cluster_id, shards in cluster_shards_map.items():
            with cluster_lock(ctx, cluster_id):
                for shard in shards:
                    try:
                        cluster = get_cluster(ctx, cluster_id)
                        delete_pillar_key(ctx, key, shard_id=shard['id'])
                        print(f'Key "{key}" was deleted in pillar of shard {shard["id"]}')
                    except PillarNotFound:
                        if not ignore_missing:
                            raise

                comment = f"Deleted key '{key}' from shard pillar(s)."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)

    elif hostnames:
        hosts = get_hosts(ctx, hostnames=hostnames)
        if not hosts:
            raise ClickException('No hosts found.')

        cluster_hosts_map = defaultdict(list)
        for host in hosts:
            cluster_hosts_map[host['cluster_id']].append(host)

        for cluster_id, hosts in cluster_hosts_map.items():
            with cluster_lock(ctx, cluster_id):
                for host in hosts:
                    try:
                        cluster = get_cluster(ctx, cluster_id)
                        delete_pillar_key(ctx, key, host=host['public_fqdn'])
                        print(f'Key "{key}" was deleted in pillar of host {host["public_fqdn"]}')
                    except PillarNotFound:
                        if not ignore_missing:
                            raise

                comment = f"Deleted key '{key}' from host pillar(s)."
                if rollback_protection:
                    _create_finished_task(ctx, cluster, comment)


@pillar_group.command('diff')
@option('--from', '--from-revision', 'from_revision', type=int, default=1)
@option('--to', '--to-revision', 'to_revision', type=int)
@option('--key')
@option('--cluster-type', type=ClusterType())
@option('-c', '--cluster', 'cluster_id')
@option('--sc', '--subcluster', 'subcluster_id')
@option('-S', '--shard', 'shard_id')
@option('-H', '--host', 'hostname')
@option('--decrypt', 'decrypt_values', is_flag=True, help='Decrypt retrieving values.')
@pass_context
def diff_pillar_command(
    ctx,
    from_revision,
    to_revision,
    key,
    cluster_type,
    cluster_id,
    subcluster_id,
    shard_id,
    hostname,
    decrypt_values,
):
    """Compare pillar revisions."""
    option_count = sum(bool(v) for v in (cluster_id, subcluster_id, shard_id, hostname, cluster_type))
    if option_count == 0:
        ctx.fail('One of --cluster, --subcluster, --shard, --host or --cluster-type option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --cluster, --subcluster, --shard, --host or --cluster-type option can be specified.')

    from_value = get_pillar(
        ctx,
        cluster_type=cluster_type,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=hostname,
        revision=from_revision,
    )

    to_value = get_pillar(
        ctx,
        cluster_type=cluster_type,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=hostname,
        revision=to_revision,
    )

    if key:
        from_value = get_key(from_value, key, separators=PILLAR_KEY_SEPARATORS)
        to_value = get_key(to_value, key, separators=PILLAR_KEY_SEPARATORS)

    if decrypt_values:
        from_value = _decrypt(ctx, from_value)
        to_value = _decrypt(ctx, to_value)

    print_diff(diff_objects(from_value, to_value), key_separator=PILLAR_KEY_SEPARATORS[0])


@pillar_group.command('history')
@option('--from', '--from-revision', 'from_revision', type=int, default=1)
@option('--to', '--to-revision', 'to_revision', type=int)
@option('--key')
@option('-c', '--cluster', 'cluster_id')
@option('--sc', '--subcluster', 'subcluster_id')
@option('-S', '--shard', 'shard_id')
@option('-H', '--host', 'hostname')
@option('--decrypt', 'decrypt_values', is_flag=True, help='Decrypt retrieving values.')
@pass_context
def pillar_history_command(
    ctx,
    from_revision,
    to_revision,
    key,
    cluster_id,
    subcluster_id,
    shard_id,
    hostname,
    decrypt_values,
):
    """Get history of pillar changes."""
    option_count = sum(bool(v) for v in (cluster_id, subcluster_id, shard_id, hostname))
    if option_count == 0:
        ctx.fail('One of --cluster, --subcluster, --shard or --host option must be specified.')
    elif option_count > 1:
        ctx.fail('Only one of --cluster, --subcluster, --shard or --host option can be specified.')

    pillar_history = get_pillar_history(
        ctx,
        from_revision=from_revision,
        to_revision=to_revision,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=hostname,
    )

    base_pillar = None
    first_item = True
    for item in pillar_history:
        pillar = item['pillar']
        revision = item['revision']
        timestamp = item['timestamp']

        if key:
            pillar = get_key(pillar, key, separators=PILLAR_KEY_SEPARATORS)

        if decrypt_values:
            pillar = _decrypt(ctx, pillar)

        if first_item:
            print_header(f'Revision {revision}, {format_timestamp(ctx, timestamp)}')
            print_response(ctx, pillar)
            base_pillar = pillar
            first_item = False
            continue

        diff = diff_objects(base_pillar, pillar)
        if diff:
            print_header(f'\nRevision {revision}, {format_timestamp(ctx, timestamp)}')
            print_diff(diff, key_separator=PILLAR_KEY_SEPARATORS[0])
            base_pillar = pillar


def _create_finished_task(ctx, cluster, comment):
    operation_type = get_cluster_type(cluster) + '_cluster_modify'
    return create_finished_task(
        ctx,
        cluster_id=cluster['id'],
        operation_type=operation_type,
        revision=ctx.obj['cluster_rev'],
        comment=comment,
        hidden=True,
    )
