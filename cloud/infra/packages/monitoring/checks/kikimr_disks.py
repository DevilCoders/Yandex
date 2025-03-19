#!/usr/bin/env python3

# disable logging from ycinfra lib
# because log messages in stderr are switched the Juggler check into the WARN state instead CRIT
import logging
import os
from collections import Counter
from typing import List, Optional

from yc_monitoring import (
    JugglerPassiveCheck,
    HwWathcherDisks,
    HwWatcherDbReadError,
    HwWatcherDbParseError
)
from ycinfra import (
    KikimrViewerApi,
    KikimrErrorConnectionProblemToCluster,
    KikimrErrorNodeDisconnected,
    KikimrErrorPDiskStateInfo,
    Lsblk,
    check_disk_vendor_model,
    DiskOwners,
)

logging.getLogger().setLevel(logging.CRITICAL)


class Disk(object):
    NVME_PREFIX = "nvme"
    HDD_PREFIX = "sd"
    # For NVME we have three variants of size(in bloks 512) nvmeXn1p1 (CLOUD-27144 RNDSVC-308)
    # 3123671040
    # 3125624832
    # 3123527680
    # We get lesser of three - 3123527680
    # CLOUD-37756
    # For HDD we have two diff sizes
    # 23435814912
    # 23435671552
    MIN_SIZE_TRESHOLD_BLOCKS = {HDD_PREFIX: 23435671552, NVME_PREFIX: 3123527680}

    @staticmethod
    def get_disk_partition(labels: List[str]) -> List[str]:
        partitions = []
        for label in labels:
            full_label_path = "{}/{}".format(DiskOwners.LABEL_PATH, label)
            partitions.append(os.path.basename(os.readlink(full_label_path)))
        return partitions

    def get_mismatch_size_partition(self, check: JugglerPassiveCheck, partitions: List[str]):
        mismatch_partition = []

        def _get_disk_size(disk_partition: str) -> Optional[int]:
            partition_size_file = "/sys/class/block/{}/size".format(disk_partition)
            try:
                with open(partition_size_file) as partition_size:
                    return int(partition_size.read().strip())
            except IOError:
                return None

        for partition in partitions:
            partition_size = _get_disk_size(partition)
            if not partition_size:
                check.crit("Error: Got error during get partition size {}".format(partition))
                continue
            if self.NVME_PREFIX in partition:
                reference_size = self.MIN_SIZE_TRESHOLD_BLOCKS[self.NVME_PREFIX]
            elif self.HDD_PREFIX in partition:
                reference_size = self.MIN_SIZE_TRESHOLD_BLOCKS[self.HDD_PREFIX]
            else:
                check.crit("Can't get reference size for partition {}".format(partition))
                continue
            if partition_size < reference_size:
                mismatch_partition.append(partition)
        return mismatch_partition


