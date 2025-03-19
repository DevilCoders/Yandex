from collections import OrderedDict

from click import argument, group, option, pass_context
from cloud.mdb.cli.common.formatting import format_bytes, format_bytes_per_second, print_response
from cloud.mdb.cli.common.parameters import ListParamType
from cloud.mdb.cli.dbaas.internal.metadb.flavor import FlavorType, get_flavor, get_flavors

FIELD_FORMATTERS = {
    'memory_guarantee': format_bytes,
    'memory_limit': format_bytes,
    'network_guarantee': format_bytes_per_second,
    'network_limit': format_bytes_per_second,
    'io_limit': format_bytes_per_second,
}


@group('resource-preset')
def resource_preset_group():
    """Commands to manage flavors / resource presets."""
    pass


@resource_preset_group.command('get')
@argument('name')
@pass_context
def get_resource_preset_command(ctx, name):
    """Get flavor / resource preset."""
    print_response(ctx, get_flavor(ctx, untyped_id=name), field_formatters=FIELD_FORMATTERS)


@resource_preset_group.command('list')
@option(
    '-t', '--type', 'flavor_type', type=FlavorType(), help='Filter objects to output by flavor / resource preset type.'
)
@option('--platform', help='Filter objects to output by platform.')
@option(
    '-G',
    '--generation',
    '--generations',
    'generations',
    type=ListParamType(int),
    help='Filter objects to output by one or several generations. Multiple values can be specified through a comma.',
)
@option('-l', '--limit', type=int, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only cluster IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_resource_presets_command(ctx, limit, quiet, separator, **kwargs):
    """List flavors / resource presets."""

    def _table_formatter(flavor):
        return OrderedDict(
            (
                ('name', flavor['name']),
                ('CPU guarantee / limit', f'{flavor["cpu_guarantee"]} / {flavor["cpu_limit"]}'),
                ('GPU limit', f'{flavor["gpu_limit"]}'),
                ('memory guarantee / limit', f'{flavor["memory_guarantee"]} / {flavor["memory_limit"]}'),
                ('network guarantee / limit', f'{flavor["network_guarantee"]} / {flavor["network_limit"]}'),
                ('IO limit', flavor['io_limit']),
                ('visible', flavor['visible']),
            )
        )

    print_response(
        ctx,
        get_flavors(ctx, limit=limit, **kwargs),
        default_format='table',
        field_formatters=FIELD_FORMATTERS,
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )
