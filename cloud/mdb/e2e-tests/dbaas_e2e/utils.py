"""
DBaaS e2e tests utils
"""
import socket

from retrying import retry

SCENARIOS = []


def scenario(cls):
    """
    Decorator for adding scenario class
    """
    SCENARIOS.append(cls)
    return cls


def all_scenarios():
    """
    Returns all scenarios to execute
    """
    return SCENARIOS


def wait_tcp_conn(hostname, port, timeout=300):
    """
    Wait for host:port to become available
    """
    retry(wait_random_min=1000, wait_random_max=2000, stop_max_delay=(timeout * 1000))(socket.create_connection)(
        (hostname, port), timeout=1
    )


def geo_name(config, geo):
    """
    Returns mapped geo name
    """
    return config.geo_map.get(geo, geo)
