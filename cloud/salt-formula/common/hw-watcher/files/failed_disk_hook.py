#!/usr/bin/env python
import json
import socket
import os
import sys
import logging
import requests
import salt.client

"""
import salt.client
This module changes default logging behavior, so we need to create new logger
"""
logger = logging.getLogger(__name__)
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)
formatter = logging.Formatter("%(asctime)s -  %(levelname)s - %(message)s")
console_handler.setFormatter(formatter)
logger.addHandler(console_handler)


class SaltGrains(object):

    def __init__(self):
        pass

    def get_salt_grains(self):
        salt_caller = salt.client.Caller()
        grains = salt_caller.cmd("grains.items")
        return grains


class KikimrPermission(object):
    BASE_URL = "http://{}:8765/cms/api/{}"
    PERMISSION_REQUEST = "permissionrequest"
    CLUSTERSTATE_REQUEST = "clusterstaterequest"

    def __init__(self, grains):
        self.grains = grains

    def _get_json(self, data):
        return json.dumps(data)

    def _get_hostname(self):
        return socket.gethostname()

    def _http_req(self, request_type, data):
        kikimr_cluster_hosts = self._get_endpoints()
        for endpoint in kikimr_cluster_hosts:
            try:
                url = self.BASE_URL.format(endpoint, request_type)
                raw_data = requests.post(url, data=self._get_json(data))
            except requests.exceptions.ConnectionError as err:
                logger.warning("I couldn't connect to kikimr CMS:\n%s", err)
                if endpoint == kikimr_cluster_hosts[-1]:
                    logger.error("The last connection retry to kikimr finished with error:\n%s", err)
                    sys.exit(1)
                continue
            else:
                if raw_data.status_code == 200:
                    return raw_data.json()

    def _get_endpoints(self):
        """Get kikimr cms endpoints for current cluster"""
        grains = self.grains
        kikimr_section_description = grains["cluster_map"]["hosts"][self._get_hostname()]["kikimr"]
        if kikimr_section_description.get('cluster_id'):
            kikimr_cluster = kikimr_section_description["cluster_id"]
        else:
            kikimr_cluster = kikimr_section_description["nbs_cluster_id"]
        logger.warning("This host belongs to %s Kikimr cluster", kikimr_cluster.upper())
        return grains["cluster_map"]["kikimr"]["clusters"][kikimr_cluster]["storage_nodes"]

    def get_remove_permission(self):
        """Make a content json for request to kikimr cms"""
        request_content = {
            "Reason": "Check shutdown",
            "User": "hw-watcher",
            "PartialPermissionAllowed": False,
            "Schedule": False,
            "Duration": 60,
            "DryRun": True,
            "Actions": [
                {"Duration": 60,
                 "Type": "SHUTDOWN_HOST"}
            ]
        }
        request_content["Actions"][0]["Host"] = self._get_hostname()
        return self._http_req(self.PERMISSION_REQUEST, request_content)

    def get_cluster_status(self):
        """Get kikimr cluster status"""
        request_content = dict()
        return self._http_req(self.CLUSTERSTATE_REQUEST, request_content)


class KikimrResources(object):
    UP_STATUS = "UP"
    DOWN_STATUS = "DOWN"

    def __init__(self, kikimr_resources):
        self.kikimr_resources = kikimr_resources

    def _get_resources(self):
        """Get all hosts in kikimr cluster"""
        return self.kikimr_resources["State"]["Hosts"]

    def get_failed_hosts(self):
        """Get failed hosts from kikimr cms"""
        failed_hosts = list()
        for host in self._get_resources():
            if host["State"] == self.DOWN_STATUS:
                failed_hosts.append(host["Name"])
        return failed_hosts

    def get_failed_devices(self):
        """Get failed devices from kikimr cms"""
        failed_disks = {}
        for host in self._get_resources():
            for device in host.get("Devices", []):
                if device["State"] == self.DOWN_STATUS:
                    if not failed_disks.get(host["Name"]):
                        failed_disks[host["Name"]] = {}
                    if "vdisk" in device["Name"]:
                        if not failed_disks[host["Name"]].get("vdisks"):
                            failed_disks[host["Name"]]["vdisks"] = []
                        failed_disks[host["Name"]]["vdisks"].append(device["Name"])
                    elif "pdisk" in device["Name"]:
                        if not failed_disks[host["Name"]].get("pdisks"):
                            failed_disks[host["Name"]]["pdisks"] = []
                        failed_disks[host["Name"]]["pdisks"].append(device["Name"])
        return failed_disks


