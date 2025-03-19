#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import socket

from utils.ctl_logging import log


def read_pid(pidfile):
    """Read pid from pidfile"""
    if not os.path.exists(pidfile):
        return None
    try:
        with open(pidfile) as fobj:
            pid_str = fobj.read().strip()
    except IOError:
        raise RuntimeError("failed to read {}".format(pidfile))
    try:
        pid = int(pid_str)
    except ValueError:
        raise RuntimeError("failed to convert {} from {} to int".format(pid_str, pidfile))
    return pid


def check_pid(pid):
    """Check the existence of a unix pid."""
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    else:
        return True


def get_pids(pidfile):
    pid = read_pid(pidfile)
    return [pid] if pid and check_pid(pid) else []


def detect_restart(restart_count, pids, new_pids):
    if pids is None:
        return restart_count
    if sorted(pids) != sorted(new_pids):
        restart_count -= 1
        log.debug("detected Redis pids change ({} restarts left): was {}, now {}".format(restart_count, pids, new_pids))
    elif new_pids == []:
        restart_count -= 1
        log.debug("detected no Redis pids ({} restarts left): was {}, now {}".format(restart_count, pids, new_pids))
    else:
        log.debug("current Redis pids: {}".format(new_pids))
    return restart_count


def get_ip(fqdn=None):
    """
    Translate the fqdn to IP address.
    Tries IPv4 first, then IPv6.
    If fqdn is None, returns the IP of the current host.
    """
    if fqdn is None:
        fqdn = socket.gethostname()
    try:
        return socket.gethostbyname(fqdn)
    except Exception:
        log.debug('Unable to translate "{}" to IPv4 address'.format(fqdn))
    try:
        return socket.getaddrinfo(fqdn, None, socket.AF_INET6)[0][4][0]
    except Exception:
        log.debug('Unable to translate "{}" to IPv6 address'.format(fqdn))
    return fqdn
