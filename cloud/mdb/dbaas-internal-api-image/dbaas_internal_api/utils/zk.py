"""
Common utils for working with zookeeper
"""


def choice_zk_hosts(all_hosts, usage):
    """
    Choice less loaded zk hosts. Param all_hosts is 'zk' from config,
    Param usage is list of (zk_hosts, count). Returns (zk_id, list of zk_hosts)
    """
    host_to_id = {}
    all_hosts_usage = {}

    for zk_id, hosts in all_hosts.items():
        hosts_str = ','.join(hosts)
        all_hosts_usage[hosts_str] = 0
        host_to_id[hosts_str] = zk_id

    for zk_hosts, count in usage:
        if zk_hosts in all_hosts_usage:
            all_hosts_usage[zk_hosts] = count
    min_count = min(all_hosts_usage.values())
    for hosts, count in all_hosts_usage.items():
        if count == min_count:
            return host_to_id[hosts], hosts.split(',')
