#!/usr/bin/env python

import sys
import logging

from ycinfra import (
    pm_kikimr_disk_detach,
    pm_mdb_disk_detach,
    pm_nbs_disk_detach,
    pm_dedicated_disk_detach,
    determine_disk_owner,
    new_device_object,
    Systemctl,
    DiskOwners,

    HWWATCHER_RETRY_EXIT_CODE,
    SUCCESS_EXIT_CODE,
)

logger = logging.getLogger()

HW_WATCHER_USER = "hw-watcher"

LOG_LEVEL = logging.DEBUG
LOG_FORMAT = "%(asctime)s - %(levelname)s - %(message)s"


def main():
    logging.basicConfig(level=LOG_LEVEL, format=LOG_FORMAT)
    console_handler = logging.StreamHandler()
    console_handler.setLevel(LOG_LEVEL)
    console_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(console_handler)
    logger.setLevel(LOG_LEVEL)
    lib_logger = logging.getLogger("ycinfra")
    lib_logger.addHandler(console_handler)
    lib_logger.setLevel(LOG_LEVEL)

    try:
        hook_disk = sys.argv[1]
    except IndexError:
        logger.info("Disk for removing doesn't find")
        sys.exit(HWWATCHER_RETRY_EXIT_CODE)

    if not hook_disk or hook_disk == "None":
        """
        If we call
        sudo -u hw-watcher /usr/sbin/hw_watcher -b disk run -v
        hw-watcher send to this script first argument equal None
        """
        logger.info("Disk for removing is None.")
        sys.exit(SUCCESS_EXIT_CODE)

    disk = new_device_object(hook_disk)
    try:
        disk_owner = determine_disk_owner(disk=disk, username=HW_WATCHER_USER)
    except Exception:
        logger.error("Exception raised while getting disk owner. Check log for details")
        sys.exit(HWWATCHER_RETRY_EXIT_CODE)
    if not disk.is_exists():
        logger.warning("Disk wasn't found in the system. We allow change it without any checks.")
        if disk_owner == DiskOwners.NBS:
            logger.info(
                "Disk owner was NBS. So blockstore-disk-agent.service will be restarted to release file descriptors")
            Systemctl.restart("blockstore-disk-agent.service")
        sys.exit(SUCCESS_EXIT_CODE)

    if disk_owner == DiskOwners.SYSTEM_RAID:
        logger.info("Skip raid disks. Hw-watcher does all manipulation with raids")
        sys.exit(SUCCESS_EXIT_CODE)
    elif disk_owner == DiskOwners.KIKIMR:
        ack_code = pm_kikimr_disk_detach(disk, HW_WATCHER_USER)
    elif disk_owner == DiskOwners.MDB:
        ack_code = pm_mdb_disk_detach(disk, HW_WATCHER_USER)
    elif disk_owner == DiskOwners.NBS:
        ack_code = pm_nbs_disk_detach(disk, HW_WATCHER_USER)
    elif disk_owner == DiskOwners.DEDICATED:
        ack_code = pm_dedicated_disk_detach(disk, HW_WATCHER_USER)
    elif disk_owner == DiskOwners.NO_OWNER:
        logger.info("Disk '%s' has no owner. So we can replace it.", disk.path)
        sys.exit(SUCCESS_EXIT_CODE)
    else:
        logger.warning("Could not determine what disk '%s' belongs to. So we can replace it.", disk.path)
        sys.exit(SUCCESS_EXIT_CODE)

    if ack_code != SUCCESS_EXIT_CODE:
        sys.exit(ack_code)

    logger.info("Disk removing allowed.")
    disk.remove()
    if disk_owner == DiskOwners.NBS:
        logger.info(
            "Disk owner was NBS. So blockstore-disk-agent.service will be restarted to release file descriptors")
        Systemctl.restart("blockstore-disk-agent.service")


if __name__ == "__main__":
    main()
