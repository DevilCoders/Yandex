from click import group, option, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.config import config_option
from cloud.mdb.cli.dbaas.internal.intapi import rest_request


@group('quota')
def quota_group():
    """Quota management commands."""
    pass


@quota_group.command('get')
@option('--cloud')
@pass_context
def get_quota_command(ctx, cloud):
    """Get quota."""
    if not cloud:
        cloud = config_option(ctx, 'compute', 'cloud')

    response = rest_request(ctx, 'GET', '', f'quota/{cloud}')
    print_response(ctx, response)
