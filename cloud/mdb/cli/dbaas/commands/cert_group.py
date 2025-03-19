from click import argument, group, option, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.dbaas.internal.certificator import delete_certificate, get_certificate


@group('cert')
def cert_group():
    """Commands to manage certificates."""
    pass


@cert_group.command('get')
@argument('hostname')
@pass_context
def get_cert_command(ctx, hostname):
    """Get certificate."""
    print_response(ctx, get_certificate(ctx, hostname))


@cert_group.command('delete')
@argument('hostname')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def delete_cert_command(ctx, hostname, force):
    """Delete certificate."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    delete_certificate(ctx, hostname)
    print(f'Certificate issued for "{hostname}" deleted.')
