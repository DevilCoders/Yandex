#!/usr/bin/env python3

import glob
import hashlib
import json
import logging
import os
import re
import subprocess
import time

UNDERLAY_PROFILE = "underlay-v4"
VOID_PROFILE = "V01D"

HOST_VFS = {0: UNDERLAY_PROFILE}  # VFs, that we leave on host

log = logging.getLogger(__file__)


def retry(exception=Exception, max_tries=4, timeout=1, backoff_coefficient=3):
    """Retry decorator, with linear backoff"""
    def retry_decorator(func):
        def wrappe(*args, **kwargs):
            tries = 0
            sleep_timeout = timeout
            saved_exception = None
            while tries < max_tries:
                try:
                    if tries:
                        time.sleep(sleep_timeout)
                        sleep_timeout *= backoff_coefficient
                    return func(*args, **kwargs)
                except exception as e:
                    saved_exception = e
                    tries += 1
            raise saved_exception  # pylint: disable=E0702
        return wrappe
    return retry_decorator


def vf_mac_address(pf_addr, index):
    """Generates mac address for VF, based on PF address and VF index

    We need VFs to have predictable macs and kernel driver would
    just generate totally random macs.
    The macs we generate look like this XX:XX:YY:ZZ:ZZ:ZZ
    For XX:XX part we take OUI of a mac of a PF (first 3 octets),
    hash it with md5 and take 2 first octets. In the 1 octet we set the
    least- significant bit to 0 (unicast mac) and the
    2d-least-significant bit to 1 (locally administered address).
    YY is the index of the VF
    ZZ:ZZ:ZZ is copied from the NIC-specific part (last 3 octets)
    """

    h = hashlib.md5(pf_addr[:8].encode('utf-8')).hexdigest()
    first = int(h[:2], 16)  # first octet
    first &= 0xfe  # set unicast
    first |= 0x02  # set locally administered
    vf_mac = "{:02x}:{}:{:02x}:{}".format(first, h[2:4], index, pf_addr[9:])
    return vf_mac


# FIXME: After CLOUD-3393 is fixed and after we have
# multiple projects on CVM the following two functions
# should be updated to use rdisc6 to guess project name
def get_network_names(project_ids):
    return ["default" for i in range(len(project_ids))]


def get_project_id(iface_name):
    return 'default'


def supports_sriov(pciid):
    """Returns True if device identified by `pciid` supports sriov"""
    vfs_file = "/sys/bus/pci/devices/{}/sriov_totalvfs".format(pciid)
    return os.path.exists(vfs_file)


def bind_override(pciid, driver):
    """Bind device to a driver, using driver_override

    If driver is an empty string the device will be bound using standard
    matching mechanism.
    """
    # echo driver-name > /sys/bus/pci/devices/0000:03:00.0/driver_override
    # echo 0000:03:00.0 > /sys/bus/pci/devices/0000:03:00.0/driver/unbind
    # echo 0000:03:00.0 > /sys/bus/pci/drivers_probe
    override = '/sys/bus/pci/devices/{}/driver_override'.format(pciid)
    with open(override, 'w') as f:
        # \n is required when writing an empty string
        f.write(driver + '\n')
    unbind = '/sys/bus/pci/devices/{}/driver/unbind'.format(pciid)
    if os.path.exists(unbind):
        # it's possible that device is already unbound
        with open(unbind, 'w') as f:
            f.write(pciid)
    probe = '/sys/bus/pci/drivers_probe'
    with open(probe, 'w') as f:
        f.write(pciid)


def _host_vf_correct_mac(vf_info):
    addr_glob = glob.glob(
        "/sys/bus/pci/devices/{}/net/*/address".format(vf_info['id']))
    if not addr_glob:
        return False

    with open(addr_glob[0]) as f:
        vf_addr = f.readline().strip()
    return vf_addr == vf_info['mac_address']


def _guest_vf_correct_mac(vf_info):
    command = ["ip", "link", "show", vf_info['pf_name']]

    p = subprocess.Popen(command, stdout=subprocess.PIPE)
    stdout, _ = p.communicate()
    m = re.search('vf {:d} MAC (?P<mac>.+?),'.format(int(vf_info['vf_num'])), stdout.decode())
    if not m:
        return False
    return m.groupdict()['mac'] == vf_info['mac_address']


