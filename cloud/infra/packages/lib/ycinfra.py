#!/ust/bin/env python
import json
import logging
import re
import socket
import shlex
import subprocess
import os
import urllib3
from collections import defaultdict
from urllib3.connection import HTTPConnection
from time import sleep

SYS_BLOCK_PATH = "/sys/block/"
SYS_BUS_PATH = "/sys/bus/"
SYS_CLASS_PATH = "/sys/class/"
DEV_PATH = "/dev/"

SUCCESS_EXIT_CODE = 0
ERROR_EXIT_CODE = 1
HWWATCHER_RETRY_EXIT_CODE = 123
UNIX_SOCKET_TIMEOUT = 5


class DiskOwners:
    LABEL_PATH = "/dev/disk/by-partlabel"
    KIKIMR_NVME_LABEL_PATTERN = "NVMEKIKIMR"
    KIKIMR_ROT_LABEL_PATTERN = "ROTKIKIMR"
    NBS_LABEL_PATTERN = "NVMENBS"
    COMPUTE_LABEL_PATTERN = "NVMECOMPUTE"
    KIKIMR = "kikimr"
    NBS = "nbs"
    MDB = "mdb"
    DEDICATED = "dedicated"
    SYSTEM_RAID = "system_raid"
    NO_OWNER = "no_owner"
    UNKNOWN = "unknown"

    LABELED_OWNERS = {
        # KIKIMR: {"label_pattern": KIKIMR_LABEL_PATTERN},
        NBS: {"label_pattern": NBS_LABEL_PATTERN, "labels_number": 0},
        MDB: {"label_pattern": COMPUTE_LABEL_PATTERN, "labels_number": 0},
        DEDICATED: {"label_pattern": COMPUTE_LABEL_PATTERN, "labels_number": 0},
    }
    COMPUTE = [MDB, DEDICATED]
    FRAGILE = [MDB, NBS, DEDICATED]
    ALL_LABELED_OWNERS = list(LABELED_OWNERS.keys())


logger = logging.getLogger()


def get_hostname(short=False):
    if not short:
        return socket.gethostname()
    return socket.gethostname().split(".")[0]


class InfraProxyException(Exception):
    pass


class DiskNotFoundException(InfraProxyException):
    pass


class DiskOwnerNotFoundException(InfraProxyException):
    pass


class KikimrApiException(Exception):
    pass


class KikimrErrorClusterWithoutStorageNodes(KikimrApiException):
    pass


class KikimrErrorConnectionProblemToCluster(KikimrApiException):
    pass


class KikimrErrorNodeDisconnected(KikimrApiException):
    pass


class KikimrErrorPDiskStateInfo(KikimrApiException):
    pass


class Popen(object):
    @staticmethod
    def communicate(command, stdin=None):
        if not isinstance(command, (list, tuple)):
            command = shlex.split(command)
        try:
            logger.debug("Executing command: %s", command)
        except NameError:
            pass
        p = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        if stdin:
            stdout, stderr = p.communicate(str.encode(stdin))
        else:
            stdout, stderr = p.communicate()
        return p.returncode, stdout.decode(), stderr.decode()

    def exec_command(self, command, retry=1, retry_backoff=10):
        """ Run any commands by subprocess and retry X times if it fails """
        returncode = 1
        stdout = ""
        stderr = ""
        for try_num in range(retry):
            returncode, stdout, stderr = self.communicate(command)
            if returncode != 0 and try_num != retry - 1:
                message = "Return code {} of {!r}, I will retry it".format(returncode, command)
                try:
                    logger.info(message)
                except NameError:
                    print(message)
                sleep(retry_backoff)
                continue
            break
        return returncode, stdout, stderr


class Req(object):
    ALLOWED_METHODS = {'GET', 'DELETE', 'HEAD', 'PUT', 'POST'}
    CONN_TIMEOUT = 5
    TIMEOUT = 30
    RETRIES = 3

    def make_request(self, method, url, data=None, params=None, headers=None):

        if method not in self.ALLOWED_METHODS:
            logging.error("Method '%s' is not allowed", method)
            return None
        if data:
            data = json.dumps(data)
        try:
            http = urllib3.PoolManager(
                timeout=urllib3.Timeout(connect=self.CONN_TIMEOUT, read=self.TIMEOUT),
                retries=urllib3.Retry(self.RETRIES)
            )
            # fields - Query parameters
            # body - data for requests
            try:
                req = http.request(method=method, url=url, headers=headers, fields=params, body=data)
            except (urllib3.exceptions.HTTPError, TypeError, ValueError) as err:
                logging.error(err)
                return None
            # TODO(nuraev): remove this hack after kikimr api migrates the api to https
            if req.status == 400:
                try:
                    req = http.request(
                        method=method, url=url.replace("http://", "https://"), headers=headers, fields=params, body=data
                    )
                except Exception as err:
                    logging.error(err)
                    return None
        except urllib3.exceptions.HTTPError as err:
            logger.error(err)
            return None
        if req.status == 200:
            return req.data.decode("utf-8")

    def make_request_json(self, method, url, data=None, params=None, headers=None):
        response = self.make_request(method=method, url=url, data=data, params=params, headers=headers)
        if response is None:
            return None
        try:
            return json.loads(response)
        except json.JSONDecodeError as err:
            logging.error("Got error while parsing json: %s", err)
            return None


class UnixHTTPConnection(HTTPConnection):
    def __init__(self, host, **kwargs):
        self.socket_path = host
        super(UnixHTTPConnection, self).__init__(host='localhost', **kwargs)
        self.sock = None

    def _new_conn(self):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.settimeout(UNIX_SOCKET_TIMEOUT)
        sock.connect(self.socket_path)
        self.sock = sock
        return sock

    def __del__(self):
        if self.sock:
            self.sock.close()


class UnixHTTPConnectionPool(urllib3.HTTPConnectionPool):
    scheme = "unix"
    ConnectionCls = UnixHTTPConnection


class UnixReq(object):
    @staticmethod
    def make_request(method, socket_path, url):
        try:
            req = UnixHTTPConnectionPool(host=socket_path).urlopen(method, url)
        except Exception as ex:
            logger.error("Unix socket request error: %s", str(ex))
            return {}
        if req.status != 200:
            logger.error("Unix socket request failed with status '%s'\n'%s'", req.status, req.data.decode())
            return {}
        return req.data.decode()

    def make_request_json(self, method, socket_path, url):
        response = self.make_request(method=method, socket_path=socket_path, url=url)
        try:
            return json.loads(response)
        except json.JSONDecodeError as err:
            logging.error("Got error while parsing json: %s", err)
            return None


