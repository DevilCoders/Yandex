"""
Helpers for forwarding tcp tunnel through ssh
"""

import sh
import time
import socket
import logging

from helpers import compute

LOG = logging.getLogger('tunnel')


class TunnelException(Exception):
    """
    Common exception for tunnel helper
    """


class TunnelNotFound(TunnelException):
    """
    Tunnel not created or died.
    """


def _get_free_tcp_port():
    """
    Returns free tcp port for tunnel forwarding
    """
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(("", 0))
        s.listen(1)
        port = s.getsockname()[1]
        s.close()
        return port
    except Exception:
        return None


def _wait_tcp_connection(host, port, timeout=60.0):
    """
    Wait until {host}:{port} became available
    """
    start_at = time.time()
    exc = None
    while time.time() - start_at <= timeout:
        try:
            socket.create_connection((host, port), timeout=5.0)
            return
        except ConnectionError as e:
            LOG.debug(f'Failed connect to {host}:{port}')
            exc = e
            time.sleep(1)
    raise exc


def tunnel_open(ctx, name, port):
    """
    Method creates a child process for forwarding tcp socket to {addr}:{port}.
    Process with run in ctx and create {localhost}:{random_port} socket for forwarding.
    """
    try:
        tunnel = tunnel_get(ctx, port)
        return tunnel
    except TunnelNotFound:
        # It's ok, just create new one.
        pass

    local_port = _get_free_tcp_port()
    if local_port is None:
        raise TunnelException('Failed to allocate local port')
    fqdn = compute.instance_fqdn(ctx, name)
    addr = compute.get_public_ip_address(ctx, name)
    LOG.debug(f'Starting tunnel localhost:{local_port} <=> {fqdn}:{port}')
    process = sh.ssh(
        '-L',
        f'0.0.0.0:{local_port}:{fqdn}:{port}',
        '-t',
        '-o',
        'StrictHostKeyChecking=no',
        '-o',
        'UserKnownHostsFile=/dev/null',
        f'root@{addr}',
        _bg=True,
        _tty_in=True,
        _no_out=True,
        _no_err=True)
    try:
        _wait_tcp_connection('0.0.0.0', local_port)
    except ConnectionError as exc:
        raise TunnelException('Failed to connect to local_port', exc)
    ctx.state['tunnels'][str(port)] = (local_port, process)
    return f'localhost:{local_port}'


def tunnel_get(ctx, port):
    """
    Return local address of forwarded port
    """
    tunnel = ctx.state['tunnels'].get(str(port))
    if tunnel:
        lport = tunnel[0]
        (lport, process) = tunnel
        if process.is_alive():
            return f'localhost:{lport}'
    raise TunnelNotFound(f'Tunnel for port {port} not found')


def tunnel_close(ctx, name, port):
    """
    Terminate tunnel to {name}:{port}
    """
    tunnels = ctx.state['tunnels']
    tunnel = tunnels.get(str(port))
    if not tunnel:
        LOG.warn(f'Unable to terminate tunnel {name}:{port} tunnel not found')
    if tunnel:
        (lport, process) = tunnel
        if process.is_alive():
            process.terminate()
            return
        LOG.warn(f'Tunnel {name}:{port} already terminated')


def tunnels_clean(ctx):
    """
    Terminate all tunnels
    """
    tunnels = ctx.state['tunnels']
    if not tunnels:
        return
    LOG.info('Terminating all tunnels')
    for (_, process) in tunnels.values():
        if process.is_alive():
            process.terminate()
    ctx.state['tunnels'] = {}
