import os
import sshtunnel

from click import argument, Choice, command, pass_context

from cloud.mdb.cli.dbaas.internal.db import get_dsn


@command('psql')
@argument('database', type=Choice(['metadb', 'saltdb', 'katandb', 'dbm', 'cmsdb', 'deploydb', 'vpcdb', 'billingdb']))
@pass_context
def psql_command(ctx, database):
    """Connection to the specified database using psql."""
    tunnel_config, dsn = get_dsn(ctx, database)
    if tunnel_config:
        with sshtunnel.open_tunnel(**tunnel_config):
            os.system(f'psql "{dsn}"')
    else:
        os.system(f'psql "{dsn}"')
