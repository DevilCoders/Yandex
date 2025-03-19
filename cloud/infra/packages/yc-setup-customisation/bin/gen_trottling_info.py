#!/usr/bin/env python3

import argparse
import json
import sys

import yaml

from ycinfra import (
    write_file_content,
)

COMPUTE_TOPOLOGY_FILE = "/etc/yc/infra/compute_cpu_topology.yaml"
NBS_THROTTLING_FILE = "/etc/yc/infra/nbs-throttling.json"


def get_compute_cpus_num() -> int:
    try:
        with open(COMPUTE_TOPOLOGY_FILE) as f:
            topology = yaml.safe_load(f)
            compute_cpu_list = topology.get("compute")
            if not compute_cpu_list:
                raise Exception(
                    "Unexpected format in '{}' file. Key 'compute' not found!".format(COMPUTE_TOPOLOGY_FILE))
            return len(compute_cpu_list)
    except OSError as ex:
        raise Exception("Cannot read '{}' file: {}".format(COMPUTE_TOPOLOGY_FILE, ex))


class Interface:
    def __init__(self, name) -> None:
        self._name = name
        self._speed = self._get_speed()

    def _get_speed(self):
        speed_file = "/sys/class/net/{}/speed".format(self._name)
        try:
            with open(speed_file) as f:
                return f.readline().strip()
        except OSError as ex:
            raise Exception("Cannot read '{}' file: {}".format(speed_file, ex))

    @property
    def speed(self) -> str:
        return self._speed


def parse_args():
    parser = argparse.ArgumentParser(description='Generate trottling info')
    parser.add_argument("--ifaces", type=str, nargs="+", help="List of iterfaces to collect info about")

    args = parser.parse_args()
    if args.ifaces is None:
        parser.print_help()
        sys.exit(1)

    return args


def main():
    args = parse_args()

    n_compute_cpus = get_compute_cpus_num()
    trottling_info = {
        "compute_cores_num": n_compute_cpus,
        "interfaces": [],
    }

    # for now working only with eth0
    for iface_name in args.ifaces:
        iface = Interface(iface_name)
        trottling_info["interfaces"].append({iface_name: {"speed": iface.speed}})

    try:
        write_file_content(NBS_THROTTLING_FILE, json.dumps(trottling_info))
    except OSError as ex:
        print(ex)
        sys.exit(1)


if __name__ == "__main__":
    main()
