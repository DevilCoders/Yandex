#!/usr/bin/env python
# coding=utf-8

import csv
import copy
import json
import os
import pickle
import requests
import subprocess
import sys

from collections import defaultdict, OrderedDict

import abc_api
import resources_counter
import tools
import gencfg_resources

ELEMENTARY_RESOURCES = ["Cores", "RAM", "HDD", "SSD", "NVME"]
ELEMENTARY_RESOURCES_TO_QUOTA = {
    "Cores": ["cpu"],
    "RAM": ["mem"],
    "HDD": ["hdd", "hdd_io"],
    "SSD": ["ssd", "ssd_io"],
    "NVME": ["ssd", "ssd_io"],
}

RESULTS_DIR = "sku_results"


def _load_config(config_path=None):
    if not config_path:
        config_path = os.path.join(os.path.dirname(sys.argv[0]), "sku_evaluation.json")
    return json.load(sys.stdin if config_path == "stdin" else file(config_path))


def _load_hosts_data():
    command = "./utils/common/dump_hostsdata.py -i invnum,name,walle_tags "
    print command
    data = subprocess.check_output(command, shell=True)

    host_to_walle = {}
    fqdn_to_invnum = {}

    for line in data.split("\n"):
        line = line.split()
        if not line:
            continue

        invnum = line[0]
        fqdn = line[1]
        tags = line[2].split(",") if len(line) > 2 else []

        walle_prj = [el for el in tags if el.startswith("g:")]
        walle_prj = walle_prj[0].split(":")[1] if walle_prj else "unknown"

        host_to_walle[invnum] = walle_prj
        host_to_walle[fqdn] = walle_prj

        fqdn_to_invnum[fqdn] = invnum

    return host_to_walle, fqdn_to_invnum


def stat():
    host_to_walle, fqdn_to_invnum = _load_hosts_data()

    for inv in sys.stdin.read().replace(",", " ").split():
        sys.stdout.write("%s\t%s\n" % (inv, host_to_walle.get(inv)))


def _prepare(silent=True):
    commands = [
        "./utils/common/dump_hostsdata.py -i invnum,name,walle_tags > ALL_WALLE.txt",
        "./dump_hostsdata.sh > ALL_HOSTS.txt"
    ]
    for command in commands:
        if not silent:
            print command
        subprocess.check_output(command, shell=True)

    if not silent:
        print


def classification(debug=False, config_path=None, segments=None, tco_cases=None):
    sku_evaluator = SkuEvaluator(debug=debug)

    sku_evaluator.load_walle_projects_data()

    sku_evaluator.format_clouds_sku(
        segments=segments,
        tco_cases=tco_cases,
        config_path=config_path
    )


class QuotaLoader():
    def __init__(self, debug=False):
        self._cache = {}

        self._cache_path = os.path.join(RESULTS_DIR, "load_quota.pickle")

        if debug:
            self.load_cache()

    def load_cache(self):
        if os.path.exists(self._cache_path):
            with file(self._cache_path) as cache_file:
                self._cache = pickle.load(cache_file)

    def save_cache(self):
        with file(self._cache_path, "wb") as cache_file:
            pickle.dump(self._cache, cache_file)

    def load_quota(self,
                   invnums,
                   cache_key=None,
                   hosts_to_quota_mode=None,
                   meta_segments_evaluator=None,
                   meta_segments=None):
        res = self._cache.get(cache_key)

        if res is not None:
            return res

        res = _load_quota(
            invnums,
            hosts_to_quota_mode=hosts_to_quota_mode,
            meta_segments_evaluator=meta_segments_evaluator,
            meta_segments=meta_segments
        )

        self._cache[cache_key] = res
        self.save_cache()

        return res


def _load_quota(invnums,
                hosts_to_quota_mode=None,
                meta_segments_evaluator=None,
                meta_segments=None,
                return_hosts_data=False,
                ):
    invnums = set(invnums)

    res = []
    with file("ALL_HOSTS.txt") as inp:
        for line in inp:
            if line.split()[1] in invnums:
                res.append(line)

    data = gencfg_resources.calculate_yp_quota_for_hosts("".join(res), hosts_to_quota_mode)

    NAMES_MAP = {
        "memory": "mem",
        "io_hdd": "hdd_io",
        "io_ssd": "ssd_io",
    }

    def _to_quota(data):
        res = resources_counter.ResourcesCounter()
        temp = resources_counter.empty_resources()
        for key, val in data.iteritems():
            if key in ["ipv4", "gpu_count", "gpu_model"]:  # GPU hosts are handled at _process_gpu_hosts
                continue

            key = NAMES_MAP.get(key, key)
            mult = resources_counter.RESOURCES_RESERVE_TO_SKU.get(key)
            if mult:
                val = float(val) * mult
            temp[key] = val

        res.add_resource(temp)

        return res

    hosts_quota_data = {}
    for host_data in data["hosts"]:
        hosts_quota_data[host_data["inv"]] = _to_quota(host_data["quota"])

    if return_hosts_data:
        return hosts_quota_data

    hosts_quota_raw = sum(hosts_quota_data.values(), resources_counter.ResourcesCounter())

    if meta_segments_evaluator:
        meta_segments_evaluator.correct_quota(meta_segments, hosts_quota_data)

        hosts_quota = sum(hosts_quota_data.values(), resources_counter.ResourcesCounter())
    else:
        hosts_quota = hosts_quota_raw

    res = hosts_quota_raw, hosts_quota

    return res


def _process_gpu_hosts(invnums):
    gpu_names_map = {
        'NVIDIA - GEFORCE GTX1080TI': "gpu_geforce_1080ti",
        'NVIDIA - GTX1080': "gpu_geforce_1080ti",
        "Asus - TURBO-GTX1080TI-11G": "gpu_geforce_1080ti",
        "Gigabyte - GV-N1080D5X-8GD-B": "gpu_geforce_1080ti",
        "Gigabyte - GV-N108TTURBO-11GD": "gpu_geforce_1080ti",
        "nVidia - GeForce GTX1080Ti (PG611)": "gpu_geforce_1080ti",
        "Palit - GTX1080Ti (NEB108T019LC-1021F)": "gpu_geforce_1080ti",
        "Zotac - ZT-P10810B-10B": "gpu_geforce_1080ti",
        "Zotac - ZT-P10810B-10P": "gpu_geforce_1080ti",
        "Gigabyte - GV-N1080NC-8GD": "gpu_geforce_1080ti",

        'TESLA K40': "gpu_tesla_k40",
        "NVIDIA-TeslaK40": "gpu_tesla_k40",

        'TESLA M40': "gpu_tesla_m40",
        "NVIDIA-TeslaM40": "gpu_tesla_m40",

        'NVIDIA - TESLA P40': "gpu_tesla_p40",
        "Nvidia - Tesla P40 (PG610)": "gpu_tesla_p40",

        'NVIDIA - TESLA V100': "gpu_tesla_v100",
        'V100-SXM2-32GB': "gpu_tesla_v100",

        "nVidia - Tesla V100-PCIE-16GB": "gpu_tesla_v100",
        "nVidia - Tesla V100-PCIE-32GB": "gpu_tesla_v100",
        "nVidia - Tesla V100-SXM2-32GB": "gpu_tesla_v100",

        'NVIDIA - A100': "gpu_tesla_a100",
        "nVidia - A100-40GB": "gpu_tesla_a100",
    }
    for el in gpu_names_map.values():
        gpu_names_map[el] = el

    cards_count = defaultdict(int)
    cards_hosts = defaultdict(list)
    unknown_gpu_models = set()
    with file("ALL_HOSTS.txt") as inp:
        for line in inp:
            line = line.split("\t")
            inv = line[1]
            if inv not in invnums:
                continue

            gpu_count = line[11]
            if not gpu_count.isdigit():
                sys.stderr.write("Invalid number of gpu cards for %s: %s\n" % (inv, gpu_count))
                continue

            gpu_models = set()
            for gpu_name in line[10].split(","):
                model = gpu_names_map.get(gpu_name)
                if model:
                    gpu_models.add(model)
                else:
                    unknown_gpu_models.add(gpu_name)

            if len(gpu_models) > 1:
                sys.stderr.write("GPU host with multiple card types %s: %s\n" % (inv, ",".join(sorted(gpu_models))))
                gpu_model = "gpu"
            elif not gpu_models:
                gpu_model = "gpu"
            else:
                gpu_model = list(gpu_models)[0]

            cards_count[gpu_model] += int(gpu_count)
            cards_hosts[gpu_model].append(inv)

    if unknown_gpu_models:
        sys.stderr.write("unknown_gpu_models: %s\n" % ",".join(sorted(unknown_gpu_models)))

    return cards_count, cards_hosts


def _load_gpu_quota(invnums):
    cards_count, cards_hosts = _process_gpu_hosts(invnums)

    temp = resources_counter.empty_resources()
    temp.update(cards_count)

    res = resources_counter.ResourcesCounter()
    res.add_resource(temp)

    return res, res


def _walle_projects_from_filter(settings, projects_groups):
    projects = settings.get("walle_projects", [])
    for group in settings.get("walle_groups", []):
        projects.extend(projects_groups[group])

    walle_prj_prefixes = settings.get("walle_proups_prefixes", [])

    for group, group_projects in projects_groups.iteritems():
        for prefix in walle_prj_prefixes:
            if group.startswith(prefix):
                projects.extend(group_projects)

    exclude_projects = settings.get("walle_projects_exclude", [])
    for group in settings.get("walle_groups_exclude", []):
        exclude_projects.extend(projects_groups[group])

    if exclude_projects:
        projects = sorted(set(projects).difference(exclude_projects))

    return sorted(projects)


def _walle_projects_to_invnum(projects, walle_projects_map):
    invnums = []
    for project in projects:
        invs = walle_projects_map[project]

        invnums.extend(invs)

    return invnums


def _apply_discounts(quota, segment_settings, out):
    discounted_quota = quota
    utilization_mode = segment_settings.get("discounts", {}).get("utilization_mode", "utilization")

    for resource, mult in sorted(segment_settings.get("discounts", {}).get(utilization_mode, {}).iteritems()):
        discounted_quota = discounted_quota.multiply_resources([resource], mult)

    out.write("quota discounted by utilizaion: %s\n" % discounted_quota.format())

    reserve_sum = resources_counter.ResourcesCounter()
    for case, reserve_mult in sorted(segment_settings.get("discounts", {}).get("reserves", {}).iteritems()):
        reserve_quota = discounted_quota * reserve_mult
        reserve_sum += reserve_quota
        out.write("reserve %s: %s\n" % (case, reserve_quota.format()))

    discounted_quota = discounted_quota - reserve_sum

    out.write("result discounted quota: %s\n" % discounted_quota.format())

    return discounted_quota


