from click import argument, group, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.config import activate_profile, delete_profile, get_profiles, rename_profile


@group('config-profile')
def profile_group():
    """Commands to manage config profiles."""
    pass


@profile_group.command('list')
@pass_context
def list_profiles_command(ctx):
    """List profiles."""
    profiles = sorted(get_profiles(ctx), key=lambda profile: profile['name'])
    print_response(ctx, profiles, default_format='table', fields=['name', 'environment', 'int api', 'active'])


@profile_group.command('activate')
@argument('profile_name')
@pass_context
def activate_profile_command(ctx, profile_name):
    """Set profile as default."""
    activate_profile(ctx, profile_name)


@profile_group.command('rename')
@argument('profile_name')
@argument('new_profile_name')
@pass_context
def rename_profile_command(ctx, profile_name, new_profile_name):
    """Rename profile."""
    rename_profile(ctx, profile_name, new_profile_name)


@profile_group.command('delete')
@argument('profile_name')
@pass_context
def delete_profile_command(ctx, profile_name):
    """Delete profile."""
    delete_profile(ctx, profile_name)
