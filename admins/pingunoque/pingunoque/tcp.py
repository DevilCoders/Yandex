"""pingunoque tcp check"""
import socket
from pingunoque import log

def check(host):
    """Check remote host by open tcp connection"""

    for res in host.ips():
        addr_family, socktype, proto, _, sock_addr = res
        sock = None
        try:
            sock = socket.socket(addr_family, socktype, proto)
            sock.settimeout(host.cfg.tcp_timeout)
            sock.connect(sock_addr)
            log.trace("Success connect tcp socket")
            return True
        except socket.timeout as exc:
            log.debug("TCP check: error connecting: %s", exc)
        except socket.error as exc:
            log.trace("TCP Check failed %s", exc)
            # if socket.error appear while tcp check
            # assume check as passed, becaus we should
            # detect only timeouts
            if not host.cfg.close_on_socket_error:
                return True
            log.trace("Close on socket errors (not timeouts)")
        finally:
            if sock is not None:
                sock.close()