def _segment_to_tco(invnum_to_resources, invnums):
    elem_resources = SimpleCounter()
    lost_invnums = []

    for inv in invnums:
        res = invnum_to_resources.get(inv)
        if not res:
            lost_invnums.append(inv)
            continue

        elem_resources += res

    return elem_resources, lost_invnums


class SkuDistributionEvaluator:
    def __init__(self):
        self.distribution = defaultdict(dict)

    def init_cost_distribution(self, cost, distribution):
        for sku, sku_cost in cost.iteritems():
            sku_distribution = distribution.get(sku, {})
            total_sku = sum(sku_distribution.itervalues())
            if not total_sku:
                continue

            for source, source_cost in sku_distribution.iteritems():
                self.distribution[sku][source] = sku_cost * source_cost / total_sku

    def add_new_cost(self, cost, source):
        for sku, cost in cost.iteritems():
            old_cost = sum(self.distribution.get(sku, {}).itervalues())
            cost_diff = cost - old_cost
            if cost_diff > 0:
                self.distribution[sku][source] = cost_diff

    def format_distribution(self, out, sku_list=None):
        if not sku_list:
            sku_list = sorted(self.distribution.keys())

        for sku in sku_list:
            out.write("%s:\n" % sku)
            _format_cost(out, self.distribution[sku])
            out.write("\n")


def _evaluate_sku_cost(quota, total_cost_per_sku, segment):
    sku_cost = SimpleCounter()
    for sku, capacity in quota.iteritems():
        if not capacity:
            cost = total_cost_per_sku.get("sku")
            if cost:
                sys.stderr.write("No capacity for %s sku at %s\n" % (sku, segment))
            continue

        sku_cost[sku] = total_cost_per_sku[sku] / capacity

    return sku_cost


def distribute_cost(elem_resources, extra_cost):
    res = copy.deepcopy(elem_resources)

    acceptors_cost = defaultdict(int)
    total_acceptor_cost = 0
    for name, cost in elem_resources.iteritems():
        assert isinstance(name, (tuple, list))
        acceptors_cost[name[0]] += cost
        total_acceptor_cost += cost

    for acceptor, acceptor_cost in acceptors_cost.iteritems():
        for source, cost in extra_cost.iteritems():
            key = (acceptor, source)
            assert not key in res
            if total_acceptor_cost:
                res[key] = cost * acceptor_cost / total_acceptor_cost
            else:
                res[key] = cost / len()
    return res


def evaluate_sku_cost(segment, segment_settings, quota, elem_resources, correct=False, out=None):
    cost_config = segment_settings["cost_evaluation"]

    elem_resources_to_sku_map = cost_config["elem_resources_to_sku"]

    total_cost_per_sku = defaultdict(float)
    sku_cost_source_distribution = defaultdict(lambda: defaultdict(float))
    for name, cost in elem_resources.iteritems():
        coeff_sum = 0
        if isinstance(name, (tuple, list)):
            name, source = name
        else:
            name = name
            source = None

        for sku, coeff in elem_resources_to_sku_map[name].iteritems():
            sku_cost = cost * coeff
            total_cost_per_sku[sku] += sku_cost
            sku_cost_source_distribution[sku][source] += sku_cost

            coeff_sum += coeff
        assert coeff_sum == 1

    total_quota = quota.total()

    sku_cost = _evaluate_sku_cost(quota=total_quota, total_cost_per_sku=total_cost_per_sku, segment=segment)

    sku_redistribution_config = cost_config.get("sku_redistribution")
    if correct and sku_redistribution_config:
        sku_fixed = {}
        sku_cost_corrected = copy.deepcopy(total_cost_per_sku)
        for sku, sku_config in sorted(sku_redistribution_config.iteritems()):
            target = sku_config["target"]
            current_cost = sku_cost[sku]

            if current_cost < target:
                continue

            sku_fixed[sku] = target

            extra_cost = (current_cost - target) * total_quota[sku]
            if False:
                out.write("distribute %s extra cost %s / %s\n" % (sku, extra_cost, total_cost_per_sku[sku]))

            acceptors = sku_config["distribute_prop_to_cost"]

            total_acceptor_cost = sum(
                [cost for acceptor, cost in total_cost_per_sku.iteritems() if acceptor in acceptors])
            for acceptor in sorted(acceptors):
                sku_extra_cost = extra_cost * total_cost_per_sku[acceptor] / total_acceptor_cost
                sku_cost_corrected[acceptor] += sku_extra_cost
                if False:
                    out.write("distribute %s cost to %s: +%s\n" % (sku, acceptor, sku_extra_cost))

        sku_cost = _evaluate_sku_cost(quota=total_quota, total_cost_per_sku=sku_cost_corrected,
                                      segment=segment)

        sku_cost.update(sku_fixed)

    return sku_cost, sku_cost_source_distribution


def evaluate_and_print_sku_cost(segment, segment_settings, quota, elem_resources, out, correct=False):
    sku_cost, sku_cost_source_distribution = evaluate_sku_cost(segment,
                                                               segment_settings,
                                                               quota=quota,
                                                               elem_resources=elem_resources,
                                                               correct=correct,
                                                               out=out
                                                               )

    _format_cost(out, sku_cost, total=False)
    out.write("\n")

    return sku_cost, sku_cost_source_distribution


def _format_money(data):
    res = float(data)
    mult = 1000
    last = None
    for el in ["K", "M"]:
        temp = res / mult
        if temp < 1:
            break

        res = temp
        last = el

    return "%.2f %srub" % (res, (last + " ") if last else "")


def _format_cost(out, cost, total=True):
    for key, val in sorted(cost.iteritems(), key=lambda x: -x[-1]):
        if not val:
            continue
        if isinstance(key, (tuple, list)):
            key = " ".join(key)

        out.write("%s: %s\n" % (key, _format_money(val)))

    if total:
        out.write("total: %s\n" % _format_money(sum(cost.values())))


def _get_element(data, key):
    return data.get(key, 0) if isinstance(data, dict) else data


def _apply_counter_operation(left, right, operation):
    if isinstance(left, (int, float, long)) and isinstance(right, (int, float, long)):
        return operation(left, right)

    res = {}
    keys = []
    if isinstance(left, dict):
        keys = left.keys()
    if isinstance(right, dict):
        keys.extend(right)

    for key in set(keys):
        res[key] = _apply_counter_operation(_get_element(left, key), _get_element(right, key), operation)

    return res


class SimpleCounter(dict):
    def __add__(self, other):
        return SimpleCounter(_apply_counter_operation(self, other, lambda x, y: x + y))

    def __sub__(self, other):
        return SimpleCounter(_apply_counter_operation(self, other, lambda x, y: x - y))

    def __mul__(self, other):
        return SimpleCounter(_apply_counter_operation(self, other, lambda x, y: x * y))

    def __div__(self, other):
        return SimpleCounter(_apply_counter_operation(self, other, lambda x, y: x / y))

    def __nonzero__(self):
        for el in self.itervalues():
            if el:
                return True

        return False


class PerHostCostEvaluator(object):
    def __init__(self, config, meta_segments_evaluator, debug_out=None):
        self._debug_out = debug_out
        self._cases = {}

        for case, case_config in sorted(config["per_host_cost"].iteritems()):
            cost = self._evaluate_cost(case, case_config, meta_segments_evaluator)

            flat_all_hosts = case_config.get("flat_all_hosts")
            meta_segments = case_config.get("meta_segments", [])
            if not flat_all_hosts:
                assert meta_segments
                invnums = meta_segments_evaluator.get_meta_segments_invnums(meta_segments)
            else:
                invnums = []

            if debug_out:
                debug_out.write("per host cost %s\n" % case)
                if flat_all_hosts:
                    debug_out.write("flat add to all hosts\n")
                else:
                    debug_out.write("total\n")
                _format_cost(debug_out, cost)
                if not flat_all_hosts:
                    debug_out.write("meta_segments %s\n" % ",".join(sorted(meta_segments)))
                    debug_out.write("hosts %s\n" % len(invnums))
                debug_out.write("\n")

            self._cases[case] = {
                "flat_all_hosts": flat_all_hosts,
                "invnums": invnums,
                "meta_segments": meta_segments,
                "cost": cost
            }

    def _evaluate_quota_cost(self, case, config, meta_segments_evaluator):
        cost = defaultdict(int)

        quota = config.get("quota")
        if quota:
            quota_price = config.get("quota_price")
            if not quota_price:
                raise NotImplemented()

            for key, val in quota.iteritems():
                cost["quota"] += val * quota_price[key]

        hosts_filter = config.get("hosts_filter")
        if hosts_filter:
            elementary_resources = meta_segments_evaluator.evaluate_hosts_filter_cost("infra.%s" % case, hosts_filter)
            cost["hosts"] += sum(elementary_resources.itervalues())

        return SimpleCounter(cost)

    def _evaluate_cost(self, case, case_config, meta_segments_evaluator):
        cost = SimpleCounter()

        path = case_config.get("cost_path")
        if path:
            full_path = os.path.join(os.path.dirname(sys.argv[0]), path)
            data = json.load(file(full_path))

            cost += data["per_host_cost"][case]

        quota_config = case_config.get("quota_cost")
        if quota_config:
            cost += self._evaluate_quota_cost(case, quota_config, meta_segments_evaluator)

        return cost

    def evaluate(self, meta_segments, invnums):
        total = SimpleCounter()
        for case, case_data in sorted(self._cases.iteritems()):
            full_cost = case_data["cost"]

            if case_data.get("flat_all_hosts"):
                cost = full_cost * len(invnums)
            else:
                case_meta_segments = case_data.get("meta_segments")
                if case_meta_segments and not set(case_meta_segments).intersection(meta_segments):
                    continue

                case_invnums = case_data["invnums"]
                common_invnums_count = len(set(case_invnums).intersection(invnums))

                cost = full_cost * common_invnums_count / len(case_invnums)

            if not cost:
                continue

            cost = dict([("%s.%s" % (case, key), val) for key, val in cost.iteritems()])

            if self._debug_out:
                self._debug_out.write("%s cost:\n" % case)
                _format_cost(self._debug_out, cost)
                self._debug_out.write("\n")

            total += cost

        return total


def _find_dict_intersection(data, key, val):
    other = []
    for key0, val0 in data.iteritems():
        if key0 != key:
            other.extend(val0)

    return sorted(set(other).intersection(val))


