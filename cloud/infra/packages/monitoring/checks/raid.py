#!/usr/bin/env python3

import os
import re
from typing import Dict, List, Set, Union
from yc_monitoring import (
    JugglerPassiveCheck,
    JugglerPassiveCheckException,
)
from ycinfra import Popen


def parse_file(file_path: str, check: JugglerPassiveCheck, substring: str = None) -> Union[str, List[str]]:
    """Function for open and parse file"""
    try:
        with open(file_path) as file:
            if not substring:
                return file.read().strip()
            return [line for line in file if substring in line]
    except (FileNotFoundError, PermissionError) as err:
        check.crit("Error: Found problems working with file {}: {}".format(file_path, err))
        return ""


class RaidCheck(JugglerPassiveCheck):
    """Raid Class"""
    RAID_FILE = "/proc/mdstat"
    RAID_CONFIG_FILE = "/etc/mdadm/mdadm.conf"
    RAID_CRIT_STATUSES = {"faulty", "remove", "degraded"}
    RAID_RECOVERY_STATUSES = {"recovering"}
    RAID_RESYNC_STATUSES = {"resyncing (DELAYED)"}
    # https://st.yandex-team.ru/CLOUD-32116
    RAID_DISK_SIZES = [4, 6]
    SLAVE_DISK_PATTERN = re.compile(r"(?P<disk>((sd\w)|(nvme\d*n\d*)))")

    def __init__(self):
        super(RaidCheck, self).__init__("raid")

    def _get_system_raid(self) -> Set[str]:
        """Get real /proc/mdstat"""
        raids = [raid.split()[0] for raid in parse_file(self.RAID_FILE, check=self, substring="md")]
        return set(sorted(raids))

    def _get_config_raid(self) -> Set[str]:
        """Get Raid information from /etc/mdadm/mdadm.conf"""
        try:
            raids = ["md{}".format(raid.split()[1].split("/")[3])
                     for raid in parse_file(self.RAID_CONFIG_FILE, check=self, substring="ARRAY")]
        except IndexError:
            raise JugglerPassiveCheckException("can't parse /etc/mdadm/mdadm.conf")
        return set(sorted(raids))

    def get_consistency_raids(self) -> Set[str]:
        """Check raids we found in configs and system"""
        system_raid = self._get_system_raid()
        config_raid = self._get_config_raid()
        if system_raid != config_raid:
            unexpected_raid = list(set(system_raid) - set(config_raid))
            self.warn("Raids from system and configs are not the same\nUnexpected raids: {}".format(", ".join(unexpected_raid)))
        return system_raid

    def get_raid_status(self, raids):
        """Check raids status and add status, messages if needed"""
        for raid in raids:
            command = "/sbin/mdadm --detail /dev/{}".format(raid)
            raid_desc = {}
            returncode, stdout, stderr = Popen().exec_command(command)
            if returncode != 0:
                self.crit("Command: {}. ret_code: {}. STDERR: {}".format(
                    command, returncode, stderr))
                continue
            for string_status in stdout.split("\n"):
                if string_status:
                    list_string_status = string_status.strip().split(":", 1)
                    if len(set(list_string_status)) > 1:
                        raid_desc[list_string_status[0].strip()] = list_string_status[1].strip()
            raid_state = set(raid_desc["State"].split(", "))
            if self.RAID_RECOVERY_STATUSES.intersection(raid_state):
                self.warn("Raid {} in recovering state: {}".format(raid, raid_desc["Rebuild Status"]))
            elif self.RAID_RESYNC_STATUSES.intersection(raid_state):
                self.warn("Raid {} waiting for time slot for recovery".format(raid))
            elif self.RAID_CRIT_STATUSES.intersection(raid_state):
                self.crit("Raid {} in degraded state".format(raid))

    def check_raid_slave_disk_size(self, raids):
        """Check slave disks size in all raids"""

        def __compare_disk_size(slaves_desc):
            """Compare slave disks in each raid"""
            local_statuses = set()
            local_messages = set()
            for raid, disks in slaves_desc.items():
                sizes = set()
                for disk in disks:
                    disk_size_path = "/sys/block/{}/size".format(disk)
                    # convert blocks to TB
                    try:
                        sizes.add(int(parse_file(disk_size_path, check=self)) * 512 / 1024 / 1024 / 1024 / 1024)
                    except ValueError:
                        self.warn("Can't parse file {}".format(disk_size_path))
                if len(sizes) > 1 and max(sizes) > max(self.RAID_DISK_SIZES):
                    local_statuses.add(1)
                    local_messages.add("Size of disks in raid {} not the same".format(raid))
            for status, message in zip(local_statuses, local_messages):
                self.add_message(status, message)

        def __get_raid_slaves(raids_) -> Dict[str, List[str]]:
            """Determine slave disks in raids"""
            slaves = {}
            for raid in raids_:
                try:
                    raid_slaves_listdir = os.listdir("/sys/block/{}/slaves".format(raid))
                    slaves[raid] = [
                        self.SLAVE_DISK_PATTERN.match(slave).group("disk") for slave
                        in raid_slaves_listdir if self.SLAVE_DISK_PATTERN.match(slave)
                    ]
                except FileNotFoundError:
                    self.crit("Found problems with raid {}".format(raid))
                except IndexError:
                    self.crit("Raid {} slaves {} is not matching for predefined regexp".format(raid, raid_slaves_listdir))
            return slaves

        raid_slaves = __get_raid_slaves(raids)
        __compare_disk_size(raid_slaves)


def main():
    check = RaidCheck()
    try:
        all_raids = check.get_consistency_raids()
        check.get_raid_status(all_raids)
        check.check_raid_slave_disk_size(all_raids)
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
