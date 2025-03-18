#!/skynet/python/bin/python
import os
import sys
import json
import logging

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg

from argparse import ArgumentParser
from core.db import CURDB

from datetime import date

import yt.wrapper as yt

def parse_cmd():
    parser = ArgumentParser(description='Dump group resources (cpu, mem, hdd, ssd)')
    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='Optional. Explain what is being done.')
    parser.add_argument('-p', '--path', type=str, default="//home/data_com/cubes/gencfg/groups_data",
                        help='Path to yt folder')
    parser.add_argument('-c', '--cluster', type=str, default="hahn",
                        help='YT cluster')
    parser.add_argument('-m', '--master', action='store_true', default=False,
                        help='Optional. Add leftover resources on hosts to master group')

    options = parser.parse_args()

    if options.verbose:
        logging.basicConfig(level=logging.INFO)

    return options


ABC_NAME_TO_ID = {x['slug']: x['id'] for x in CURDB.abcgroups.abc_services.values()}
GROUPS_RESOURCES = dict()  # group_name to group_resources


def power_to_cores(power):
    return power / 40


def get_instance_count(group):
    return len(group.get_kinda_busy_instances())


def instances_cpu_cores(instances_list):
    hosts_power = defaultdict(float)
    for instance in instances_list:
        hosts_power[instance.host] += instance.power
    power = [int(x + .5) for x in hosts_power.values()]
    return power_to_cores(sum(power))


def fill_group_resources(group):
    if group.card.name in GROUPS_RESOURCES:
        return

    account_id = ""
    if group.card.dispenser['project_key'] is not None:
        account_id = "abc:service:" + str(ABC_NAME_TO_ID[group.card.dispenser['project_key'].lower()])

    instance_count = get_instance_count(group)
    group_card = { # https://st.yandex-team.ru/GENCFG-4456
        'account_id': account_id,
        'group_name': group.card.name,
        'is_rtc_group': hosts_intersect_with_all_runtime(group),
        'cpu': instances_cpu_cores(group.get_kinda_busy_instances()),
        'memory': int(group.card.reqs.instances.memory_guarantee.value * instance_count),
        'hdd_storage': int(group.card.reqs.instances.disk.value * instance_count),
        'ssd_storage': int(group.card.reqs.instances.ssd.value * instance_count)
    }
    GROUPS_RESOURCES[group.card.name] = group_card


def fill_master_resources(master_group):
    slaves_resources = {
        'cpu': 0,
        'memory': 0,
        'hdd_storage': 0,
        'ssd_storage': 0,
    }
    for slave in master_group.card.slaves:
        logging.info('Filling slave %s', slave.card.name)
        fill_group_resources(slave)
        for k in slaves_resources:
            slaves_resources[k] += GROUPS_RESOURCES[slave.card.name][k]

    fill_group_resources(master_group)

    hosts_resources = {
        'cpu': 0,
        'memory': 0,
        'hdd_storage': 0,
        'ssd_storage': 0,
    }
    for host in master_group.getHosts():
        hosts_resources['cpu'] += int(0.5 + power_to_cores(host.power))

        gb_to_b = 1024 * 1024 * 1024
        hosts_resources['memory'] += int(host.memory * gb_to_b)
        hosts_resources['hdd_storage'] += int(host.disk * gb_to_b)
        hosts_resources['ssd_storage'] += int(host.ssd * gb_to_b)

    this_group_resources = GROUPS_RESOURCES[master_group.card.name]
    for k in slaves_resources:
        this_group_resources[k] = max(this_group_resources[k], hosts_resources[k] - slaves_resources[k])


ALL_RUNTIME_HOSTS = set(CURDB.groups.get_group('ALL_RUNTIME').getHostNames())


def hosts_intersect_with_all_runtime(group):
    return len(set(group.getHostNames()) & ALL_RUNTIME_HOSTS) > 0


def fill_all_groups_resources(options):
    groups = CURDB.groups.get_groups()
    groups_len = len(groups)
    for i, group in enumerate(groups):

        logging.info('Filling %s (%d from %d)', group.card.name, i, groups_len)

        if options.master and group.card.master is None:
            fill_master_resources(group)
        else:
            fill_group_resources(group)


def upload_to_yt(options):
    yt.config["pickling"]["module_filter"] = lambda module: "hashlib" not in getattr(module, "__name__", "") and getattr(module, "__name__", "") != "hmac"
    yt.config.set_proxy(options.cluster)

    result = []
    for group in GROUPS_RESOURCES:
        result.append(GROUPS_RESOURCES[group])

    today = date.today().strftime("%Y-%m-%d")
    yt.write_table(options.path + "/"+ today, result, yt.JsonFormat())


def main():
    options = parse_cmd()
    fill_all_groups_resources(options)
    upload_to_yt(options)
    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