class MetaSegmentsEvaluator(object):
    def __init__(self, config, projects_groups, walle_projects_map, invnum_to_resources, debug_out=None):
        self._projects_groups = projects_groups
        self._walle_projects_map = walle_projects_map
        self._invnum_to_resources = invnum_to_resources
        self._debug_out = debug_out

        meta_config = config.get("meta_segments", {})

        self._meta_to_projects = {}
        self._meta_to_invnums = defaultdict(SimpleCounter)
        self._meta_to_lost_invnums = {}

        for segment, settings in sorted(meta_config.get("segments", {}).iteritems()):
            self._load_segment_data(segment, settings["filter"])

        for case, settings in sorted(meta_config.get("joined_segments", {}).iteritems()):
            segment = "joined." + case
            self._load_segment_data(segment, settings["filter"])

            invnums = self._meta_to_invnums[segment].keys()

            rest_division = SimpleCounter(dict([(key, 1) for key in ELEMENTARY_RESOURCES]))
            for segment, division in settings["division"].iteritems():
                division = SimpleCounter(division)

                rest_division -= division

                segment_to_invnums = SimpleCounter()
                for invnum in invnums:
                    segment_to_invnums[invnum] = division

                self._meta_to_invnums[segment] += segment_to_invnums

                if debug_out:
                    debug_out.write("%s part: %s\n" % (segment, division))

                    invnum_to_resources = self._servers_cost_data(segment_to_invnums)
                    elem_resources, _ = _segment_to_tco(invnum_to_resources, invnum_to_resources.keys())
                    _format_cost(debug_out, elem_resources)
                    debug_out.write("\n")

            rest_segment = settings["rest_segment"]
            segment_to_invnums = SimpleCounter()
            for invnum in invnums:
                segment_to_invnums[invnum] = rest_division
            self._meta_to_invnums[rest_segment] += segment_to_invnums

            if debug_out:
                debug_out.write("%s part: %s\n" % (rest_segment, rest_division))
                invnum_to_resources = self._servers_cost_data(segment_to_invnums)
                elem_resources, _ = _segment_to_tco(invnum_to_resources, invnum_to_resources.keys())
                _format_cost(debug_out, elem_resources)
                debug_out.write("\n")

        for segment, projects in sorted(self._meta_to_projects.iteritems()):
            intersection = _find_dict_intersection(self._meta_to_projects, segment, projects)

            if intersection:
                sys.stderr.write("meta segments wall-e projects intersection: %s\n" % ",".join(intersection))
                if debug_out:
                    debug_out.write("meta segments wall-e projects intersection: %s\n" % ",".join(intersection))

    def _evaluate_segment_data(self, segment, hosts_filter):
        invnum_path = hosts_filter.get("invnum_file")
        if invnum_path:
            invnums = file(invnum_path).read().split()
            walle_projects = []
        else:
            walle_projects = _walle_projects_from_filter(hosts_filter, self._projects_groups)
            invnums = _walle_projects_to_invnum(walle_projects, self._walle_projects_map)

        segment_to_invnums = SimpleCounter()
        for invnum in invnums:
            segment_to_invnums[invnum] = SimpleCounter(dict([(key, 1) for key in ELEMENTARY_RESOURCES]))

        invnum_to_resources = self._servers_cost_data(segment_to_invnums)
        elem_resources, lost_invnum = _segment_to_tco(invnum_to_resources, invnum_to_resources.keys())

        if self._debug_out:
            self._debug_out.write("meta segment %s\n" % segment)
            self._debug_out.write("projects: %s\n" % ",".join(sorted(walle_projects)))
            self._debug_out.write("hosts: %s\n" % len(invnums))
            self._debug_out.write("lost hosts: %s\n" % len(lost_invnum))
            self._debug_out.write("cost:\n")
            _format_cost(self._debug_out, elem_resources)
            self._debug_out.write("\n")

        with file(os.path.join(RESULTS_DIR, "meta_segments_lost_invnums.txt"), "wb") as out:
            for segment, invnums in sorted(self._meta_to_lost_invnums.iteritems()):
                for inv in invnums:
                    out.write("%s\t%s\n" % (segment, inv))

        return walle_projects, segment_to_invnums, elem_resources, lost_invnum

    def _load_segment_data(self, segment, hosts_filter):
        walle_projects, segment_to_invnums, elem_resources, lost_invnum = self._evaluate_segment_data(segment,
                                                                                                      hosts_filter)
        self._meta_to_projects[segment] = walle_projects
        self._meta_to_invnums[segment] = segment_to_invnums
        self._meta_to_lost_invnums[segment] = lost_invnum

    def evaluate_hosts_filter_cost(self, log_name, hosts_filter):
        walle_projects, segment_to_invnums, elem_resources, lost_invnum = self._evaluate_segment_data(
            segment=log_name,
            hosts_filter=hosts_filter,
        )

        return elem_resources

    def _servers_cost_data(self, meta_to_invnums):
        res = SimpleCounter()
        for inv, mult in meta_to_invnums.iteritems():
            resource_cost = self._invnum_to_resources.get(inv)
            if resource_cost is None:
                continue

            cost = {}
            for key, val in resource_cost.iteritems():
                res_name = key[0] if isinstance(key, (tuple, list)) else key

                cost[key] = val * mult.get(res_name, 1)

            if inv in res:
                cost += res[inv]
            res[inv] = cost

        return res

    def servers_cost_data(self, meta_segments):
        res = SimpleCounter()
        for segment in meta_segments:
            res += self._servers_cost_data(self._meta_to_invnums[segment])

        return res

    def correct_quota(self, meta_segments, hosts_quota):
        # in place to avoid cost of making hosts_quota copy
        for segment in meta_segments:
            for inv, quota in hosts_quota.iteritems():
                mult = self._meta_to_invnums[segment].get(inv)
                if mult is None:
                    continue

                for el_resource, el_mult in mult.iteritems():
                    if el_mult == 1:
                        continue

                    for quota_key in ELEMENTARY_RESOURCES_TO_QUOTA[el_resource]:
                        for resource in quota.resources.itervalues():
                            resource[quota_key] *= el_mult

    def get_meta_segments_invnums(self, meta_segments):
        res = set()
        for meta in meta_segments:
            res.update(self._meta_to_invnums[meta].keys())

        return sorted(res)


