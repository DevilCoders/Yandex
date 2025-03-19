#!/usr/bin/env python3

import logging
import os
import re
import shlex
import socket
import subprocess
import sys
import time
from enum import IntEnum
from typing import List

from ycinfra import (
    InventoryApi,
    get_os_codename,
)

logging.basicConfig(level=logging.INFO, format="%(levelname)s - %(message)s")

SYSFS_NET = "/sys/class/net/"
SETTLE_DEVICE_DELAY_SEC = 5

INTEL_VENDOR = "8086"
MELLANOX_VENDOR = "15b3"

MLNX_CX3 = MELLANOX_VENDOR + ":1003"
MLNX_CX3_PRO = MELLANOX_VENDOR + ":1007"
MLNX_CX4 = MELLANOX_VENDOR + ":1015"
MLNX_CX5 = MELLANOX_VENDOR + ":1017"
MLNX_CX6 = MELLANOX_VENDOR + ":101b"
MLNX_CX6_LX = MELLANOX_VENDOR + ":101f"
MLNX_CX6_DX = MELLANOX_VENDOR + ":101d"
INTEL_710 = INTEL_VENDOR + ":1584"
INTEL_X710 = INTEL_VENDOR + ":1572"

VENDOR_PRIORITY = {
    MLNX_CX6: 1,  # Mellanox CX-6
    MLNX_CX4: 1,  # Mellanox CX-4
    MLNX_CX5: 1,  # Mellanox CX-5
    MLNX_CX3: 1,  # Mellanox CX-3
    MLNX_CX3_PRO: 1,  # Mellanox CX-3 pro
    INTEL_710: 2,  # Intel 710
    INTEL_X710: 2,  # Intel X710
}

IP_PRIORITY = {
    "2a02": 1,  # Yandex underlay net
    "2a0d": 1,  # GPN/Private-Testing underlay net
    "2a11": 1,  # IL underlay net
    "fc01": 2,  # A100 interconnect net
    "default": 3,
}


class AF_PRIORITY(IntEnum):
    AF_INET6 = 1,
    AF_INET = 2,
    AF_LINK = 2
    AF_DEFAULT = 2


SXM4_40G_PLATFORM = "nvidia-ampere-a100-sxm4-40G"
SXM4_80G_PLATFORM = "nvidia-ampere-a100-sxm4-80G"


def list_interfaces() -> List[str]:
    return os.listdir(SYSFS_NET)


def get_good_ifaces():
    # During Setup process network interfaces always have ethX names
    SETUP_KERNEL_IFACE_PREFIX = "eth"

    ifaces = []
    for iface_name in list_interfaces():
        if not iface_name.startswith(SETUP_KERNEL_IFACE_PREFIX):
            continue
        ifaces.append(Interface(iface_name))
    return ifaces


def exec(cmd: str) -> str:
    if not cmd:
        raise Exception("Empty command passed!")

    try:
        logging.debug("Executing: %s", cmd)
        stdout = subprocess.check_output(shlex.split(cmd))
        return stdout.decode()
    except subprocess.CalledProcessError as ex:
        raise Exception("{}".format(ex))
    except OSError as ex:
        raise Exception("Cmd '{}' failed! Error: {}".format(cmd, ex))


