from collections import OrderedDict

from click import argument, group, option, pass_context, ClickException

from cloud.mdb.cli.common.formatting import (
    format_bytes,
    print_response,
)
from cloud.mdb.cli.common.parameters import BytesParamType, ListParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.dbaas.internal import compute
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.metadb.host import get_host, get_hosts
from cloud.mdb.cli.dbaas.internal.metadb.security_group import get_security_groups
from cloud.mdb.cli.dbaas.internal.utils import cluster_status_options, DELETED_CLUSTER_STATUSES

FIELD_FORMATTERS = {
    'disk_size': format_bytes,
}


@group('compute')
def compute_group():
    """Compute management commands."""
    pass


@compute_group.group('instance')
def instance_group():
    """Instance management commands."""
    pass


@instance_group.command('get')
@argument('untyped_id', metavar='ID')
@pass_context
def get_instance_command(ctx, untyped_id):
    """Get compute instance.

    For getting compute instance by related object, ID argument accepts hostname
    in addition to instance ID."""
    try:
        instance_id = get_host(ctx, hostname=untyped_id)['vtype_id']
    except Exception:
        instance_id = untyped_id

    print_response(ctx, compute.get_instance(ctx, instance_id), field_formatters=FIELD_FORMATTERS)


@instance_group.command('list')
@option(
    '--id',
    '--instance-id',
    '--instance-ids',
    'instance_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several instance IDs. Multiple values can be specified through a comma.',
)
@option(
    '--exclude-id',
    '--exclude-instance-id',
    '--exclude-instance-ids',
    'exclude_instance_ids',
    type=ListParamType(),
    help='Filter objects to not output by one or several instance IDs. Multiple values can be specified through a comma.',
)
@option('-f', '--folder', 'folder_id', help='Filter objects to output by folder.')
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several clusters. Multiple values can be specified through a comma.',
)
@option(
    '--xcluster',
    '--exclude-cluster',
    '--exclude-clusters',
    'exclude_cluster_ids',
    type=ListParamType(),
    help='Filter objects to not output by one or several clusters. Multiple values can be specified through a comma.',
)
@option(
    '--sc',
    '--subcluster',
    '--subclusters',
    'subcluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several subclusters. Multiple values can be specified through a comma.',
)
@option(
    '-S',
    '--shard',
    '--shards',
    'shard_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several shards. Multiple values can be specified through a comma.',
)
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Filter objects to output by one or several host names. Multiple values can be specified through a comma.',
)
@option(
    '--task',
    '--tasks',
    'task_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several tasks. Multiple values can be specified through a comma.',
)
@option(
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-e', '--env', '--cluster-env', 'cluster_env', help='Filter objects to output by cluster environment.')
@cluster_status_options()
@option('-r', '--role', help='Filter objects to output by subcluster role.')
@option('--flavor', '--resource-preset', 'flavor', help='Filter objects to output by resource preset.')
@option('--disk-type', help='Filter objects to output by disk type.')
@option(
    '-g',
    '--geo',
    '--zone',
    '--zones',
    'zones',
    type=ListParamType(),
    help='Filter objects to output by one or several availability zones. Multiple values can be specified through a comma.',
)
@option('--metadata-key-eq', '--metadata-key-equal', 'metadata_eq_filter', type=(str, str))
@option('--metadata-key-ne', '--metadata-key-not-equal', 'metadata_ne_filter', type=(str, str))
@option('--boot-disk-size-eq', '--boot-disk-size-equal', 'boot_disk_size_eq_filter', type=BytesParamType())
@option('--boot-disk-size-ne', '--boot-disk-size-not-equal', 'boot_disk_size_ne_filter', type=BytesParamType())
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only host names.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_instances_command(
    ctx,
    instance_ids,
    exclude_instance_ids,
    cluster_ids,
    subcluster_ids,
    shard_ids,
    hostnames,
    cluster_statuses,
    exclude_cluster_statuses,
    metadata_eq_filter,
    metadata_ne_filter,
    boot_disk_size_eq_filter,
    boot_disk_size_ne_filter,
    limit,
    quiet,
    separator,
    **kwargs,
):
    """List compute instances."""

    def _table_formatter(host):
        return OrderedDict(
            (
                ('id', host['vtype_id']),
                ('fqdn', host['fqdn']),
                ('roles', host['roles']),
                ('zone', host['zone']),
                ('flavor', host['flavor']),
                ('disk type', host['disk_type']),
                ('disk size', host['disk_size']),
                ('cluster id', host['cluster_id']),
            )
        )

    def _filter(host):
        instance = compute.get_instance(ctx, host['vtype_id'])
        if metadata_eq_filter:
            key, value = metadata_eq_filter
            if instance.metadata[key] != value:
                return False

        if metadata_ne_filter:
            key, value = metadata_ne_filter
            if instance.metadata[key] == value:
                return False

        if boot_disk_size_eq_filter:
            boot_disk = compute.get_disk(ctx, instance.boot_disk.disk_id)
            if boot_disk.size != boot_disk_size_eq_filter:
                return False

        if boot_disk_size_ne_filter:
            boot_disk = compute.get_disk(ctx, instance.boot_disk.disk_id)
            if boot_disk.size == boot_disk_size_ne_filter:
                return False

        return True

    do_client_filtering = any(
        (metadata_eq_filter, metadata_ne_filter, boot_disk_size_eq_filter, boot_disk_size_ne_filter)
    )

    if exclude_cluster_statuses is None and not any(
        (cluster_statuses, cluster_ids, subcluster_ids, shard_ids, hostnames, instance_ids)
    ):
        exclude_cluster_statuses = DELETED_CLUSTER_STATUSES

    hosts = get_hosts(
        ctx,
        vtype='compute',
        vtype_ids=instance_ids,
        exclude_vtype_ids=exclude_instance_ids,
        cluster_ids=cluster_ids,
        subcluster_ids=subcluster_ids,
        shard_ids=shard_ids,
        hostnames=hostnames,
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        limit=limit if not do_client_filtering else None,
        **kwargs,
    )

    if do_client_filtering:
        hosts = [host for host in hosts if _filter(host)]

    print_response(
        ctx,
        hosts,
        default_format='table',
        field_formatters=FIELD_FORMATTERS,
        table_formatter=_table_formatter,
        quiet=quiet,
        id_key='vtype_id',
        separator=separator,
        limit=limit if do_client_filtering else None,
    )


@instance_group.command('set-metadata-key')
@argument('instance_ids', metavar='IDS', type=ListParamType())
@argument('key')
@argument('value')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def set_instance_metadata_key_command(ctx, instance_ids, key, value, force):
    """Set metadata key for one or several compute instances."""
    _check_instance_ids(ctx, instance_ids)

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for instance_id in instance_ids:
        compute.update_instance_metadata(ctx, instance_id, upsert={key: value})
        print(f'Key "{key}" was updated in metadata of instance {instance_id}')


@instance_group.command('set-security-groups')
@argument('instance_ids', metavar='IDS', type=ListParamType())
@argument('security_group_ids', metavar='SECURITY_GROUPS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def set_instance_security_groups_command(ctx, instance_ids, security_group_ids, force):
    """Set security groups for one or several compute instances."""
    _check_instance_ids(ctx, instance_ids)

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for instance_id in instance_ids:
        compute.update_instance_security_groups(ctx, instance_id, security_group_ids)
        print(f'Instance {instance_id} updated')


@instance_group.command('restart')
@argument('instance_ids', metavar='IDS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def restart_instance_command(ctx, instance_ids, force):
    """Restart one or several compute instances."""
    if not force:
        _check_instance_ids(ctx, instance_ids)

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for instance_id in instance_ids:
        compute.restart_instance(ctx, instance_id)
        print(f'Instance "{instance_id}" restarted')


@instance_group.command('delete-metadata-key')
@argument('instance_ids', metavar='IDS', type=ListParamType())
@argument('key')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def delete_instance_metadata_key_command(ctx, instance_ids, key, force):
    """Delete metadata key for one or several compute instances."""
    _check_instance_ids(ctx, instance_ids)

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for instance_id in instance_ids:
        compute.update_instance_metadata(ctx, instance_id, delete=[key])
        print(f'Key "{key}" was deleted in metadata of instance {instance_id}')


@compute_group.group('disk')
def disk_group():
    """Disk management commands."""
    pass


@disk_group.command('get')
@argument('disk_id', metavar='ID')
@pass_context
def get_disk_command(ctx, disk_id):
    """Get disk."""
    print_response(ctx, compute.get_disk(ctx, disk_id))


@compute_group.group('sg')
def sg_group():
    """Security group management commands."""
    pass


@sg_group.command('get')
@argument('security_group_id', metavar='ID')
@pass_context
def get_security_group_command(ctx, security_group_id):
    """Get security group."""
    print_response(ctx, compute.get_security_group(ctx, security_group_id))


@sg_group.command('list')
@option(
    '-f',
    '--folder',
    '--folders',
    'folder_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several folders. Multiple values can be specified through a comma.',
)
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several cluster IDs. Multiple values can be specified through a comma.',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only object IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_security_groups_command(ctx, quiet, separator, **kwargs):
    """List security groups."""

    def _table_formatter(sgroup):
        return OrderedDict(
            (
                ('id', sgroup['id']),
                ('cluster id', sgroup['cluster_id']),
                ('type', sgroup['type']),
                ('hash', sgroup['hash']),
                ('allow_all', sgroup['allow_all']),
            )
        )

    sgroups = get_security_groups(ctx, **kwargs)
    print_response(
        ctx, sgroups, default_format='table', table_formatter=_table_formatter, quiet=quiet, separator=separator
    )


def _check_instance_ids(ctx, instance_ids):
    hosts = get_hosts(ctx, vtype='compute', vtype_ids=instance_ids, exclude_cluster_statuses=DELETED_CLUSTER_STATUSES)

    if not hosts:
        raise ClickException('No instances found.')

    if len(hosts) != len(instance_ids):
        not_found_ids = set(instance_ids) - set(host['vtype_id'] for host in hosts)
        if len(not_found_ids) == 1:
            raise ClickException(f'Instance "{not_found_ids.pop()}" not found.')
        else:
            raise ClickException(f'Multiple instances were not found: {",".join(not_found_ids)}')