class SkuEvaluator():
    def __init__(self, debug=False):
        self.debug = debug

        self.base_config = _load_config()

        self.quota_loader = QuotaLoader(debug=debug)

    def load_walle_projects_data(self):
        silent = True
        if not self.debug:
            _prepare(silent=silent)

        tools.create_dir(RESULTS_DIR)

        walle_mapping_path = os.path.join(RESULTS_DIR, "walle_mapping.txt")
        walle_mapping_out = file(walle_mapping_path, "wb")

        fqdn_map = {}
        walle_projects_map = defaultdict(list)
        projects_groups = defaultdict(set)
        projects_to_tags = {}

        for line in file("ALL_WALLE.txt"):
            line = line.split()
            invnum = line[0]
            fqdn = line[1]
            tags = line[2].split(",") if len(line) > 2 else []

            walle_prj = [el for el in tags if el.startswith("g:")]
            walle_prj = walle_prj[0].split(":")[1] if walle_prj else "unknown"

            if walle_prj != "unknown":
                current_tags = projects_to_tags.get(walle_prj)
                if current_tags is None:
                    projects_to_tags[walle_prj] = sorted(tags)

            walle_projects_map[walle_prj].append(invnum)
            fqdn_map[fqdn] = invnum

            walle_mapping_out.write("%s\t%s\t%s\n" % (invnum, fqdn, walle_prj))

            yt_cluster_prefix = "rtc.yt_cluster-"
            yt_cluster = [tag for tag in tags if tag.startswith(yt_cluster_prefix)]
            if yt_cluster:
                assert len(yt_cluster) == 1
                assert "yt" in tags
                yt_cluster = yt_cluster[0].split("-", 1)[1]
            else:
                yt_cluster = None

            # https://wiki.yandex-team.ru/runtime-cloud/walle-projects/
            if "rtc" in tags:
                gpu_tag = [tag for tag in tags if tag.startswith("rtc.gpu-")][0]
                if "gpu" in walle_prj and gpu_tag == "rtc.gpu-none":
                    sys.stderr.write("gpu project without gpu tag: %s\n" % walle_prj)

                if gpu_tag != "rtc.gpu-none":
                    if "yt" in tags:
                        projects_groups["yt-gpu"].add(walle_prj)
                    else:
                        projects_groups["rtc-gpu"].add(walle_prj)
                elif "yt" in tags:
                    if yt_cluster == "arnold" and "runtime" in tags:
                        projects_groups["yt-arnold-with-rtc"].add(walle_prj)
                    elif yt_cluster:
                        projects_groups["yt-" + yt_cluster].add(walle_prj)
                    elif "yp" in tags:
                        projects_groups["yp-main"].add(walle_prj)
                    else:
                        projects_groups["yt"].add(walle_prj)
                elif "yp" in tags:
                    projects_groups["rtc-yp"].add(walle_prj)
                elif "qloud" in tags:
                    projects_groups["rtc-qloud"].add(walle_prj)
                elif "runtime" in tags:
                    projects_groups["rtc-gencfg"].add(walle_prj)
                else:
                    projects_groups["unsorted-rtc_tag"].add(walle_prj)

        if not silent:
            print walle_mapping_path

        invnum_to_group = defaultdict(list)
        used_walle_projects = {"unknown"}

        rtc_groups_path = os.path.join(RESULTS_DIR, "rtc_groups.txt")
        rtc_groups_out = file(rtc_groups_path, "wb")

        for group, projects in sorted(projects_groups.iteritems()):
            rtc_groups_out.write("%s:\n" % group)

            clash = used_walle_projects.intersection(projects)
            if clash:
                sys.stderr.write("wall-e groups intersection: %s\n" % ",".join(sorted(clash)))

            used_walle_projects.update(projects)

            projects_invnums = [(project, walle_projects_map[project]) for project in projects]

            for project, invnums in sorted(projects_invnums, key=lambda x: -len(x[1])):
                if False:
                    tags = projects_to_tags.get(project, [])
                    rtc_groups_out.write("%s\t%s\t%s\n" % (project, len(invnums), ",".join(tags)))
                else:
                    rtc_groups_out.write("%s\t%s\n" % (project, len(invnums)))

                for invnum in invnums:
                    invnum_to_group[invnum].append(group)

            rtc_groups_out.write("------\n")

        hosts_projects = walle_projects_map.keys()
        unsorted_projects = set(hosts_projects).difference(used_walle_projects)
        rtc_groups_out.write("unsorted_projects:\n")
        for project in sorted(unsorted_projects):
            invnums = walle_projects_map.get(project)
            rtc_groups_out.write("%s\t%s\n" % (project, len(invnums)))
        rtc_groups_out.write("------\n")
        if not silent:
            print rtc_groups_path

        sorted_path = os.path.join(RESULTS_DIR, "rtc_sorted.txt")
        sorted_out = file(sorted_path, "wb")
        for invnum, groups in sorted(invnum_to_group.iteritems()):
            sorted_out.write("%s\t%s\n" % (invnum, ",".join(groups)))

        if not silent:
            print sorted_path
            print

        self.projects_groups = projects_groups
        self.walle_projects_map = walle_projects_map

    def _get_segment_invnums(self, segment_settings, out=None):
        meta_segments = segment_settings["meta_segments"]
        if out:
            out.write("meta_segments: %s\n" % ",".join(meta_segments))

        invnum_to_resources = self.meta_segments_evaluator.servers_cost_data(meta_segments)

        invnums = invnum_to_resources.keys()
        segment_filter = segment_settings.get("filter")
        if segment_filter:
            chosen_invnums = segment_filter.get("invnums")
            if chosen_invnums:
                invnums = chosen_invnums
                if out:
                    out.write("chosen invnums: %s\n" % len(invnums))
                    out.write("\n")
            else:
                walle_projects = _walle_projects_from_filter(segment_filter, self.projects_groups)
                invnums = _walle_projects_to_invnum(walle_projects, self.walle_projects_map)

                if out:
                    out.write("walle_projects: %s\n" % ", ".join(walle_projects))
                    out.write("\n")

        return invnums, invnum_to_resources

    def _prepare_segment_sku(self, segment, segment_settings, out):
        out.write("segment: %s\n" % segment)

        meta_segments = segment_settings["meta_segments"]
        invnums, invnum_to_resources = self._get_segment_invnums(segment_settings, out=out)

        elem_resources, lost_invnums = _segment_to_tco(
            invnum_to_resources,
            invnums
        )

        self.segments_to_invnums[segment] = invnums
        self.segments_to_lost_invnums[segment] = lost_invnums

        self.segments_invnums_data[segment]["hosts"] = invnums
        self.segments_invnums_data[segment]["lost_hosts"] = lost_invnums

        self.segments_result_data[segment]["hosts"] = len(invnums)
        self.segments_result_data[segment]["lost_hosts"] = len(lost_invnums)
        self.segments_result_data[segment]["elem_resources"] = join_elementary_resources(elem_resources)

        out.write("hosts: %s\n" % len(invnums))
        out.write("hosts absent at tco table: %s\n" % len(lost_invnums))

        all_invnums = invnums
        invnums = sorted(set(invnums).difference(lost_invnums))
        out.write("hosts for evaluation: %s\n" % len(invnums))
        out.write("\n")

        out.write("elementary resources base cost:\n")
        _format_cost(out, join_elementary_resources(elem_resources))
        out.write("\n")

        per_host_cost = self.per_host_cost_evaluator.evaluate(meta_segments, all_invnums)
        elem_resources = distribute_cost(elem_resources, per_host_cost)
        self.segments_result_data[segment]["elem_resources_corrected"] = join_elementary_resources(elem_resources)

        out.write("elementary resources corrected cost:\n")
        _format_cost(out, join_elementary_resources(elem_resources))
        out.write("\n")

        if segment_settings.get("evaluate_sku"):
            hosts_to_quota_mode = segment_settings.get("hosts_to_quota_mode", "infra_only")

            if hosts_to_quota_mode == "gpu":
                hosts_quota_raw, hosts_quota = _load_gpu_quota(invnums)
            else:
                hosts_quota_raw, hosts_quota = self.quota_loader.load_quota(
                    invnums=invnums,
                    cache_key=segment,
                    hosts_to_quota_mode=hosts_to_quota_mode,
                    meta_segments_evaluator=self.meta_segments_evaluator,
                    meta_segments=meta_segments
                )

            out.write("hosts quota raw: %s\n" % hosts_quota_raw.format())

            self.segments_result_data[segment]["hosts_quota_raw"] = hosts_quota.total()
            out.write("hosts quota segmented: %s\n" % hosts_quota.format())

            extra_hosts_quota = segment_settings.get("extra_hosts_quota", {})
            if extra_hosts_quota:
                extra_hosts_quota = resources_counter.ResourcesCounter().add_resource_from_json(extra_hosts_quota)
                out.write("extra_hosts_quota: %s\n" % extra_hosts_quota.format_total())

                hosts_quota += extra_hosts_quota

            self.segments_result_data[segment]["hosts_quota"] = hosts_quota.total()
            out.write("hosts quota corrected: %s\n" % hosts_quota.format())
            out.write("\n")

            hosts_discounted_quota = _apply_discounts(hosts_quota, segment_settings, out)

            out.write("\n")
            out.write("hosts discounted quota: %s\n" % hosts_discounted_quota.format())
            out.write("\n")

            sku_results = {}
            sku_distribution_evaluator = SkuDistributionEvaluator()

            distribution_init_done = False
            if hosts_to_quota_mode is None or hosts_to_quota_mode == "infra_ony":
                raw_quota = hosts_quota + resources_counter.ResourcesCounter().add_resource_from_json({
                    "cpu": 2 * len(invnums),
                    "mem": 7 * len(invnums)
                })

                out.write("raw hosts sku without infra agents cost:\n")
                sku_cost, sku_cost_source_distribution = evaluate_and_print_sku_cost(
                    segment,
                    segment_settings,
                    quota=raw_quota,
                    elem_resources=elem_resources,
                    out=out
                )
                sku_results["raw hosts sku cost"] = sku_cost

                sku_distribution_evaluator.init_cost_distribution(sku_cost, sku_cost_source_distribution)
                distribution_init_done = True

            out.write("hosts sku cost:\n")
            sku_cost, sku_cost_source_distribution = evaluate_and_print_sku_cost(
                segment,
                segment_settings,
                quota=hosts_quota,
                elem_resources=elem_resources,
                out=out
            )
            sku_results["hosts sku cost"] = sku_cost

            if distribution_init_done:
                sku_distribution_evaluator.add_new_cost(sku_cost, "rtc_infra.hosts_agents")
            else:
                sku_distribution_evaluator.init_cost_distribution(sku_cost, sku_cost_source_distribution)

            out.write("discounted sku cost:\n")
            sku_cost, _ = evaluate_and_print_sku_cost(
                segment,
                segment_settings,
                quota=hosts_discounted_quota,
                elem_resources=elem_resources,
                out=out
            )
            sku_results["discounted sku cost"] = sku_cost
            sku_distribution_evaluator.add_new_cost(sku_cost, "utilization discount")

            out.write("discounted and corrected sku cost:\n")
            sku_cost, _ = evaluate_and_print_sku_cost(
                segment,
                segment_settings,
                quota=hosts_discounted_quota,
                elem_resources=elem_resources,
                out=out,
                correct=True
            )
            sku_results["discounted and corrected sku cost"] = sku_cost
            sku_distribution_evaluator.add_new_cost(sku_cost, "disks correction")

            margin_multiplier = self.base_config.get("margin_multiplier")
            if margin_multiplier:
                assert margin_multiplier > 1
                sku_cost *= margin_multiplier
                sku_distribution_evaluator.add_new_cost(sku_cost, "margin correction")

                out.write("sku cost + margin:\n")
                _format_cost(out, sku_cost, total=False)
                out.write("\n")

            out.write("final sku cost:\n")
            _format_cost(out, sku_cost, total=False)
            out.write("\n")

            out.write("sku cost sources:\n")
            sku_distribution_evaluator.format_distribution(out)

            sku_results["result_rate"] = sku_cost
            sku_results["sku_cost_sources"] = sku_distribution_evaluator.distribution

            self.segments_result_data[segment]["sku_results"] = sku_results

        out.write("---------\n\n")

    def _format_clouds_sku(self, config_path=None, segments=None, tco_case=None, out_path_suffix=None):
        out_path_suffix = "%s_%s" % ((out_path_suffix if out_path_suffix else ""), tco_case)
        out_path_suffix = out_path_suffix.lower()

        segments = segments.replace(",", " ").split() if segments else []

        silent = False

        config = _load_config()
        if config_path:
            config.update(_load_config(config_path))

        out_dir = tools.create_dir(RESULTS_DIR)

        self.segments_to_invnums = {}
        self.segments_to_lost_invnums = {}
        self.segments_result_data = defaultdict(dict)
        self.segments_invnums_data = defaultdict(dict)

        self.invnum_to_resources = _load_hosts_tco(
            config=self.base_config["tco_evaluation"][tco_case]
        )

        debug_path = os.path.join(out_dir, "segments_init_%s.txt" % out_path_suffix)
        if not silent:
            print debug_path
        init_out = file(debug_path, "wb")

        self.meta_segments_evaluator = MetaSegmentsEvaluator(
            config=config,
            projects_groups=self.projects_groups,
            walle_projects_map=self.walle_projects_map,
            invnum_to_resources=self.invnum_to_resources,
            debug_out=init_out,
        )

        self.per_host_cost_evaluator = PerHostCostEvaluator(
            config=config,
            meta_segments_evaluator=self.meta_segments_evaluator,
            debug_out=init_out,
        )

        debug_path = os.path.join(out_dir, "segments_results_%s.txt" % out_path_suffix)
        if not silent:
            print debug_path

        with file(debug_path, "wb") as out:
            for segment, segment_settings in sorted(config["segments"].iteritems()):
                if segments and segment not in segments:
                    continue

                inherit = segment_settings.get("inherit_from")
                if inherit:
                    base_config = copy.deepcopy(config["segments"][inherit])
                    base_config.update(segment_settings)
                    segment_settings = base_config

                if segment_settings.get("hosts_to_quota_mode") == "gpu":
                    invnums, _ = self._get_segment_invnums(segment_settings)
                    cards_count, cards_hosts = _process_gpu_hosts(invnums)

                    rates = {}
                    elementary_resources = SimpleCounter()
                    hosts_quota = SimpleCounter()

                    for card_type, invnums in sorted(cards_hosts.iteritems()):
                        sub_setttings = copy.deepcopy(segment_settings)
                        if not "filter" in sub_setttings:
                            sub_setttings["filter"] = {}
                        sub_setttings["filter"]["invnums"] = invnums

                        elem_resources_to_sku = {}
                        for resource in ELEMENTARY_RESOURCES:
                            elem_resources_to_sku[resource] = {
                                card_type: 1
                            }
                        if not "cost_evaluation" in sub_setttings:
                            sub_setttings["cost_evaluation"] = {}
                        sub_setttings["cost_evaluation"]["elem_resources_to_sku"] = elem_resources_to_sku

                        sub_name = segment + "." + card_type
                        self._prepare_segment_sku(sub_name, sub_setttings, out)

                        rates.update(self.segments_result_data[sub_name]["sku_results"]["result_rate"])

                        elementary_resources[card_type] = sum(
                            self.segments_result_data[sub_name]["elem_resources_corrected"].itervalues())
                        hosts_quota += self.segments_result_data[sub_name]["hosts_quota"]

                    self.segments_result_data[segment]["sku_results"] = {
                        "result_rate": rates
                    }
                    self.segments_result_data[segment]["elem_resources_corrected"] = elementary_resources
                    self.segments_result_data[segment]["hosts_quota"] = hosts_quota

                    out.write("segment: %s\n" % segment)
                    out.write("joined cost:\n")
                    _format_cost(out, elementary_resources, total=False)
                    out.write("\n")
                    out.write("hosts_quota:\n")
                    _format_cost(out, hosts_quota, total=False)
                    out.write("\n")
                    out.write("joined rates:\n")
                    _format_cost(out, rates, total=False)
                    out.write("\n")

                    out.write("---------\n\n")
                else:
                    self._prepare_segment_sku(segment, segment_settings, out)

        path = os.path.join(out_dir, "segments_invnums_%s.txt" % out_path_suffix)
        if not silent:
            print path

        with file(path, "wb") as out:
            for segment, invnums in self.segments_to_invnums.iteritems():
                for inv in invnums:
                    out.write("%s\t%s\n" % (segment, inv))

        path = os.path.join(out_dir, "segments_lost_invnums_%s.txt" % out_path_suffix)
        if not silent:
            print path

        with file(path, "wb") as out:
            for segment, invnums in self.segments_to_lost_invnums.iteritems():
                for inv in invnums:
                    out.write("%s\t%s\n" % (segment, inv))

        path = os.path.join(out_dir, "segments_results_%s.json" % out_path_suffix)
        if not silent:
            print path

        tools.dump_json(self.segments_result_data, path)

        tools.dump_json(self.segments_invnums_data,
                        os.path.join(out_dir, "segments_invnums_data_%s.json" % out_path_suffix))

    def format_clouds_sku(self, config_path=None, segments=None, tco_cases=None):
        if not tco_cases:
            tco_cases = self.base_config.get("tco_evaluation_modes")

        for case in self.base_config["tco_evaluation"]:
            if tco_cases and not case in tco_cases:
                continue

            out_path_suffix = "custom" if segments else "all"

            sys.stdout.write("TCO case %s:\n" % case)
            self._format_clouds_sku(
                config_path=config_path,
                segments=segments,
                tco_case=case,
                out_path_suffix=out_path_suffix
            )
            sys.stdout.write("\n")


