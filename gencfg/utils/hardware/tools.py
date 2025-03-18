#!/usr/bin/env python
# coding=utf-8

import json
import os
import sys

from collections import defaultdict


def create_dir(dir_name):
    if not os.path.exists(dir_name):
        os.system("mkdir -p %s" % dir_name)

    return dir_name


def dump_json(data, path=None, out=None):
    res = json.dumps(data, ensure_ascii=False, sort_keys=True, indent=4)
    if type(res) == unicode:
        res = res.encode("utf-8")

    if path:
        out = file(path, "wb")

    if out:
        out.write(res)

    return res


def services_snaphot_limits():
    services_to_limits = {}
    for line in file("cleanup_limits.txt"):
        line = line.split()
        if not line:
            continue

        services_to_limits[line[0]] = line[1]

    services_to_groups = _load_service_groups()

    res = {}

    for service, limit in services_to_limits.iteritems():
        groups = services_to_groups.get(service)
        if not groups:
            # print service, groups
            continue

        for group in groups:
            res[group] = limit

    for group, limit in sorted(res.iteritems()):
        print group, limit


def _ext_get(data, path, default=None):
    res = data
    for el in path:
        if not isinstance(res, dict):
            break

        res = res.get(el, default)

    return res


def _load_service_groups(path="/Users/sereglond/source/nanny-dump"):
    if os.path.isdir(path):
        res = {}
        for el in os.listdir(path):
            res.update(_load_service_groups(os.path.join(path, el)))
        return res

    if not path.endswith(".json"):
        return {}

    data = json.load(file(path))
    groups = []

    service = data["_id"]

    for group in _ext_get(data, ['runtime_attrs', 'content', 'instances', 'extended_gencfg_groups', 'groups'], []):
        groups.append(group["name"])

    return {service: groups}


def _load_groups(path, exclude=None, save_to=None):
    res = []
    for line in file(path):
        temp = line.split()
        if not temp:
            continue

        group = temp[0]
        if not exclude or not group in exclude:
            res.append(group)
            if save_to is not None:
                save_to.write(line)

    return res


def intersection_list():
    group_name = "MAN_WEB_TIER1_BACKGROUND_BUILD"
    prev_path = "%s_prev.txt" % group_name

    gencfg_path = "/home/sereglond/source/gencfg"

    all_groups_path = "%s.txt" % group_name

    command = "%s/utils/common/show_machine_types.py -s %s > %s" % (gencfg_path, group_name, all_groups_path)
    print command
    os.system(command)

    prev_groups = _load_groups(prev_path)
    print "prev groups: %s" % (",".join(sorted(prev_groups)))

    new_groups = _load_groups(all_groups_path, exclude=prev_groups, save_to=file("%s_new.txt" % group_name, "wb"))
    print "new groups: %s" % (",".join(sorted(new_groups)))

    owners_path = "%s_owners.txt" % group_name
    if new_groups:
        command = "%s/utils/common/show_groups.py -i name,owners -g %s > %s" % (
            gencfg_path, ",".join(new_groups), owners_path)
        print command
        os.system(command)

        owners = set()
        for line in file(owners_path):
            line = line.split()
            if not line or len(line) < 2:
                continue

            owners.update(line[1].split(","))

        print ",".join(["%s@yandex-team.ru" % el for el in sorted(owners)])
    else:
        print "no new owners"


def qloud_quota_distribution():
    example = {
        "quota": {
            "cpu": 1517,
            "memory": 4190
        },
        "dc": {
            "SAS": 30,
            "MAN": 20,
            "MYT": 25,
            "IVA": 25
        }
    }

    json.dump(example, sys.stdout, indent=2)
    sys.stdout.write("\n\n")

    data = json.load(sys.stdin)

    quota = data["quota"]
    dc_list = data["dc"]
    dc_sum = sum(dc_list.values())

    for dc_name, mult in dc_list.iteritems():
        mult = float(mult) / dc_sum
        print dc_name
        for key, val in quota.iteritems():
            print key, int(val * mult)

        print


def groups_owners_stat():
    owners_map = {}

    # ./utils/common/show_groups.py -i name,resolved_owners > ALL_GROUPS_OWNERS.txt
    for line in file("ALL_GROUPS_OWNERS.txt"):
        line = line.split()
        if len(line) < 2:
            continue
        group, owners = line
        owners_map[group] = owners.split()

    for line in sys.stdin:
        group = line.split()[0]

        owners = owners_map.get(group, [])
        owners = ",".join(sorted(owners))

        sys.stdout.write("%s: %s\n" % (line.split(":")[0], owners))


def switches_filter(max_hosts_per_switch):
    switch_hosts = defaultdict(int)

    for line in sys.stdin:
        sline = line.split()
        switch = sline[7]

        switch_hosts[switch] += 1
        if switch_hosts[switch] > max_hosts_per_switch:
            continue

        print line.strip()


def main():
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("--qloud-quota-distribution", action="store_true")
    parser.add_option("--owners", action="store_true")
    parser.add_option("--switches-filter", type=int)
    parser.add_option("--test", action="store_true")

    (options, args) = parser.parse_args()

    if options.qloud_quota_distribution:
        qloud_quota_distribution()
    if options.owners:
        groups_owners_stat()
    if options.switches_filter:
        switches_filter(options.switches_filter)
    if options.test:
        intersection_list()


if __name__ == "__main__":
    main()