class MlxDevice:
    IB_CLASS = "0207"
    ETH_CLASS = "0200"

    IB_LINK_TYPE = 1
    ETH_LINK_TYPE = 2

    def __init__(self, dev_class: str, model_id: str, pci_ep: str) -> None:
        self._dev_class = dev_class
        self._model_id = model_id
        self._pci_ep = pci_ep

        # b1:00.0 -> b1, 00.0
        pci_addr_port = self._pci_ep.split(":")
        if len(pci_addr_port) < 2:
            raise Exception("Unexpected pci endpoint format passed!\nvalue:%s", self._pci_ep)
        self._pci_port = pci_addr_port[1]
        self._mac = self._get_mac()
        pass

    def _get_mac(self) -> str:
        def int_to_mac(mac: int) -> str:
            return ":".join(re.findall("..", "{:012x}".format(mac)))

        logging.debug("Getting mac for %s", self._pci_ep)
        out = exec("flint -d {} query ".format(self._pci_ep))
        if not out:
            raise Exception("No output from device {}! Can't obtain mac address!".format(self._pci_ep))

        mac = None
        for line in out.splitlines():
            if not line.startswith("Base MAC"):
                continue
            mac = line.split()
            break

        if not mac or len(mac) < 3:
            raise Exception("Unexpected format from 'flint' command!\nvalue:%s", line)
        mac = mac[2]
        try:
            hex_mac = int(mac, 16)
        except ValueError:
            raise Exception("Unexpected format from 'flint' command!\nvalue:%s", line)

        port = self._pci_port.split(".")
        if len(port) < 2:
            raise Exception("Unexpected pci port format passed!\nvalue:%s", self._pci_port)

        # https://docs.nvidia.com/networking/pages/viewpage.action?pageId=39258074
        # In dual-port cards, the HOST MAC belongs to the first port, and the HOST MAC of the second port increases by 1 (in HEX).
        # here for second port on dual cards port==1, for usual port==0
        try:
            port = int(port[1], 16)
            hex_mac += port
        except ValueError:
            raise Exception("Unexpected pci port format passed!\nvalue:%s", self._pci_port)

        mac = int_to_mac(hex_mac)
        logging.debug("Found mac %s for device %s", mac, self._pci_ep)
        return mac

    def set_options(self, options: str):
        logging.info("Updating mellanox fw settings for %s with PCI address %s", self.device_id, self._pci_ep)
        exec("/usr/bin/mlxconfig --yes --dev {} set {}".format(self._pci_ep, options))

    @property
    def mac_addr(self) -> str:
        return self._mac

    @property
    def device_id(self) -> str:
        return "{}:{}".format(MELLANOX_VENDOR, self._model_id)

    @property
    def is_IB(self) -> bool:
        return self._dev_class == MlxDevice.IB_CLASS

    @property
    def pci_ep(self) -> str:
        return self._pci_ep