def _init_csv_dialect(delimiter=","):
    dialect_name = {
        ";": "my_semic",
        ",": "my_comma",
        "\t": "my_tab",
    }[delimiter]

    dialect = csv.excel()
    dialect.delimiter = delimiter

    csv.register_dialect(dialect_name, dialect)

    return dialect_name


def _load_csv_table(data, names_dict=None, delimiter=","):
    if names_dict is None:
        names_dict = {}

    lines = list(csv.reader(data, dialect=_init_csv_dialect(delimiter=delimiter)))
    header = [el.strip() for el in lines[0]]
    header_len = len(header)

    res = []

    for line in lines[1:]:
        temp = {}
        for num, val in enumerate(line):
            val = val.strip()
            if num >= header_len:
                continue

            name = header[num]
            name = names_dict.get(name, name)

            temp[name] = val

        res.append(temp)

    return res


def _save_csv_table(stream, data, header=None):
    header = header if header else []
    for row in data:
        for key in row.keys():
            if not key in header:
                header.append(key)

    writer = csv.DictWriter(stream, fieldnames=header)

    writer.writeheader()
    for row in data:
        writer.writerow(row)


def _load_hosts_tco(config):
    path = config["path"]
    mode = config["D&A mode"]
    tvm = config.get("TVM")
    tvm_add = None
    if path == "tco_servers_v3.csv":
        cases = {
            "Servers D&A": "DA%s_%%s_TTL%s" % (("_" + mode) if mode != "4Y" else "", "_TVM" if tvm else ""),
            "DC TCO": "TCO_%s"
        }
    elif path == "tco_servers_v2.csv":
        tvm_add = config.get("TVM_add")
        cases = {
            "Servers D&A": "%%s_D&A_%s_ttl" % mode,
            "DC TCO": "%s_TCO"
        }
    else:
        assert False

    data = file(path).read()
    data = data.replace("\0", "").lower().split("\n")

    rows = _load_csv_table(data)

    invnum_to_resources = {}

    for row in rows:
        inv = row["inv"]
        if not inv:
            continue

        if not inv.isdigit():
            sys.stderr.write("invalid invnum: %s\n" % inv)
            continue

        temp = defaultdict(float)

        for resource in ELEMENTARY_RESOURCES:
            for case, formatter in cases.iteritems():
                name = (formatter % resource).lower()
                val = row[name]
                if val in ["-", "no_data"]:
                    continue

                temp[(resource, case)] += float(val)
                if case == "Servers D&A" and tvm_add:
                    temp[(resource, "Servers D&A TVM")] += float(val) * tvm_add

        invnum_to_resources[inv] = temp

    return invnum_to_resources


def _load_servers_years(path):
    data = file(path).read()
    data = data.replace("\0", "").lower().split("\n")

    rows = _load_csv_table(data)

    res = {}
    for row in rows:
        inv = row["inv"]
        if not inv:
            continue

        year = row.get("sy")
        if year:
            res[inv] = year

    return res


def join_elementary_resources(data):
    res = defaultdict(float)
    for key, val in data.iteritems():
        if isinstance(key, (tuple, list)):
            key = key[0]
        res[key] += val

    return res


def diff(path):
    new_inv = set(file(path).read().split())

    lost_path = "diff_lost.txt"
    new_path = "diff_new.txt"
    lost_out = file(lost_path, "wb")
    new_out = file(new_path, "wb")

    rtc_inv = []
    for line in file("rtc_sorted.txt"):
        inv, segment = line.split()
        if inv not in new_inv:
            new_out.write(line)

        rtc_inv.append(inv)

    lost_inv = new_inv.difference(rtc_inv)
    lost_out.write("\n".join(sorted(lost_inv)) + "\n")

    print lost_path
    print new_path


def load_groups_to_hosts():
    command = "./utils/common/show_machine_types.py --no-background"
    print command
    data = subprocess.check_output(command, shell=True)

    res = {}
    for line in data.split("\n"):
        if not line.strip():
            continue

        group = line.split()[0]
        hosts = line.split(":")[1].replace(",", " ").split()
        res[group] = hosts

    return res


def check_classification():
    good_clouds = "gencfg yp qlod arnold".split()
    good_invnums = []

    for line in file("rtc_sorted.txt"):
        invnum, cloud = line.split()
        if cloud in good_clouds:
            good_invnums.append(invnum)

    good_invnums = set(good_invnums)

    groups_to_hosts = load_groups_to_hosts()

    host_to_walle, fqdn_to_invnum = _load_hosts_data()

    diff_stat_path = "diff_stat.txt"
    diff_list_path = "diff_list.txt"
    diff_stat_out = file(diff_stat_path, "wb")
    diff_list_out = file(diff_list_path, "wb")

    for group, hosts in sorted(groups_to_hosts.iteritems()):
        invnums = [fqdn_to_invnum[host] for host in hosts]

        if not good_invnums.intersection(invnums):
            continue
        diff = set(invnums).difference(good_invnums)

        if not diff:
            continue

        diff_stat_out.write("%s\t%s / %s\n" % (group, len(diff), len(invnums)))
        diff_list_out.write("%s\t%s\n" % (group, ",".join(sorted(diff))))

    print diff_stat_path
    print diff_list_path


def new_quota_diff_gencfg(path):
    proc_list = set([el.split()[3] for el in file(path).readlines()])

    def read_total(command):
        data = os.popen(command).readlines()

        last = data[-1].lower()
        assert last.startswith("total:")

        return float(last.split()[1])

    command_old = "cat %s | grep -w %s | ./simple_evaluate.sh"
    command_new = "cat %s | grep -w %s | ./calculate_yp_quota_for_hosts/calculate_yp_quota_for_hosts --dump"

    res = []

    for proc in proc_list:
        old = read_total(command_old % (path, proc))
        new = read_total(command_new % (path, proc))

        res.append((proc, old, new, old - new))

    sys.stdout.write("#|\n")
    sys.stdout.write("||proc|old|new|diff|perc diff||\n")
    for proc, old, new, diff in sorted(res, key=lambda x: -x[-1]):
        sys.stdout.write("||%s||\n" % "|".join(map(str, (proc, old, new, old - new, int((old - new) * 100 / old)))))
    sys.stdout.write("|#\n")


def new_quota_diff_yp(path):
    dc_list = set()
    all_hosts = set()
    for line in file(path).readlines():
        line = line.split()
        if line[0] == "unknown":
            continue

        dc_list.add(line[0])
        all_hosts.add(line[2])

    print "run first:"
    print
    print "./update.sh && ./reserves.sh &"
    for dc in dc_list:
        command = "ya tool yp_util nodes-resources --segment default --cluster {dc} --format json --no-pretty-units > yp_hosts_data_{dc}.txt &"
        print command.format(dc=dc)
    print "time wait"
    print

    def load_yp_data(path):
        data = json.load(file(path))

        res = {}
        for el in data:
            if el["host"]:
                res[el["host"]] = float(el["total_vcpu"]) / 1000

        return res

    total = 0
    yp_hosts = set()
    print "yp data"
    for dc in dc_list:
        yp_data = load_yp_data("yp_hosts_data_{dc}.txt".format(dc=dc))
        yp_hosts.update(yp_data.keys())
        cores = int(sum([cpu for host, cpu in yp_data.iteritems() if host in all_hosts]))

        total += cores
        print dc, cores

    print "total", total
    print
    common_hosts = all_hosts.intersection(yp_hosts)

    tmp_path = "yp_hosts_filtered.txt"
    with file(tmp_path, "wb") as out:
        with file(path) as inp:
            for line in inp:
                if line.split()[2] in common_hosts:
                    out.write(line)

    command = "cat {path} | ./simple_evaluate.sh".format(path=tmp_path)
    print command
    os.system(command)
    print

    command = "cat {path} | ./calculate_yp_quota_for_hosts/calculate_yp_quota_for_hosts --dump".format(path=tmp_path)
    print command
    os.system(command)
    print

    command = "cat {path} | ./calculate_yp_quota_for_hosts/calculate_yp_quota_for_hosts --dump --no-discount".format(
        path=tmp_path)
    print command
    os.system(command)
    print

    print
    yp_lost = all_hosts.difference(yp_hosts)
    file("yp_lost_yp.txt", "wb").write("\n".join(yp_lost))
    print "yp_lost_yp.txt", len(yp_lost)

    walle_lost = yp_hosts.difference(all_hosts)
    file("yp_lost_walle.txt", "wb").write("\n".join(walle_lost))
    print "yp_lost_walle.txt", len(walle_lost)

    print "common_hosts", len(common_hosts)


