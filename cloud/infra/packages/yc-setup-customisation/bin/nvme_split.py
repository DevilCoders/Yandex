#!/usr/bin/env python3.5

"""
NVME split -- a script for splitting NVMEs into multiple namespaces
"""

import getopt
from ycinfra import InventoryApi
import logging
import os
import pipes
import re
import socket
import subprocess
import sys
import time
from collections import namedtuple
from typing import Any, Dict, List

logging.basicConfig(level="DEBUG")
log = logging.getLogger("")

SYS_CLASS_NVME_PATH = "/sys/class/nvme"


class NvmeNamespace:
    __slots__ = {"id", "blocks_count"}

    def __init__(self, id: int, blocks_count: int):
        self.id = id
        self.blocks_count = blocks_count

    def __repr__(self):
        return "{}: {} ({:0.2f} Gib)".format(
            self.id,
            self.blocks_count,
            self.blocks_count * NvmeDevice.BLOCK_SIZE / 10 ** 9,
        )


class NvmeProfile:
    __slots__ = {"name", "namespaces"}

    def __init__(self, name: str, namespaces: List[NvmeNamespace] = None):
        self.name = name
        self.namespaces = namespaces if namespaces else []

    def __repr__(self):
        return "{}: with namespaces {}".format(self.name, self.namespaces)

    @property
    def ns_count(self) -> int:
        return len(self.namespaces)


_LBAF_REGEX = re.compile(r"lbaf[ ]*(?P<lbaf>\d+)[ ]*:[ ]*ms:(?P<ms>\d+)\s+" +
                         r"lbads:(?P<lbads>\d+)\s+rp:(?P<rp>0x[0-9a-f]*|0)")
_NS_LIST_REGEX = re.compile(r"\[\s+\d+\]:(0x[0-9a-f]+)")

MTAB_PATH = "/etc/mtab"
MTAB_LINK_SRC = "/proc/self/mounts"
NVME_DEVICE_CLASSES = {}


class TemporarySymbolicLink:
    """A hack for hioadm: it checks /etc/mtab which doesn't exist in setup, so
    temporarily provide it"""

    def __init__(self, src, dst):
        self.src = src
        self.dst = dst
        self.__created = False

    def __enter__(self):
        if os.path.islink(self.dst):
            src = os.path.normpath(os.path.join(os.path.dirname(self.dst), os.readlink(self.dst)))
            if src == self.src:
                return

        os.symlink(self.src, self.dst)
        self.__created = True

    def __exit__(self, *excinfo):
        if self.__created:
            os.remove(self.dst)


class NvmeDeviceMeta(type):
    """Meta class for registering Nvme implementations in NVME_DEVICE_CLASSES"""

    def __init__(cls, name, bases, namespace, **kwargs):
        global NVME_DEVICE_CLASSES
        NVME_DEVICE_CLASSES[cls.VENDOR_ID] = cls


