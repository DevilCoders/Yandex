#!/usr/bin/env python
# coding=utf-8

import copy
import json
import os
import pickle
import subprocess
import sys

import resources_counter
import servers_order

from collections import defaultdict


def calculate_yp_quota_for_hosts(data, mode, remove_quota_gpu_models=False):
    command = './calculate_yp_quota_for_hosts/calculate_yp_quota_for_hosts --dump --json'
    command += ' --discount-mode {}'.format(mode) if mode else ''
    process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    stdout, stderr = process.communicate(data)

    if process.returncode:
        print(stderr)
        raise OSError.ChildProcessError('calculate_yp_quota_for_hosts ended with code {}'.format(process.returncode))

    out_data = json.loads(stdout)

    out_data['quota'].pop('total', None)
    if remove_quota_gpu_models:
        for k, v in out_data['quota'].items():
            v.pop('gpu_models', None)

    return out_data


def evaluate_groups(cloud, segment, groups):
    assert groups

    command = "./dump_hostsdata.sh -s {}".format(",".join(sorted(set(groups))))
    print command

    data = subprocess.check_output(command, shell=True)

    quota_for_hosts = calculate_yp_quota_for_hosts(data, mode='infra_only', remove_quota_gpu_models=True)
    resources = quota_for_hosts['quota']

    resources_names_map = {
        "memory": "mem"
    }

    res = resources_counter.ResourcesCounter()
    for dc, dc_data in resources.iteritems():
        temp = resources_counter.empty_resources()
        temp["cloud"] = cloud
        temp["segment"] = segment
        temp["dc"] = dc

        for key, val in dc_data.iteritems():
            temp[resources_names_map.get(key, key)] = val

        res.add_resource(temp)

    return res


def evaluate_dynamic_free_resources(group):
    command = "./optimizers/dynamic/main.py -a get_stats -m %s" % group
    print command

    data = subprocess.check_output(command, shell=True)

    """
Location MAN:
    Free: None power=1400969 memory=309844.61922 disk=27696718.34 ssd=2074212.54172 net=2206061.4875 exclusive=165174
    Total: None power=2522700 memory=449760.680001 disk=28378819 ssd=2368885 net=2223750.0 exclusive=177900
    """

    res = resources_counter.ResourcesCounter()
    dc = None

    for line in data.split("\n"):
        line = line.split()
        if not line:
            continue

        if line[0] == "Location":
            dc = line[1].strip(":").split("_")[-1].lower()
            assert dc in resources_counter.DC_LIST
        elif line[0] == "Free:":
            assert dc
            power = [el.split("=")[-1] for el in line if el.startswith("power=")]
            assert len(power) == 1
            power = int(power[0])

            temp = resources_counter.empty_resources()
            temp["cloud"] = "gencfg"
            temp["dc"] = dc
            temp["segment"] = group
            temp["cpu"] = power * 30 / 32 / 40

            res.add_resource(temp)

            dc = None

    return res


def evaluate_gencfg_reserves(config, reload=False):
    res_pickle_path = os.path.join(resources_counter.create_resources_dumps_dir(), "gencfg_reserve.pickle")
    if not reload and os.path.exists(res_pickle_path):
        with file(res_pickle_path) as f:
            return pickle.load(f)

    res = evaluate_groups(cloud="gencfg", segment="reserve", groups=config["gencfg"]["reserve_groups"])

    for group in config["gencfg"]["dynamic_groups"]:
        res += evaluate_dynamic_free_resources(group)

    with file(res_pickle_path, "wb") as f:
        pickle.dump(res, f)

    return res


def load_groups_abc():
    command = "./utils/common/show_groups.py -i name,dispenser.project_key -f 'lambda x: not x.card.properties.background_group'"
    print command

    data = subprocess.check_output(command, shell=True)

    res = {}
    for line in data.split("\n"):
        if not line:
            continue

        line = line.split("\t")

        name, dispenser = line

        if dispenser != "None":
            res[name] = dispenser

    return res


def load_groups_resources():
    command = "./utils/common/show_power.py -d -t json -g ALL"
    print command

    data = subprocess.check_output(command, shell=True)
    data = json.loads(data)

    """
    {
         "ALL_UNSORTED": {
              "by_dc": {
                   "iva": {
                        "hosts_count": 713,
                        "instances_memory_avg": 0.0,
                        "instances_count": 713,
                        "instances_memory": 0.0,
                        "instances_cpu_avg": 980,
                        "instances_cpu": 699150
                   },
    """

    res = {}
    for group, group_data in data.iteritems():
        temp = resources_counter.ResourcesCounter()

        for dc, dc_data in group_data.get("by_dc", {}).iteritems():
            resource = resources_counter.empty_resources()
            resource["cloud"] = "gencfg"
            resource["dc"] = dc

            resource["cpu"] = dc_data["instances_cpu"] / 40
            resource["mem"] = dc_data["instances_memory"]

            # TODO add hdd/ssd/gpu

            temp.add_resource(resource)

        res[group] = temp

    return res


def get_rtc_groups_list():
    fqnd_path = os.path.join(resources_counter.create_resources_dumps_dir(), "ALL_RUNTIME_fqnd.txt")
    command = "./utils/common/dump_hostsdata.py -i name -g ALL_RUNTIME > %s" % fqnd_path
    print command
    subprocess.check_call(command, shell=True)

    command = "./utils/common/show_machine_types.py --no-background -s \"#%s\"" % fqnd_path
    print command
    data = subprocess.check_output(command, shell=True)

    res = []
    for line in data.split("\n"):
        line = line.split()
        if line:
            res.append(line[0])

    return set(res)


def load_rtc_groups_data():
    command = "./utils/common/dump_group_resources.py"
    print command

    data = subprocess.check_output(command, shell=True)
    data = json.loads(data)

    services_quota = defaultdict(resources_counter.ResourcesCounter)
    groups_stat = {}

    for group_data in data:
        group = group_data["group_name"]
        abc_service = group_data.get("abs_service")
        if not abc_service:  # group_data can contain None values
            abc_service = "abc_unknown.gencfg"

        resources = resources_counter.ResourcesCounter()

        for dc, dc_resources in group_data["resources_per_dc"].iteritems():
            temp = resources_counter.empty_resources()
            temp["cloud"] = "gencfg"
            temp["dc"] = dc
            for key, val in dc_resources.iteritems():
                mult = resources_counter.RESOURCES_RESERVE_TO_SKU.get(key)
                if mult:
                    val = float(val) / mult
                temp[key] = val
            resources.add_resource(temp)

        services_quota[abc_service] += resources

        key = {
            "abc_service": abc_service,
            "group": group,
        }
        key = frozenset(key.iteritems())

        assert key not in groups_stat
        groups_stat[key] = resources

    return services_quota, groups_stat