def new_quota_diff(path, mode):
    if mode == "gencfg":
        return new_quota_diff_gencfg(path)
    elif mode == "yp":
        return new_quota_diff_yp(path)

    assert False


def hosts_efficiency_stat():
    config = _load_config()
    tco_case = config["tco_evaluation_modes"][0]
    tco_config = config["tco_evaluation"][tco_case]

    invnum_to_resources = _load_hosts_tco(
        config=tco_config
    )

    servers_years = _load_servers_years(path=tco_config["path"])

    tarif_data = json.load(file(os.path.join(RESULTS_DIR, "segments_results_all_%s.json" % tco_case.lower())))
    rates = tarif_data["rtc"]["sku_results"]["result_rate"]

    input_data = [line.split() for line in sys.stdin.readlines()]
    invnums = [line[1] for line in input_data]

    hosts_quota = _load_quota(invnums, return_hosts_data=True)

    sku_to_skip = ["hdd", "ssd"]
    prices_data = json.load(file(os.path.join(os.path.dirname(sys.argv[0]), "sku_evaluation_secret_prices.json")))
    port_cost = prices_data["per_host_cost"]["net_inter_dc"]["per_port_cost"]

    rows = []
    for line in input_data:
        inv = line[1]
        cpu = line[3]

        resources = invnum_to_resources.get(inv)
        if not resources:
            continue

        server_cost = sum(resources.itervalues()) + port_cost

        server_quota = hosts_quota.get(inv)
        server_quota = server_quota.total()

        temp = OrderedDict()
        temp["inv"] = inv
        temp["cpu_model"] = cpu
        temp["year"] = servers_years.get(inv, "")
        temp["server_cost"] = server_cost

        sum_price = 0
        for key, val in sorted(server_quota.iteritems()):
            if key in sku_to_skip:
                continue

            price = val * rates.get(key, 0)
            if price:
                sum_price += price
                temp[key] = val

        temp["sum_price"] = sum_price
        temp["efficiency"] = sum_price / server_cost if server_cost else 0

        rows.append(temp)

    _save_csv_table(sys.stdout, rows)


def evaluate_cost(resources, rates):
    res = defaultdict(int)
    for name, val in resources.total().iteritems():
        res[name] += rates.get(name, 0) * val * resources_counter.RESOURCES_RESERVE_TO_SKU.get(name, 1)

    return SimpleCounter(res)


def compare_abc_services_cost(prices, tarif_data, joined_quota, out):
    abcapi = abc_api.load_abc_api()

    compare_res = {}
    old_price_for_all = prices["old_rate"]

    for abc, resources in sorted(joined_quota.iteritems()):
        abc_res = {}
        abc_res["name"] = abcapi.get_service_name(abc)
        out.write(("%s (%s):\n" % (abcapi.get_service_name(abc), abc)).encode("utf-8"))

        segments_cost = _quota_to_segments_cost(prices, tarif_data, resources)
        total_new_cost = SimpleCounter()
        total_old_cost = SimpleCounter()

        for segment, segment_data in sorted(segments_cost.iteritems()):
            quota = segment_data["quota"]
            new_cost = segment_data["cost"]
            old_price = prices["segments"][segment].get("old_rate", old_price_for_all)
            old_cost = evaluate_cost(quota, old_price)

            total_old_cost += old_cost
            total_new_cost += new_cost

            abc_res["%s old cost" % segment] = sum(old_cost.itervalues())
            abc_res["%s new cost" % segment] = sum(new_cost.itervalues())

            out.write("%s quota: %s\n" % (segment, quota.format_total()))
            out.write("%s old cost:\n" % segment)
            _format_cost(out, old_cost)
            out.write("\n")
            out.write("%s new cost:\n" % segment)
            _format_cost(out, new_cost)
            out.write("\n")

        out.write("total quota: %s\n" % resources_counter._format_quota(resources.total()))
        out.write("total old cost:\n")
        _format_cost(out, total_old_cost)
        out.write("\n")
        out.write("total new cost\n")
        _format_cost(out, total_new_cost)
        out.write("\n")

        total_new_cost = sum(total_new_cost.itervalues())
        total_old_cost = sum(total_old_cost.itervalues())

        abc_res["total old cost"] = total_old_cost
        abc_res["total new cost"] = total_new_cost

        if total_old_cost > 0:
            out.write("diff: %s %%\n" % int((total_new_cost - total_old_cost) / total_old_cost * 100))

        out.write("\n")
        out.write("-------\n")
        out.write("\n")

        compare_res[abc] = abc_res

    return compare_res


def _quota_to_segments_cost(prices, tarif_data, quota):
    res = {}

    for segment, segment_settings in sorted(prices["segments"].iteritems()):
        filter_settings = {
            "cloud": segment_settings.get("clouds", []),
            "segment": segment_settings.get("segments", [])
        }

        rate_name = segment_settings["rate_name"]
        new_rate = tarif_data[rate_name]["sku_results"]["result_rate"]

        segment_quota = quota.filter(filter_settings)
        if not segment_quota:
            continue

        cost = evaluate_cost(segment_quota, new_rate)

        res[segment] = {
            "quota": segment_quota,
            "cost": cost,
        }

    return res


def _get_segment_prices(config, prices):
    res = {}
    for segment in config["quota_segments"]:
        res[segment] = prices["segments"][segment]

    return {
        "segments": res,
        "old_rate": prices["old_rate"]
    }


def _compare_tarifs_case(abcapi, config, prices, tarif_data, sum_services_quota, out):
    compare_res = {}

    out.write("preparing total quota\n")
    abc_services_to_ignore = set(config.get("abc_services_to_ignore", []))
    quota_for_segments = resources_counter.ResourcesCounter()
    total_ignored = resources_counter.ResourcesCounter()
    for abc_service, service_quota in sum_services_quota.iteritems():
        if abc_service in abc_services_to_ignore or abc_services_to_ignore.intersection(
                abcapi.get_service_parents(abc_service)):
            out.write("ignoring quota for abc service %s: %s\n" % (abc_service, service_quota.format_total()))
            total_ignored += service_quota
            continue

        quota_for_segments += service_quota

    out.write("total ignored: %s\n" % total_ignored.format_total())
    out.write("\n")

    total_price = SimpleCounter()

    segment_prices = _get_segment_prices(config, prices)

    segments_cost = _quota_to_segments_cost(segment_prices, tarif_data, quota_for_segments)

    total_quota = resources_counter.ResourcesCounter()

    for segment, segment_data in sorted(segments_cost.iteritems()):
        segment_quota = segment_data["quota"]
        total_quota += segment_quota

        price = segment_data["cost"]
        total_price += price

        out.write("%s:\n" % segment)
        out.write("quota: %s\n" % segment_quota.format_total())
        out.write("price:\n")
        _format_cost(out, price)
        out.write("\n")

    out.write("total\n")

    cost = 0
    total_hosts_resource = SimpleCounter()
    for segment in config["hardware_segments"]:
        cost += sum(tarif_data[segment]["elem_resources_corrected"].itervalues())
        total_hosts_resource += tarif_data[segment]["hosts_quota"]

    out.write("quota: %s\n" % total_quota.format_total())
    out.write("price:\n")
    _format_cost(out, total_price)
    sum_price = sum(total_price.itervalues())

    out.write("cost: %.1f M rub\n" % (cost / 1000 / 1000))
    out.write("diff: %s %%\n" % int(sum_price * 100 / cost - 100))
    out.write("\n")

    compare_res["total_price"] = total_price

    out.write("quota vs cloud resources\n")
    total_quota = total_quota.total()
    for resource in resources_counter.ordered_resources_names(total_hosts_resource):
        res_quota = total_quota[resource] * resources_counter.RESOURCES_RESERVE_TO_SKU.get(resource, 1)
        hosts_quota = total_hosts_resource.get(resource)
        if hosts_quota:
            out.write(
                "%s\t%s / %s == %.2f %%\n" % (
                    resource,
                    res_quota,
                    hosts_quota,
                    res_quota * 100 / hosts_quota
                ))

    return compare_res


def compare_tarifs_case(abcapi, config, case, joined_quota, sum_services_quota):
    tarif_data = json.load(file(os.path.join(RESULTS_DIR, "segments_results_%s.json" % case)))
    prices = config["prices"]

    path = os.path.join(RESULTS_DIR, "compare_abc_services_cost_%s.txt" % case)
    print path
    with file(path, "wb") as abc_out:
        services_data = compare_abc_services_cost(prices, tarif_data, joined_quota, out=abc_out)

    path = os.path.join(RESULTS_DIR, "rates_compare_%s.txt" % case)
    print path
    segments_data = {}
    with file(path, "wb") as out:
        for case_name, case_config in sorted(config["compare"]["cases"].iteritems()):
            out.write("%s compare\n" % case_name)

            data = _compare_tarifs_case(
                abcapi=abcapi,
                config=case_config,
                prices=prices,
                tarif_data=tarif_data,
                sum_services_quota=sum_services_quota,
                out=out
            )
            segments_data[case_name] = data

            out.write("\n------\n")

    compare_data = {
        "services": services_data,
        "segments": segments_data,
    }

    path = os.path.join(RESULTS_DIR, "rates_compare_%s.json" % case)
    print path
    with file(path, "wb") as out:
        tools.dump_json(compare_data, out=out)


