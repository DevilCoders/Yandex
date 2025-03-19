#!/usr/bin/env python3
import logging
import os
import socket
import sys
import json
from typing import Dict
from ycinfra import (
    InventoryApi,
    InfraProxyException,
    DiskOwners,
)
from ycinfra import Popen


class MalformedLsblkJsonError(Exception):
    def __init__(self, message="JSON is malformed"):
        self.message = message
        super().__init__(self.message)


class FieldMissingError(MalformedLsblkJsonError):
    def __init__(self, field):
        self.message = "Missing '{}' field in 'lsblk' JSON output".format(field)
        super().__init__(self.message)


def error_exit(msg: str, *args, **kwargs) -> None:
    print("ERROR:", msg.format(*args, **kwargs), file=sys.stderr)
    sys.exit(1)


def get_partitions_labels(disk: str) -> Dict[str, str]:
    """Run lsblk and check if disk not in raid and generate current disk"s config"""
    partitions = {}
    raw_dev_data = {}
    returncode, stdout, stderr = Popen().exec_command("lsblk -o NAME,PARTTYPE,PARTLABEL,FSTYPE,ROTA --json", retry=2)
    if returncode != 0:
        error_exit("Can't get list of block devices because of:\nstdout:\n{1}\nstderr:\n{2}", stdout, stderr)
    print("Successfully collected block devices list")
    if stdout:
        try:
            raw_dev_data = json.loads(stdout)
        except ValueError as ex:
            raise MalformedLsblkJsonError("Malformed json: {0}".format(ex))

    try:
        blockdevices = raw_dev_data["blockdevices"]
    except KeyError as ex:
        raise FieldMissingError(ex)

    for dev in blockdevices:
        try:
            disk_name = dev["name"]
        except KeyError as ex:
            raise FieldMissingError(ex)
        if disk_name.startswith("sr"):
            print("Ignoring CD-ROM {}".format(disk_name))
            continue
        if disk not in disk_name:
            continue
        try:
            print("Show current partition for '{}'".format(disk))
            disk_children = dev.get("children", [])
            if len(disk_children) == 0:
                partitions[disk_name] = None
            else:
                partitions[disk_name] = dev.get("children")[0].get("partlabel", "")
        except (KeyError, TypeError) as ex:
            raise FieldMissingError(ex)

    return partitions


def create_partition_label(disk: str, label_name: str) -> None:
    """ Run parted for creating part label """
    command = "parted /dev/{0} name 1 {1}".format(disk, label_name)
    pos_message = "Label {0} was successfully added to first partition of {1}".format(label_name, disk)
    err_message = "Can't create part label for first of {0} because of:".format(disk)
    returncode, stdout, stderr = Popen().exec_command(command, retry=5)
    if returncode != 0:
        error_exit("{0}\nstdout:\n{1}\nstderr:\n{2}", err_message, stdout, stderr)
    print(pos_message)


def generate_label_name(label_pattern: str, label_index: int) -> str:
    # return NVMENBS01
    return "{}{:02d}".format(label_pattern, label_index)


def generate_partition_name(disk: str, ns_index: int) -> str:
    # return: example nvme3n1
    return "{}n{}".format(disk, ns_index)


def trigger_udevd() -> None:
    """In case we're running in chroot environment under LUI, run our own udevd"""
    if os.path.isdir("/run/udev/data"):
        return

    returncode, stdout, stderr = Popen().exec_command(
        "/lib/systemd/systemd-udevd --daemon && udevadm trigger && udevadm settle", retry=2)
    if returncode != 0:
        print("Failed to build udev db:\nstdout:\n{0}\nstderr:\n{1}", stdout, stderr)
        return
    print("Successfully started udev daemon and built udev db")
    return


if __name__ == "__main__":
    try:
        disks_description = InventoryApi().get_disks_data(socket.gethostname())
    except InfraProxyException as ex:
        logging.error(ex)
        sys.exit(1)
    owners = DiskOwners.LABELED_OWNERS
    for disk_name, disk_description in sorted(disks_description.items()):
        disk_owner = disk_description.get("Owner")
        if disk_owner not in DiskOwners.ALL_LABELED_OWNERS:
            print("Disk '{}' has owner '{}', not '{}'".format(disk_name, disk_owner, DiskOwners.ALL_LABELED_OWNERS))
            continue
        trigger_udevd()
        any_changes = False
        try:
            current_partitions = get_partitions_labels(disk_name)
        except MalformedLsblkJsonError as ex:
            print("ERROR: {}".format(ex))
            sys.exit(1)
        for partition_index in range(1, len(disk_description.get("Namespaces")) + 1):
            partition = generate_partition_name(disk_name, partition_index)
            owners[disk_owner]["labels_number"] += 1
            label_index = owners[disk_owner]["labels_number"]
            label = generate_label_name(owners[disk_owner]["label_pattern"], label_index)
            if current_partitions[partition] == label:
                print("Partition '{}' has proper label '{}', so skip it..".format(partition, label))
                continue
            create_partition_label(partition, label)
            any_changes = True
        if any_changes:
            returncode, stdout, stderr = Popen().exec_command("partprobe && udevadm settle", retry=5)
            if returncode != 0:
                error_exit("Failed to update partitions:\nstdout:\n{0}\nstderr:\n{1}", stdout, stderr)
            print("Successfully updated partitions")
