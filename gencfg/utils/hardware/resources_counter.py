#!/usr/bin/env python
# coding=utf-8

import copy
import json
import math
import os
import re
import sys

from collections import OrderedDict, defaultdict

RESOURCES_LOCATIONS = OrderedDict()
RESOURCES_LOCATIONS["cloud"] = ["gencfg", "yp", "qloud", "qloud-ext"]
RESOURCES_LOCATIONS["dc"] = ["sas", "man", "vla", "iva", "myt"]
RESOURCES_LOCATIONS["segment"] = None

LOCATIONS_KEYS = RESOURCES_LOCATIONS.keys()
DC_LIST = RESOURCES_LOCATIONS["dc"]
RESOURCES_NAMES = ["cpu", "mem", "hdd", "ssd", "gpu", "hdd_io", "ssd_io"]

RESOURCES_TO_HOSTS = {
    "yp cpu": 256 - 16,
    "yp mem": 1024 - 64,
    "yp ssd": 6.4 * (1000 ** 4) / (1024 ** 4),
    "yp hdd": 14 * 2 * (1000 ** 4) / (1024 ** 4),
    "yp hdd_io": 150 * 2,
    "yp ssd_io": 3000,
}

RESOURCES_RESERVE_TO_SKU = {
    "hdd": 1024,  # TB to GB
    "ssd": 1024  # TB to GB
}


def empty_resources():
    quota = OrderedDict()
    for key, values in RESOURCES_LOCATIONS.iteritems():
        quota[key] = None

    for resource in RESOURCES_NAMES:
        quota[resource] = 0

    return quota


def ordered_resources_names(data):
    assert isinstance(data, dict)

    res = [name for name in RESOURCES_NAMES if data.get(name)]
    for key, val in sorted(data.iteritems()):
        if key not in res and val:
            res.append(key)

    return res


def _sum_resources(a, b):
    if a is None:
        return b
    if b is None:
        return a

    res = a
    for key, val in b.iteritems():
        if key in LOCATIONS_KEYS:
            a_loc = a.get(key)
            if not a_loc:
                a[key] = val
            else:
                assert val == a_loc
            continue

        a[key] = a.get(key, 0) + val

    return res


def _locations_key_as_tuple(data):
    return tuple([data[key] for key in LOCATIONS_KEYS])


def _format_quota(data, delimiter=" ", abc_format=False):
    res = []
    # abc format loc:MAN-seg:default-cpu:100-mem:320-hdd:1.1
    if abc_format:
        delimiter = "-"
    for key, val in data.iteritems():
        if abc_format:
            if key not in RESOURCES_NAMES and key != "dc":
                continue
            if key == "dc":
                key = "loc"
                val = val.upper()

        if val is None:
            continue

        res.append("%s:%s" % (key, val))

    return delimiter.join(res)


def resource_to_hosts(resource_name, resource):
    for key, val in RESOURCES_TO_HOSTS.iteritems():
        cloud, key = key.split()
        if key != resource_name:
            continue

        return float(resource) / val

    return 0


def resources_to_hosts(resources):
    clouds_hosts = defaultdict(list)
    for key, val in RESOURCES_TO_HOSTS.iteritems():
        cloud, key = key.split()
        clouds_hosts[cloud].append((float(resources.get(key, 0)) / val) if val else 0)

    total_hosts = 0
    for cloud, hosts_data in clouds_hosts.iteritems():
        if clouds_hosts:
            total_hosts += max(hosts_data)

    return total_hosts


def cpu_to_full_hosts(cloud, dc, cpu):
    resource = empty_resources()

    resource["cloud"] = cloud
    resource["dc"] = dc

    resource["cpu"] = cpu

    cpu_per_host = RESOURCES_TO_HOSTS["%s cpu" % cloud]
    hosts = float(cpu) / cpu_per_host

    for resource_name, mult in RESOURCES_TO_HOSTS.iteritems():
        res_cloud, res_name = resource_name.split()
        if res_cloud != cloud or res_name == "cpu":
            continue

        resource[res_name] = int(hosts * mult)

    return resource


