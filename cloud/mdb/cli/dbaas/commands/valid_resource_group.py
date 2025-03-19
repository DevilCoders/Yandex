from collections import OrderedDict

from click import group, option, pass_context
from cloud.mdb.cli.common.formatting import format_bytes, format_list, print_response
from cloud.mdb.cli.common.parameters import ListParamType, FieldsParamType
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.metadb.common import to_cluster_type
from cloud.mdb.cli.dbaas.internal.metadb.flavor import FlavorType, get_valid_resources


@group('valid-resource')
def valid_resource_group():
    """Commands to manage valid resources."""
    pass


@valid_resource_group.command('list')
@option(
    '-t',
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(type=ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-r', '--role', help='Filter objects to output by role.')
@option('--flavor-type', type=FlavorType(), help='Filter objects to output by flavor / resource preset type.')
@option('--platform', help='Filter objects to output by platform.')
@option(
    '-G',
    '--generation',
    '--generations',
    'generations',
    type=ListParamType(int),
    help='Filter objects to output by one or several generations. Multiple values can be specified through a comma.',
)
@option('--disk-type', help='Filter objects to output by disk type.')
@option(
    '--fields',
    type=FieldsParamType(),
    default='cluster_type,role,flavor,cpu,memory,disk_types,disk_size,host_count,feature_flag',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "cluster_type,role,flavor,cpu,memory,disk_types,disk_size,host_count,feature_flag".',
)
@option('-l', '--limit', type=int, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only cluster IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_valid_resources_command(ctx, fields, limit, quiet, separator, **kwargs):
    """List valid resources."""
    valid_resources = [_format_valid_resource(vr) for vr in get_valid_resources(ctx, limit=limit, **kwargs)]
    print_response(
        ctx,
        valid_resources,
        default_format='table',
        fields=fields,
        quiet=quiet,
        separator=separator,
    )


def _format_valid_resource(vr):
    if vr['disk_sizes']:
        if len(vr['disk_sizes']) > 1:
            min_size = format_bytes(vr['disk_sizes'][0])
            max_size = format_bytes(vr['disk_sizes'][-1])
            disk_size = f'{min_size}-{max_size}'
        else:
            disk_size = format_bytes(vr['disk_sizes'][0])
    else:
        min_size = format_bytes(vr['min_disk_size'])
        max_size = format_bytes(vr['max_disk_size'])
        disk_size = f'{min_size}-{max_size}'

    return OrderedDict(
        (
            ('cluster_type', to_cluster_type(vr['cluster_type'])),
            ('role', vr['role']),
            ('flavor', vr['flavor']),
            ('cpu', vr['cpu_guarantee']),
            ('memory', format_bytes(vr['memory_guarantee'])),
            ('zones', format_list(vr['zone_ids'])),
            ('disk_types', format_list(vr['disk_types'])),
            ('disk_size', disk_size),
            ('host_count', f'{vr["min_hosts"]}-{vr["max_hosts"]}'),
            ('feature_flag', vr['feature_flag']),
        )
    )
