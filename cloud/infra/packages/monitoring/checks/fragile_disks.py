#!/usr/bin/env python3


"""
Checks:
- number of disks/NSs in infra-proxy and in system should be equal
- size of disks/NSs in infra-proxy and in system should be equal
- symlinks for part-labels should be present in system for NBS disks
- number of disks/NSs in infra-proxy and in compute-node config should be equal for mdb/dedicated disks
"""

import collections
import json
import os
import re
import socket
import time

import yaml
from yc_monitoring import JugglerPassiveCheck

from ycinfra import (
    check_disk_vendor_model,
    InventoryApi,
    InfraProxyException,
    DiskOwners,
)

CACHE_PATH = "/var/tmp/infra_fragile_disks.cache"
CACHE_VALID_TIME = 10800  # 3 hours


class Disks:
    NVME_PIECES_NUMBER = 15
    SYS_BLOCK_PATH = "/sys/block/"
    DEV_PATH = "/dev/"
    ALLOWED_NVME_MIN_DISK_SIZES = (  # in blocks
        # Huawei disks (e.g. HWE32P43016M000N) have a different size from all other disks
        195312504,
        195312500,  # all other disks
    )
    NVME_DISK_PATTERN = re.compile(r"^(?P<nvme_disk_name>\w+)n\d{1,2}$")

    @classmethod
    def get_disk_size(cls, name):
        """
        Get disk size in blocks by 512 bytes
        """
        try:
            with open(os.path.join(cls.SYS_BLOCK_PATH, name, "size")) as fd:
                return int(fd.readline())
        except FileNotFoundError:
            return 0


class BadInventoryInformation(Exception):
    pass


class CacheExpired(Exception):
    pass


class InvalidCache(Exception):
    pass


class HostWithoutAnyDisks(Exception):
    pass


class BadComputeNodeConfig(Exception):
    pass


def read_json_cache(path, cache_valid_time):
    """
    Read information about disks from cache file
    """
    if os.stat(path).st_mtime + cache_valid_time < time.time():
        raise CacheExpired
    with open(path) as fd:
        try:
            cache_content = json.loads(fd.readline())
        except json.decoder.JSONDecodeError:
            raise InvalidCache
    if not cache_content:
        raise InvalidCache
    return cache_content


def write_json_cache(path, data):
    """
    Write information about disks to cache file
    """
    with open(path, "w+") as fd:
        try:
            write_data = json.dumps(data)
        except TypeError:
            write_data = None
        fd.write(write_data)


class ComputeNode:
    COMPUTE_NODE_CONFIG = "/etc/yc/compute-node/config.yaml"

    def get_nvme_disks(self):
        """
        Read compute-node config and get "nvme_disks" section
        """
        compute_nvme_disk_section = "nvme_disks"
        if not os.path.exists(self.COMPUTE_NODE_CONFIG):
            raise BadComputeNodeConfig
        try:
            with open(self.COMPUTE_NODE_CONFIG) as file_fd:
                compute_config = yaml.safe_load(file_fd)
        except yaml.YAMLError:
            raise BadComputeNodeConfig
        try:
            compute_nvme_disks = compute_config[compute_nvme_disk_section]
        except KeyError:
            raise BadComputeNodeConfig
        return sorted(compute_nvme_disks)


def get_disk_owner():
    """
    Get owners for all disk from infra-proxy
    """
    try:
        infra_proxy_disks = read_json_cache(CACHE_PATH, CACHE_VALID_TIME)
    except (CacheExpired, InvalidCache, OSError):
        infra_proxy_disks = InventoryApi().get_disks_data(host=socket.gethostname())
    if not infra_proxy_disks:
        raise HostWithoutAnyDisks
    for disk_prop in infra_proxy_disks.values():
        if disk_prop.get("Owner", "") != "":
            break
    else:
        raise HostWithoutAnyDisks
    write_json_cache(CACHE_PATH, infra_proxy_disks)
    disk_owners = {}
    for _, disk_prop in infra_proxy_disks.items():
        try:
            owner = disk_prop["Owner"]
        except KeyError:
            raise BadInventoryInformation
        if not disk_owners.get(owner):
            disk_owners[owner] = {}
        try:
            disk_nss = disk_prop["Namespaces"]
        except KeyError:
            raise BadInventoryInformation
        for ns_name, ns_prop in disk_nss.items():
            disk_owners[owner][ns_name] = ns_prop
    if not disk_owners:
        raise BadInventoryInformation
    return disk_owners