class ResourcesCounter:
    def __init__(self, resources=None):
        self.resources = resources if resources else {}

    def add_resource(self, resource):
        resource = copy.deepcopy(resource)
        if resource["dc"]:
            resource["dc"] = resource["dc"].lower()

        key = _locations_key_as_tuple(resource)
        self.resources[key] = _sum_resources(self.resources.get(key), resource)

        return self

    def add_resource_from_json(self, resource):
        temp = empty_resources()

        temp.update(resource)

        self.add_resource(temp)

        return self

    def add_resources_from_abc_line(self, line):
        # loc:MAN-seg:default-cpu:1200-mem:9880-hdd:156-ssd:72-ip4:0
        resources_location_map = {
            "loc": "dc",
            "seg": "segment"
        }

        res = empty_resources()
        res["cloud"] = "yp"

        try:
            for piece in line.replace(",", ".").split("-"):
                key, _, val = piece.partition(":")
                if key in resources_location_map.keys():
                    res[resources_location_map[key]] = val.lower()
                else:
                    res[key] = float(val)
        except:
            sys.stderr.write("Failed to parse %s quota line\n" % line)
            raise

        self.add_resource(res)

    def add_resources_from_yp_json(self, dc, segment, data, cloud, json_type=None):
        res = empty_resources()
        res["cloud"] = cloud
        res["dc"] = dc
        res["segment"] = segment
        if json_type == "yp_util":
            """
            {
                "host": "",
                "segment": "",
                "used_vcpu": 4697762,
                "total_vcpu": 6779982,
                "used_memory": 30606941487104,
                "total_memory": 71370949514895,
                "used_hdd": 0,
                "total_hdd": 0,
                "used_ssd": 175013168676864,
                "total_ssd": 700977694908416,
                "used_hdd_bw": 0,
                "total_hdd_bw": 0,
                "used_ssd_bw": 22914531328,
                "total_ssd_bw": 179488233087,
                "used_hdd_lvm": 0,
                "total_hdd_lvm": 0,
                "used_ssd_lvm": 0,
                "total_ssd_lvm": 0,
                "used_hdd_lvm_bw": 0,
                "total_hdd_lvm_bw": 0,
                "used_ssd_lvm_bw": 0,
                "total_ssd_lvm_bw": 0,
                "used_net_bw": 2261778432,
                "total_net_bw": 202500000000,
                "used_gpu_geforce_1080ti": 8,
                "total_gpu_geforce_1080ti": 8,
                "used_gpu_tesla_a100_80g": 0,
                "total_gpu_tesla_a100_80g": 8,
                "used_gpu_tesla_v100": 800,
                "total_gpu_tesla_v100": 1064,
                "ipv4": "",
                "net_10G": "",
                "alerts": ""
            }
            """
            res["cpu"] = float(data.get("total_vcpu", 0)) / 1000
            res["mem"] = float(data.get("total_memory", 0)) / 2 ** 30
            res["hdd"] = float(data.get("total_hdd", 0) + data.get("total_hdd_lvm", 0)) / 2 ** 40
            res["ssd"] = float(data.get("total_ssd", 0) + data.get("total_ssd_lvm", 0)) / 2 ** 40

            res["hdd_io"] = (float(data.get("total_hdd_bw", 0) + data.get("total_hdd_lvm_bw", 0))) / 2 ** 20
            res["ssd_io"] = (float(data.get("total_ssd_bw", 0) + data.get("total_ssd_lvm_bw", 0))) / 2 ** 20

            for key, val in data.iteritems():
                if key.startswith("total_gpu_") and val:
                    model = key.replace("total_gpu_", "")
                    res[model] = res.get(model, 0) + int(val)
        else:
            """
                {
                    "cpu": {
                        "capacity": 71552769
                    },
                    "disk_per_storage_class": {
                        "hdd": {
                            "bandwidth": 2831155200,
                            "capacity": 18259741595062272
                        },
                        "ssd": {
                            "bandwidth": 1048576000,
                            "capacity": 1926033684987904
                        }
                    },
                    "memory": {
                        "capacity": 414347733230487
                    },
                    "gpu_per_model" = {
                        "gpu_tesla_v100" = {
                            "capacity" = 8;
                        };
                    };
                }
            """
            res["cpu"] = float(data.get("cpu", {}).get("capacity", 0)) / 1000
            res["mem"] = float(data.get("memory", {}).get("capacity", 0)) / 2 ** 30
            # TODO add lvm storages?
            if "disk_per_storage_class" in data:
                res["hdd"] = float(data["disk_per_storage_class"].get("hdd", {}).get("capacity", 0)) / 2 ** 40
                res["ssd"] = float(data["disk_per_storage_class"].get("ssd", {}).get("capacity", 0)) / 2 ** 40

                res["hdd_io"] = float(data["disk_per_storage_class"].get("hdd", {}).get("bandwidth", 0)) / 2 ** 20
                res["ssd_io"] = float(data["disk_per_storage_class"].get("ssd", {}).get("bandwidth", 0)) / 2 ** 20
            if "gpu_per_model" in data:
                for model, model_data in data["gpu_per_model"].iteritems():
                    res[model] = res.get(model, 0) + model_data.get("capacity", 0)

        self.add_resource(res)

    def add_resources_from_qloud(self, cloud, data):
        """
        {
            "mtn": {
                "CPU": {
                    "IVA": 3.0,
                    "MAN": 0.0,
                    "MYT": 2.0,
                    "SAS": 0.0,
                    "VLA": 0.0
                },
                "MEMORY_GB": {
                    "IVA": 12.0,
                    "MAN": 0.0,
                    "MYT": 5.0,
                    "SAS": 0.0,
                    "VLA": 0.0
                }
            }
        }
        """
        resources_map = {
            "CPU": "cpu",
            "MEMORY_GB": "mem"
        }

        for segment, segment_data in data.iteritems():
            dc_resources = defaultdict(dict)
            for resource_name, resource_data in segment_data.iteritems():
                for dc, resource_size in resource_data.iteritems():
                    maped_name = resources_map.get(resource_name)
                    if not maped_name:
                        continue

                    dc_resources[dc][maped_name] = resource_size

            for dc, resources in dc_resources.iteritems():
                res = empty_resources()
                res["cloud"] = cloud
                res["dc"] = dc
                res["segment"] = segment

                res.update(resources)

                self.add_resource(res)

    def total(self, filter=None):
        total_sum = OrderedDict()
        for location_key, data in sorted(self.resources.iteritems()):
            data = copy.deepcopy(data)

            if filter:
                bad = False
                for key, val in filter.iteritems():
                    if data[key] not in val:
                        bad = True
                        break

                if bad:
                    continue

            for key in LOCATIONS_KEYS:
                data[key] = "total"
            total_sum = _sum_resources(total_sum, data)

        for key, val in total_sum.items():
            if key in LOCATIONS_KEYS or val == 0:
                del total_sum[key]

        return total_sum

    def format(self, total=False, hosts=False, abc_format=False):
        res = []
        total_sum = OrderedDict()

        for location_key, data in sorted(self.resources.iteritems()):
            if not sum([el for el in data.values() if isinstance(el, (int, float))]):
                continue

            if hosts:
                data = copy.deepcopy(data)
                data["hosts"] = resources_to_hosts(data)

            res.append(_format_quota(data, abc_format=abc_format))

            if total:
                data = copy.deepcopy(data)
                for key in LOCATIONS_KEYS:
                    data[key] = "total"
                total_sum = _sum_resources(total_sum, data)

        if total:
            total_sum["hosts"] = resources_to_hosts(total_sum)
            res.append(_format_quota(total_sum, abc_format=abc_format))

        return "\n".join(res)

    def format_total(self):
        return _format_quota(self.total())

    def filter(self, settings):
        res = ResourcesCounter()

        for resource in self.resources.itervalues():
            chosen = True
            for key, val in settings.iteritems():
                if val and resource[key] not in val:
                    chosen = False
                    break

            if chosen:
                res.add_resource(resource)

        return res

    def format_chosen(self, key_field, val_field):
        res = defaultdict(int)

        for data in sorted(self.resources.itervalues()):
            res[data[key_field]] += data[val_field]

        res = ["%s %s %s" % (key, val, val_field) for key, val in sorted(res.iteritems())]

        return "\n".join(res)

    def __add__(self, other):
        res = copy.deepcopy(self.resources)

        for key, val in other.resources.iteritems():
            res[key] = _sum_resources(res.get(key), val)
        return ResourcesCounter(res)

    def __sub__(self, other):
        res = copy.deepcopy(self.resources)

        for key, val in other.resources.iteritems():
            inversed = {}
            for resource, resource_value in val.iteritems():
                if isinstance(resource_value, (int, float)):
                    resource_value = -resource_value
                inversed[resource] = resource_value

            res[key] = _sum_resources(res.get(key), inversed)
        return ResourcesCounter(res)

    def multiply_resources(self, names, mult):
        assert isinstance(mult, (int, float))
        assert isinstance(names, (list, tuple))

        res = copy.deepcopy(self.resources)

        for location, resources in res.iteritems():
            for key, val in resources.iteritems():
                if not key in RESOURCES_NAMES:
                    continue

                if not names or key in names:
                    resources[key] = val * mult

        return ResourcesCounter(res)

    def __mul__(self, mult):
        return self.multiply_resources(names=[], mult=mult)

    def __nonzero__(self):
        for resource in self.resources.itervalues():
            for key in RESOURCES_NAMES:
                if resource[key]:
                    return True

        return False

    def cloud_resources(self):
        res = defaultdict(ResourcesCounter)
        for location, resource in self.resources.iteritems():
            res[resource["cloud"]].add_resource(resource)

        return res


