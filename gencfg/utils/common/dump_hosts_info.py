#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import json
import gencfg
from core.db import CURDB


def get_groups_by_hosts():
    groups_by_hosts = {}
    for host in CURDB.hosts.get_hosts():
        groups_by_hosts[host.name] = sorted(x.card.name for x in CURDB.groups.get_host_groups(host))

    return groups_by_hosts


def get_hardware_by_hosts():
    hardware_by_hosts = {}
    for host in CURDB.hosts.get_hosts():
        hardware_by_hosts[host.name] = host.save_to_json_object()

    return hardware_by_hosts


def main(options):
    if options[1] == 'groups':
        print(json.dumps(get_groups_by_hosts(), indent=4))
    elif options[1] == 'hardware':
        print(json.dumps(get_hardware_by_hosts(), indent=4))


if __name__ == '__main__':
    main(sys.argv)