def compare_rates(tco_cases=None):
    config = _load_config()

    sum_services_quota = pickle.load(file("resources_dump/sum_services_quota.pickle"))

    joined_quota = defaultdict(resources_counter.ResourcesCounter)
    abcapi = abc_api.load_abc_api()

    for abc_slug, resources in sum_services_quota.iteritems():
        if not abcapi.has_service_info(abc_slug):
            continue

        joined_quota[abcapi.get_service_vs_or_top(abc_slug)] += resources

    path = "sku_results_combinator/rtc_now.json"
    save_resources_for_combinator(path, sum_services_quota, date="now")
    print path

    if not tco_cases:
        tco_cases = config.get("tco_evaluation_modes")

    for case in config["tco_evaluation"]:
        if tco_cases and case not in tco_cases:
            continue

        compare_tarifs_case(
            abcapi,
            config=config,
            case="all_" + case.lower(),
            joined_quota=joined_quota,
            sum_services_quota=sum_services_quota
        )


def presentation():
    config = _load_config()

    out = sys.stdout

    out.write("**Elementary resources cost** (Hosts D&A + DC TCO + net + RTC HR)\n\n")
    for tco_case in sorted(config["tco_evaluation_modes"]):
        sku_data = json.load(file(os.path.join(RESULTS_DIR, "segments_results_all_%s.json" % tco_case.lower())))
        sku_compare_data = json.load(file(os.path.join(RESULTS_DIR, "rates_compare_all_%s.json" % tco_case.lower())))
        for segment in ["rtc", "rtc-gpu"]:
            cost = sku_data[segment]["elem_resources_corrected"]
            price = sku_compare_data["segments"][segment]["total_price"]

            out.write("%s %s:\n" % (tco_case, segment))
            out.write("cost:\n")
            _format_cost(out, cost)
            out.write("price:\n")
            _format_cost(out, price)

            cost = sum(cost.itervalues())
            price = sum(price.itervalues())
            if cost > 0:
                out.write("diff: %s %%\n" % int((price - cost) / cost * 100))

            out.write("\n")

    out.write("**SKU rates** (name, price, percent in summary bill)\n\n")
    for tco_case in sorted(config["tco_evaluation_modes"]):
        sku_data = json.load(file(os.path.join(RESULTS_DIR, "segments_results_all_%s.json" % tco_case.lower())))
        sku_compare_data = json.load(file(os.path.join(RESULTS_DIR, "rates_compare_all_%s.json" % tco_case.lower())))
        for segment in ["rtc", "rtc-gpu"]:
            price = sku_compare_data["segments"][segment]["total_price"]
            price_sum = sum(price.itervalues())
            rates = sku_data[segment]["sku_results"]["result_rate"]

            out.write("%s %s:\n" % (tco_case, segment))

            for name in resources_counter.ordered_resources_names(rates):
                out.write("%s: %s (%.2f %%)\n" % (
                    name,
                    _format_money(rates[name]),
                    price[name] * 100 / price_sum
                ))

            out.write("\n")

    out.write("**SKU cost sources**\n\n")
    for tco_case in sorted(config["tco_evaluation_modes"]):
        sku_data = json.load(file(os.path.join(RESULTS_DIR, "segments_results_all_%s.json" % tco_case.lower())))
        for segment in ["rtc"]:
            distribution = sku_data[segment]["sku_results"]["sku_cost_sources"]
            for resource, cost in sorted(distribution.iteritems()):
                out.write("%s:\n" % resource)
                _format_cost(out, cost)
                out.write("\n")

    out.write("**BU costs**\n\n")
    for tco_case in sorted(config["tco_evaluation_modes"]):
        sku_compare_data = json.load(file(os.path.join(RESULTS_DIR, "rates_compare_all_%s.json" % tco_case.lower())))

        bu_services = config["presentation"]["bu_services"]
        old_total = 0
        new_total = 0
        for service, service_data in sorted(sku_compare_data["services"].iteritems()):
            if not service in bu_services:
                continue

            name = service_data["name"]
            old_cost = service_data["total old cost"]
            new_cost = service_data["total new cost"]

            out.write(("%s (%s):\n" % (name, service)).encode("utf-8"))
            out.write("old cost %s\n" % _format_money(old_cost))
            out.write("new cost %s\n" % _format_money(new_cost))
            if old_cost:
                out.write("diff %s%%\n" % int((new_cost - old_cost) * 100 / old_cost))

            gencfg_new_cost = service_data.get("gencfg new cost", 0)
            if False and gencfg_new_cost:
                out.write("gencfg_perc\t%s\t%s%%\n" % (service, int(gencfg_new_cost * 100 / new_cost)))

            out.write("\n")

            old_total += old_cost
            new_total += new_cost

        out.write("total:\n")
        out.write("old cost %s\n" % _format_money(old_total))
        out.write("new cost %s\n" % _format_money(new_total))
        if old_total:
            out.write("diff %s%%\n" % int((new_total - old_total) * 100 / old_total))


def dump_bu_orders(reload=True):
    import dispenser

    abc_api_holder = abc_api.load_abc_api(reload=reload)

    dispenser_config_base = {
        "orders": {
            "runtime_clouds": [
                "qloud",
                "gencfg",
                "yp"
            ],
            "dispenser_campains": {
                "aug2018": False,
                "aug2019": True,
                "feb2020": True,
                "aug2020": True,
                "feb2021": False,
            }
        }
    }

    order_dates = [
        "2020",
        "2021-06-30",
        "2021-09-30",
        "2021-12-31"
    ]

    config = _load_config()
    bu_services = config["presentation"]["bu_services"]
    tco_case = config["tco_evaluation_modes"][0]

    sku_data = json.load(file(os.path.join(RESULTS_DIR, "segments_results_all_%s.json" % tco_case.lower())))

    rates = {}
    for segment in ["rtc", "rtc-gpu"]:
        rates.update(sku_data[segment]["sku_results"]["result_rate"])

    dispenser_data = dispenser.load_orders(reload=reload)

    for date in order_dates:
        dispenser_config = copy.deepcopy(dispenser_config_base)
        if date.startswith("2021"):
            dispenser_config["orders"]["order_dates"] = {
                date: True
            }
        else:
            dispenser_config["orders"]["dispenser_campains"]["aug2020"] = False

        orders_remaining, orders_ready = dispenser.get_dispenser_orders_resources_data(
            config=dispenser_config,
            data=dispenser_data,
            res_key="abc"
        )

        date_prefix = "-".join(date.split("-")[:2])
        path = "sku_results_combinator/rtc_orders_%s.json" % date_prefix
        save_resources_for_combinator(path, orders_remaining, date="order-" + date_prefix)

        bu_orders = defaultdict(resources_counter.ResourcesCounter)
        for abc_service, resources in orders_remaining.iteritems():
            top_abc_service = abc_api_holder.get_service_vs_or_top(abc_service)

            if top_abc_service not in bu_services:
                continue

            bu_orders[top_abc_service] += resources

        sys.stdout.write("%s:\n" % date)
        for service, resources in sorted(bu_orders.iteritems()):
            cost = evaluate_cost(resources, rates)

            sys.stdout.write("%s\t%s\t%s\n" % (
                service,
                _format_money(sum(cost.itervalues())),
                resources.format_total()
            ))

            if False:
                _format_cost(sys.stdout, cost, total=False)
                sys.stdout.write("\n")

        sys.stdout.write("\n")


def save_resources_for_combinator(path, resources, date):
    res = defaultdict(lambda: defaultdict(list))
    for abc, resource in resources.iteritems():
        for cloud, cloud_resource in resource.cloud_resources().iteritems():
            if cloud == "order":
                cloud = "yp"

            res[cloud]["sku_usage"].append(
                {
                    "abc": abc,
                    "date": date,
                    "sku": cloud_resource.total()
                }
            )

    tools.create_dir(os.path.dirname(path))

    tools.dump_json(res, path)


def _monthly_cost(date, cost, rollover=True):
    if not rollover:
        return {
            date: cost,
            "year": cost
        }

    if not date.startswith("2021-"):
        date = 0
    else:
        date = int(date.split("-")[1])

    res = {}
    for month in range(1, 13):
        if date <= month:
            res["2021-%02d" % month] = cost

    res["year"] = sum(res.values())

    return res


class CloudCombinator:
    def __init__(self, cloud, abc_api_holder, chosen_abc, price_mult):
        self.cloud = cloud
        self.abc_api_holder = abc_api_holder

        self.chosen_abc = chosen_abc
        self.price_mult = price_mult

        self.usage = defaultdict(lambda: defaultdict(lambda: defaultdict(float)))
        self.prices = {}

        self.unknown_sku = set()

        self.money = defaultdict(lambda: defaultdict(lambda: defaultdict(float)))

    def _top_abc_service(self, abc):
        if abc in self.chosen_abc:
            return abc

        for service in self.abc_api_holder.get_service_parents(abc):
            if service in self.chosen_abc:
                return service

        return self.abc_api_holder.get_service_vs_or_top(abc)

    def _join_by_abc(self, data):
        res = defaultdict(lambda: defaultdict(lambda: defaultdict(float)))

        for date, date_data in data.iteritems():
            for abc, abc_data in date_data.iteritems():
                for sku_name, sku_val in abc_data.iteritems():
                    top_abc = self._top_abc_service(abc)

                    res[date][top_abc][sku_name] += sku_val

        return res

    def _init_data_from_json(self, acceptor, data, mult=None):
        for el in data:
            abc = el["abc"]
            date = el["date"]
            if date.startswith("order-"):
                date = date.split("-", 1)[1]

            if date not in ["now", "2020"] and not date.startswith("2021"):
                sys.stderr.write("invalid date: %s\n" % date)

            for key, val in el["sku"].iteritems():
                if mult:
                    val *= mult

                acceptor[date][abc][key] += val

    def add_usage(self, data):
        self._init_data_from_json(self.usage, data)

    def add_prices(self, data, price_mult=1):
        for key, val in data.iteritems():
            old_price = self.prices.get(key)
            new_price = val * price_mult
            if old_price is not None and new_price != old_price:
                sys.stderr.write("Duplicate prices for %s.%s: %s vs %s\n" % (self.cloud, key, new_price, old_price))

            self.prices[key] = new_price

    def add_money(self, data, price_mult):
        self._init_data_from_json(self.money, data, mult=price_mult)

    def add_data(self, data):
        price_mult = data.get("price_mult", self.price_mult)

        self.add_usage(data.get("sku_usage", []))
        self.add_prices(data.get("sku_prices", {}), price_mult=price_mult)
        self.add_money(data.get("sku_result_money", []), price_mult=price_mult)

    def evaluate_money(self):
        for date, date_data in self.usage.iteritems():
            for abc, abc_data in date_data.iteritems():
                for sku_name, sku_val in abc_data.iteritems():
                    price = self.prices.get(sku_name)
                    if price:
                        self.money[date][abc][sku_name] += sku_val * price
                    elif price is None:
                        self.unknown_sku.add(sku_name)

        if self.unknown_sku:
            sys.stderr.write("Unknown sku for %s: %s\n" % (self.cloud, ",".join(self.unknown_sku)))

    def dump_resources_stat(self):
        res = []
        for date, date_data in self.money.iteritems():
            for abc, abc_data in date_data.iteritems():
                for sku_name, sku_cost in abc_data.iteritems():
                    sku_volume = self.usage.get(date, {}).get(abc, {}).get(sku_name, 0)

                    temp = OrderedDict()

                    temp["cloud"] = self.cloud
                    temp["date"] = date
                    temp["top_abc"] = self._top_abc_service(abc)
                    temp["abc"] = abc
                    temp["sku_name"] = sku_name
                    temp["sku_volume"] = sku_volume
                    temp["cost"] = sku_cost

                    res.append(temp)

        return res

    def budget_stat(self, abc_filter, rollover=True):
        res = SimpleCounter()

        for date, date_data in self._join_by_abc(self.money).iteritems():
            for abc, abc_data in date_data.iteritems():
                if abc not in abc_filter:
                    continue

                res += _monthly_cost(date=date, cost=sum(abc_data.itervalues()), rollover=rollover)

        res = dict([(key, int(val)) for key, val in res.iteritems()])

        return res


