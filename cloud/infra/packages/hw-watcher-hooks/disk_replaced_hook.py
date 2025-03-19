#!/usr/bin/env python

import sys
import logging
from ycinfra import (
    KikimrDisks,
    Systemctl,

    check_disk_vendor_model,
    determine_disk_owner,
    get_disk_namespaces_from_inventory,
    get_first_partition,
    get_kikimr_storage_cluster_id,
    new_device_object,
    prepare_nvme,
    pm_kikimr_disk_attach,
    reload_partition,
    check_huawei_firmware_version,
    HWWATCHER_RETRY_EXIT_CODE,
    SUCCESS_EXIT_CODE,
    ERROR_EXIT_CODE,
    DiskOwners,
)

logger = logging.getLogger()

HW_WATCHER_USER = "hw-watcher"

LOG_LEVEL = logging.DEBUG
LOG_FORMAT = "%(asctime)s - %(levelname)s - %(message)s"

NVME_HUAWEI_FW_MIN_VERSION = 3.0
NVME_HUAWEI_VENDOR_PREFIX = "HWE"


def main():
    console_handler = logging.StreamHandler()
    console_handler.setLevel(LOG_LEVEL)
    console_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(console_handler)
    logger.setLevel(LOG_LEVEL)

    try:
        hook_disk = sys.argv[1]
    except IndexError:
        logger.info("Disk for replacing is not specified")
        sys.exit(HWWATCHER_RETRY_EXIT_CODE)

    disk = new_device_object(hook_disk)
    if not disk.is_exists():
        logger.error("disk %s not found in system", disk.path)
        sys.exit(HWWATCHER_RETRY_EXIT_CODE)

    logger.info("I've got disk %s", disk.name)

    is_disk_valid, message = check_disk_vendor_model(hook_disk)
    if not is_disk_valid:
        logger.error(message)
        sys.exit(ERROR_EXIT_CODE)
    try:
        owner = determine_disk_owner(disk=disk, username=HW_WATCHER_USER)
    except Exception:
        logger.error("Exception raised while getting disk owner. Check log for details")
        sys.exit(HWWATCHER_RETRY_EXIT_CODE)
    if owner == DiskOwners.SYSTEM_RAID:
        logger.info("system disk replaced. Nothing to do.")
        sys.exit()
    if owner in [DiskOwners.MDB, DiskOwners.NBS, DiskOwners.DEDICATED, DiskOwners.NO_OWNER]:
        logger.info("%s disk replaced", owner)
        # check firmware version for huawei devices
        namespaces = get_disk_namespaces_from_inventory(disk.name)
        if not namespaces:
            logger.error("No namespaces were found for the disk %s", disk.name)
            sys.exit(ERROR_EXIT_CODE)
        if disk.model().startswith(NVME_HUAWEI_VENDOR_PREFIX):
            if len(namespaces) > 1 and not check_huawei_firmware_version(
                    disk.firmware_ver(), NVME_HUAWEI_FW_MIN_VERSION):
                logger.error("the old version of the firmware does not support the required number of namespaces")
                sys.exit(ERROR_EXIT_CODE)
        if not prepare_nvme(disk.name, namespaces, owner):
            logger.error("error for preparing nvme for %s", owner)
            sys.exit(ERROR_EXIT_CODE)
        sys.exit()
    if owner == DiskOwners.KIKIMR:
        logger.info("disk was replaced. Returning it to kikimr")
        label = KikimrDisks.get_full_label_path(disk)
        if disk.is_nvme:
            try:
                if not reload_partition(disk.namespaces()[0].path):
                    sys.exit(ERROR_EXIT_CODE)
            except IndexError:
                logger.info("no any namespace found on nvme. Can't reload partitions. Retry")
                sys.exit(ERROR_EXIT_CODE)
        else:
            if not reload_partition(disk.path):
                sys.exit(ERROR_EXIT_CODE)
        partition = get_first_partition(disk)
        if not KikimrDisks.check_partition_size(partition):
            sys.exit(ERROR_EXIT_CODE)
        if get_kikimr_storage_cluster_id(HW_WATCHER_USER):
            logger.info("Getting permission to stop kikimr service")
            ack_code = pm_kikimr_disk_attach(disk, HW_WATCHER_USER)
            if ack_code != SUCCESS_EXIT_CODE:
                logger.info("Permission was not obtained. PM returned %s. Exiting.", ack_code)
                sys.exit(ERROR_EXIT_CODE)
            ret_code, _, err = Systemctl.stop("kikimr.service")
            if ret_code:
                logger.error("Cannot stop kikimr.service: %s", err)
                sys.exit(ERROR_EXIT_CODE)
            disk_obliterated = KikimrDisks.obliterate_disk(label)
            ret_code, _, err = Systemctl.start("kikimr.service")
            if ret_code:
                logger.error("Cannot start kikimr.service: %s", err)
                sys.exit(ERROR_EXIT_CODE)
            if not disk_obliterated:
                sys.exit(ERROR_EXIT_CODE)
            sys.exit()

    logger.warning("Could not determine what disk '%s' belongs to.", disk.name)
    sys.exit()


if __name__ == "__main__":
    main()
