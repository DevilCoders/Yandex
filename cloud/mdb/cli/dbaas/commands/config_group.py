from click import argument, group, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import JsonParamType
from cloud.mdb.cli.dbaas.internal.config import get_config, get_config_key, update_config_key, delete_config_key


@group('config')
def config_group():
    """Config management commands."""
    pass


@config_group.command('get')
@argument('key', required=False)
@pass_context
def get_command(ctx, key):
    """Get config."""
    if key:
        result = get_config_key(ctx, key)
    else:
        result = get_config(ctx)

    print_response(ctx, result)


@config_group.command('set')
@argument('key')
@argument('value', type=JsonParamType())
@pass_context
def set_command(ctx, key, value):
    """Set value for config setting with the specified key."""
    update_config_key(ctx, key, value)


@config_group.command('delete')
@argument('key')
@pass_context
def delete_command(ctx, key):
    """Delete config setting with the specified key."""
    delete_config_key(ctx, key)