class KikimrDisks(object):
    ALL = [DiskOwners.KIKIMR_NVME_LABEL_PATTERN, DiskOwners.KIKIMR_ROT_LABEL_PATTERN]

    NVME_LABELS = {
        "NVMEKIKIMR01": "nvme0n1p1",
        "NVMEKIKIMR02": "nvme1n1p1",
        "NVMEKIKIMR03": "nvme2n1p1",
        "NVMEKIKIMR04": "nvme3n1p1",
        "NVMEKIKIMR05": "nvme4n1p1",
        "NVMEKIKIMR06": "nvme5n1p1",
    }

    LABELS = {
        "4hdd": {
            "ROTKIKIMR01": "sdc1",
            "ROTKIKIMR02": "sdd1",
            **NVME_LABELS,
        },
        "default": {
            "ROTKIKIMR01": "sdb1",
            "ROTKIKIMR02": "sdc1",
            "ROTKIKIMR03": "sde1",
            "ROTKIKIMR04": "sdf1",
            **NVME_LABELS,
        }
    }

    @staticmethod
    def get_kikimr_labeled_partition() -> List[str]:
        kikimr_labels = list(filter(lambda x: x[:-2] in KikimrDisks.ALL, os.listdir(DiskOwners.LABEL_PATH)))
        if not kikimr_labels:
            return []
        return kikimr_labels

    def get_kikimr_labeled_partition_full_path(self) -> List[str]:
        return [os.path.join(DiskOwners.LABEL_PATH, disk) for disk in self.get_kikimr_labeled_partition()]

    @staticmethod
    def get_kikimr_disks() -> List[str]:
        """
        Get list of partlabels in KIKIMR
        """
        kikimr = KikimrViewerApi()
        kikimr_disks = [os.path.basename(disk) for disk in kikimr.get_node_disks()]
        return kikimr_disks

    @staticmethod
    def get_blockdevices() -> dict:
        """
        Get dict of disks in system and their partlabels
        """
        raw_disks = Lsblk().all(columns=["KNAME", "PARTLABEL"])
        return {disk["kname"]: disk["partlabel"] for disk in raw_disks.get("blockdevices", {})}

    def get_mistmatch_kikimr_size_partition(self) -> List[str]:
        mistmatch_partition = []
        kikimr = KikimrViewerApi()
        kikimr_disks = self.get_kikimr_labeled_partition_full_path()
        kikimr_disk_sizes = kikimr.get_node_disks_size()
        for kikimr_disk in kikimr_disks:
            part_size = Lsblk.get_partsize(kikimr_disk)
            if kikimr_disk in kikimr_disk_sizes:
                if kikimr_disk_sizes[kikimr_disk]:
                    if not kikimr_disk_sizes[kikimr_disk] - part_size == 0:
                        mistmatch_partition.append(kikimr_disk)
        return mistmatch_partition

    def get_disk_by_partlabel(self, partlabel) -> str:
        return os.path.basename(os.readlink(os.path.join(DiskOwners.LABEL_PATH, partlabel)))

    def get_mistmatch_kikimr_partlabel(self, check: JugglerPassiveCheck):
        """
        Get disks which partlabels mistmatch required ones
        """
        wrong_disk_msg = "Partlabel '{}' is applied to wrong disk ({}), expected '{}'"
        try:
            local_disks = HwWathcherDisks()
            hdd_profile = "{}hdd".format(len(local_disks.get_hdd_disks()))
        except HwWatcherDbReadError:
            check.crit("hw-watcher database file cannot be read. \n\
Run 'sudo -u hw-watcher /usr/sbin/hw_watcher disk run' to restore it.")
            return
        except HwWatcherDbParseError:
            check.crit("hw-watcher database file has unexpected disks data format.")
            return
        expected_labels = self.LABELS.get(hdd_profile, self.LABELS.get("default"))
        if not expected_labels:
            check.crit("Unexpected disks profile '{}' found. Can't get expected disks labels".format(hdd_profile))
            return
        existing_labels_on_disks = {}
        # Check that all partlabels from 'by-partlabel' folder points to valid disk
        for partlabel in os.listdir(DiskOwners.LABEL_PATH):
            existing_labels_on_disks[partlabel] = self.get_disk_by_partlabel(partlabel)
            # Check that there are no more KIKIMR-like partlabels in directory
            if partlabel not in expected_labels:
                if partlabel.startswith(DiskOwners.KIKIMR_NVME_LABEL_PATTERN) or partlabel.startswith(DiskOwners.KIKIMR_ROT_LABEL_PATTERN):
                    check.warn("Found unknown KIKIMR partlabel '{}'".format(partlabel))
        for partlabel in expected_labels:
            if partlabel in existing_labels_on_disks and existing_labels_on_disks[partlabel] != expected_labels[partlabel]:
                check.crit(wrong_disk_msg.format(
                    partlabel,
                    existing_labels_on_disks[partlabel],
                    expected_labels[partlabel]
                ))

        # Check that partlabels belongs only to one disk at once
        real_disks = self.get_blockdevices()
        if not real_disks:
            check.crit("lsblk returned empty response")
        duplicate_labels_counter = Counter(real_disks.values())
        for partlabel in expected_labels:
            if duplicate_labels_counter[partlabel] > 1:
                check.crit("Partlabel '{}' is applied to several disks at once".format(partlabel))


def check_kikimr_disks(check: JugglerPassiveCheck):
    kikimr_labels = KikimrDisks().get_kikimr_labeled_partition()
    real_disks = Disk()
    real_kikimr_disks = real_disks.get_disk_partition(kikimr_labels)
    for disk in real_kikimr_disks:
        is_disk_valid, message = check_disk_vendor_model(disk)
        if not is_disk_valid:
            check.crit(message)
    mismatch_size_partitions = real_disks.get_mismatch_size_partition(check, real_kikimr_disks)
    if mismatch_size_partitions:
        check.crit("Next partitions have unsatisfactory size: {}".format(", ".join(mismatch_size_partitions)))

    failed_pdisks = None
    mistmatch_kikimr_size_partition = None
    try:
        failed_pdisks = KikimrViewerApi().get_node_disks(failed=True)
        mistmatch_kikimr_size_partition = KikimrDisks().get_mistmatch_kikimr_size_partition()
    except KikimrErrorConnectionProblemToCluster:
        check.crit("Lost connection to kikimr cluster")
    except KikimrErrorNodeDisconnected:
        check.crit("Node disconected from kikimr cluster")
    except KikimrErrorPDiskStateInfo:
        check.crit("Api response does not contain PDisks info")

    if failed_pdisks:
        check.crit("Kikimr lost next disks: {}".format(", ".join(failed_pdisks)))

    if mistmatch_kikimr_size_partition:
        check.crit("Kikimr disk and real partition size are mistmatch: {}".format(
            ", ".join(mistmatch_kikimr_size_partition)))

    KikimrDisks().get_mistmatch_kikimr_partlabel(check)


def main():
    check = JugglerPassiveCheck("kikimr_disks")
    try:
        check_kikimr_disks(check)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