class InventoryApi(Req):
    __slots__ = ("__api_url",)
    conn_timeout = 5
    timeout = 30
    retries = 3

    def __init__(self):
        self.__api_url = os.environ.get("INFRA_PROXY", "https://infra-proxy.cloud.yandex.net").rstrip('/')

    def get_disks_data(self, host):
        url = "{}/get_disks_data/{}".format(self.__api_url, host)
        raw_data = self.make_request_json("GET", url=url)
        if raw_data is None:
            raise InfraProxyException("Can't decode answer from infra-proxy data for host %s" % host)
        try:
            disks_data = raw_data["Disks"]
        except KeyError:
            raise InfraProxyException("Can't get disk data for host %s in infra-proxy data: %s" % (host, url))
        return disks_data

    def get_disk_owner(self, host, disk):
        disks_data = self.get_disks_data(host)
        try:
            disk_data = disks_data[disk]
        except KeyError as err:
            raise DiskNotFoundException(
                "Can't get data for disk %s in infra-proxy response: '%s'\nError:%s" % (disk, disks_data, err))
        try:
            return disk_data["Owner"]
        except KeyError:
            raise DiskOwnerNotFoundException("Can't find owner for disk %s on host %s in infra-proxy data: %s" % (
                disk, host, disks_data[disk]))

    def get_disks_namespaces(self, host, disk):
        disks_data = self.get_disks_data(host)
        try:
            return disks_data[disk]["Namespaces"]
        except KeyError:
            raise Exception("Can't found namespaces for disk %s on host %s in infra-proxy data: %s" % (
                disk, host, disks_data[disk]))

    def get_gpu_platform(self, host):
        url = "{}/gpu_platform/{}".format(self.__api_url, host)
        gpu_platform = self.make_request("GET", url=url)
        if gpu_platform is None:
            raise InfraProxyException("Can't get gpu platform for host %s in infra-proxy: %s" % (host, url))
        return gpu_platform

    def get_cpu_platform(self, host):
        url = "{}/cpu_platform/{}".format(self.__api_url, host)
        cpu_platform = self.make_request("GET", url=url)
        if cpu_platform is None:
            raise InfraProxyException("Can't get cpu platform for host %s in infra-proxy: %s" % (host, url))
        return cpu_platform


class WalleClient(Req):

    def __init__(self, useragent='yc-setup'):
        self.__api_url = os.environ.get("WALLE_URL", "https://api.wall-e.yandex-team.ru").rstrip('/')
        self.__headers = {'Content-Type': 'application/json;charset=UTF-8',
                          'User-Agent': useragent,
                          }

    def get_project(self, hostname):
        params = {'fields': 'project'}
        url = '{}/hosts/{}'.format(self.__api_url, hostname)
        res = self.make_request_json(url=url, params=params)
        if res is None:
            raise Exception('Walle project cannot be obtained from walle api')
        if res.get('result', '') == 'FAIL':
            if 'message' in res:
                message = res['message']
            else:
                message = res.data.decode('utf-8').rstrip()
            raise Exception('Wall-E request error: %s, http_code: %s' % (message, res.status))
        assert 'project' in res, 'No field `project` in Wall-E response!'
        return res['project']


class Lsblk(Popen):
    BIN_PATH = "/bin/lsblk"

    @staticmethod
    def all(path=None, columns=None):
        cmd = [Lsblk.BIN_PATH, "--json"]
        if columns:
            cmd.extend(("--output", ",".join(columns)))
        if path:
            cmd.append(path)
        _, stdout, _ = Lsblk.communicate(cmd)
        return json.loads(stdout)

    @staticmethod
    def get_partlabel(partition_path):
        res = Lsblk.all(path=partition_path, columns=["PARTLABEL"])
        return res["blockdevices"][0]["partlabel"]

    @staticmethod
    def get_partsize(partition_path):
        cmd = [Lsblk.BIN_PATH, "--json", "-b", "--output", "SIZE", partition_path]
        ret, stdout, stderr = Lsblk.communicate(cmd)
        if ret:
            logger.error("Command: %s. ret_code: %s. STDERR: %s", " ".join(cmd), ret, stderr)
            return 0
        partsize = json.loads(stdout)
        try:
            return int(partsize["blockdevices"][0]["size"])
        except KeyError as err:
            logger.error("Command: %s. Wrong key: %s", " ".join(cmd), err)
        except ValueError as err:
            logger.error("Command: %s returned wrong value: %s", " ".join(cmd), err)
        except IndexError as err:
            logger.error("Command: %s. Wrong index: %s", " ".join(cmd), err)
        return 0

    @staticmethod
    def get_partname_by_partlabel(partition_label):
        cmd = [Lsblk.BIN_PATH, "--json", "--output", "KNAME",
               "{}/{}".format(DiskOwners.LABEL_PATH, partition_label)]
        ret, stdout, stderr = Lsblk.communicate(cmd)
        if ret:
            logger.error("Command: %s. ret_code: %s. STDERR: %s", " ".join(cmd), ret, stderr)
            return None
        partsize = json.loads(stdout)
        try:
            return partsize["blockdevices"][0]["kname"]
        except KeyError as err:
            logger.error("Command: %s. Wrong key %s", " ".join(cmd), err)
        except IndexError as err:
            logger.error("Command: %s. Wrong index %s", " ".join(cmd), err)
        return None


class Systemctl(Popen):
    BIN = "/bin/systemctl"

    @staticmethod
    def restart(name):
        logger.info("Restarting %s service", name)
        return Systemctl.communicate([Systemctl.BIN, "restart", name])

    @staticmethod
    def stop(name):
        logger.info("Stopping %s service", name)
        return Systemctl.communicate([Systemctl.BIN, "stop", name])

    @staticmethod
    def start(name):
        logger.info("Starting %s service", name)
        return Systemctl.communicate([Systemctl.BIN, "start", name])


class Device(Popen):
    __slots__ = ("_id", "_short", "_type")
    REMOVE_DEVICE_CHAR = "1"

    def __init__(self, short_name, _id, _type):
        self._id = _id
        self._short = short_name
        self._type = _type

    def __str__(self):
        return "{}({})".format(self.__class__.__name__, self.name)

    def __lt__(self, other):
        return self.id < other.id

    def __hash__(self):
        return hash(self.name)

    def __eq__(self, other):
        return hash(self) == hash(other)

    @property
    def _size_path(self):
        return os.path.join(SYS_BLOCK_PATH, self.name, "size")

    @property
    def _block_size_path(self):
        return os.path.join(SYS_BLOCK_PATH, self.name, "queue", "hw_sector_size")

    @property
    def _holders_path(self):
        return os.path.join(SYS_BLOCK_PATH, self.name, "holders")

    @property
    def id(self):
        return self._id

    def is_exists(self):
        """Check that disk exists in system"""
        if not os.path.exists("{}".format(self.path)):
            logger.debug("Device %s wasn't found in the system.", self.name)
            return False
        logger.info("Device %s was found in the system.", self.name)
        return True

    @property
    def is_hdd(self):
        return self._type == "hdd"

    @property
    def is_md(self):
        return self._type == "md"

    @property
    def is_nvme(self):
        return self._type == "nvme"

    def holders(self):
        return [new_device_object(x, "md") for x in os.listdir(self._holders_path)]

    @property
    def name(self):
        return self._short + str(self._id)

    @property
    def path(self):
        return DEV_PATH + self.name

    def size(self):
        """Size in blocks"""
        with open(self._size_path) as fd:
            return int(fd.readline())

    def block_size(self):
        with open(self._block_size_path) as fd:
            return int(fd.readline().strip())

    @property
    def short_name(self):
        return self._short

    @property
    def type(self):
        return self._type


