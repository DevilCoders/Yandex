import os

from click import ClickException

import grpc
from cloud.mdb.cli.dbaas.internal.config import get_config, get_environment_name
from cloud.mdb.cli.dbaas.internal.utils import open_ssh_tunnel_if_required


def create_grpc_channel(ctx, config_section, iam_token=None):
    config = get_config(ctx).get(config_section, {})

    grpc_endpoint = config.get('grpc_endpoint')
    if not grpc_endpoint:
        raise ClickException(f'Service "{config_section}" is not available in "{get_environment_name(ctx)}".')

    # TODO: rework and add closing of ssh tunnel
    ssh_tunnel = open_ssh_tunnel_if_required(ctx, grpc_endpoint)
    if ssh_tunnel:
        ssh_tunnel.start()
        grpc_endpoint = f'{ssh_tunnel.local_bind_host}:{ssh_tunnel.local_bind_port}'

    ca_path = config.get('ca_path')
    if isinstance(ca_path, str):
        with open(os.path.expanduser(ca_path), 'rb') as f:
            root_certificates = f.read()
    else:
        root_certificates = None

    if iam_token:
        grpc_credentials = grpc.composite_channel_credentials(
            grpc.ssl_channel_credentials(root_certificates=root_certificates),
            grpc.access_token_call_credentials(iam_token),
        )
    else:
        grpc_credentials = grpc.ssl_channel_credentials(root_certificates=root_certificates)

    grpc_options = None
    server_name = config.get('grpc_server_name')
    if server_name:
        grpc_options = (('grpc.ssl_target_name_override', server_name),)

    return grpc.secure_channel(grpc_endpoint, grpc_credentials, grpc_options)