class NvmeDevice:
    """Default nvme implementation for handling nvme using nvme-cli"""
    LBAFormat = namedtuple("LBAFormat", ["lba_format", "metadata_size", "block_size", "rel_perf"])

    # Can be redefined by subclasses to pick proper LBA format
    # NOTE (KIKIMR-5504): Samsung NVMEs treat 4k blocks in undocumented way, so 512 for everyone!
    LBA_FORMAT = 0
    BLOCK_SIZE = 512
    # NOTE(CLOUD-90323) INTEL SSDPF2KX038TZ ns size should be <=3.2TB
    MODELS_TO_RESIZE_NAMESPACE_CAPACITY = {"INTEL SSDPF2KX038TZ": 3200 * 10 ** 9}

    def __init__(self, name, dry_run=False):
        self.name = name
        self.pciid = _get_nvme_pciid(name)
        self.model = _get_nvme_device_model(name)

        self._cntlid = None
        self._capacity = None

        self.dryrun = dry_run

    def rescan(self):
        """Rescans nvme on the pci bus"""
        log.info("Rescanning %r...", self.name)
        if self.dryrun:
            return

        with open(os.path.join("/sys/bus/pci/devices", self.pciid, "remove"), "w") as devf:
            devf.write("1")
        log.info("Rescanning PCI...")
        with open("/sys/bus/pci/rescan", "w") as rescanf:
            rescanf.write("1")

        wait_for_device(self)

    @property
    def dev_path(self):
        return os.path.join("/dev/", self.name)

    def get_namespaces(self):
        """Returns list of nvme ids"""
        proc = _popen(["nvme", "list-ns", self.dev_path], stdout=subprocess.PIPE)
        out, _ = proc.communicate()

        return [int(_NS_LIST_REGEX.match(str(line, encoding="ascii")).group(1), base=16)
                for line in out.splitlines()]

    def initialize(self, ns_count: int):
        """Performs initialization of nvme and (optionally) formats it"""
        pass

    def get_ns_size(self, ns_id: int) -> int:
        """Returns size of the namespace in blocks (not necessarily 4k if it was formatted differently)"""
        proc = _popen(["nvme", "id-ns", self.dev_path, "-n", str(ns_id)], stdout=subprocess.PIPE)
        out, _ = proc.communicate()

        for line in out.splitlines():
            line = str(line, encoding="ascii")
            if ":" not in line:
                continue

            key, value = line.split(":", 1)
            key = key.strip()
            if key == "nsze":
                return int(value, base=16)

    def get_lba_format(self, ns_id: int) -> LBAFormat:
        """Returns active lba format of the specified nvme"""
        proc = _popen(["nvme", "id-ns", self.dev_path, "-n", str(ns_id)], stdout=subprocess.PIPE)
        out, _ = proc.communicate()

        for line in out.splitlines():
            line = str(line, encoding="ascii")
            if not line.startswith("lbaf"):
                continue
            if "in use" not in line:
                continue

            match = _LBAF_REGEX.match(line)
            if not match:
                log.warning("Unparseable line: %r", line)
                raise Exception("Unparseable line in 'nvme id-ns' output")

            return NvmeDevice.LBAFormat(int(match.group("lbaf")),
                                        int(match.group("ms")),
                                        2 ** int(match.group("lbads")),
                                        int(match.group("rp"), base=16))

    def get_controller_id(self) -> int:
        """Returns controller id reported by id-ctrl"""
        if self._cntlid is None:
            self._id_ctrl()
            if self._cntlid is None:
                raise Exception("No controller id was reported by id-ctrl!")

        return self._cntlid

    def get_capacity(self) -> int:
        """Returns tnvmcap reported by id-ctrl"""
        if self._capacity is None:
            self._id_ctrl()
            if self._capacity is None:
                raise Exception("No capacity was reported by id-ctrl!")

        return self._capacity // self.BLOCK_SIZE

    def _id_ctrl(self):
        proc = _popen(["nvme", "id-ctrl", self.dev_path], stdout=subprocess.PIPE)
        out, _ = proc.communicate()

        for line in out.splitlines():
            line = str(line, encoding="ascii")
            if ":" not in line:
                continue

            key, value = line.split(":", 1)
            key = key.strip()

            if key == "cntlid":
                if self._cntlid is not None:
                    raise Exception("nvme-cli reports two controller ids, this is not expected!")

                self._cntlid = int(value, base=16)

            if key == "tnvmcap":
                self._capacity = int(value)
            # NOTE: Hack for CLOUD-90323
            if self.model in self.MODELS_TO_RESIZE_NAMESPACE_CAPACITY:
                self._capacity = self.MODELS_TO_RESIZE_NAMESPACE_CAPACITY[self.model]

    def is_multipath_support(self):
        """Returns multipath capability
        Controller Multi-Path I/O and Namespace Sharing Capabilities (CMIC):
        Bit 2: '1' - SR-IOV Virtual Function. '0' - PCI Function.
        Bit 1: '1' - NVM subsystem may contain two or more controllers. '0' - only a single controller.
        Bit 0: '1' - NVM subsystem may contain two or more PCI-E ports. '0' - only a single PCI-E port."""
        proc = _popen(["nvme", "id-ctrl", self.dev_path], stdout=subprocess.PIPE)
        out, _ = proc.communicate()
        if [line.split(":")[1].strip() for line in out.decode("ascii").splitlines() if 'cmic' in line][0] == "0":
            return False
        return True

    def create_ns(self, ns_id: int, size_block: int = None):
        """Creates namespace identified by ns_id (should be guessed, though) with a predefined size.
        If size_block is not defined, fills entire nvme with a single namespace"""
        controller_id = self.get_controller_id()

        _check_call(["nvme", "create-ns", self.dev_path, "-s{}".format(size_block), "-c{}".format(size_block),
                     "-f{}".format(self.LBA_FORMAT), "-d0", "-m{}".format(int(self.is_multipath_support()))],
                    _dryrun=self.dryrun)
        _check_call(["nvme", "attach-ns", self.dev_path, "-n{}".format(ns_id), "-c0x{:x}".format(controller_id)],
                    _dryrun=self.dryrun)

    def validate_ns(self, ns_id: int):
        lba_format = self.get_lba_format(ns_id)

        try:
            if lba_format.block_size != self.BLOCK_SIZE:
                raise Exception("Incorrect block size {} for NS#{}".format(lba_format.block_size, ns_id))
            if lba_format.metadata_size != 0:
                raise Exception("Incorrect metadata size {} for NS#{}".format(lba_format.metadata_size, ns_id))
        except Exception:
            if self.dryrun:
                return

            raise

    def delete_ns(self, ns_id: int):
        """Detaches and deletes specified nvme"""
        controller_id = self.get_controller_id()

        _check_call(["nvme", "detach-ns", self.dev_path, "-n{}".format(ns_id), "-c0x{:x}".format(controller_id)],
                    _dryrun=self.dryrun)
        _check_call(["nvme", "delete-ns", self.dev_path, "-n{}".format(ns_id)], _dryrun=self.dryrun)

    def blocks2gib(self, size_block: int) -> int:
        return size_block * self.BLOCK_SIZE // 10 ** 9


