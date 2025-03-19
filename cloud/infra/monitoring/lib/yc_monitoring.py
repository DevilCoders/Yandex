#!/usr/bin/env python3

import json
import os
import re
import shlex
import subprocess
import time
from typing import Any, Optional, Dict, List

import urllib3
import yaml


class JugglerPassiveCheck:
    __slots__ = ("name", "__status", "__messages")
    CHECK_TYPE = "PASSIVE-CHECK"
    OK = 0
    OK_MSG = "Ok"
    WARN = 1
    CRIT = 2
    MSG_DELIM = "; "
    LVL_DELIM = " ==> "
    PREFIX_ON_ONE_LVL = False

    def __init__(self, name: str):
        self.name = name
        self.__status = self.OK
        self.__messages = {
            self.OK: [],
            self.WARN: [],
            self.CRIT: [],
        }

    def __str__(self) -> str:
        return "{}:{};{};{}".format(
            self.CHECK_TYPE,
            self.name,
            str(self.status),
            self.format_messages(),
        )

    @property
    def messages(self) -> Dict[int, List[str]]:
        return self.__messages

    @property
    def status(self) -> int:
        return self.__status

    def format_messages(self) -> str:
        prefix = None
        result = []
        if len(self.__messages[self.CRIT]) > 0:
            prefix = "CRIT: "
            joined_msg = self.MSG_DELIM.join(self.__messages[self.CRIT])
            result.append(prefix + joined_msg)
        if len(self.__messages[self.WARN]) > 0:
            prefix = "WARN: "
            joined_msg = self.MSG_DELIM.join(self.__messages[self.WARN])
            result.append(prefix + joined_msg)
        if len(self.__messages[self.OK]) > 0:
            prefix = "OK: "
            joined_msg = self.MSG_DELIM.join(self.__messages[self.OK])
            result.append(prefix + joined_msg)
        if len(result) == 0:
            return self.OK_MSG
        if not self.PREFIX_ON_ONE_LVL and len(result) == 1:
            result[0] = result[0].replace(prefix, "", 1)
        return self.LVL_DELIM.join(result)

    def crit(self, message: str):
        if message in self.__messages[self.CRIT]:
            return
        self.__messages[self.CRIT].append(message)
        self.__status = max(self.__status, self.CRIT)

    def warn(self, message: str):
        if message in self.__messages[self.WARN]:
            return
        self.__messages[self.WARN].append(message)
        self.__status = max(self.__status, self.WARN)

    def ok(self, message: str):
        if message in self.__messages[self.OK]:
            return
        self.__messages[self.OK].append(message)
        self.__status = max(self.__status, self.OK)

    def add_message(self, status: int, message: str):
        if status == self.OK:
            self.ok(message)
        elif status == self.WARN:
            self.warn(message)
        elif status == self.CRIT:
            self.crit(message)
        else:
            raise JugglerPassiveCheckException("status {:d} is not supported. Message: {:s}".format(status, message))

    def restore_state(self, status: int, messages: Dict[int, List[str]]):
        self.__messages = messages
        self.__status = status


class JugglerPassiveCheckException(Exception):
    def __init__(self, message: str, *args, **kwargs):
        self.description = message


class CacheFile:
    __slots__ = ("__filename", "__filepath")

    def __init__(self, filename: str, cache_dir="/var/tmp/"):
        self.__filename = filename
        self.__filepath = os.path.join(cache_dir, filename)

    def __read(self) -> Optional[Dict]:
        try:
            with open(self.__filepath) as fd:
                return yaml.load(fd, Loader=yaml.SafeLoader)
        except FileNotFoundError:
            pass

    def __write(self, data: Dict):
        with open(self.__filepath, "w+") as fd:
            yaml.dump(data, fd, Dumper=yaml.SafeDumper)

    def store(self, key: Any, value: Any):
        data = self.__read()
        if data is None:
            data = {}
        data[key] = {"value": value}
        self.__write(data)

    def load(self, key: Any) -> Any:
        data = self.__read()
        if data is None or key not in data:
            raise KeyError(key)

        return data[key]["value"]


class SystemdService:
    @classmethod
    def unit_is_run(cls, service_name: str, minimal_uptime_sec: int = 5) -> bool:
        """
        Check systemd unit is running properly
        :return: (bool) return True if service starts and works properly
        """
        service_info = cls.parameters(service_name)
        current_service_state = cls.get_parameter(service_name, "ActiveState", service_info)
        current_service_substate = cls.get_parameter(service_name, "SubState", service_info)

        if current_service_state != "active" or current_service_substate != "running":
            raise JugglerPassiveCheckException(
                "service {} is in incorrect state: {}/{}!".format(
                    service_name, current_service_state, current_service_substate))

        if cls.unit_uptime_sec(service_name, service_info) < minimal_uptime_sec:
            raise JugglerPassiveCheckException("{} service uptime less than {} sec!".format(
                service_name, minimal_uptime_sec))

        return True

    @classmethod
    def unit_uptime_sec(cls, service_name: str, service_info: Dict[str, str] = None) -> int:
        """
        Return systemd service uptime in seconds
        :param (str) service_name: SystemD unit name
        :param (str) service_info: all SystemD unit parameters. Use it as cache
        :return: (int): uptime duration (sec)
        """
        # service_monotonic_uptime in microseconds
        service_monotonic_uptime = cls.get_parameter(service_name, "ActiveEnterTimestampMonotonic", service_info)
        return int(time.monotonic() - (int(service_monotonic_uptime) / 1e+6))

    @staticmethod
    def parameters(service_name: str) -> Dict[str, str]:
        """
        Return all SystemD unit parameters
        :param (str) service_name: SystemD unit name
        """
        service_data = exec_command("systemctl show {}".format(service_name))
        service_info = {}
        if service_data["ret_code"] != 0:
            raise JugglerPassiveCheckException(
                "Can't get {} systemd unit info: {}".format(service_name, service_data["stderr"]))
        for item in service_data["stdout"].split('\n'):
            kv_pair = item.split("=", 1)
            if len(kv_pair) == 2:
                service_info[kv_pair[0]] = kv_pair[1]

        return service_info

    @classmethod
    def get_parameter(cls, service_name: str, param: str, service_info: Dict[str, str] = None) -> str:
        """
        Return any SystemD unit parameter value
        :param (str) service_name: SystemD unit name
        :param (str) param:        SystemD unit parameter
        :param (str) service_info: all SystemD unit parameters. Use it as cache
        :return: (str): SystemD parameter value
        """
        if service_info is None:
            service_info = cls.parameters(service_name)
        try:
            return service_info[param]
        except KeyError:
            raise JugglerPassiveCheckException(
                "Parameter {} for systemd unit {} not found!".format(service_name, param))