class ExcelCombinator:
    def __init__(self, abc_api_holder, price_mult=None):
        self.chosen_abc = _load_config()["presentation"]["bu_services"]

        self.abc_api_holder = abc_api_holder

        self.price_mult = price_mult

        self.clouds_data = {}

        paths = [
            "sku_results_combinator/rtc_prices.json",
            "sku_results_combinator/rtc_now.json",
            "sku_results_combinator/rtc_orders_2020.json",
            "sku_results_combinator/rtc_orders_2021-06.json",
            "sku_results_combinator/rtc_orders_2021-09.json",
            "sku_results_combinator/rtc_orders_2021-12.json",

            # "sku_results_combinator/noc_data.json",

            "sku_results_combinator/mdb.json",
            "sku_results_combinator/mds.json",

            "sku_results_combinator/yt_2020.json",
            "sku_results_combinator/yt_jan.json",
            "sku_results_combinator/yt_order.json",

            "sku_results_combinator/old_budget.json",
        ]

        for path in paths:
            self._load_clouds_data(path)

        for cloud, cloud_combinator in self.clouds_data.iteritems():
            cloud_combinator.evaluate_money()

    def _load_clouds_data(self, path):
        for cloud, cloud_data in json.load(file(path)).iteritems():
            cloud_combinator = self.clouds_data.get(cloud)
            if cloud_combinator is None:
                cloud_combinator = CloudCombinator(
                    cloud=cloud,
                    abc_api_holder=self.abc_api_holder,
                    chosen_abc=self.chosen_abc,
                    price_mult=self.price_mult,
                )
                self.clouds_data[cloud] = cloud_combinator

            cloud_combinator.add_data(cloud_data)

    def dump_resources_stat(self, path):
        res = []

        for cloud, cloud_combinator in sorted(self.clouds_data.iteritems()):
            res.extend(cloud_combinator.dump_resources_stat())

        _save_csv_table(file(path, "wb"), res)
        print path

    def _dump_budget_stat(self, out, abc_filter):
        res = []

        total = defaultdict(int)
        total["case"] = "total"
        prev_total = None

        for cloud, cloud_combinator in sorted(self.clouds_data.iteritems()):
            cloud_stat = cloud_combinator.budget_stat(abc_filter=abc_filter, rollover=cloud != "old_budget")
            if sum(cloud_stat.itervalues()) == 0:
                continue

            cloud_stat["case"] = cloud

            if cloud == "old_budget":
                prev_total = cloud_stat
                continue

            for month, val in cloud_stat.iteritems():
                if month != "case":
                    total[month] += val

            res.append(cloud_stat)

        res.append(total)

        if prev_total:
            res.append(prev_total)
            diff = {
                "case": "diff"
            }
            for month, val in total.iteritems():
                if month != "case":
                    prev = prev_total.get(month, 0)
                    diff[month] = ((val - prev) * 100 / prev) if prev else 0
            res.append(diff)

        header = ["case"] + sorted([el for el in total.keys() if el != "case"])

        _save_csv_table(out, res, header=header)

    def dump_budget_stat(self, path):
        with file(path, "wb") as out:
            for abc in sorted(self.chosen_abc):
                out.write("%s\n" % abc)
                self._dump_budget_stat(out=out, abc_filter=[abc])
                out.write("\n")

        print path

    def dump_prices(self, path):
        res = {}
        for cloud, cloud_combinator in sorted(self.clouds_data.iteritems()):
            res[cloud] = cloud_combinator.prices

        tools.dump_json(res, path)
        print path

    def dump(self):
        self.dump_resources_stat("sku_results_combinator/resources_stat.csv")

        self.dump_budget_stat("sku_results_combinator/bu_budget.csv")

        self.dump_prices("sku_results_combinator/prices.json")


def prepare_rtc_prices():
    data = json.load(file("sku_results/segments_results_all_v2_4y_tvm.json"))
    clouds_map = {
        "rtc": ["yp", "gencfg"],
        "rtc-no-disks": ["qloud", "qloud-ext"],
        "rtc-gpu": ["yp"]
    }

    res = defaultdict(lambda: defaultdict(dict))

    for segment, clouds in clouds_map.iteritems():
        price = data[segment]["sku_results"]["result_rate"]
        for cloud in clouds:
            for name, val in price.iteritems():
                if not val:
                    continue
                res[cloud]["sku_prices"][name] = val * resources_counter.RESOURCES_RESERVE_TO_SKU.get(name, 1)

    res["yp"]["sku_prices"]["gpu"] = res["yp"]["sku_prices"]["gpu_tesla_v100"]
    res["yp"]["sku_prices"]["gpu_tesla_v100_nvlink"] = res["yp"]["sku_prices"]["gpu_tesla_v100"]
    res["yp"]["sku_prices"]["hdd_io"] = 0
    res["yp"]["sku_prices"]["ssd_io"] = 0

    tools.dump_json(res, "sku_results_combinator/rtc_prices.json")


def prepare_old_budget():
    data = file("sku_results_combinator/old_budget.csv").read()
    data = data.decode("utf-8-sig").encode("utf-8").replace("\0", "").split("\n")

    data = _load_csv_table(data)

    abc_map = {
        "Taxi": "taxi",
        "Classifieds": "verticals",
        "Market": "meta_market",
        "Media": "meta_media",
        "Geo": "meta_content",
        "Cloud": "cloud",
        "Education": "onlineeducation",
        "Zen": "discovery",
        "AV": "yandexsdc",
        "Investments": "fintech",
        "Edadeal": "edadeal",
    }

    res = []

    for row in data:
        abc = row["abc"]
        abc = abc_map[abc]

        for key, val in row.iteritems():
            if key == "abc":
                continue

            key = int(key)
            assert key > 0 and key < 13
            date = "2021-%02d" % key

            res.append({
                "abc": abc,
                "date": date,
                "sku": {
                    "total": int(val)
                }
            })

    res = {
        "old_budget": {
            "price_mult": 1,
            "sku_result_money": res,
        }
    }

    tools.dump_json(res, path="sku_results_combinator/old_budget.json")


def dump_bu_excel(debug=False):
    if not debug:
        prepare_rtc_prices()
    prepare_old_budget()

    price_mult = 1.04

    abc_api_holder = abc_api.load_abc_api()
    combinator = ExcelCombinator(abc_api_holder=abc_api_holder, price_mult=price_mult)
    combinator.dump()


def hosts_elementary_resources_cost():
    config = _load_config()
    tco_case = config["tco_evaluation_modes"][0]

    invnum_to_resources = _load_hosts_tco(
        config=config["tco_evaluation"][tco_case]
    )

    invnums = set(sys.stdin.read().replace(",", " ").split())

    resources_sum = SimpleCounter()
    lost_hosts = []
    for inv in invnums:
        inv_res = invnum_to_resources.get(inv)
        if inv_res is None:
            lost_hosts.append(inv)
        else:
            resources_sum += inv_res

    sys.stdout.write("total hosts: %s\n" % len(invnums))
    if lost_hosts:
        sys.stdout.write("lost hosts: %s\n" % (",".join(sorted(lost_hosts))))
    sys.stdout.write("\n")
    sys.stdout.write("Elementary resources:\n")
    _format_cost(sys.stdout, resources_sum)


def test():
    return


def main():
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("--classification", action="store_true", help="OBSOLETE: use --calculate")
    parser.add_option("--calculate", action="store_true", help="all sku evaluations")
    parser.add_option("--compare-rates", action="store_true", help="compare sku rates from --calculate")
    parser.add_option("--presentation", action="store_true", help="prepare data for presentation")
    parser.add_option("--bu-excel", action="store_true", help="prepare data for presentation")
    parser.add_option("--bu-orders", action="store_true", help="dump bu orders")

    parser.add_option("--config", help="config path. use stdin to read config from stdin")
    parser.add_option("--segments", help="coma separated chosen segments")
    parser.add_option("--tco", help="coma separated chosen tco cases")

    parser.add_option("--stat", action="store_true")
    parser.add_option("--diff")

    parser.add_option("--check", action="store_true")
    parser.add_option("--new-quota-diff")
    parser.add_option("--hosts-efficiency", action="store_true")
    parser.add_option("--hosts-elementary-resources-cost", action="store_true")

    parser.add_option("--debug", action="store_true")
    parser.add_option("--test", action="store_true")

    (options, args) = parser.parse_args()

    tco_cases = options.tco.upper().replace(",", " ").split() if options.tco else None

    if options.calculate or options.classification:
        classification(
            debug=options.debug,
            config_path=options.config,
            segments=options.segments,
            tco_cases=tco_cases
        )
    if options.compare_rates:
        compare_rates(tco_cases=tco_cases)
    if options.presentation:
        presentation()
    if options.bu_orders:
        dump_bu_orders()
    if options.bu_excel:
        dump_bu_excel(debug=options.debug)

    if options.stat:
        stat()
    if options.diff:
        diff(options.diff)

    if options.new_quota_diff:
        new_quota_diff(path=args[0], mode=options.new_quota_diff)
    if options.hosts_efficiency:
        hosts_efficiency_stat()

    if options.check:
        check_classification()

    if options.hosts_elementary_resources_cost:
        hosts_elementary_resources_cost()

    if options.test:
        test()


if __name__ == "__main__":
    main()
