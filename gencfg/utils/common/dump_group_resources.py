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


ABC_NAME_TO_ID = {x['slug']: x['id'] for x in CURDB.abcgroups.abc_services.values()}


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


class GroupsEvaluator():
    def __init__(self):
        self.groups_resource = {}

        self.init_gencfg_hosts_set()

        self.force_master_evaluation = [
            "SAS_SAAS_CLOUD",
            "MAN_SAAS_CLOUD",
            "VLA_SAAS_CLOUD",
        ]

    def init_gencfg_hosts_set(self):
        res = []

        must_have_tags = [
            "rtc",
            "runtime"
        ]
        bad_tags = [
            "gpu",
            "yt",
            "yp",
            "qloud"
        ]

        for host in CURDB.hosts.get_hosts():
            tags = set(host.walle_tags)
            if tags.issuperset(must_have_tags) and not tags.intersection(bad_tags):
                res.append(host.invnum)

        res.extend([host.invnum for host in CURDB.groups.get_group('VLA_YT_RTC').getHosts()])

        self.good_gencfg_hosts_set = set(res)

    def count_resources(self, group, instances):
        instances_count = len(instances)
        return {
            'cpu': instances_cpu_cores(instances),
            'mem': group.card.reqs.instances.memory_guarantee.gigabytes() * instances_count,
            'hdd': group.card.reqs.instances.disk.gigabytes() * instances_count,
            'ssd': group.card.reqs.instances.ssd.gigabytes() * instances_count,
        }

    def fill_group_resources(self, group):
        if group.card.name in self.groups_resource:
            return

        account_id = ""
        if group.card.dispenser['project_key'] is not None:
            account_id = "abc:service:" + str(ABC_NAME_TO_ID[group.card.dispenser['project_key'].lower()])

        instances = group.get_kinda_busy_instances()
        instances = [f for f in instances if f.host.invnum in self.good_gencfg_hosts_set]
        dc_instances = defaultdict(list)
        for instance in instances:
            dc_instances[instance.host.dc].append(instance)

        group_card = {
            'group_name': group.card.name,
            'abs_service': group.card.dispenser['project_key'],
            'account_id': account_id,
            'master': group.card.master.card.name if group.card.master else None,
            'resources': self.count_resources(group, instances),
            'resources_per_dc':
                {dc: self.count_resources(group, per_dc_instances) for dc, per_dc_instances in dc_instances.items()}
        }
        self.groups_resource[group.card.name] = group_card

    def is_good_group(self, group):
        if group.card.properties.background_group:
            return False
        if group.card.reqs.instances.cpu_policy == "idle":
            return False

        return True

    def fill_master_resources(self, master_group):
        def add_host_resources(resources, host_object):
            resources['cpu'] += power_to_cores(host_object.power)
            resources['mem'] += host_object.memory
            resources['hdd'] += host_object.disk
            resources['ssd'] += host_object.ssd

        assert master_group.card.master is None

        hosts_resources = defaultdict(int)
        hosts_resources_per_dc = defaultdict(lambda: defaultdict(int))
        has_rtc_hosts = False
        for host in master_group.getHosts():
            if host.invnum not in self.good_gencfg_hosts_set:
                continue

            has_rtc_hosts = True
            add_host_resources(hosts_resources, host)
            add_host_resources(hosts_resources_per_dc[host.dc], host)

        if not has_rtc_hosts:
            return

        self.fill_group_resources(master_group)
        self.groups_resource[master_group.card.name]['hosts_resource'] = hosts_resources
        self.groups_resource[master_group.card.name]['hosts_resource_per_dc'] = hosts_resources_per_dc

        slaves_resources = defaultdict(int)
        slaves_resources_per_dc = defaultdict(lambda: defaultdict(int))

        if master_group.card.name not in self.force_master_evaluation:
            for slave in master_group.card.slaves:
                if not self.is_good_group(slave):
                    continue

                self.fill_group_resources(slave)
                for key, val in self.groups_resource[slave.card.name]['resources'].iteritems():
                    slaves_resources[key] += val
                for dc, dc_resources in self.groups_resource[slave.card.name]['resources_per_dc'].iteritems():
                    for key, val in dc_resources.iteritems():
                        slaves_resources_per_dc[dc][key] += val

        this_group_resources = self.groups_resource[master_group.card.name]['resources']
        this_group_resources_per_dc = self.groups_resource[master_group.card.name]['resources_per_dc']
        for res_name in this_group_resources:
            if slaves_resources[res_name] + this_group_resources[res_name] - hosts_resources[res_name] > 1:
                sys.stderr.write("group %s greater than hosts for %s: %s + %s / %s\n" % (
                    res_name,
                    master_group.card.name,
                    slaves_resources[res_name],
                    this_group_resources[res_name],
                    hosts_resources[res_name]
                ))

            this_group_resources[res_name] = max(
                this_group_resources[res_name],
                hosts_resources[res_name] - slaves_resources[res_name]
            )

            for dc, dc_resources in this_group_resources_per_dc.items():
                this_group_resources_per_dc[dc][res_name] = max(
                    this_group_resources_per_dc[dc][res_name],
                    hosts_resources_per_dc[dc][res_name] - slaves_resources_per_dc[dc][res_name]
                )

    def fill_all_groups_resources(self):
        for group in CURDB.groups.get_groups():
            if not self.is_good_group(group):
                continue

            if group.card.master is None:
                self.fill_master_resources(group)

    def json_dump(self):
        return json.dumps(self.groups_resource.values(), indent=4)

    def debug_report(self):
        total_groups_cpu = 0
        total_hosts_cpu = 0
        for data in self.groups_resource.itervalues():
            total_groups_cpu += data["resources"]["cpu"]
            total_hosts_cpu += data.get("hosts_resource", {}).get("cpu", 0)

        sys.stdout.write("total_groups_cpu: %s\n" % total_groups_cpu)
        sys.stdout.write("total_hosts_cpu: %s\n" % total_hosts_cpu)


def parse_cmd():
    parser = ArgumentParser(description='Dump group resources (cpu, mem, hdd, ssd)')
    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='Optional. Explain what is being done.')
    parser.add_argument('-d', '--debug', action='store_true', default=False,
                        help='Optional. Debug mode')

    options = parser.parse_args()

    if options.verbose:
        logging.basicConfig(level=logging.INFO)

    return options


def main():
    options = parse_cmd()

    evaluator = GroupsEvaluator()

    evaluator.fill_all_groups_resources()

    if options.debug:
        evaluator.debug_report()
    else:
        sys.stdout.write(evaluator.json_dump())

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