class Interface:
    def __init__(self, name: str) -> None:
        self.name = name
        self._mac = None

        self._ip_priority = None
        self._vendor_priority = None
        self._af_priority = None
        self.can_up = True

    def _get_ip_priority(self) -> int:
        """Get iface priority by ip
        get priority from IP_PRIORITY if iface has ip
        if iface doesn't have any remote_address, priority will be 3(default_ip_priority)
        """
        ip_ifaces_path = "/proc/net/if_inet6"
        local_ipv6_prefix = "fe80"
        default_ip_priority = IP_PRIORITY["default"]

        ip_prefixes = list()
        try:
            with open(ip_ifaces_path) as fd:
                # Example of content:
                # fc010000000c00000e42a1fffead7e8a 08 40 00 00     eth6
                # fe800000000000000e42a1fffead7e8a 08 40 20 80     eth6
                # fe800000000000000e42a1fffe94f581 05 40 20 80     eth0
                # 2a0206b8bf00102f0e42a1fffe94f581 05 40 00 00     eth0
                # we looking for only remote_address for iface and suppose that iface has only one remote_address address
                for line in fd:
                    if line.endswith(self.name + "\n") and not line.startswith(local_ipv6_prefix):
                        ip_prefixes.append(line[0:4])  # append only ip prefix
        except Exception:
            raise RuntimeError("Couldn't get ip for iface '{}'".format(self.name))
        if len(ip_prefixes) == 0:
            logging.debug("Iface '%s' doesn't have any ip, so ip priority is '%s'", self.name, default_ip_priority)
            return default_ip_priority
        if len(ip_prefixes) > 1:
            raise RuntimeError("Found too many ips for iface '{}'".format(self.name))

        try:
            ip_priority = IP_PRIORITY[ip_prefixes[0]]
        except KeyError:
            logging.error("Couldn't find ip_prefix '%s' in IP_PRIORITY map", ip_prefixes[0])
            raise
        logging.debug("Iface '%s'(%s) has '%s' ip priority", self.name, self.mac_addr, ip_priority)
        return ip_priority

    def _get_vendor_priority(self):
        """Check if device's pci id is in VENDOR_PRIORITY
        get priority from VENDOR_PRIORITY if iface has known vendor
        if iface doesn't have any known vendor, priority will be 3(default_vendor_priority)
        """

        iface_path = os.path.join(SYSFS_NET, self.name)
        default_vendor_priority = 10

        # Correctly traverse symlinks in sysfs
        dev_path = os.path.join(os.path.dirname(iface_path), os.readlink(iface_path), "../..")
        if not os.path.exists(dev_path):
            raise RuntimeError("Problem with reading iface '%s' files")

        with open(os.path.join(dev_path, "vendor")) as f:
            vendor = f.readline().strip()[2:]
        with open(os.path.join(dev_path, "device")) as f:
            device = f.readline().strip()[2:]
        vendor_dev_id = "{}:{}".format(vendor, device)
        try:
            vendor_priority = VENDOR_PRIORITY[vendor_dev_id]
            logging.debug("Iface '%s'(%s) has '%s' vendor priority", self.name, self.mac_addr, vendor_priority)
        except KeyError:
            vendor_priority = default_vendor_priority
            logging.debug("Unknown iface '%s' with vendor_dev_id '%s', so it has '%s' vendor priority",
                          self.name, vendor_dev_id, vendor_priority)
        return vendor_priority

    def _has_ipv6(self) -> bool:
        return bool(exec("ip -6 addr show " + self.name))

    def _has_ipv4(self) -> bool:
        return bool(exec("ip -4 addr show " + self.name))

    def _has_link(self) -> bool:
        return bool(exec("ip link show " + self.name))

    def _get_iface_priority_by_af(self) -> int:
        try:
            if self._has_ipv6():
                logging.debug("Iface '%s'(%s) has ipv6, priority='%s'", self.name, self.mac_addr,
                              int(AF_PRIORITY.AF_INET6))
                return int(AF_PRIORITY.AF_INET6)
            if self._has_ipv4():
                logging.debug("Iface '%s'(%s) has ipv4, priority='%s'", self.name, self.mac_addr,
                              int(AF_PRIORITY.AF_INET))
                return int(AF_PRIORITY.AF_INET)
            if self._has_link():
                logging.debug("Iface '%s'(%s) has link addr, priority='%s'", self.name, self.mac_addr,
                              int(AF_PRIORITY.AF_LINK))
                return int(AF_PRIORITY.AF_LINK)
        except subprocess.CalledProcessError as ex:
            logging.error("Command cannot be executed(using default priority=%s): %s", int(AF_PRIORITY.AF_DEFAULT), ex)

        return int(AF_PRIORITY.AF_DEFAULT)

    def _get_mac(self) -> str:
        with open(os.path.join(SYSFS_NET, self.name, "address")) as f:
            return f.readline().strip()

    def link_up(self) -> bool:
        if not self.can_up:
            logging.info("Interface %s is raw device. Skip link up.", self.name)
            return True

        logging.debug("link up %s(%s)", self.mac_addr, self.name)
        exec("ip link set dev {} up".format(self.name))
        return True

    def from_mlx_dev(self, device: MlxDevice):
        logging.info("Using info from raw device for interface %s(%s)", self.name, device.mac_addr)
        # can't up raw MlxDevice
        self.can_up = False
        self._mac = device.mac_addr
        self._ip_priority = IP_PRIORITY["default"]
        self._af_priority = AF_PRIORITY.AF_DEFAULT
        self._vendor_priority = VENDOR_PRIORITY[device.device_id]
        logging.debug("priorities: ip=%s, af=%s, vendor=%s", self._ip_priority, int(self._af_priority),
                      self._vendor_priority)

    @property
    def priority(self) -> int:
        if all([self._ip_priority, self._vendor_priority, self._af_priority]):
            return self._ip_priority + self._vendor_priority + self._af_priority

        logging.debug("Calculating priority for %s", self.name)
        self._ip_priority = self._get_ip_priority()
        self._vendor_priority = self._get_vendor_priority()
        self._af_priority = self._get_iface_priority_by_af()
        return self._ip_priority + self._vendor_priority + self._af_priority

    @property
    def mac_addr(self) -> str:
        if not self._mac:
            self._mac = self._get_mac()
        return self._mac


