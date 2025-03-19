import glob
import os

NETWORKS = [
    'management',       # General-purpose IPv6 interface
    'dataplane',        # OpenContrail IPv4 dataplane
    'upstream_ipv6',    # IPv6-External connectivity for cloudgates
    'upstream_ipv4',    # IPv4-External connectivity for cloudgates
]


def _get_network_devices(virtio=False):
    """Returns a list of sorted pciids of network interfaces

    if `virtio` is True only returns virtio interfaces, otherwise only
    non-virtio interfaces
    """
    VIRTIO_NET = [
        "1af4:1000",  # "legacy" virtio network
        "1af4:1041",  # virtio 1.0 network
    ]
    NETWORK_CLASS = "0200"

    net_devices = __salt__['cmd.run'](  # noqa
        'lspci -Dnd::{}'.format(NETWORK_CLASS)).strip()
    if not net_devices:
        return []

    pci_devices = []
    for line in net_devices.split('\n'):
        fields = line.split()
        pciid = fields[0]
        vendor_device = fields[2]

        is_virtio = vendor_device in VIRTIO_NET
        if virtio == is_virtio:
            pci_devices.append(pciid)

    # Just in case. lspci does not guarantee order.
    pci_devices.sort()
    return pci_devices


def _get_virtio_name(pci_dev):
    dev_path = os.path.join('/sys/bus/pci/devices/', pci_dev, 'virtio*/net/*')
    return os.path.basename(glob.glob(dev_path)[0])


def _get_pcidev_name(pci_dev):
    return os.listdir(os.path.join('/sys/bus/pci/devices', pci_dev, "net"))[0]


def _virtio():
    """Return underlay interfaces for v4 and v6 in virtual infrastructure"""
    pci_devices = _get_network_devices(virtio=True)

    return {
        network: _get_virtio_name(pci_dev)
        for network, pci_dev
        in zip(NETWORKS, pci_devices)
    }


def _sriov_vm():
    """Return underlay interfaces for v4 and v6 on Service VM with VFs"""
    pci_devices = _get_network_devices(virtio=False)

    return {
        network: _get_pcidev_name(pci_dev)
        for network, pci_dev
        in zip(NETWORKS, pci_devices)
    }


def _sriov_bm():
    """Return well-known hardware interface names"""
    return {
        'management': 'eth0',
        'dataplane': 'eth0vf0',
    }


def _slb_adapter():
    """Return SLB adapter SVM's devices.

    SLB adapter SVM is a special sort of virtual machine
    with two network interfaces: one underlay and one overlay.
    Underlay network is connected to NOC L3 load balancers.
    The SVM serves as a proxy between that L3
    and some service VMs in overlay network.
    NOTE: SLB adapter doesn't bridge IP traffic between those interfaces.
    There could only be two interfaces due to the following:
      * No support for multiple overlay adapters because of
        grey ipv4 blocks collision
      * No support for multiple underlay adapters
        because both of them are assigned to the same network
        which serves no obvious task but introduces unnecessary complexity
    """

    pci_devices = _get_network_devices(virtio=False)
    virtio_devices = _get_network_devices(virtio=True)

    if len(pci_devices) != 1 or len(virtio_devices) != 1:
        return {}

    devices = {}
    devices['overlay'] = _get_virtio_name(virtio_devices[0])
    devices['management'] = _get_pcidev_name(pci_devices[0])

    return devices


def interfaces():
    salt_virtual = __grains__['virtual']  # noqa

    # If we're on a physical machine, return well-known interface names
    if salt_virtual == 'physical':
        return _sriov_bm()

    # Otherwise we're on a VM:
    # Return one of those:
    # * Both SLB adapter VM's interfaces.
    # * SR-IOV VFs if there are two or four of them
    # * virtIO interfaces if there are two of them as a fallback.
    return _slb_adapter() or _sriov_vm() or _virtio()


def virtio_for_passthrough():
    """Returns a list of virtio interfaces, that CVM may use for passthrough"""

    pci_devices = _get_network_devices(virtio=True)[2:]
    interfaces = []

    dev_path = "/sys/bus/pci/devices"
    for dev in pci_devices:
        iface_name = os.path.basename(
            glob.glob('{}/{}/virtio*/net/*'.format(dev_path, dev))[0]
        )
        interfaces.append(iface_name)
    return interfaces