def get_main_owner(disk_owners):
    """
    Get main owner for fragile disks
    """
    disks_list = list(disk_owners.keys())
    if len(disks_list) == 2 and "" in disks_list:
        disks_list.remove("")
    if len(disks_list) == 1:
        return disks_list[0]
    raise BadInventoryInformation


def check_disk_in_system(disks):
    """
    Check disks in system:
    - all disks exist in system
    - any unnecessary disks don't exist in system
    """
    failed_disks = []
    unnecessary_disks = []
    api_disks_pattern = collections.Counter()
    for disk in disks:
        match = Disks.NVME_DISK_PATTERN.match(disk)
        api_disks_pattern[match.group("nvme_disk_name")] += 1
        disk_size = Disks.get_disk_size(disk)
        if not disk_size:
            failed_disks.append(disk)
    system_disks_pattern = collections.Counter()
    for system_disks in os.listdir(Disks.DEV_PATH):
        match = Disks.NVME_DISK_PATTERN.match(system_disks)
        if match:
            system_disks_pattern[match.group("nvme_disk_name")] += 1
    for disk_pattern, disk_count in api_disks_pattern.items():
        if system_disks_pattern[disk_pattern] != disk_count:
            unnecessary_disks.append(disk_pattern)
    if not failed_disks and not unnecessary_disks:
        return True, None
    # TODO(staerist): make this message more informative
    return False, "Inconsistency was found for disks in system and api"


def check_owner(owner, all_owners):
    """
    Check correct disk owner that we have got from infra-proxy
    """
    if owner not in all_owners:
        return False, "Unknown disk owner '{}'".format(owner)
    return True, None


def check_in_compute_node_config(disks, owner):
    """
    Compare disks from compute-node config and from infra-proxy
    """
    disks = list(map(lambda x: "/dev/{}".format(x), disks))
    compute_node_disks = ComputeNode().get_nvme_disks()
    # TODO(staerist):
    # It's maybe good idea use sets here and get symmetric_difference() between them
    if (disks == compute_node_disks and owner in DiskOwners.COMPUTE) or (not compute_node_disks and owner == DiskOwners.NBS):
        return True, None
    return False, "Disks from infra-proxy and compute-node config are not equal"


def check_symlink_for_nbs_disks():
    """
    Check symlinks for NBS disks in /dev/disk/by-partlabel
    """
    non_exists_labels = []
    for partition_number in range(1, Disks.NVME_PIECES_NUMBER + 1):
        partition_label = "{}{:02d}".format(DiskOwners.NBS_LABEL_PATTERN, partition_number)
        label_path = os.path.join(DiskOwners.LABEL_PATH, partition_label)
        if not os.path.exists(label_path):
            non_exists_labels.append(partition_label)

    if not non_exists_labels:
        return True, None
    return False, "Following labels were not found: {}".format(", ".join(non_exists_labels))


def check_disks_size(disks):
    """
    Check disks size with reference sizes for splitted NVME
    """
    failed_disks = []
    for disk in disks:
        size = Disks.get_disk_size(disk)
        if size < min(Disks.ALLOWED_NVME_MIN_DISK_SIZES):
            failed_disks.append(disk)
    if not failed_disks:
        return True, None
    return False, "Following disks: '{}' have wrong size".format(", ".join(failed_disks))


def check_fragile_disks(check: JugglerPassiveCheck):
    disk_owners = get_disk_owner()
    main_owner = get_main_owner(disk_owners)
    isOK, message = check_owner(main_owner, DiskOwners.FRAGILE)
    if not isOK:
        check.crit(message)
    disks = sorted(list(disk_owners[main_owner].keys()))
    if len(disks):
        # fragile disks could be located on one nvme only, so we check only the first disk.
        isOK, message = check_disk_vendor_model(disks[0])
        if not isOK:
            check.crit(message)
    isOK, message = check_disk_in_system(disks)
    if not isOK:
        check.crit(message)
    isOK, message = check_in_compute_node_config(disks, main_owner)
    if not isOK:
        check.crit(message)
    if main_owner == DiskOwners.NBS:
        isOK, message = check_symlink_for_nbs_disks()
        if not isOK:
            check.crit(message)
    if main_owner in DiskOwners.FRAGILE:
        isOK, message = check_disks_size(disks)
        if not isOK:
            check.crit(message)


def main():
    check = JugglerPassiveCheck("fragile_disks")
    try:
        check_fragile_disks(check)

    except HostWithoutAnyDisks:
        pass
    except BadInventoryInformation:
        check.crit("Got bad host information from infra-proxy")
    except BadComputeNodeConfig:
        check.crit("Got bad compute-node config")
    except InfraProxyException as ex:
        check.crit(ex)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
