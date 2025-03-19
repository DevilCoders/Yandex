"icmp check implementation"
import time
import argparse
import select
import struct
import socket
from pingunoque import log

def ping(sock, addr, time_remaining):
    """Send ping to the target host"""
    # AF_INET  Echo request (per RFC792)
    props = argparse.Namespace(**{'type': 8, 'sfmt': "bbHHh", 'idx': 20})
    if sock.family == socket.AF_INET6:
        # AF_INET6 Echo request (per RFC4443)
        props = argparse.Namespace(**{'type': 128, 'sfmt': "BbHHh", 'idx': 0})

    p_id = hash(addr) & 0xFFFF
    dsz = struct.calcsize("d")  # size of double

    # build header with dummy checksum = 0
    packet = struct.pack(props.sfmt, props.type, 0, 0, p_id, 1)
    # add package data
    packet += struct.pack("d", time.time()) + (192 - dsz) * "Q"

    log.trace("Send ICMP package id: %s, len: %s", p_id, len(packet))
    sock.sendto(packet, addr)

    while time_remaining > 0:
        start_time = time.time()
        readable = select.select([sock], [], [], time_remaining)
        time_received = time.time()
        time_spent = time_received - start_time
        log.trace("Spent on receiving %.4f ms", time_spent * 1000)
        if readable[0] == []:  # Timeout
            return 0

        # pylint: disable=no-member
        packet, _ = sock.recvfrom(1024)
        icmp_header = packet[props.idx:props.idx + 8]
        got_packetid = struct.unpack("bbHHh", icmp_header)[3]
        log.trace("package id: send(%s) - recv(%s)", p_id, got_packetid)
        if got_packetid == p_id:
            props.idx += 8
            time_sent = struct.unpack("d", packet[props.idx:props.idx + dsz])[0]
            return (time_received - time_sent) * 1000
        time_remaining -= time_spent
    return 0


def check(host):
    """Check remote host by send icmp ping"""
    for res in host.ips():
        addr_family, socktype, proto, _, sock_addr = res
        sock = None
        try:
            if addr_family == socket.AF_INET6:
                proto = socket.IPPROTO_ICMPV6
            sock = socket.socket(addr_family, socktype, proto)
            ret = ping(sock, sock_addr, host.cfg.icmp_timeout)
            if ret > 0:
                log.trace("Get pong in %0.2f ms", ret)
                return True
        except socket.timeout as exc:
            log.debug("Error connecting: %s", exc)
            log.trace("Ping failed. (%s)", exc)
        except socket.error as exc:
            log.trace("Ping failed. (%s)", exc)
            # if socket.error appear while icmp check
            # assume check as passed, becaus we should
            # detect only timeouts
            if not host.cfg.close_on_socket_error:
                return True
            log.trace("Close on socket errors (not timeouts)")
        finally:
            if sock is not None:
                sock.close()