def save_csv(data, path, header=None, skip_fields=None, delimeter=';'):
    header = header if header else []
    for row in data:
        for el in row.iterkeys():
            if not el in header and (not skip_fields or el not in skip_fields):
                header.append(el)

    with file(path, "wb") as out:
        out.write(delimeter.join(header) + "\n")

        for row in data:
            temp = []
            for key in header:
                temp.append(str(row.get(key, "")))

            out.write(delimeter.join(temp) + "\n")


def _save_services_quota(data, path, keys_to_join, add_heads=False, abc_api_holder=None):
    assert not set(keys_to_join).difference(LOCATIONS_KEYS)
    assert not add_heads or abc_api_holder

    total_table = []
    for service, total_quota in sorted(data.iteritems()):
        sum_quota = ResourcesCounter()
        for location, resources in total_quota.resources.iteritems():
            temp = copy.deepcopy(resources)
            for key in keys_to_join:
                temp[key] = None
            sum_quota.add_resource(temp)

        for location, resources in sum_quota.resources.iteritems():
            row = OrderedDict()
            if not sum([el for el in resources.values() if isinstance(el, (int, float))]):
                continue

            if add_heads:
                heads = list(reversed(abc_api_holder.get_service_parents(service)))
                for i, head in enumerate(heads[:3]):
                    row["head_%s" % (i + 1)] = abc_api_holder.get_service_name(head).encode("utf-8")

                service_name = abc_api_holder.get_service_name(service)
                row["service"] = service_name.encode("utf-8") if service_name else service
            else:
                row["service"] = service

            for key, val in resources.iteritems():
                if key not in keys_to_join:
                    row[key] = val

            total_table.append(row)

    path = os.path.join(create_resources_dumps_dir(), path)
    save_csv(total_table, path)

    print path


