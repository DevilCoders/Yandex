#!/usr/bin/env python3

import argparse
import itertools
import logging
import re
import socket
import sys
from collections import defaultdict

import yaml

from ycinfra import (
    InventoryApi,
    write_file_content,
)

"""
CPU         PHYS_CORES(2 NUMAS)

Gold 6338   64
Gold 6230R  52
Gold 6230   40
Gold 6354   36
E5-2683 v4  32
E5-2660 v4  28
E5-2690 v4  28
E5-2667 v4  16
E5-2660     16
E5-2650 v2  16
EPYC 7502   64
EPYC 7452   64
EPYC 7662   128
EPYC 7702   128
EPYC 7713   128
"""

RESOURCE_TEMPLATES = {
    "Gold 6354": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                  {"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 2, "numas": [0]},
                  {"type": "compute", "phys_cores": 26, "numas": [0, 1]}],
    "Gold 6338": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                  {"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 2, "numas": [0]},
                  {"type": "compute", "phys_cores": 54, "numas": [0, 1]}],
    "Gold 6230 without YDB": [{"type": "system", "phys_cores": 1, "numas": [0]},
                              {"type": "network", "phys_cores": 2, "numas": [0]},
                              {"type": "nbs", "phys_cores": 4, "numas": [1]},
                              {"type": "compute", "phys_cores": 33, "numas": [0, 1]}],
    "Gold 6230R without YDB": [{"type": "system", "phys_cores": 1, "numas": [0]},
                               {"type": "network", "phys_cores": 2, "numas": [0]},
                               {"type": "nbs", "phys_cores": 4, "numas": [1]},
                               {"type": "compute", "phys_cores": 45, "numas": [0, 1]}],
    "Gold 6230R": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                   {"type": "system", "phys_cores": 1, "numas": [0]},
                   {"type": "network", "phys_cores": 2, "numas": [0]},
                   {"type": "nbs", "phys_cores": 2, "numas": [0]},
                   {"type": "compute", "phys_cores": 42, "numas": [0, 1]}],
    "Gold 6230": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                  {"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 2, "numas": [0]},
                  {"type": "compute", "phys_cores": 30, "numas": [0, 1]}],
    "E5-2683 v4": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                   {"type": "system", "phys_cores": 1, "numas": [0]},
                   {"type": "network", "phys_cores": 2, "numas": [0]},
                   {"type": "nbs", "phys_cores": 2, "numas": [0]},
                   {"type": "compute", "phys_cores": 22, "numas": [0, 1]}],
    "E5-2660 v4": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                   {"type": "system", "phys_cores": 1, "numas": [0]},
                   {"type": "network", "phys_cores": 2, "numas": [0]},
                   {"type": "nbs", "phys_cores": 2, "numas": [0]},
                   {"type": "compute", "phys_cores": 18, "numas": [0, 1]}],
    "E5-2690 v4": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                   {"type": "system", "phys_cores": 1, "numas": [0]},
                   {"type": "network", "phys_cores": 2, "numas": [0]},
                   {"type": "nbs", "phys_cores": 2, "numas": [0]},
                   {"type": "compute", "phys_cores": 18, "numas": [0, 1]}],
    "E5-2667 v4": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                   {"type": "system", "phys_cores": 1, "numas": [0]},
                   {"type": "network", "phys_cores": 1, "numas": [0]},
                   {"type": "nbs", "phys_cores": 2, "numas": [0]},
                   {"type": "compute", "phys_cores": 7, "numas": [0, 1]}],
    "E5-2660": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                {"type": "system", "phys_cores": 1, "numas": [0]},
                {"type": "network", "phys_cores": 1, "numas": [0]},
                {"type": "nbs", "phys_cores": 2, "numas": [0]},
                {"type": "compute", "phys_cores": 7, "numas": [0, 1]}],
    "E5-2650 v2": [{"type": "kikimr", "phys_cores": 4, "numas": [1]},
                   {"type": "system", "phys_cores": 1, "numas": [0]},
                   {"type": "network", "phys_cores": 1, "numas": [0]},
                   {"type": "nbs", "phys_cores": 1, "numas": [0]},
                   {"type": "compute", "phys_cores": 9, "numas": [0, 1]}],
    "EPYC 7502": [{"type": "kikimr", "phys_cores": 5, "numas": [1]},
                  {"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 2, "numas": [0]},
                  {"type": "compute", "phys_cores": 54, "numas": [0, 1]}],
    "EPYC 7452": [{"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 4, "numas": [1]},
                  {"type": "compute", "phys_cores": 57, "numas": [0, 1]}],
    "EPYC 7662": [{"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 4, "numas": [1]},
                  {"type": "compute", "phys_cores": 121, "numas": [0, 1]}],
    "EPYC 7702": [{"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 4, "numas": [1]},
                  {"type": "compute", "phys_cores": 121, "numas": [0, 1]}],
    "EPYC 7713": [{"type": "system", "phys_cores": 1, "numas": [0]},
                  {"type": "network", "phys_cores": 2, "numas": [0]},
                  {"type": "nbs", "phys_cores": 4, "numas": [1]},
                  {"type": "compute", "phys_cores": 121, "numas": [0, 1]}],
}

CPUINFO = "/proc/cpuinfo"
COMPUTE_TOPOLOGY_FILE = "/etc/yc/infra/compute_cpu_topology.yaml"
logging.basicConfig(level=logging.ERROR, format="%(levelname)s - %(message)s")


def parse_args():
    parser = argparse.ArgumentParser(description='Isolate threads')
    subparsers = parser.add_subparsers(dest="action")

    show = subparsers.add_parser('show', help="Show Isolate Threasd")
    show.add_argument('--type', required=True, action='store',
                      help='show Isolated threads: '
                           'all,'
                           'network,'
                           'compute,'
                           'kikimr,'
                           'nbs,'
                           'irqaffinity,'
                           'isolcpus,'
                           'node0_system(node0),'
                           'node1_system(node1),'
                           'node0_service,'
                           'node1_service'
                      )
    show.add_argument('--string', action='store_true', help='show Isolate Threads in string view')

    subparsers.add_parser('generate', help="Generate CPU topology file for host")

    args = parser.parse_args()
    if args.action is None:
        parser.print_help()
        sys.exit(1)

    return args


class Threads:
    KIKIMR = "kikimr"
    NBS = "nbs"
    NETWORK = "network"
    SYSTEM = "system"
    COMPUTE = "compute"

    IRQAFFINITY = "irqaffinity"
    ISOLCPUS = "isolcpus"
    SYSTEMDAFFINITY = "systemdaffinity"
    NODE0 = "node0"
    NODE1 = "node1"
    NODE0SYSTEM = "node0_system"
    NODE1SYSTEM = "node1_system"
    NODE0SERVICE = "node0_service"
    NODE1SERVICE = "node1_service"
    ALL = "all"

    ALL_THREADS = [
        KIKIMR,
        NBS,
        NETWORK,
        SYSTEM,
        COMPUTE,
        IRQAFFINITY,
        ISOLCPUS,
        SYSTEMDAFFINITY,
        NODE0,
        NODE1,
        NODE0SYSTEM,
        NODE1SYSTEM,
        NODE0SERVICE,
        NODE1SERVICE,
        ALL,
    ]


class AvailableResources:
    CPU_MODEL_PATTERN = re.compile(r"(E\d{1}-|Gold\ |EPYC\ )(\d{4}|-\d{4})(\ v\d{1})?")

    def __init__(self, src):
        self.src = src

    def _describe_cpus(self):
        src = self.src
        cpus_list = []
        with open(src, "r") as cpuinfo:
            data = cpuinfo.read()
            for cpu_descr in data.split("\n\n"):
                cpu_descr_dict = {}
                if cpu_descr:
                    for cpu_descr_line in cpu_descr.split("\n"):
                        key, val = cpu_descr_line.split(":")
                        if val.lstrip().isdigit():
                            cpu_descr_dict[key.strip()] = int(val.lstrip())
                        else:
                            cpu_descr_dict[key.strip()] = val.lstrip()
                    cpus_list.append(cpu_descr_dict)
        return cpus_list

    def get_cpu_model(self):
        cpu_platform_from_api = InventoryApi().get_cpu_platform(socket.gethostname())
        if cpu_platform_from_api:
            return cpu_platform_from_api
        cpus_list = self._describe_cpus()
        for model_description in cpus_list[0]:
            if model_description == "model name":
                return self.CPU_MODEL_PATTERN.search(cpus_list[0][model_description]).group()
        return None

    def get_threads_scheme(self):
        threads_scheme = {}
        for key, group in itertools.groupby(
                sorted(self._describe_cpus(), key=lambda x: (x["physical id"], x["core id"])),
                lambda x: (x["physical id"], x["core id"])):
            threads = []
            for thread in group:
                threads.append(thread["processor"])
            threads_scheme[key] = threads
        return threads_scheme


class RequiredResources:
    CPU = AvailableResources(CPUINFO)

    def __init__(self, resource_templates):
        self.resource_templates = resource_templates

    def _get_required_template(self):
        if self.CPU.get_cpu_model() not in self.resource_templates:
            logging.error("I can't find '%s' CPU model in templates", self.CPU.get_cpu_model())
            sys.exit(1)
        return self.resource_templates[self.CPU.get_cpu_model()]

    def _select_threads(self):
        selected_threads = defaultdict(list)
        require_cores = self._get_required_template()
        threads_scheme = self.CPU.get_threads_scheme()
        for cores_description in require_cores:
            reserved_threads = 0
            for _ in range(cores_description["phys_cores"]):
                for numa in cores_description["numas"]:
                    if len(cores_description["numas"]) > 1:
                        if reserved_threads == cores_description["phys_cores"]:
                            break
                    try:
                        threads_pair = sorted(threads_scheme.keys(), key=lambda x: (
                            x[0] not in cores_description["numas"], x[0] != numa, x[1]))[0]
                        threads = threads_scheme.pop(threads_pair)
                        reserved_threads += 1
                        selected_threads[cores_description["type"]].extend(threads)
                    except IndexError as err:
                        logging.error("You have wrong template")
                        raise err

        nodes = {
            "node0": [],
            "node1": [],
            "node0_system": [],
            "node1_system": [],
            "node0_service": [],
            "node1_service": [],
        }
        for threads_type in require_cores:
            if threads_type["type"] == Threads.COMPUTE:
                continue
            nodes["node{}".format(threads_type["numas"][0])].extend(selected_threads[threads_type["type"]])
            nodes["node{}_system".format(threads_type["numas"][0])].extend(selected_threads[threads_type["type"]])
            if threads_type["type"] != Threads.NETWORK:
                nodes["node{}_service".format(threads_type["numas"][0])].extend(selected_threads[threads_type["type"]])
            selected_threads.update(nodes)
        return selected_threads

    def get_req_cores(self, threads_type, string=False):
        selected_threads = self._select_threads()
        if threads_type in Threads.ALL_THREADS:
            if threads_type == "all":
                selected_threads_list = selected_threads
            elif threads_type == "irqaffinity":
                selected_threads_list = selected_threads["system"] + selected_threads["kikimr"] + selected_threads[
                    "nbs"]
            elif threads_type == "isolcpus":
                selected_threads_list = selected_threads["compute"] + selected_threads["network"]
            elif threads_type == "systemdaffinity":
                selected_threads_list = selected_threads["system"] + selected_threads["kikimr"] + selected_threads[
                    "nbs"]
            elif threads_type in ["node0", "node0_system"]:
                selected_threads_list = selected_threads["node0_system"]
            elif threads_type in ["node1", "node1_system"]:
                selected_threads_list = selected_threads["node1_system"]
            elif threads_type == "node0_service":
                selected_threads_list = selected_threads["node0_service"]
            elif threads_type == "node1_service":
                selected_threads_list = selected_threads["node1_service"]
            else:
                selected_threads_list = selected_threads[threads_type]
            if string:
                return ",".join([str(threads) for threads in selected_threads_list])
            return selected_threads_list

        logging.error("Incorrect Isolate threads type: %s", threads_type)
        sys.exit(1)

    def generate_compute_cpu_topology(self):
        """
        compute: X
        ....
        irqaffinity: X
        node0_system: X
        node0_service: Y
        node1_system: X
        node1_service: Y
        """
        topology = {
            "compute": self.get_req_cores("compute"),
            "irqaffinity": self.get_req_cores("irqaffinity"),
            "isolcpus": self.get_req_cores("isolcpus"),
            "kikimr": self.get_req_cores("kikimr"),
            "nbs": self.get_req_cores("nbs"),
            "network": self.get_req_cores("network"),
            "node0_service": self.get_req_cores("node0_service"),
            "node0_system": self.get_req_cores("node0_system"),
            "node1_service": self.get_req_cores("node1_service"),
            "node1_system": self.get_req_cores("node1_system"),
            "system": self.get_req_cores("system"),
            "systemdaffinity": self.get_req_cores("systemdaffinity"),
        }
        return yaml.dump(topology)


def main():
    args = parse_args()
    threads = RequiredResources(RESOURCE_TEMPLATES)

    if args.action == "show":
        print(threads.get_req_cores(args.type, args.string))
    elif args.action == "generate":
        compute_cpu_topology = threads.generate_compute_cpu_topology()
        try:
            write_file_content(COMPUTE_TOPOLOGY_FILE, compute_cpu_topology)
        except OSError as ex:
            print(ex)
            sys.exit(1)
    else:
        logging.error("no show args")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())
