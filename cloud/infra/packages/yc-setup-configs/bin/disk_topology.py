#!/usr/bin/env python3
import argparse
import logging
import socket
import sys
import os
import yaml
from glob import glob
from ycinfra import (
    InventoryApi,
    write_file_content,
    InfraProxyException,
    DiskOwners,
)

logging.basicConfig(level=logging.INFO, format="%(levelname)s - %(message)s")
COMPUTE_DISK_TOPOLOGY_FILE = "/etc/yc/infra/compute_disk_topology.yaml"
ALL_DISKS_TOPOLOGY_FILE = "/etc/yc/infra/disk_topology.yaml"


class TopologyGenerateException(Exception):
    pass


def generate_server_disk_topology():
    all_disks = []
    # glob results examples:
    # >> > glob("/dev/sd[a-z]")
    # ['/dev/sdf', '/dev/sde', '/dev/sdd', '/dev/sdc', '/dev/sdb', '/dev/sda']
    # >> > glob("/dev/nvme[0-9]")
    # ['/dev/nvme3', '/dev/nvme2', '/dev/nvme1', '/dev/nvme0']
    for disk in glob("/dev/sd[a-z]"):
        all_disks.append(os.path.split(disk)[-1])
    for disk in glob("/dev/nvme[0-9]"):
        all_disks.append(os.path.split(disk)[-1])
    logging.info("All disk topology: %s", all_disks)
    return yaml.dump(all_disks)


def generate_compute_disk_topology():
    """
    - nvme0n1
    - nvme1n1
    ....
    """
    compute_nvmes = []
    owned_disks_data = InventoryApi().get_disks_data(socket.gethostname())

    if not owned_disks_data:
        raise TopologyGenerateException
    try:
        label_counter = 0
        for _, disk in sorted(owned_disks_data.items()):
            if disk["Owner"] not in DiskOwners.COMPUTE:
                logging.info("Compute doesn't use disks with owner %s", disk["Owner"])
                continue
            for _ in disk["Namespaces"]:
                label_counter += 1
                compute_nvmes.append(
                    "{}/{}{:02d}".format(DiskOwners.LABEL_PATH, DiskOwners.COMPUTE_LABEL_PATTERN, label_counter))
        compute_nvmes = sorted(compute_nvmes)

    except KeyError:
        raise TopologyGenerateException

    logging.info("Compute disk topology: %s", compute_nvmes)
    return yaml.dump(compute_nvmes)


def parse_args():
    parser = argparse.ArgumentParser(description='disks topology')
    subparsers = parser.add_subparsers(dest="action")
    subparsers.add_parser('generate', help="Generate disks topology file for host")

    args = parser.parse_args()
    if args.action is None:
        parser.print_help()
        return None

    return args


def main():
    args = parse_args()
    if not args:
        sys.exit(1)
    if args.action == "generate":

        logging.info("Generating all disks topology")
        try:
            all_disk_topology = generate_server_disk_topology()
            logging.info("Writing all disks topology to file '%s'", ALL_DISKS_TOPOLOGY_FILE)
            try:
                write_file_content(ALL_DISKS_TOPOLOGY_FILE, all_disk_topology)
            except OSError as ex:
                print(ex)
                sys.exit(1)
        except Exception as err:
            logging.error("Got error while generate all disks topology: %s", err)
            sys.exit(1)

        logging.info("Generating compute disk topology")
        try:
            compute_disk_topology = generate_compute_disk_topology()
            logging.info("Writing topology to file '%s'", COMPUTE_DISK_TOPOLOGY_FILE)
            write_file_content(COMPUTE_DISK_TOPOLOGY_FILE, compute_disk_topology)
        except TopologyGenerateException:
            logging.error("Got error while generate compute disk topology")
            sys.exit(1)
        except InfraProxyException as ex:
            logging.error(ex)
            sys.exit(1)
        except Exception as ex:
            logging.error(ex)
            sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())
