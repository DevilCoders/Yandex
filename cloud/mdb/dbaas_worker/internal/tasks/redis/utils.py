# -*- coding: utf-8 -*-
"""
Utilities specific for Redis clusters.
"""


def separate_slaves_and_masters(hosts: dict, api_masters: list) -> list[dict]:
    """
    Separating hosts to list of dicts according to received from api masters list (if any)
    :param hosts:
    :param api_masters:
    :return: 1st dict = all slaves, other dicts = all masters each
    """
    slaves = {}
    master_dicts = []
    for host, data in hosts.items():
        if api_masters and host in api_masters:
            master_dicts.append({host: data})
        else:
            slaves[host] = data
    return [slaves] + master_dicts


def get_one_by_one_order(hosts, masters) -> list[str]:
    """
    Get order for one-by-one host processing
    :param hosts:
    :param masters:
    :return: list of fqdns starting with slaves
    """
    order = []
    separated = separate_slaves_and_masters(hosts, masters)
    for host_dict in separated:
        for host in host_dict:
            order.append(host)
    return order


def up_timeouts(hosts):
    for data in hosts.values():
        data['post_restart_timeout'] = 60 * 60
