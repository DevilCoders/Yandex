import contextlib
import socket
import threading
from typing import Callable

import urllib3.util.connection


__all__ = ["urllib3_use_only_ipv6_if_available", "start_daemon_thread", "urllib3_force_ipv4"]


def urllib3_use_only_ipv6_if_available():
    """
    This function helps to set AF_INET6 for urllib3 if ipv6 stack is available, otherwise set AF_INET.
    Use as urllib3.util.connection.allowed_gai_family = urllib3_use_only_ipv6_if_available.
    """
    family = socket.AF_INET
    if urllib3.util.connection.HAS_IPV6:
        family = socket.AF_INET6
    return family


def urllib3_use_only_ipv4():
    """
    This function helps to set AF_INET for urllib3 always.
    Use as urllib3.util.connection.allowed_gai_family = urllib3_use_only_ipv4.
    """
    return socket.AF_INET


@contextlib.contextmanager
def urllib3_force_ipv4():
    """
    Helper for force using IPv4 via urllib3. Usage:
        with urllib3_force_ipv4():
            requests.get(...)
    """
    saved_function = urllib3.util.connection.allowed_gai_family
    try:
        urllib3.util.connection.allowed_gai_family = urllib3_use_only_ipv4
        yield
    finally:
        urllib3.util.connection.allowed_gai_family = saved_function


def start_daemon_thread(name: str, target: Callable):
    """
    This functions just starts new daemon thread
    """
    thread = threading.Thread(target=target)
    thread.name = name
    thread.daemon = True
    thread.start()