class DeviceNvme(Device):
    """
    List of valid NVME models on wiki: https://nda.ya.ru/t/0rrlnrXy3iMAPb
    0x19e5 Huawei   https://devicehunt.com/search/type/pci/vendor/19E5/device/any
    0x144d Samsung  https://devicehunt.com/search/type/pci/vendor/144d/device/any
    0x8086 Intel    https://devicehunt.com/search/type/pci/vendor/8086/device/any
    0x1344 Micron   https://devicehunt.com/search/type/pci/vendor/1344/device/any
    """
    VALID_NVME_MODELS = {
        "0x19e5": {
            "HWE32P43016M000N",
            "HWE32P43032M000N",
        },
        "0x144d": {
            "SAMSUNG MZWLL1T6HEHP-00003",
            "SAMSUNG MZWLL3T2HMJP-00003",
            "SAMSUNG MZWLL1T6HAJQ-00005",
            "SAMSUNG MZWLL3T2HAJQ-00005",
            "SAMSUNG MZWLJ3T8HBLS-00007",
            "SAMSUNG MZWLR3T8HBLS-00007",
        },
        "0x8086": {
            "INTEL SSDPE2KE016T8",
            "INTEL SSDPE2KE032T8",
            "INTEL SSDPF2KX038TZ",
        },
        "0x1344": {
            "Micron_9300_MTFDHAL3T2TDR",
        },
        "0x1b96": {
            "WUS4BA138DSP3X1",
            "WUS4C6432DSP3X1",
        }
    }

    def __init__(self, short_name, _id):
        super(DeviceNvme, self).__init__(short_name, _id, "nvme")

    @property
    def size(self):
        raise Exception("size for nvme root device is not supported yet")

    @property
    def holders(self):
        raise Exception("holder for nvme root device is not supported")

    def firmware_ver(self):
        with open(os.path.join(SYS_CLASS_PATH, "nvme", self.name, "firmware_rev")) as fd:
            return fd.readline().strip()

    def model(self):
        with open(os.path.join(SYS_CLASS_PATH, "nvme", self.name, "model")) as fd:
            return fd.readline().strip()

    def namespaces(self):
        result = set()
        for x in os.listdir(DEV_PATH):
            if x.startswith(self.name) and len(x) > len(self.name):
                result.add(new_device_object(x, "namespace"))
        return sorted(result)

    def pci_addr(self):
        """Get NVME PCI Address"""
        pci_addr_string = "PCI_SLOT_NAME"
        nvme_info_file = os.path.join(SYS_CLASS_PATH, "nvme", self.name, "device/uevent")
        with open(nvme_info_file) as file:
            for line in file:
                if pci_addr_string in line:
                    pci_addr = line.split("=")[1].strip()
                    logger.info("PCI addr of deleted NVME is %s", pci_addr)
                    return pci_addr

    def remove(self):
        """Remove disk from system"""
        delete_device_file = os.path.join(SYS_BUS_PATH, "pci/devices", self.pci_addr(), "remove")
        with open(delete_device_file, "w") as file:
            file.write(self.REMOVE_DEVICE_CHAR)
        logger.info("Disk %s was successfully removed from system", self.name)

    def vendor(self):
        with open(os.path.join(SYS_CLASS_PATH, "nvme", self.name, "device/vendor")) as fd:
            return fd.readline().strip()


class DeviceNvmeNamespace(Device):
    __slots__ = ("_disk",)

    def __init__(self, short_name, _id, disk):
        super(DeviceNvmeNamespace, self).__init__(short_name, _id, "nvme")
        self._disk = disk

    @property
    def disk(self):
        return self._disk

    def first_partition(self):
        partitions = self.partitions()
        if partitions:
            return partitions[0]

    def partitions(self):
        result = set()
        for x in os.listdir(DEV_PATH):
            if x.startswith(self.name + "p"):
                result.add(new_device_object(x, "partition"))
        return sorted(result)


class DeviceNvmePartition(Device):
    __slots__ = ("_namespace",)

    def __init__(self, short_name, _id, namespace):
        super(DeviceNvmePartition, self).__init__(short_name, _id, "nvme")
        self._namespace = namespace

    @property
    def _holders_path(self):
        return os.path.join(SYS_BLOCK_PATH, self._namespace.name, self.name, "holders")

    @property
    def _size_path(self):
        return os.path.join(SYS_BLOCK_PATH, self._namespace.name, self.name, "size")


class DeviceHdd(Device):
    __slots__ = ()

    def __init__(self, short_name, _id):
        super(DeviceHdd, self).__init__(short_name, _id, "hdd")

    @property
    def id(self):
        return self._id

    def remove(self):
        """Remove disk from system"""
        with open(os.path.join(SYS_BLOCK_PATH, self.name, "device/delete"), "w") as fd:
            fd.write(self.REMOVE_DEVICE_CHAR)
        logger.info("Disk %s was successfully removed from system", self.name)

    def first_partition(self):
        partitions = self.partitions()
        if partitions:
            return partitions[0]

    def model(self):
        with open(os.path.join(SYS_BLOCK_PATH, self.name, "device/model")) as fd:
            return fd.readline().strip()

    def partitions(self):
        result = set()
        for x in os.listdir(DEV_PATH):
            if x.startswith(self.name) and len(x) > len(self.name):
                result.add(new_device_object(x, "partition"))
        return sorted(result)

    def vendor(self):
        with open(os.path.join(SYS_BLOCK_PATH, self.name, "device/vendor")) as fd:
            return fd.readline().strip()


class DeviceHddPartition(Device):
    __slots__ = ("_disk",)

    def __init__(self, short_name, _id, disk):
        super(DeviceHddPartition, self).__init__(short_name, _id, "hdd")
        self._disk = disk

    @property
    def disk(self):
        return self._disk

    @property
    def _holders_path(self):
        return os.path.join(SYS_BLOCK_PATH, self._disk.name, self.name, "holders")

    @property
    def _size_path(self):
        return os.path.join(SYS_BLOCK_PATH, self._disk.name, self.name, "size")


class DeviceMd(Device):
    RAID_GOOD_STATUS = 0

    def __init__(self, short_name, _id):
        super(DeviceMd, self).__init__(short_name, _id, "md")

    def slaves(self):
        result = []
        for slave in os.listdir(os.path.join(SYS_BLOCK_PATH, self.name, "slaves")):
            result.append(new_device_object(slave, _type="partition"))
        return result


