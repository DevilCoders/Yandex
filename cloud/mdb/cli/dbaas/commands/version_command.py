from click import command

from cloud.mdb.cli.dbaas.internal.version import get_version


@command('version')
def version_command():
    """Get tool version."""
    print(get_version())