def should_bind_to_vfio(vf_info):
    """Returns True if the device should be bound to vfio_pci"""
    # TODO: check vf0/vf1 for configuration
    pciid = vf_info['id']
    driver_path = "/sys/bus/pci/devices/{}/driver".format(pciid)
    if not os.path.exists(driver_path):
        return True
    if os.path.basename(os.readlink(driver_path)) == 'vfio-pci':
        logging.info("{}vf{} ({}) is already bound to vfio".format(
            vf_info["pf_name"], vf_info["vf_num"], pciid))
        return False

    # if this is a host VF we need to check ensure it's not configured yet.
    if int(vf_info['vf_num']) in HOST_VFS and \
            _host_vf_correct_mac(vf_info):
        log.info("{} already has correct mac"
                 ", not binding it to vfio".format(pciid))
        return False
    return True


@retry()
def configure_host_vf(vf_info):
    """Configures VF for host use

    This function:
    * re-binds VF to it's native driver unless it's mac is already correct
    * re-names VF to a predictable name
    * calls config-vf with correct network profile
    """
    vf_num = vf_info['vf_num']
    if int(vf_num) not in HOST_VFS:
        raise ValueError("Don't know how to configure VF {}".format(vf_info))
    if not _host_vf_correct_mac(vf_info):
        log.info("Setting mac {} for {}vf{}".format(vf_info["mac_address"], vf_info["pf_name"], vf_info["vf_num"]))
        subprocess.check_call([
            "ip", "link", "set", vf_info["pf_name"], "vf", vf_info["vf_num"], "mac", vf_info["mac_address"]
        ])
        # bind it to native driver
        bind_override(vf_info['id'], driver='')

    name_glob = glob.glob(
        "/sys/bus/pci/devices/{}/net/*".format(vf_info['id']))
    if not name_glob:
        raise RuntimeError("Can't get interface name for {}".format(vf_info))
    vf_name = os.path.basename(name_glob[0])
    pf_name = vf_info['pf_name']

    new_name = "{}vf{}".format(pf_name, vf_num)
    if vf_name != new_name:
        subprocess.check_call(
            ["ip", "link", "set", vf_name, "down"])
        subprocess.check_call(
            ["ip", "link", "set", vf_name, "name", new_name])
    subprocess.check_call(
        ["config-vf", "-p", pf_name, "-v", vf_num, HOST_VFS[int(vf_num)]])


@retry()
def configure_guest_vf(vf_info):
    if _guest_vf_correct_mac(vf_info):
        log.info("{}vf{} already has correct mac ({})"
                 ", looks like it is already confiugured. Skipping".format(
                     vf_info["pf_name"], vf_info["vf_num"], vf_info["mac_address"]))
        return

    log.info("Setting mac {} for {}vf{}".format(vf_info["mac_address"], vf_info["pf_name"], vf_info["vf_num"]))
    subprocess.check_call([
        "ip", "link", "set", vf_info["pf_name"], "vf", vf_info["vf_num"], "mac", vf_info["mac_address"]
    ])
    subprocess.check_call([
        "config-vf", "-p", vf_info["pf_name"], "-v", vf_info["vf_num"], VOID_PROFILE
    ])


def get_v4_vf_ip_pool():
    ip_pool = None
    with open("/etc/yc/yc-underlay-ipv4-pool.json") as f:
        try:
            ip_pool = json.load(f)
        except ValueError:
            log.error("Could not load v4 IP pool from /etc/yc/yc-underlay-ipv4-pool.json")
    if not isinstance(ip_pool, list):
        raise ValueError("V4_VF_IP_POOL should be a list "
                         " it is {}".format(ip_pool))
    log.debug("Loaded ip pool '{}'".format(ip_pool))
    return ip_pool