class UdevRulesEditor:
    UDEV_RULE_TEMPLATE = """SUBSYSTEM=="net", ACTION=="add", DRIVERS=="?*", ATTR{{address}}=="{mac}", \
ATTR{{dev_id}}=="0x0", ATTR{{type}}=="1", KERNEL=="{kernel_iface_prefix}*", NAME="{iface_name}"\n"""  # noqa
    UDEV_RULES_FILE = "/etc/udev/rules.d/65-setup-persistent-net.rules"
    DESIRED_PREFIX = "eth"
    UBUNTU_KERNEL_IFACE_PREFIXES = {
        "xenial": {"eth"},
        "focal": {"en"},
    }

    def __init__(self) -> None:
        pass

    def write_rules(self, interfaces: List[Interface]) -> bool:
        logging.debug("Generating udev rules...")

        os_codename = get_os_codename()
        if not os_codename:
            logging.error("Couldn't get OS codename. Exiting")
            return False
        iface_prefixes = UdevRulesEditor.UBUNTU_KERNEL_IFACE_PREFIXES.get(os_codename)
        if not iface_prefixes:
            logging.error("Network interface prefixes don't configured for '%s' distrib", os_codename)
            return False

        try:
            with open(UdevRulesEditor.UDEV_RULES_FILE, "w") as fd:
                for num, iface in enumerate(interfaces):
                    logging.debug("Processing rules for %s", iface.mac_addr)
                    iface_name = "{}{}".format(UdevRulesEditor.DESIRED_PREFIX, num)
                    for iface_prefix in iface_prefixes:
                        fd.write(UdevRulesEditor.UDEV_RULE_TEMPLATE.format(mac=iface.mac_addr,
                                                                           kernel_iface_prefix=iface_prefix,
                                                                           iface_name=iface_name
                                                                           ))
                    logging.info("Iface (mac=%s) has priority=%s", iface.mac_addr, iface.priority)
                    logging.info("Save iface (mac=%s) as %s", iface.mac_addr, iface_name)
        except IOError as ex:
            logging.error("Fail to write rules to '%s'", UdevRulesEditor.UDEV_RULES_FILE)
            logging.error("Raised: (%s): %s", ex.__class__.__name__, ex)
            return False
        return True


def get_mlx_devices() -> List[MlxDevice]:
    mlx_devices = []
    # Going through all mellanox devices
    for line in exec("lspci -d {}: -m -n".format(MELLANOX_VENDOR)).splitlines():
        device_info = line.split()
        if len(device_info) < 4:
            raise Exception("Unexpected output from lspci!\nvalue:%s", line)

        pci_endpoint = device_info[0].strip('"')
        dev_class = device_info[1].strip('"')
        model_id = device_info[3].strip('"')
        mlx_devices.append(MlxDevice(dev_class, model_id, pci_endpoint))
    return mlx_devices


def switch_ib_to_eth(mlx_devices: List[MlxDevice]) -> List[Interface]:
    mlx_ib_to_eth_ifaces = []

    for device in mlx_devices:
        if not device.is_IB:
            continue
        logging.info("Change interconnect link type to RocE for melanox card with PCI address %s", device.pci_ep)
        device.set_options("LINK_TYPE_P1={}".format(MlxDevice.ETH_LINK_TYPE))
        ib_iface = Interface(device.pci_ep)
        ib_iface.from_mlx_dev(device)
        mlx_ib_to_eth_ifaces.append(ib_iface)

    return mlx_ib_to_eth_ifaces


def main():
    mlx_devices = get_mlx_devices()

    host_gpu_platform = InventoryApi().get_gpu_platform(socket.gethostname())

    # Set params for cx3,cx4,cx5,cx6 cards
    for device in mlx_devices:
        if device.device_id == MLNX_CX3:
            device.set_options("SRIOV_EN=1 NUM_OF_VFS=63 IP_VER_P1=IPv6_IPv4 IP_VER_P2=IPv6_IPv4")
        if device.device_id in [MLNX_CX4, MLNX_CX5, MLNX_CX6, MLNX_CX6_LX, MLNX_CX6_DX]:
            device.set_options("SRIOV_EN=1 NUM_OF_VFS=64 IP_VER=IPv6_IPv4")
        if host_gpu_platform == SXM4_80G_PLATFORM and device.is_IB:
            device.set_options("ATS_ENABLED=true")

    switched_ib_to_eth = []
    if host_gpu_platform == SXM4_40G_PLATFORM:
        switched_ib_to_eth = switch_ib_to_eth(mlx_devices)

    good_ifaces = get_good_ifaces()
    good_ifaces.extend(switched_ib_to_eth)
    if not good_ifaces:
        logging.error("Couldn't find any suitable network interfaces! Exiting...")
        sys.exit(1)

    for iface in good_ifaces:
        if not iface.link_up():
            logging.error("Cannot up iface %s", iface)
            sys.exit(1)
    time.sleep(SETTLE_DEVICE_DELAY_SEC)  # Intel cards take a couple of seconds to get up

    good_ifaces = sorted(good_ifaces, key=lambda iface: iface.priority)
    editor = UdevRulesEditor()
    if not editor.write_rules(good_ifaces):
        logging.error("Udev rules were not written!")
        sys.exit(1)


if __name__ == "__main__":
    main()