class NvmeCli(Popen):
    BIN = "/usr/sbin/nvme"

    @staticmethod
    def id_ctrl(name):
        ret, data, err = NvmeCli.communicate(
            (NvmeCli.BIN, "id-ctrl", DEV_PATH + name, "--output-format=json"))
        if ret != 0:
            raise Exception(err)
        return json.loads(data)

    @classmethod
    def _get_param(cls, name, param):
        try:
            return cls.id_ctrl(name)[param]
        except KeyError:
            raise Exception("unknown nvme-cli parameter %s" % param)

    @classmethod
    def get_tnvmcap(cls, name):
        """This method returns device capacity in blocks"""
        return int(cls._get_param(name, "tnvmcap"))

    @classmethod
    def get_cntlid(cls, name):
        """This method returns NVME controller ID"""
        return int(cls._get_param(name, "cntlid"))

    @staticmethod
    def list_ns(name):
        ret, data, err = NvmeCli.communicate((NvmeCli.BIN, "list-ns", DEV_PATH + name, "--all"))
        if ret != 0:
            raise Exception(err)
        namespaces = []
        disk = new_device_object(name)
        for line in data.split("\n"):
            if line.find(":") == -1:
                continue
            _, hex_id = line.split(":")
            _id = int(hex_id.strip(), 16)
            namespaces.append(DeviceNvmeNamespace(disk.name + "n", _id, disk))
        return sorted(namespaces)

    @staticmethod
    def reset(name):
        ret, _, err = NvmeCli.communicate((NvmeCli.BIN, "reset", DEV_PATH + name))
        if ret != 0:
            logger.error("command failed. Stderr: %s", err)
            return False
        return True

    @staticmethod
    def detach_ns(name, ns, cntlid):
        ret, _, err = NvmeCli.communicate(
            (NvmeCli.BIN, "detach-ns", DEV_PATH + name, "-n", hex(ns), "-c", hex(cntlid)))
        if ret != 0:
            logger.error("command failed. Stderr: %s", err)
            return False
        return True

    @staticmethod
    def delete_ns(name, ns):
        ret, _, err = NvmeCli.communicate((NvmeCli.BIN, "delete-ns", DEV_PATH + name, "-n", hex(ns)))
        if ret != 0:
            logger.error("command failed. Stderr: %s", err)
            return False
        return True

    @staticmethod
    def create_ns(name, size_blocks):
        ret, _, err = NvmeCli.communicate(
            (NvmeCli.BIN, "create-ns", DEV_PATH + name, "-s", str(size_blocks), "-c", str(size_blocks)))
        if ret != 0:
            logger.error("command failed. Stderr: %s", err)
            return False
        return True

    @staticmethod
    def attach_ns(name, ns, controller_id):
        ret, _, err = NvmeCli.communicate(
            (NvmeCli.BIN, "attach-ns", DEV_PATH + name, "-n", hex(ns), "-c", hex(controller_id)))
        if ret != 0:
            logger.error("command failed. Stderr: %s", err)
            return False
        return True


class NvmeDisk(NvmeCli):
    DEFAULT_BLOCK_SIZE_KB = 512
    # NOTE(CLOUD-90323) INTEL SSDPF2KX038TZ ns size should be <=3.2TB
    MODELS_TO_RESIZE_NAMESPACE_CAPACITY = {"INTEL SSDPF2KX038TZ": 3200 * 10 ** 9}

    def __init__(self, name):
        self.name = name
        self.nvme_controller_id = self.get_cntlid(name)
        self.pciid = self._get_nvme_pciid(name)
        self.model = self._get_nvme_device_model(name)

    def get_namespaces_info(self):
        nvme_controller_id = self.nvme_controller_id
        logger.debug("controller id: %s", hex(nvme_controller_id))
        active_namespaces = self.list_ns(self.name)
        logger.debug("active namespaces: %s", [x.name for x in active_namespaces])
        current_state = defaultdict(lambda: defaultdict(int))
        if active_namespaces:
            for ns in active_namespaces:
                ns_obj = new_device_object(ns.name, _type="namespace")
                current_state[ns.name]["BlockSize"] = ns_obj.block_size()
                if not current_state[ns.name]["BlockSize"]:
                    raise Exception("can't get namespace block size %s (probably not attached)" % ns)
                current_state[ns.name]["BlocksCount"] = ns_obj.size()
                if not current_state[ns.name]["BlocksCount"]:
                    raise Exception("can't get namespace blocks count %s (probably not attached)" % ns)
        return current_state

    def delete_all_namespaces(self):
        logger.debug("controller id: %s", hex(self.nvme_controller_id))
        active_namespaces = self.list_ns(self.name)
        logger.debug("active namespaces: %s", [x.name for x in active_namespaces])
        if active_namespaces:
            for ns in active_namespaces:
                logger.debug("detaching namespace: %s", ns.id)
                if not self.detach_ns(self.name, ns.id, self.nvme_controller_id):
                    logger.error("detaching namespace: %s", ns.id)
                    return False
                if not self.delete_ns(self.name, ns.id):
                    logger.error("deleting namespace: %s", ns.id)
                    return False
        logger.debug("rescan devices ...")
        self.rescan_controller()
        self.rescan()
        return True

    def get_capacity_in_blocks(self, block_size):
        try:
            # NOTE: Hack for CLOUD-90323
            if self.model in self.MODELS_TO_RESIZE_NAMESPACE_CAPACITY:
                return self.MODELS_TO_RESIZE_NAMESPACE_CAPACITY[self.model] / block_size
            return self.get_tnvmcap(self.name) / block_size
        except Exception as ex:
            logger.error("Can not get disk capacity for '%s': %s", self.name, ex)
            return None

    def rescan_controller(self):
        """Rescan nvme controller"""
        logger.info("Rescanning controller %r...", self.name)
        with open("/sys/class/nvme/{}/rescan_controller".format(self.name), "w") as rescan_controller:
            rescan_controller.write("1")

    def rescan(self):
        """Rescan pci bus"""
        logger.info("Rescanning PCI bus")
        with open("/sys/bus/pci/rescan", "w") as rescan_pci_bus:
            rescan_pci_bus.write("1")

    def wait_for_namespace(self, ns_id, wait_timeout_sec=2, retries=6):
        full_namespace_name = "{}n{}".format(self.name, str(ns_id))
        full_namespace_path = os.path.join(DEV_PATH, full_namespace_name)
        for _ in range(retries):
            try:
                with open(full_namespace_path):
                    return full_namespace_path
            except IOError:
                logger.warning("Waiting for %r to arrive...", full_namespace_name)

            sleep(wait_timeout_sec)
        raise Exception("Rescan of %r has timed out!", full_namespace_name)

    @staticmethod
    def _get_nvme_pciid(name):
        return os.path.basename(os.readlink(os.path.join(SYS_CLASS_PATH, "nvme", name, "device")))

    @staticmethod
    def _get_nvme_device_model(name):
        with open(os.path.join(SYS_CLASS_PATH, "nvme", name, "model")) as modelf:
            return modelf.read().strip()