class InventoryApi:
    __slots__ = ("_api_url",)
    conn_timeout = 5
    timeout = 30
    retries = 3

    def __init__(self):
        self._api_url = os.environ.get("INFRA_PROXY", "http://infra-proxy.cloud.yandex.net")

    def _make_request(self, host: str, uri: str):
        url = "{}/{}/{}".format(self._api_url, uri, host)
        try:
            http = urllib3.PoolManager(
                timeout=urllib3.Timeout(
                    connect=self.conn_timeout,
                    read=self.timeout),
                retries=urllib3.Retry(self.retries)
            )
            response = http.request('GET', url=url).data.decode("utf-8")
        except urllib3.exceptions.HTTPError:
            raise
        return response

    def get_disks_data(self, host: str):
        """
        Get information from infra-proxy about fragile disks in host
        """
        uri = "get_disks_data"
        response = self._make_request(host=host, uri=uri)

        try:
            response = json.loads(response)
        except json.JSONDecodeError:
            raise BadInventoryInformation
        try:
            response = response["Disks"]
        except KeyError:
            raise BadInventoryInformation
        return response

    def get_gpu_platform(self, host: str):
        """
        Get information from infra-proxy about gpu platform of host
        """
        uri = "gpu_platform"
        response = self._make_request(host=host, uri=uri)
        return response


class BadInventoryInformation(Exception):
    pass


class HwWatcherDbReadError(Exception):
    pass


class HwWatcherDbParseError(Exception):
    pass


class HwWathcherDisks:
    DB_PATH = "/var/cache/hw_watcher/disk.db"

    def __init__(self):
        self._hwwatcher_data = self._get_hw_watcher_disks_data()

    def _get_hw_watcher_disks_data(self):
        try:
            with open(self.DB_PATH) as hwwatcher_db:
                return json.loads(hwwatcher_db.read())
        except OSError:
            raise HwWatcherDbReadError
        except json.JSONDecodeError:
            raise HwWatcherDbParseError

    def get_bot_disks(self):
        bot_disks = list()
        disk_type_by_speed = {
            "SSD": "NVME",
            "7.2K": "HDD",
        }
        try:
            bot_info = self._hwwatcher_data["dss"]["bot_info"]["server"]
        except KeyError:
            raise HwWatcherDbParseError
        """
        'bot_info' data format:
         "bot_info": {
            "server": [
                [
                    "WD-WD4000FYYZ",
                    "4000GB/SATA/7.2K/3.5",
                    "WMC130323422"
                ],
        """
        disk_data_regex = re.compile(r"(?P<capacity>\S+)\/(?P<interface>\S+)\/(?P<speed>\S+)\/(?P<size>\S+)")
        for disk in bot_info:
            if len(disk) < 3:
                raise HwWatcherDbParseError
            re_match = disk_data_regex.match(disk[1])
            if not re_match:
                raise HwWatcherDbParseError
            bot_disks.append({
                "model": disk[0],
                "sn": disk[2],
                "capacity": re_match.group("capacity"),
                "interface": re_match.group("interface"),
                "speed": re_match.group("speed"),
                "size": re_match.group("size"),
                "disk_type": disk_type_by_speed.get(re_match.group("speed"), "UNKNOWN")
            })
        return bot_disks

    def get_disks(self):
        try:
            return self._hwwatcher_data["dss"]["disks"]
        except KeyError:
            raise HwWatcherDbParseError

    def get_hdd_disks(self):
        disks = self.get_disks()
        try:
            return {disk for disk in disks if disks[disk]["disk_type"] == "HDD"}
        except KeyError:
            raise HwWatcherDbParseError

    def get_nvme_disks(self):
        disks = self.get_disks()
        try:
            return {disk for disk in disks if disks[disk]["disk_type"] == "NVME"}
        except KeyError:
            raise HwWatcherDbParseError

    def get_raids(self):
        try:
            return self._hwwatcher_data["dss"]["raids"]
        except KeyError:
            raise HwWatcherDbParseError


def exec_command(command: str) -> dict:
    cmd_result = {}
    cmd = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = cmd.communicate()
    cmd_result["stdout"] = stdout.decode().rstrip()
    cmd_result["stderr"] = stderr.decode().rstrip()
    cmd_result["ret_code"] = cmd.returncode
    return cmd_result


def check_systemd_service_status(service, uptime_sec):
    check = JugglerPassiveCheck(service)
    try:
        if not SystemdService.unit_is_run(service_name=service, minimal_uptime_sec=uptime_sec):
            check.crit("service is not active")
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    return check