class SamsungNvmeDevice(NvmeDevice, metaclass=NvmeDeviceMeta):
    VENDOR_ID = "0x144d"
    VENDOR_NAME = "Samsung"


class MicronNvmeDevice(NvmeDevice, metaclass=NvmeDeviceMeta):
    VENDOR_ID = "0x1344"
    VENDOR_NAME = "Micron"


class IntelNvmeDevice(NvmeDevice, metaclass=NvmeDeviceMeta):
    VENDOR_ID = "0x8086"
    VENDOR_NAME = "Intel"


class HuaweiNvmeDevice(NvmeDevice, metaclass=NvmeDeviceMeta):
    class NSCommands:
        CREATE = "0"
        DELETE = "1"
        ATTACH = "2"
        DETACH = "3"
        SET_NUMBER = "6"

    VENDOR_ID = "0x19e5"
    VENDOR_NAME = "Huawei"

    HIO_LBA_FORMAT_4K = 1  # according to ES3000 V3 NVMe PCIe SSD User Guide, p.102

    def initialize(self, ns_count: int):
        # Set max namespace number
        with TemporarySymbolicLink(MTAB_LINK_SRC, MTAB_PATH):
            _check_call(["hioadm", "namespace", "-d", self.name, "-t", HuaweiNvmeDevice.NSCommands.SET_NUMBER,
                         "-n", str(ns_count)], _dryrun=self.dryrun)

            if self.BLOCK_SIZE == 4096:
                # Format using 4K blocks, 0 metadata
                proc = _popen(["hioadm", "format", "-d", self.name, "-t", str(self.HIO_LBA_FORMAT_4K)],
                              stdin=subprocess.PIPE, _dryrun=self.dryrun)
                if not self.dryrun:
                    proc.communicate(b"y\n")


class WDNvmeDevice(NvmeDevice, metaclass=NvmeDeviceMeta):
    VENDOR_ID = "0x1b96"
    VENDOR_NAME = "WD"


def _check_call(cmdargs, *args, **kwargs):
    log.info("Running %r...", " ".join(map(pipes.quote, cmdargs)))
    if kwargs.pop("_dryrun", False):
        return

    with open(os.devnull, "r") as devnullf:
        kwargs.setdefault("stdin", devnullf)
        return subprocess.check_call(cmdargs, *args, **kwargs)


def _popen(cmdargs, *args, **kwargs):
    log.info("Running %r...", " ".join(map(pipes.quote, cmdargs)))
    if kwargs.pop("_dryrun", False):
        return

    with open(os.devnull, "r") as devnullf:
        kwargs.setdefault("stdin", devnullf)
        return subprocess.Popen(cmdargs, *args, **kwargs)


def _get_nvme_pciid(name):
    return os.path.basename(os.readlink(os.path.join(SYS_CLASS_NVME_PATH, name, "device")))


def _get_nvme_device_class(name):
    with open(os.path.join(SYS_CLASS_NVME_PATH, name, "device/vendor")) as vendorf:
        vendorid = vendorf.read().strip()

    return NVME_DEVICE_CLASSES.get(vendorid)


def _get_nvme_device_model(name):
    with open(os.path.join(SYS_CLASS_NVME_PATH, name, "model")) as modelf:
        return modelf.read().strip()


def wait_for_device(dev: NvmeDevice, ns_id: int = None) -> str:
    dev_path = dev.dev_path
    if ns_id is not None:
        dev_path = "/dev/{}n{}".format(dev.name, ns_id)

    if dev.dryrun:
        return dev_path

    for _ in range(3):
        try:
            with open(dev_path):
                return dev_path
        except IOError:
            log.warning("Waiting for %r to arrive...", dev_path)

        time.sleep(2)

    raise Exception("Rescan of {!r} has timed out!".format(dev_path))


def get_nvme_device(name):
    cls = _get_nvme_device_class(name)
    if cls is None:
        raise Exception("Unknown vendor of nvme {!r}".format(name))

    return cls(name)


