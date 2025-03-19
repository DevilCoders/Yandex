from collections import OrderedDict

from click import argument, group, option, pass_context
from cloud.mdb.cli.common.formatting import (
    format_bytes,
    format_bytes_per_second,
    print_response,
)
from cloud.mdb.cli.common.parameters import BytesParamType, FieldsParamType, ListParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.dbaas.internal import dbm_api
from cloud.mdb.cli.dbaas.internal.db import db_transaction
from cloud.mdb.cli.dbaas.internal.dbm.dom0 import get_dom0_server, get_dom0_server_stats, get_dom0_servers
from cloud.mdb.cli.dbaas.internal.dbm.porto_container import (
    delete_porto_container,
    ensure_porto_container_exists,
    get_porto_container,
    get_porto_containers,
    update_porto_container,
)
from cloud.mdb.cli.dbaas.internal.dbm.transfer import get_transfers
from cloud.mdb.cli.dbaas.internal.dbm.volume import get_volumes
from cloud.mdb.cli.dbaas.internal.metadb.host import get_hosts
from cloud.mdb.cli.dbaas.internal.utils import format_references

DOM0_FIELD_FORMATTERS = {
    'memory_free': format_bytes,
    'memory_total': format_bytes,
    'network_free': format_bytes_per_second,
    'network_total': format_bytes_per_second,
    'io_free': format_bytes_per_second,
    'io_total': format_bytes_per_second,
    'ssd_space_free': format_bytes,
    'ssd_space_total': format_bytes,
    'hdd_space_free': format_bytes,
    'hdd_space_total': format_bytes,
    'raw_disks_space_free': format_bytes,
    'raw_disks_space_total': format_bytes,
}

CONTAINER_FIELD_FORMATTERS = {
    'memory_guarantee': format_bytes,
    'memory_limit': format_bytes,
    'network_guarantee': format_bytes_per_second,
    'network_limit': format_bytes_per_second,
    'io_limit': format_bytes_per_second,
    'volume_size': format_bytes,
}

VOLUME_FIELD_FORMATTERS = {
    'size': format_bytes,
}


@group('porto')
def porto_group():
    """Porto management commands."""
    pass


@porto_group.group('dom0')
def dom0_group():
    """Commands to manage dom0 servers."""
    pass


@dom0_group.command('get')
@argument('untyped_id', metavar='ID')
@option('-q', '--quiet', is_flag=True, help='Output only dom0 server hostname.')
@pass_context
def get_dom0_server_command(ctx, untyped_id, quiet):
    """Get dom0 server.

    For getting dom0 server by related object, ID argument accepts container hostname
    in addition to dom0 server hostname.
    """
    dom0_server = get_dom0_server(ctx, untyped_id=untyped_id)

    result = OrderedDict((key, value) for key, value in dom0_server.items())
    result['references'] = _format_references(ctx, dom0_server)

    print_response(ctx, result, field_formatters=DOM0_FIELD_FORMATTERS, quiet=quiet)


def _format_references(ctx, dom0_server):
    macros = {
        'dom0': dom0_server['id'],
    }
    return format_references(ctx, 'dom0_references', macros)