class KikimrDisks(object):
    """Class describes kikimr disks labels"""
    LABEL = {"hdd": "ROTKIKIMR", "nvme": "NVMEKIKIMR"}
    # For NVME ns1 we have two variants of size nvmeXn1p1 - CLOUD-27144
    # 3123671040
    # 3125624832
    # We get lesser of two - 3123671040
    MIN_BLOCK_SIZE = {"hdd": 23435671552, "nvme": 3123527680}
    OBLITERATE_COMMAND = "/Berkanavt/kikimr/bin/kikimr admin blobstorage disk obliterate {}"

    @staticmethod
    def is_kikimr_disk(disk, username=None):
        partition = get_first_partition(disk)
        if not partition:
            logger.warning("Not found partitions on %s", disk.name)
            return False
        logger.info("First partition for %s is: %s", disk.name, partition.name)
        partition_label = Lsblk.get_partlabel(partition.path)
        if not partition_label:
            logger.info("Not found partition label for %s.", partition.name)
            return False
        logger.info("I've found partition label  %s", partition_label)

        prefix = KikimrDisks.LABEL[partition.type]
        if not partition_label.startswith(prefix):
            logger.info("Partition label %s does not belong to kikimr", partition_label)
            return False

        if partition_label in get_kikimr_disks(username):
            logger.info("This is kikimr disk")
            return True
        return False

    @staticmethod
    def get_full_label_path(disk):
        partition = get_first_partition(disk)
        if not partition:
            logger.warning("Not found partitions on %s", disk.name)
            return None
        logger.info("First partition for %s is: %s", disk.name, partition.name)
        partition_label = Lsblk.get_partlabel(partition.path)
        if not partition_label:
            logger.info("Not found partition label for %s.", partition.name)
            return None
        logger.info("I've found partition label  %s", partition_label)
        return "{}/{}".format(DiskOwners.LABEL_PATH, partition_label)

    @staticmethod
    def check_partition_size(partition):
        reference_size = KikimrDisks.MIN_BLOCK_SIZE[partition.type]
        partition_size = partition.size()
        if partition_size < reference_size:
            logger.error("Partition %s size %d less reference %d", partition.name, partition_size, reference_size)
            return False
        logger.info("Partition %s has right size", partition.name)
        return True

    @staticmethod
    def obliterate_disk(partlabel):
        if partlabel is None:
            logging.error("None was provided instead of partlabel")
            return False

        full_command = KikimrDisks.OBLITERATE_COMMAND.format(partlabel)
        # stdout for obliterate command is empty
        ret, _, stderr = Popen.communicate(full_command)
        if ret != 0 or stderr:
            logging.error("During disk obliterating error was raised %s\nExit code: %s", stderr, ret)
            return False
        logging.info("Kikimr disk %s obliterated", partlabel)
        return True


class KikimrViewerApi(Req):
    __environment_file = "/etc/debian_chroot"
    __kikimr_all_proxy = {
        "prod": {
            "global": "ydbproxy.cloud.yandex.net:8765",
            "public": "ydbproxy-public.cloud.yandex.net:8765",
            "vla": "ydbproxy-vla.cloud.yandex.net:8765",
            "sas": "ydbproxy-sas.cloud.yandex.net:8765",
            "myt": "ydbproxy-myt.cloud.yandex.net:8765",
        },
        "pre-prod": {
            "global": "ydbproxy.cloud-preprod.yandex.net:8765",
            "public": "ydbproxy-public.cloud-preprod.yandex.net:8765",
            "vla": "ydbproxy-vla.cloud-preprod.yandex.net:8765",
            "sas": "ydbproxy-sas.cloud-preprod.yandex.net:8765",
            "myt": "ydbproxy-myt.cloud-preprod.yandex.net:8765",
        },
        "testing": {
            "global": "global-ydb.cloud-testing.yandex.net:80",
            "vla": "vla-ydb.cloud-testing.yandex.net:80",
            "sas": "sas-ydb.cloud-testing.yandex.net:80",
            "myt": "myt-ydb.cloud-testing.yandex.net:80",
        },
        "hw-infra-lab": {
            "global": "ydbproxy.hw-infra.cloud-lab.yandex.net:80",
        },
        "israel": {
            "global": "ydbproxy-global.ydb-infra.yandexcloud.co.il:8765",
            "m1a": "ydbproxy-m1a.ydb-infra.yandexcloud.co.il:8765"
        },
    }
    __storage_port = 19001
    __pdisk_ok_status = 10
    __viewer_url_nodelist = "http://{}/viewer/json/nodelist"
    __viewer_url_pdiskinfo = "http://{}/viewer/json/pdiskinfo"

    def __init__(self, username=None):
        self._local_endpoint = "localhost:8765"
        self._fake_local_cluster = "local"
        self.__full_node_name = get_hostname()
        self.__environment = self._get_environment()
        self.__current_env_proxy = self._get_current_env_proxy()
        if username is not None:
            self._iam_token = YcTokenAgent.get_iam_token_for_user(username)
        else:
            self._iam_token = YcTokenAgent.get_iam_token()

    def _get_environment(self):
        with open(self.__environment_file) as env_file:
            return env_file.read().strip().lower()

    def _get_current_env_proxy(self):
        return self.__kikimr_all_proxy.get(self.__environment)

    def _get_proxy_by_cluster_name(self, cluster_name):
        if cluster_name == self._fake_local_cluster:
            return self._local_endpoint
        try:
            return self.__current_env_proxy[cluster_name]
        except KeyError:
            logger.error("Couldn't get proxy by cluster '%s'", cluster_name)
            return None

    def _get_storage_cluster_nodes(self, proxy):
        storage_nodes = []
        headers = {}
        if self._iam_token:
            headers["Authorization"] = "Bearer {}".format(self._iam_token)
        raw_data = self.make_request_json("GET", url=self.__viewer_url_nodelist.format(proxy), headers=headers)
        if not raw_data:
            raise KikimrErrorConnectionProblemToCluster
        for host in raw_data:
            if host["Port"] == self.__storage_port:
                storage_nodes.append(host)
        return storage_nodes

    def get_cluster_nodes_list(self):
        cluster_name, _ = self._get_cluster_and_storage_nodes_id(get_hostname())
        try:
            url = self._get_proxy_by_cluster_name(cluster_name)
        except KeyError:
            return None
        return [node["Host"].encode() for node in self._get_storage_cluster_nodes(url)]

    @staticmethod
    def _get_storage_node_id(all_nodes_cluster, target_node):
        for node in all_nodes_cluster:
            if target_node in node["Host"]:
                return node["Id"]
        return None

    def _get_cluster_and_storage_nodes_id_local(self, node):
        try:
            storage_nodes = self._get_storage_cluster_nodes(self._local_endpoint)
        except KikimrErrorConnectionProblemToCluster:
            return None, None
        if not storage_nodes:
            raise KikimrErrorClusterWithoutStorageNodes
        node_id = self._get_storage_node_id(storage_nodes, node)
        if node_id:
            return self._fake_local_cluster, node_id
        return None, None

    def _get_cluster_and_storage_nodes_id(self, node):
        cluster_name, node_id = self._get_cluster_and_storage_nodes_id_local(node)
        if cluster_name and node_id:
            return cluster_name, node_id
        kikimr_error = None
        for cluster_name, url in self.__current_env_proxy.items():
            try:
                storage_nodes = self._get_storage_cluster_nodes(url)
            except KikimrErrorConnectionProblemToCluster as err:
                kikimr_error = err
                continue
            if not storage_nodes:
                raise KikimrErrorClusterWithoutStorageNodes
            node_id = self._get_storage_node_id(storage_nodes, node)
            if node_id:
                return cluster_name, node_id
        if kikimr_error:
            raise KikimrErrorConnectionProblemToCluster
        return None, None

    def get_cluster_id(self):
        for cluster_name, url in self.__current_env_proxy.items():
            storage_nodes = self._get_storage_cluster_nodes(url)
            if storage_nodes and self._get_storage_node_id(storage_nodes, self.__full_node_name):
                return cluster_name
        return None

    def _get_storage_node_disks(self, proxy, node_id):
        headers = {}
        if self._iam_token:
            headers["Authorization"] = "Bearer {}".format(self._iam_token)
        params = {"node_id": node_id}
        raw_data = self.make_request_json(
            "GET", url=self.__viewer_url_pdiskinfo.format(proxy), params=params, headers=headers)
        if raw_data.get("Error") == "Node disconnected":
            raise KikimrErrorNodeDisconnected
        try:
            return raw_data["PDiskStateInfo"]
        except KeyError:
            raise KikimrErrorPDiskStateInfo

    def get_node_disks(self, failed=False):
        pdisks = []
        cluster, node_id = self._get_cluster_and_storage_nodes_id(self.__full_node_name)
        if not cluster or not node_id:
            return pdisks

        for pdisk_info in self._get_storage_node_disks(self._get_proxy_by_cluster_name(cluster), node_id):
            if failed:
                if pdisk_info["State"] != self.__pdisk_ok_status:
                    pdisks.append(pdisk_info["Path"])
                continue
            else:
                pdisks.append(pdisk_info["Path"])
        return pdisks

    def get_node_disks_size(self):
        pdisks = {}
        cluster, node_id = self._get_cluster_and_storage_nodes_id(self.__full_node_name)
        if not cluster or not node_id:
            return pdisks
        for pdisk_info in self._get_storage_node_disks(self._get_proxy_by_cluster_name(cluster), node_id):
            try:
                pdisks[pdisk_info["Path"]] = int(pdisk_info["TotalSize"])
            except ValueError as err:
                logger.error("Kikimr returned wrong value: %s", err)
            except KeyError as err:
                logger.error("Kikimr returned value with wrong key: %s", err)
        return pdisks