def save_quota_tables(abc_api_holder, data, csv_name, split_tables=True, header=None, skip_fields=None):
    locations = set()
    for resource in data.itervalues():
        locations.update(resource.resources.keys())

    joined_table = []

    for location in locations:
        table = []
        for service, total_quota in sorted(data.iteritems()):
            row = OrderedDict()
            quota = total_quota.resources.get(location)
            if not quota:
                continue

            if not sum([el for el in quota.values() if isinstance(el, (int, float))]):
                continue

            if isinstance(service, frozenset):
                service = dict(service)
                for key, val in sorted(service.iteritems()):
                    row[key] = val
            else:
                row["service"] = service
                row["top service"] = abc_api_holder.get_service_vs_or_top(service)

            row.update(quota)
            table.append(row)

        joined_table.extend(table)

        if not table or not split_tables:
            continue

        path = os.path.join(create_resources_dumps_dir(), "%s_%s.csv" % (csv_name, "_".join(map(str, location))))
        save_csv(table, path, header=header, skip_fields=skip_fields)
        print path

    if not joined_table:
        return

    path = os.path.join(create_resources_dumps_dir(), "%s.csv" % csv_name)
    save_csv(joined_table, path, header=header, skip_fields=skip_fields)
    print path
    print


def save_services_quota_sum(data, abc_api_holder=None):
    # _save_services_quota(data, "sum_services_joined.csv", ["dc", "cloud", "segment"])
    # _save_services_quota(data, "sum_services_joined_dc.csv", ["cloud", "segment"])
    _save_services_quota(data, "sum_services_with_orders.csv", ["segment"])

    no_order = dict([(key, val) for key, val in data.iteritems() if not key.startswith("order")])
    _save_services_quota(no_order, "sum_services_actual_joined.csv", ["dc", "cloud", "segment"])
    # _save_services_quota(no_order, "sum_services_actual_joined_dc.csv", ["cloud", "segment"])
    _save_services_quota(no_order, "sum_services_actual_with_heads.csv", ["segment"], add_heads=True,
                         abc_api_holder=abc_api_holder)


def create_resources_dumps_dir():
    dir_name = "resources_dump"
    if not os.path.exists(dir_name):
        os.system("mkdir -p %s" % dir_name)

    return dir_name


def is_bad_segment(segment, segments_to_skip):
    if segments_to_skip is None:
        return False

    for segment_filter in segments_to_skip:
        if re.match(segment_filter + "$", segment):
            return True

    return False


def filter_out_segments(data, segments):
    if isinstance(data, dict):
        for key, val in data.items():
            filter_out_segments(val, segments)
    elif isinstance(data, ResourcesCounter):
        for location, resources in data.resources.items():
            segment = resources["segment"]
            if is_bad_segment(segment, segments):
                del data.resources[location]
    else:
        assert False


def sum_resources(resources, sum_keys, res=None):
    if res is None:
        res = ResourcesCounter()

    for resource in resources.resources.itervalues():
        temp = copy.deepcopy(resource)
        for key in sum_keys:
            temp[key] = None

        res.add_resource(temp)

    return res


def join_resources_map(*data):
    res = data[0]
    for map in data[1:]:
        for key, val in map.iteritems():
            if key in res:
                res[key] += val
            else:
                res[key] = val

    return res


def load_config():
    base_path = os.path.join(os.path.dirname(sys.argv[0]))

    return json.load(file(os.path.join(base_path, "reserves.json")))
