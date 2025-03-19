from click import argument, group, option, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import ListParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.dbaas.internal import conductor


@group('conductor')
def conductor_group():
    """Conductor commands."""
    pass


@conductor_group.group('host')
def host_group():
    """Commands to manage hosts in Conductor."""
    pass


@host_group.command('get')
@argument('hostname')
@pass_context
def get_host_command(ctx, hostname):
    """Get host."""
    print_response(ctx, conductor.get_host(ctx, hostname))


@host_group.command('delete')
@argument('hostnames', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def delete_host_command(ctx, hostnames, force):
    """Delete one or several hosts."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for hostname in hostnames:
        conductor.delete_host(ctx, hostname)
        print(f'Host "{hostname}" deleted from conductor.')