class PMApi(Req):
    _environment_file = "/etc/debian_chroot"
    _pm_urls = {
        "prod": "https://prod-pm.infra.cloud.yandex.net",
        "preprod": "http://infra-pm-dev.cloud.yandex.net",
        # $ cat /etc/debian_chroot
        # PRE-PROD
        "pre-prod": "http://infra-pm-dev.cloud.yandex.net",
        "testing": "http://infra-pm-dev.cloud.yandex.net",
        "hw-infra-lab": "http://infra-pm-dev.cloud.yandex.net",
    }

    def __init__(self, username=None):
        self.full_node_name = get_hostname()
        self.environment = self._get_environment()
        self.pm_url = self._get_pm_url(self._get_environment())
        if username is not None:
            self._iam_token = YcTokenAgent.get_iam_token_for_user(username)
        else:
            self._iam_token = YcTokenAgent.get_iam_token()

    @classmethod
    def _get_environment(cls):
        with open(cls._environment_file) as env_file:
            return env_file.read().strip().lower()

    @classmethod
    def _get_pm_url(cls, env):
        return cls._pm_urls.get(env, "http://infra-pm-dev.cloud.yandex.net")

    def _get_disk_maintenance_url(self):
        return "{}/api/maintenance/disk".format(self.pm_url)

    def get_disk_maintenance_permission(self, disk_name, disk_type, disk_action, dryrun=False):
        print(disk_name, disk_type, disk_action)
        request_content = {
            "node_name": self.full_node_name,
            "env": self.environment,
            "disk_name": disk_name,
            "disk_type": disk_type,
            "action": disk_action,
            "reason": "replace disk",
            "dryrun": dryrun,
        }
        headers = {}
        if self._iam_token:
            headers["Authorization"] = "Bearer {}".format(self._iam_token)
        response = self.make_request_json(
            method="POST", url=self._get_disk_maintenance_url(), data=request_content, headers=headers)
        logger.info("Got answer from PM: %s", response)
        return response


class YcTokenAgent(object):
    GRPC_PATH = "/var/run/yc/token-agent/socket"
    GRPC_HANDLE = "yandex.cloud.priv.iam.v1.TokenAgent/GetToken"
    HTTP_PATH = "/var/run/yc/token-agent/http.sock"
    HTTP_HANDLE = "/tokenAgent/v1/token"

    @classmethod
    def _get_via_grpc(cls, username=None):
        cmd = "grpcurl --plaintext --unix {} {}".format(cls.GRPC_PATH, cls.GRPC_HANDLE)
        if username is not None:
            cmd = "sudo -u {} {}".format(username, cmd)
        try:
            ret_code, stdout, stderr = Popen.communicate(cmd)
            if ret_code == 0:
                return json.loads(stdout)["iam_token"]
            logger.error(
                "getting token from yc-token-service: Exit code %d. Stdout: '%s'. Stderr: '%s'",
                ret_code,
                stdout,
                stderr)
        except Exception as ex:
            logger.error("getting token from yc-token-service: %s: %s", ex.__class__.__name__, ex)

    @classmethod
    def get_iam_token(cls):
        return cls._get_via_grpc()

    @classmethod
    def get_iam_token_for_user(cls, username):
        return cls._get_via_grpc(username)


class KikimrApiResponse(object):
    __slots__ = ("_data",)
    UP_STATUS = "UP"
    DOWN_STATUS = "DOWN"

    def __init__(self, kikimr_response):
        self._data = kikimr_response

    @property
    def hosts(self):
        """Get all hosts in kikimr cluster"""
        return self._data["State"]["Hosts"]

    @property
    def failed_hosts(self):
        """Get failed hosts from kikimr cms"""
        failed_hosts = list()
        for host in self.hosts:
            if host["State"] == self.DOWN_STATUS and host["NodeId"] <= 1024:
                failed_hosts.append(host["Name"])
        return failed_hosts

    @property
    def failed_devices(self):
        """Get failed devices from kikimr cms"""
        failed_disks = {}
        for host in self.hosts:
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


class PartitionManager(Popen):
    PARTED_BIN = "/sbin/parted"
    PARTX_BIN = "/usr/bin/partx"

    @classmethod
    def set_label(cls, namespace, label, partition_id=1):
        logger.info("Setting label '%s' on '%s'", label, namespace)
        cmd = [cls.PARTED_BIN, os.path.join(DEV_PATH, namespace), "name", str(partition_id), label]
        ret, stdout, stderr = cls.communicate(cmd)
        if ret != 0:
            logger.error("Command: %s. ret_code: %s. STDERR: %s", " ".join(cmd), ret, stderr)
            return False
        return True

    @classmethod
    def update_label(cls, namespace):
        logger.info("Updating labels for '%s' in system", namespace)
        cmd = [cls.PARTX_BIN, "-u", os.path.join(DEV_PATH, namespace)]
        ret, stdout, stderr = cls.communicate(cmd)
        if ret != 0:
            logger.error("Command: %s. ret_code: %s. STDERR: %s", " ".join(cmd), ret, stderr)
            return False
        return True

    @classmethod
    def create_whole_partition(cls, namespace):
        logger.info("Creating primary partition on %s", namespace)
        new_partition_name = cls.generate_partition_name(namespace)
        parted_commands = [
            [cls.PARTED_BIN, os.path.join(DEV_PATH, namespace), "mklabel", "gpt"],
            [cls.PARTED_BIN, "-a", "optimal", os.path.join(DEV_PATH, namespace), "mkpart", "primary", "0%", "100%"],
        ]
        for cmd in parted_commands:
            ret, stdout, stderr = cls.communicate(cmd)
            if ret:
                logger.error("Command: %s. ret_code: %s. STDERR: %s", " ".join(cmd), ret, stderr)
                return None

        cls.wait_for_partition(new_partition_name)
        cls.partprobe()

        return new_partition_name

    @classmethod
    def partprobe(cls):
        cmd = ["partprobe"]
        ret, stdout, stderr = cls.communicate(cmd)
        if ret != 0:
            logger.error("Command: %s. ret_code: %s. STDERR: %s", " ".join(cmd), ret, stderr)
            return False

        return True

    @staticmethod
    def wait_for_partition(partition_name, wait_timeout_sec=2, retries=3):
        full_partition_path = os.path.join(DEV_PATH, partition_name)
        for _ in range(retries):
            try:
                with open(full_partition_path):
                    return full_partition_path
            except IOError:
                logger.warning("Waiting for %r to arrive...", full_partition_path)

            sleep(wait_timeout_sec)
        raise Exception("Rescan of {!r} has timed out!".format(full_partition_path))

    @classmethod
    def reload_partition(cls, partition):
        reload_command = "/usr/bin/partx -u {}".format(partition)
        logger.info("Reload partitions in kernel")
        ret, stdout, stderr = cls.communicate(reload_command)
        if ret == 0:
            return True
        logger.error("Command: %s. ret_code: %s. STDERR: %s", reload_command, ret, stderr)
        return False

    @staticmethod
    def generate_partition_name(namespace, partition_id=1):
        new_partition_name = "%sp%s" % (namespace, partition_id)
        return new_partition_name