class Disks(object):
    SYS_BLOCK_PATH = "/sys/block"
    REMOVE_DEVICE_CHAR = "1"
    RAID_GOOD_STATUS = 0

    def __init__(self, disk):
        self.disk = disk

    def get_disk_name(self):
        """/dev/sda -> sda """
        return os.path.basename(self.disk)

    def check_disk(self):
        """Check that disk exists in system"""
        if not os.path.exists("/dev/{}".format(self.get_disk_name())):
            return False
        return True

    def get_nvme_pci_addr(self):
        """Get NVME PCI Address"""
        pci_addr_string = "PCI_SLOT_NAME"
        nvme_info_file = "/sys/block/{}n1/device/device/uevent".format(self.get_disk_name())
        with open(nvme_info_file) as file:
            for line in file:
                if pci_addr_string in line:
                    pci_addr = line.split("=")[1].strip()
                    logger.info("PCI addr of deleted NVME is %s", pci_addr)
                    return pci_addr

    def remove_disk(self):
        """Remove disk from system"""
        delete_device_file = "{}/{}/device/delete".format(self.SYS_BLOCK_PATH, self.get_disk_name())
        if self.get_disk_name().startswith("nvme"):
            delete_device_file = "/sys/bus/pci/devices/{}/remove".format(self.get_nvme_pci_addr())
        with open(delete_device_file, "w") as file:
            file.write(self.REMOVE_DEVICE_CHAR)
        logger.info("Disk %s was successfully removed from system", self.get_disk_name())

    def _get_all_raids(self):
        """Get all raid in system"""
        list_dev = os.listdir(self.SYS_BLOCK_PATH)
        return [raid for raid in list_dev if raid.startswith("md")]

    def is_drive_raid_slave(self):
        """Function for determining disk is a slave of raid"""
        raids = self._get_all_raids()
        slaves_list = []
        for raid in raids:
            slaves_list.extend(os.listdir("{}/{}/slaves".format(self.SYS_BLOCK_PATH, raid)))
        return True if self.get_disk_name() in (disk[:-1] for disk in slaves_list) else False

    def is_raids_ok(self):
        """Check all raids status"""
        raids = self._get_all_raids()
        raid_statuses = []
        for raid in raids:
            with open("/sys/block/{}/md/degraded".format(raid)) as raid_status:
                raid_statuses.append(int(raid_status.read()))
        return False if max(raid_statuses) > self.RAID_GOOD_STATUS else True


def main():
    """Main function"""
    try:
        sys_disk = sys.argv[1]
    except IndexError:
        logger.info("Disk for removing doesn't find")
        sys.exit(1)
    if not sys_disk:
        """
        If we call
        sudo -u hw-watcher /usr/sbin/hw_watcher -b disk run -v
        hw-watcher send to this script first argument equal None
        """
        sys.exit(0)
    failed_system_disk = Disks(sys_disk)
    if not failed_system_disk.check_disk():
        logger.warning("Disk wasn't found in the system. We allow change it without any checks.")
        sys.exit(0)
    else:
        if failed_system_disk.is_drive_raid_slave():
            logger.info("I found disk %s into raid", sys_disk)
            if not failed_system_disk.is_raids_ok():
                logger.error("One of raids isn't in good condition!!!")
                sys.exit(1)
            sys.exit(0)
        else:
            saltgrains = SaltGrains()
            cms_request = KikimrPermission(saltgrains.get_salt_grains())
            remove_permission = cms_request.get_remove_permission()
            if remove_permission["Status"]["Code"] == "ALLOW":
                logger.info("CMS response for removing disk %s is: %s", sys_disk,
                            remove_permission["Status"]["Code"])
                failed_system_disk.remove_disk()
            else:
                logger.info("%s: %s", remove_permission["Status"]["Code"],
                            remove_permission["Status"]["Reason"])
                cluster_resources = cms_request.get_cluster_status()
                cluster_status = KikimrResources(cluster_resources)
                failed_host = cluster_status.get_failed_hosts()
                if failed_host:
                    print("Next hosts are DOWN:\n{}".format(", ".join(failed_host)))
                failed_devices = cluster_status.get_failed_devices()
                if failed_devices:
                    for hostname, devices in failed_devices.iteritems():
                        logger.info("Failed devices on host %s:", hostname)
                        for device_type, disks in devices.iteritems():
                            print("{}: {}".format(device_type, ", ".join(disks)))
                sys.exit(1)


if __name__ == "__main__":
    main()
