"""
Utils for helpers
"""

import grpc
import socket

from jinja2 import Template
from retrying import retry

from typing import Optional, List


def ssh_options(ctx, additionals: Optional[List[str]] = None) -> List[str]:
    """
    Return a list of ssh options with additionals
    """
    options = ['-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null']
    private_key = ctx.state.get('ssh_private_key')
    if private_key:
        options += ['-i', private_key]
    if additionals:
        options += additionals
    return options


def rsync_options(ctx, additionals: Optional[List[str]] = None) -> List[str]:
    """
    Return a list of rsync options with additionals
    """
    options = ['--recursive', '--archive',
               '--no-perms', '--no-owner', '--no-group',
               '--rsync-path', 'sudo rsync']
    options += ['-e', 'ssh ' + ' '.join(ssh_options(ctx))]
    if additionals:
        options += additionals
    return options


def extract_code(rpc_error):
    """
    Extract grpc.StatusCode from grpc.RpcError
    """
    code = None
    # Workaround for too general error class in gRPC
    if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
        code = rpc_error.code()  # pylint: disable=E1101
    return code


def retry_if_grpc_error(rpc_error):
    """
    Defines whether to retry operation on exception or give up
    """
    code = extract_code(rpc_error)
    if code in (grpc.StatusCode.UNAVAILABLE, grpc.StatusCode.INTERNAL, grpc.StatusCode.DEADLINE_EXCEEDED):
        return True
    return False


def wait_tcp_conn(hostname, port, timeout=300):
    """
    Wait for host:port to become available
    """
    retry(wait_fixed=1000,
          stop_max_delay=timeout * 1000)(socket.create_connection)((hostname, port), timeout=3.0)


def render_template(template, ctx):
    """
    render jinja2 template with ctx
    """
    return Template(template).render(**ctx.state['render'])