def setup_sriov(saved_vfs):
    NETWORK_CLASS = '0200'
    all_vfs = []
    # PCI devices that report that they are Network Controllers
    pciid = None
    for line in subprocess.check_output(
            ['lspci', '-Dnd::{}'.format(NETWORK_CLASS)])\
            .decode('utf-8').strip().split('\n'):
        fields = line.split()
        net_pciid = fields[0]

        if os.path.exists("/sys/bus/pci/devices/{}/net/eth0".format(net_pciid)):
            if supports_sriov(net_pciid):
                pciid = net_pciid
                break
    else:
        # We did not break, e.g. we did not find eth0
        log.error("No eth0 or eth0 does not support SR-IOV")
        return all_vfs

    # NOTE(k-zaitsev): We only setup eth0 here, to avoid testing interfaces for
    # carrier connectivity.
    pf_name = "eth0"
    pf_path = "/sys/bus/pci/devices/{}/net/{}".format(pciid, pf_name)

    with open(os.path.join(pf_path, 'address')) as f:
        pf_addr = f.readline().strip()

    vfs = sorted(glob.glob("/sys/bus/pci/devices/{}/virtfn*".format(pciid)),
                 key=lambda vf: int(os.path.basename(vf).replace('virtfn', '')))
    for vf in vfs:

        vf_pciid = os.path.basename(os.readlink(vf))
        vf_num = os.path.basename(vf).replace('virtfn', '')
        vf_mac = vf_mac_address(pf_addr, int(vf_num))
        vf_info = {
            "type": "phys",
            "id": vf_pciid,
            "vf_num": vf_num,
            "pf_name": pf_name,
            "mac_address": vf_mac,
            "ipv4_address": None,
        }

        if should_bind_to_vfio(vf_info):
            bind_override(vf_pciid, driver='vfio-pci')

        if int(vf_num) in HOST_VFS:
            configure_host_vf(vf_info)
        else:
            configure_guest_vf(vf_info)
            all_vfs.append(vf_info)

    all_vfs = sorted(all_vfs, key=lambda x: x['id'])
    for ip, vf_info in zip(get_v4_vf_ip_pool(), all_vfs):
        vf_info['ipv4_address'] = ip
    return all_vfs


def scan_virtio_netdevs(saved_vfs):

    ENTER_DIR = "/sys/bus/virtio/devices/"
    NET_DEV = "0x0001"
    ifaces = []

    if not os.path.exists(ENTER_DIR):
        return ifaces

    virtio_devs = [i.path for i in os.scandir(ENTER_DIR) if i.is_dir()]

    virtio_net_devs = []
    for path in virtio_devs:
        with open(os.path.join(path, "device")) as f:
            if f.read().strip() == NET_DEV:
                virtio_net_devs.append(path)

    virtio_net_devs = sorted(virtio_net_devs)
    for i, path in enumerate(virtio_net_devs):
        net_path = os.path.join(path, "net")
        if not os.path.exists(net_path):
            log.warning("No net for {}. Skipping".format(path))
            continue
        netdir = next(os.scandir(net_path))
        with open(os.path.join(netdir.path, "address")) as fp:
            mac = fp.read().strip()
            # Preserve saved interface MAC address
            if netdir.name in saved_vfs:
                mac = saved_vfs[netdir.name]["mac_address"]
            ifaces.append({
                "type": "virt",
                "id": netdir.name,
                "if_name": netdir.name,
                "br_name": "br-" + netdir.name,
                "mac_address": mac,
                "ipv4_address": None,
            })

    project_ids = [get_project_id(iface["if_name"]) for iface in ifaces]

    network_profiles = get_network_names(project_ids)

    for i, iface in enumerate(ifaces):
        iface["profile"] = network_profiles[i]

    # Keep 2 ifaces for net config
    ifaces = ifaces[2:]

    return ifaces


def main():
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)-15s:%(levelname)s:%(message)s",
        handlers=[logging.StreamHandler()]
    )

    all_vfs = []

    saved_vfs = {}
    if os.path.exists("/run/vf-interfaces.json"):
        with open("/run/vf-interfaces.json", "r") as vfijs:
            saved_vfs = {i["id"]: i for i in json.load(vfijs) if "id" in i}

    all_vfs.extend(setup_sriov(saved_vfs))
    all_vfs.extend(scan_virtio_netdevs(saved_vfs))

    with open("/run/vf-interfaces.json", 'w') as vfijs:
        vfijs.write(json.dumps(all_vfs))


if __name__ == '__main__':
    main()
