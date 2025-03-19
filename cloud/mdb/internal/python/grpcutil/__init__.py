"""
GRPC utilities
"""

import os
from typing import NamedTuple

import grpc
from dbaas_common.tracing import grpc_channel_tracing_interceptor

from .exceptions import NotFoundError  # noqa: F401
from .service import WrappedGRPCService  # noqa: F401

DEFAULT_KEEPALIVE_OPTIONS = (
    ('grpc.keepalive_time_ms', 11000),
    ('grpc.keepalive_timeout_ms', 1000),
    ('grpc.keepalive_permit_without_calls', True),
    ('grpc.http2.max_pings_without_data', 0),
    ('grpc.http2.min_ping_interval_without_data_ms', 5000),
)


class Config(NamedTuple):
    url: str
    cert_file: str
    server_name: str = ''
    insecure: bool = False


def _get_ssl_creds(config: Config):
    cert_file = config.cert_file
    if not cert_file or not os.path.exists(cert_file):
        return grpc.ssl_channel_credentials()
    with open(cert_file, 'rb') as file_handler:
        certs = file_handler.read()
    return grpc.ssl_channel_credentials(root_certificates=certs)


def new_grpc_channel(config: Config) -> grpc.Channel:
    """
    Initialize gRPC channel using Config
    """
    if config.insecure:
        return grpc_channel_tracing_interceptor(grpc.insecure_channel(config.url))
    creds = _get_ssl_creds(config)
    options = DEFAULT_KEEPALIVE_OPTIONS
    if config.server_name:
        options += (('grpc.ssl_target_name_override', config.server_name),)
    return grpc_channel_tracing_interceptor(grpc.secure_channel(config.url, creds, options))