@dom0_group.command('list')
@argument('untyped_ids', metavar='IDS', required=False, type=ListParamType())
@option('--dom0', 'dom0_ids', help='Specify dom0 servers to output.')
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Filter objects to output by one or several hosts. Multiple values can be specified through a comma.',
)
@option('--project', default='pgaas', help='Filter objects to output by project. Default is "pgaas".')
@option(
    '-G',
    '--generation',
    '--generations',
    'generations',
    type=ListParamType(int),
    help='Filter objects to output by one or several generations. Multiple values can be specified through a comma.',
)
@option(
    '-g',
    '--geo',
    '--zone',
    '--zones',
    'zones',
    type=ListParamType(),
    help='Filter objects to output by one or several availability zones. Multiple values can be specified through a comma.',
)
@option('--allow-new-hosts', type=bool, help='Filter objects to output by allow_new_hosts flag.')
@option(
    '-c',
    '--cpu',
    '--free-cpu',
    'free_cpu',
    help='Filter dom0 servers to output by free CPU cores.'
    ' Only dom0 servers with at least the specified number of free CPU cores are added to the output.',
)
@option(
    '-m',
    '--memory',
    '--free-memory',
    'free_memory',
    type=BytesParamType(),
    help='Filter dom0 servers to output by free memory.'
    ' Only dom0 servers with at least the specified amount of free memory are added to the output.',
)
@option(
    '-s',
    '--ssd',
    '--free-ssd',
    'free_ssd',
    type=BytesParamType(),
    help='Filter dom0 servers to output by free SSD space.'
    ' Only dom0 servers with at least the specified amount of free SSD space are added to the output.',
)
@option(
    '--hdd',
    '--free-hdd',
    '--sata',
    '--free-sata',
    'free_hdd',
    type=BytesParamType(),
    help='Filter dom0 servers to output by free HDD space.'
    ' Only dom0 servers with at least the specified amount of free HDD space are added to the output.',
)
@option(
    '--raw-disks',
    '--free-raw-disks',
    'free_raw_disks',
    type=BytesParamType(),
    help='Filter dom0 servers to output by free raw disks.'
    ' Only dom0 servers with at least the specified number of free raw disks are added to the output.',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only dom0 server hostnames.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_dom0_servers_command(ctx, quiet, separator, **kwargs):
    """List dom0 servers."""

    def _table_formatter(dom0):
        return OrderedDict(
            (
                ('id', dom0['id']),
                ('generation', dom0['generation']),
                ('CPU', f'{dom0["cpu_free"]} / {dom0["cpu_total"]}'),
                ('memory', f'{dom0["memory_free"]} / {dom0["memory_total"]}'),
                ('network', f'{dom0["network_free"]} / {dom0["network_total"]}'),
                ('SSD', f'{dom0["ssd_space_free"]} / {dom0["ssd_space_total"]}'),
                (
                    'raw disks',
                    f'{dom0["raw_disks_free"]} / {dom0["raw_disks_total"]} ({dom0["raw_disks_space_free"]} / {dom0["raw_disks_space_total"]})',
                ),
            )
        )

    print_response(
        ctx,
        get_dom0_servers(ctx, **kwargs),
        default_format='table',
        field_formatters=DOM0_FIELD_FORMATTERS,
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )


@dom0_group.command('stats')
@option('--project', default='pgaas', help='Count dom0 servers with the specified project only. Default is "pgaas".')
@option(
    '-G',
    '--generation',
    '--generations',
    'generations',
    type=ListParamType(int),
    help='Filter objects to output by one or several generations. Multiple values can be specified through a comma.',
)
@option(
    '-g',
    '--geo',
    '--zone',
    '--zones',
    'zones',
    type=ListParamType(),
    help='Count dom0 servers with the specified availability zones only. Multiple values can be specified through a comma.',
)
@option('--allow-new-hosts', type=bool, help='Count dom0 servers with the specified allow_new_hosts flag value only.')
@pass_context
def get_dom0_server_stats_command(ctx, **kwargs):
    """Ger statistics on dom0 servers."""

    def _table_formatter(dom0):
        return OrderedDict(
            (
                ('generation', dom0['generation']),
                ('geo', dom0['geo']),
                ('CPU', f'{dom0["cpu_free"]} / {dom0["cpu_total"]}'),
                ('memory', f'{dom0["memory_free"]} / {dom0["memory_total"]}'),
                ('network', f'{dom0["network_free"]} / {dom0["network_total"]}'),
                ('SSD', f'{dom0["ssd_space_free"]} / {dom0["ssd_space_total"]}'),
                (
                    'raw disks',
                    f'{dom0["raw_disks_free"]} / {dom0["raw_disks_total"]} ({dom0["raw_disks_space_free"]} / {dom0["raw_disks_space_total"]})',
                ),
            )
        )

    stats = get_dom0_server_stats(ctx, **kwargs)
    print_response(
        ctx, stats, default_format='table', field_formatters=DOM0_FIELD_FORMATTERS, table_formatter=_table_formatter
    )


@dom0_group.command('set-allow-new-hosts')
@argument('dom0_id', metavar='ID')
@argument('value', type=bool)
@pass_context
def set_allow_new_hosts_command(ctx, dom0_id, value):
    """Set allow_new_hosts flag for dom0 server."""
    print_response(ctx, dbm_api.set_allow_new_hosts(ctx, dom0_id, value))


@porto_group.group('container')
def container_group():
    """Commands to manage porto containers."""
    pass


@container_group.command('get')
@argument('hostname', metavar='ID')
@pass_context
def get_container_command(ctx, hostname):
    """Get porto container."""
    container = get_porto_container(ctx, hostname)
    print_response(ctx, container, field_formatters=CONTAINER_FIELD_FORMATTERS)


@container_group.command('list')
@argument('untyped_ids', metavar='IDS', required=False, type=ListParamType())
@option(
    '--dom0',
    'dom0_ids',
    help='Filter porto containers to output by one or several dom0 hostnames. Multiple values can be specified through a comma.',
    type=ListParamType(),
)
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Specify hostnames of porto containers to output. Multiple values can be specified through a comma.',
)
@option('-c', '--cluster', '--clusters', 'cluster_ids', type=ListParamType())
@option('--project', default='pgaas', help='Filter objects to output by project. Default is "pgaas".')
@option(
    '-G',
    '--generation',
    '--generations',
    'generations',
    type=ListParamType(int),
    help='Filter objects to output by one or several generations. Multiple values can be specified through a comma.',
)
@option(
    '-g',
    '--geo',
    '--zone',
    '--zones',
    'zones',
    type=ListParamType(),
    help='Filter porto containers to output by one or several availability zones. Multiple values can be specified through a comma.',
)
@option('--bootstrap-cmd', 'bootstrap_cmd', help='Filter porto containers to output by bootstrap command.')
@option(
    '--exclude-bootstrap-cmd',
    'exclude_bootstrap_cmd',
    help='Filter porto containers to not output by bootstrap command.',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only porto container hostnames.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_porto_containers_command(ctx, quiet, separator, cluster_ids, hostnames, **kwargs):
    """List porto containers."""

    if cluster_ids or hostnames:
        hosts = get_hosts(ctx, cluster_ids=cluster_ids, hostnames=hostnames)
        hostnames = [h['fqdn'] for h in hosts]

    def _table_formatter(container):
        return OrderedDict(
            (
                ('fqdn', container['fqdn']),
                ('CPU guarantee / limit', f'{container["cpu_guarantee"]} / {container["cpu_limit"]}'),
                ('memory guarantee / limit', f'{container["memory_guarantee"]} / {container["memory_limit"]}'),
                ('network guarantee / limit', f'{container["network_guarantee"]} / {container["network_limit"]}'),
                ('IO limit', container['io_limit']),
                ('volume size', container['volume_size']),
                ('dom0', container['dom0']),
            )
        )

    porto_containers = get_porto_containers(ctx, hostnames=hostnames, **kwargs)
    print_response(
        ctx,
        porto_containers,
        default_format='table',
        field_formatters=CONTAINER_FIELD_FORMATTERS,
        table_formatter=_table_formatter,
        quiet=quiet,
        id_key='fqdn',
        separator=separator,
    )


@container_group.command('set-bootstrap-cmd')
@argument('hostnames', type=ListParamType())
@argument('value')
@pass_context
def set_bootstrap_cmd_command(ctx, hostnames, value):
    """Update bootstrap command of porto containers."""
    for hostname in hostnames:
        update_porto_container(ctx, hostname, bootstrap_cmd=value)
        print(f'Bootstrap command was updated for porto container {hostname}')


@container_group.command('unset-ip')
@argument('hostnames', type=ListParamType())
@pass_context
def unset_ip_command(ctx, hostnames):
    """Remove ip from extra properties of porto containers."""
    with db_transaction(ctx, 'dbm'):
        for hostname in hostnames:
            container = get_porto_container(ctx, hostname)

            extra_properties = container['extra_properties']
            extra_properties.pop('ip', None)

            update_porto_container(ctx, hostname, extra_properties=extra_properties)

            print(f'Porto container "{hostname}" was updated')


@container_group.command('delete')
@argument('hostname')
@option('--db', 'db_mode', is_flag=True)
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def delete_container_command(ctx, hostname, db_mode, force):
    """Delete porto container."""
    ensure_porto_container_exists(ctx, hostname)

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    if db_mode:
        delete_porto_container(ctx, hostname)
    else:
        print_response(ctx, dbm_api.delete_porto_container(ctx, hostname))


@porto_group.group('volume')
def volume_group():
    """Commands to manage porto container volumes."""
    pass


@volume_group.command('list')
@option(
    '--dom0',
    'dom0_ids',
    help='Filter porto containers to output by one or several dom0 hostnames. Multiple values can be specified through a comma.',
    type=ListParamType(),
)
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Specify hostnames of porto containers to output. Multiple values can be specified through a comma.',
)
@option(
    '--fields',
    type=FieldsParamType(),
    default='container,path,size,read_only,dom0,dom0_path',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "container,path,size,read_only,dom0".',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only porto transfer IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_volumes_command(ctx, fields, quiet, separator, **kwargs):
    """List porto container volumes."""
    print_response(
        ctx,
        get_volumes(ctx, **kwargs),
        default_format='table',
        field_formatters=VOLUME_FIELD_FORMATTERS,
        fields=fields,
        quiet=quiet,
        separator=separator,
    )


@porto_group.group('transfer')
def transfer_group():
    """Commands to manage porto container transfers."""
    pass


@transfer_group.command('list')
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only porto transfer IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_transfers_command(ctx, quiet, separator, **kwargs):
    """List porto container transfers."""
    print_response(
        ctx,
        get_transfers(ctx, **kwargs),
        default_format='table',
        fields=['id', 'src_dom0', 'src_dom0', 'dest_dom0', 'container', 'placeholder', 'started'],
        quiet=quiet,
        separator=separator,
    )


@transfer_group.command('finish')
@argument('transfer_id', metavar='ID')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def finish_transfer_command(ctx, transfer_id, force):
    """Finish porto container transfer."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    print_response(ctx, dbm_api.finish_transfer(ctx, transfer_id))


@transfer_group.command('cancel')
@argument('transfer_id', metavar='ID')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def cancel_transfer_command(ctx, transfer_id, force):
    """Cancel porto container transfer."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    print_response(ctx, dbm_api.cancel_transfer(ctx, transfer_id))