def new_device_object(path, _type=None):
    name = path.replace(DEV_PATH, "")
    if name.startswith("nvme"):
        if _type is None:
            _type = "disk"
        assert _type in {"disk", "namespace", "partition"}, "type {} is not supported for {}".format(_type, path)
        name = name.replace("nvme", "")
        if name.find("n") != -1:
            d_id, name = name.split("n")
            d_id = int(d_id)
        else:
            if not name:
                raise AssertionError("path {} is not {}".format(path, _type))
            d_id = int(name)
            name = None
        disk = DeviceNvme("nvme", d_id)
        if _type == "disk":
            return disk
        if not name:
            raise AssertionError("path {} is not {}".format(path, _type))
        if name.find("p") != -1:
            n_id, name = name.split("p")
            n_id = int(n_id)
        else:
            n_id = int(name)
            name = None
        namespace = DeviceNvmeNamespace(disk.name + "n", n_id, disk)
        if _type == "namespace":
            return namespace
        if not name:
            raise AssertionError("path {} is not {}".format(path, _type))
        p_id = int(name)
        return DeviceNvmePartition(namespace.name + "p", p_id, namespace)
    elif name.startswith("sd"):
        if _type is None:
            _type = "disk"
        assert _type in {"disk", "partition"}, "type {} is not supported for {}".format(_type, path)
        disk = DeviceHdd("sd", name[2])
        if _type == "disk":
            return disk
        name = name.replace(disk.name, "")
        if not name:
            raise AssertionError("path {} is not {}".format(path, _type))
        p_id = int(name)
        return DeviceHddPartition(disk.name, p_id, disk)
    elif name.startswith("md"):
        if _type is None:
            _type = "md"
        assert _type in {"md"}, "type {} is not supported for {}".format(_type, path)
        md_id = int(name[2])
        return DeviceMd("md", md_id)
    else:
        raise AssertionError("device {} is  not supported".format(name))


def prepare_partitions(new_partition_label, namespace):
    new_partition_name = PartitionManager.create_whole_partition(namespace)
    if not new_partition_name:
        logger.error("can't create partition in namespace %s", namespace)
        return False
    logger.info("Trying to set label %s for %s", new_partition_label, new_partition_name)
    if not PartitionManager.set_label(namespace, new_partition_label):
        return False
    if PartitionManager.update_label(namespace):
        return True
    logger.error("can't set label %s for partition %s!", new_partition_label, new_partition_name)
    return False


def nbs_partlabel_generator(max_namespaces):
    for num in range(1, max_namespaces + 1):
        yield "NVMENBS{:02d}".format(num)
        num += 1


def prepare_namespaces(nvme_disk, namespaces, owner=None):
    namespace_id_re = re.compile(r"nvme\dn(\d+)")
    nvme_controller_id = nvme_disk.nvme_controller_id
    nbs_partlabels = nbs_partlabel_generator(len(namespaces))
    for namespace in sorted(namespaces, key=_sort_nvmes):
        try:
            new_ns_size_blocks = int(namespaces[namespace].get("BlocksCount", 0))
            new_ns_block_size = int(namespaces[namespace].get("BlockSize", 0))
        except ValueError:
            logger.error("incorrect data in inventory for %s", namespace)
            return False
        if new_ns_block_size == 0:
            new_ns_block_size = NvmeDisk.DEFAULT_BLOCK_SIZE_KB
        try:
            new_ns_id = int(re.findall(namespace_id_re, namespace)[0])
        except Exception as err:
            logger.error("can't generate new namespace id for %s: %s", namespace, err)
            return False
        if new_ns_size_blocks == 0:
            if len(namespaces) > 1:
                logger.error("only one namespace without defined size allowed")
                return False
            new_ns_size_blocks = nvme_disk.get_capacity_in_blocks(new_ns_block_size)
        logger.debug("creating namespace via nvme cli: %s", hex(new_ns_id))
        if not nvme_disk.create_ns(nvme_disk.name, new_ns_size_blocks):
            logger.error("can't create namespace %s on %s", hex(new_ns_id), nvme_disk.name)
            return False
        logger.debug("attaching namespace via nvme cli: %s", hex(new_ns_id))
        if not nvme_disk.attach_ns(nvme_disk.name, new_ns_id, nvme_controller_id):
            logger.error("can't attach namespace %s on %s", hex(new_ns_id), nvme_disk.name)
            return False
        nvme_disk.rescan_controller()
        nvme_disk.wait_for_namespace(new_ns_id)
        logger.debug(" via nvme cli: %s", hex(new_ns_id))
        if owner == DiskOwners.NBS:
            if not prepare_partitions(next(nbs_partlabels), namespace):
                return False
    nvme_disk.rescan()

    return True


def prepare_nvme(name, desired_namespaces, owner=None):
    disk = NvmeDisk(name)
    try:
        current_namespaces = disk.get_namespaces_info()
    except Exception as err:
        logger.warning("can't get info about current namespaces for disk: %s", err)
        current_namespaces = {}
    disk_setup_diff = get_disk_actual_state_difference(desired_namespaces, current_namespaces)
    if not disk_setup_diff:
        logger.info("disk %s is in actual state, nothing to do", name)
        return True
    else:
        logger.debug("disk %s is in non-actual state (%s), needs to clean it", name, disk_setup_diff)
        if not disk.delete_all_namespaces():
            logger.error("error occurred during delete all namespaces on %s ", name)
            return False

    if not prepare_namespaces(disk, desired_namespaces, owner):
        logger.error("can't create namespaces for %s", name)
        return False

    return True