def split(profile: NvmeProfile, force=False, dryrun=False):
    try:
        dev = get_nvme_device(profile.name)
        dev.dryrun = dryrun
        if not os.path.exists(dev.dev_path):
            raise FileNotFoundError(dev.dev_path)
    except FileNotFoundError:
        log.warning("NVMe device %r doesn't exist, skipping", profile.name)
        return

    namespaces = dev.get_namespaces()
    if _validate_namespaces(dev, namespaces, profile) and not force:
        log.info("%r already matches profile %r, won't resplit it unless force mode is specified",
                 dev.name, profile)
        return

    for ns_id in namespaces:
        dev.delete_ns(ns_id)
    dev.rescan()

    dev.initialize(profile.ns_count)
    for ns in profile.namespaces:
        if not ns.blocks_count:
            if ns.id != 1:
                raise Exception("Cannot create namespace which consumes entire capacity, but not first!")
            size_block = dev.get_capacity()
        else:
            size_block = ns.blocks_count

        dev.create_ns(ns.id, size_block)
        dev.validate_ns(ns.id)
    dev.rescan()

    for ns in profile.namespaces:
        create_whole_partition(dev, ns.id)


def create_whole_partition(dev: NvmeDevice, ns_id: int):
    """Restores partiion created by setup on the nvme block device (it's needed by disk_part_label)"""
    log.info("Restoring primary partition on %s", dev.name)

    ns_path = wait_for_device(dev, ns_id)
    _check_call(["parted", ns_path, "mklabel", "gpt"], _dryrun=dev.dryrun)
    _check_call(["parted", "-a", "optimal", ns_path, "mkpart", "primary", "0%", "100%"],
                _dryrun=dev.dryrun)
    _check_call(["partprobe", ns_path], _dryrun=dev.dryrun)


def _validate_namespaces(dev: NvmeDevice, namespaces: List[int], profile: NvmeProfile) -> bool:
    if namespaces != [ns.id for ns in profile.namespaces]:
        log.info("Invalid set of namespaces %r found!", namespaces)
        return False

    for ns in profile.namespaces:
        # Now we just show the warning message about block size mismatch CLOUD-61756
        block_size = dev.get_lba_format(ns.id).block_size
        if block_size != dev.BLOCK_SIZE:
            log.warning("Invalid block size %d in NS #%d (nothing will be done)!", block_size, ns.id)

        size_block = dev.get_ns_size(ns.id)
        if ns.blocks_count:
            expected_ns_size = ns.blocks_count
            # TODO(staerist): this is hack for CLOUD-56931
            if dev.VENDOR_NAME == "Intel":
                expected_ns_size += 1819788
            if abs(size_block - expected_ns_size) > 16:
                log.warning("Invalid size %d blocks (%d gib) of NS #%d!", size_block, expected_ns_size, ns.id)
                return False
            return True
        elif ns.blocks_count is None and abs(size_block - dev.get_capacity()) > 16:
            log.warning("Invalid size %d blocks of NS #%d (full capacity expected)!", size_block, ns.id)
            return False

    return True


def prepare_disks_profiles(disks_data: Dict[str, Any]) -> List[NvmeProfile]:
    new_profiles = []
    ns_id_pattern = re.compile(r"^.*n(?P<id>\d+)$")
    for disk_name, disk_data in disks_data.items():
        log.debug("for disk '%s' specified owner '%s'", disk_name, disk_data.get("Owner") or "unspecified")
        profile = NvmeProfile(disk_name, [])
        new_profiles.append(profile)
        for ns_name, ns_data in disk_data.get("Namespaces", {}).items():
            match = ns_id_pattern.match(ns_name)
            if not match:
                log.warning("can't get namespace id from '%s'", ns_name)
                continue
            profile.namespaces.append(NvmeNamespace(int(match.group("id")), ns_data.get("BlocksCount")))
        profile.namespaces.sort(key=lambda ns: ns.id)
    return new_profiles


if __name__ == "__main__":
    def _usage(msg=None, rc=0, stream=sys.stdout):
        if msg:
            print(msg, file=stream)
        print("Usage: nvme_split.py [-n] [-f] [DEVICENAME]", file=stream)
        sys.exit(rc)

    logging.basicConfig(
        format="%(asctime)-12s %(levelname)s: %(message)s",
        level=logging.INFO,
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    profiles = prepare_disks_profiles(InventoryApi().get_disks_data(host=socket.gethostname()))
    force_flag = False
    dryrun_flag = False
    opts, args = getopt.getopt(sys.argv[1:], "fhn")
    if any("/" in arg for arg in args):
        _usage("Device paths are not supported in device names, use names instead.", 1, sys.stderr)

    names = args if args else [profile.name for profile in profiles]

    for opt, _ in opts:
        if opt == "-f":
            force_flag = True
        elif opt == "-n":
            dryrun_flag = True
        elif opt == "-h":
            _usage()

    for profile in profiles:
        if profile.name not in names:
            continue
        split(profile, force=force_flag, dryrun=dryrun_flag)