def get_disk_actual_state_difference(desired_state, current_state):
    diff_disks = set(current_state.keys()).symmetric_difference(set(desired_state.keys()))
    if diff_disks:
        return "following disk(s) isn't in an actual state: %s" % ", ".join(diff_disks)
    parameters_to_compare = ["BlocksCount", "BlockSize"]
    for disk in desired_state:
        for parameter in parameters_to_compare:
            try:
                if desired_state[disk][parameter] != current_state[disk][parameter]:
                    return "There is a difference in %s between %s parameter: %s required %s" % (
                        disk, parameter, current_state[disk][parameter], desired_state[disk][parameter])
            except KeyError:
                return "can't find parameter %s for disk %s" % (parameter, disk)
    return None


def reload_partition(partition):
    try:
        PartitionManager.reload_partition(partition)
        return True
    except Exception as err:
        logger.error("can't reload partition %s: %s", partition, err)
        return False


def check_disk_in_raid(name):
    with open("/proc/mdstat") as fd:
        for line in fd.readlines():
            if line.find(name) != -1:
                return True
    return False


def check_huawei_firmware_version(firmware_ver, firmware_min_version):
    """Returns True if firmware version supports more than one namespace on a disk"""
    try:
        fw = float(firmware_ver)
    except ValueError as err:
        logger.error("firmware version not float: %s", err)
        return False
    else:
        if fw < firmware_min_version:
            return False
    return True


def check_disk_vendor_model(name):
    try:
        disk = new_device_object(name)
        if disk.is_nvme:
            if disk.vendor() not in DeviceNvme.VALID_NVME_MODELS:
                return False, "Invalid {} disk vendor: {}".format(name, disk.vendor())
            if disk.model() not in DeviceNvme.VALID_NVME_MODELS[disk.vendor()]:
                return False, "Invalid {} disk model: {}".format(name, disk.model())
        if disk.is_hdd:
            logger.info("MOCK! Disk is hdd. Skip vendor/model checking")
        return True, "Disk {} has right vendor and model".format(name)
    except IOError:
        return False, "Can't get vendor/model for {} disk".format(name)


def get_first_partition(disk):
    if disk.is_hdd:
        return disk.first_partition()
    elif disk.is_nvme:
        ns = disk.namespaces()
        return ns[0].first_partition() if ns else None
    else:
        raise AssertionError("disk type {} is not supported".format(disk.type))


def get_kikimr_disks(username=None):
    """
    Get list of partlabels in KIKIMR
    """
    kikimr = KikimrViewerApi(username)
    kikimr_disks = [os.path.basename(disk) for disk in kikimr.get_node_disks()]
    return kikimr_disks


def get_disk_namespaces_from_inventory(disk):
    try:
        return InventoryApi().get_disks_namespaces(get_hostname(), disk)
    except Exception as err:
        logger.error("Can't get namespaces for '%s': %s", disk, err)
    return None


def get_os_release():
    lsb_release_file = "/etc/lsb-release"
    os_release = {}
    try:
        with open(lsb_release_file) as fd:
            for line in fd.readlines():
                key, value = line.split("=")
                os_release[key] = value.strip()
    except (IOError, OSError) as ex:
        print("Cannot read '{}' file: {}".format(lsb_release_file, ex))
    except ValueError as ex:
        print("Wrong '{}' file format: {}".format(lsb_release_file, ex))
    return os_release


def get_os_codename():
    return get_os_release().get("DISTRIB_CODENAME")


def determine_disk_owner(disk, valid_disk_owners=None, username=None):
    if valid_disk_owners is None:
        valid_disk_owners = DiskOwners.FRAGILE
    try:
        if KikimrDisks.is_kikimr_disk(disk, username):
            return DiskOwners.KIKIMR
        if check_disk_in_raid(disk.name):
            return DiskOwners.SYSTEM_RAID
        disk_owner = InventoryApi().get_disk_owner(get_hostname(), disk)
        if disk_owner and disk_owner in valid_disk_owners:
            return disk_owner
        if not disk_owner:
            return DiskOwners.NO_OWNER
    except (DiskNotFoundException, DiskOwnerNotFoundException):
        return DiskOwners.NO_OWNER
    except (KikimrApiException, InfraProxyException):
        raise
    except Exception as ex:
        logger.error("Can't determine owner for '%s': %s", disk.name, ex)
    return DiskOwners.UNKNOWN


def format_partition(partition_path, block_size=1048576, count=1):
    logger.info("Formatting partition %s", partition_path)
    source = os.open("/dev/zero", os.O_RDONLY)
    target = os.open(partition_path, os.O_WRONLY)
    written = 0
    for _ in range(0, count):
        written += os.write(target, os.read(source, block_size))
    return written


def _sort_nvmes(nvme_name):
    NVME_REGEX = re.compile(r"^nvme(?P<nvme_number>\d{1})n(?P<ns_number>\d{1,2})$")
    nvme_match = NVME_REGEX.match(nvme_name)
    return int(nvme_match.group("nvme_number")), int(nvme_match.group("ns_number"))


def get_kikimr_storage_nodes(username=None):
    kikimr_api = KikimrViewerApi(username)
    return kikimr_api.get_cluster_nodes_list()


def get_kikimr_storage_cluster_id(username=None):
    kikimr_api = KikimrViewerApi(username)
    return kikimr_api.get_cluster_id()


def pm_disk_action(disk, disk_type, action, error_exit_code=ERROR_EXIT_CODE, username=None):
    pm_client = PMApi(username)
    logger.info("Asking PM for removing '%s'", disk)
    permission = pm_client.get_disk_maintenance_permission(disk, disk_type, action)
    try:
        if permission["permission"]:
            logger.info("Got positive answer, returning '%s'", SUCCESS_EXIT_CODE)
            return SUCCESS_EXIT_CODE
        logger.info("Got negative answer , returning '%s'", error_exit_code)
        return error_exit_code
    except (KeyError, TypeError):
        logger.error("Unrecognised answer from PM with error: '%s'", permission)
        return error_exit_code


def pm_kikimr_disk_action(disk, action, username=None):
    disk_name = KikimrDisks().get_full_label_path(disk)
    if not disk_name:
        logger.error("Unknown disk '%s'", disk_name)
        return HWWATCHER_RETRY_EXIT_CODE
    return pm_disk_action(disk_name, "kikimr", action, error_exit_code=HWWATCHER_RETRY_EXIT_CODE, username=username)


def pm_kikimr_disk_attach(disk, username=None):
    return pm_kikimr_disk_action(disk, "attach", username)


def pm_kikimr_disk_detach(disk, username=None):
    return pm_kikimr_disk_action(disk, "detach", username)


def pm_mdb_disk_detach(disk, username=None):
    return pm_disk_action(disk.name, "mdb", "detach", username=username)


def pm_nbs_disk_detach(disk, username=None):
    return pm_disk_action(disk.name, "nbs", "detach", username=username)


def pm_dedicated_disk_detach(disk, username=None):
    return pm_disk_action(disk.name, "dedicated", "detach", username=username)


def write_file_content(file, content):
    dir_path = os.path.dirname(file)
    if not os.path.exists(dir_path):
        try:
            os.makedirs(dir_path)
            with open(file, "w") as open_file:
                open_file.write(content)
        except OSError as ex:
            raise OSError("Writing to '{}' failed! Err:{}".format(file, ex))
