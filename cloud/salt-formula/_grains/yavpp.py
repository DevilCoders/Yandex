#!/usr/bin/python
# -*- coding: utf-8 -*-
"""a module to collect vpp specific grains"""

import subprocess

KNOWLEDGE_BASE = {
    # Intel Corporation Ethernet Controller XL710 for 40GbE QSFP+
    '8086:1584': (12, 4096, 'FortyGigabitEthernet{}/{}/{}', 'igb_uio'),
    # Intel Corporation Ethernet Controller XL710 for 40GbE QSFP+ (rev 01)
    '8086:1583': (12, 4096, 'FortyGigabitEthernet{}/{}/{}', 'igb_uio'),
    # Intel Corporation 82599 Ethernet Controller Virtual Function
    '8086:10ed': (12, 4096, 'VirtualFunctionEthernet{}/{}/{}', 'igb_uio'),
    # Intel Corporation XL710/X710 Virtual Function (rev 01)
    '8086:154c': (12, 4096, 'VirtualFunctionEthernet{}/{}/{}', 'igb_uio'),
    # Virtio network device
    '1af4:1000': (1, 1024, 'GigabitEthernet{}/{}/{}', 'igb_uio'),
    # Mellanox Technologies MT27710 Family [ConnectX-4 Lx]
    '15b3:1015': (64, 4096, 'TwentyFiveGigabitEthernet{}/{}/{}', 'mlx5_core'),
    # Mellanox Technologies MT27710 Family [ConnectX-4 Lx Virtual Function]
    '15b3:1016': (64, 2048, 'TwentyFiveGigabitEthernet{}/{}/{}', 'mlx5_core'),
    # Mellanox Technologies MT27500 Family [ConnectX-3]
    '15b3:1003': (8, 4096, 'FiftySixGigabitEthernet{}/{}/{}', 'mlx4_core'),
    # Mellanox Technologies MT27500/MT27520 Family [ConnectX-3/ConnectX-3 Pro Virtual Function]
    '15b3:1004': (8, 2048, 'FiftySixGigabitEthernet{}/{}/{}', 'mlx4_core'),
    # Mellanox Technologies MT27520 Family [ConnectX-3 Pro]
    '15b3:1007': (8, 4096, 'FiftySixGigabitEthernet{}/{}/{}', 'mlx4_core'),
}


def _num_sockets():
    """a function to get number of physical processors"""
    with open('/proc/cpuinfo') as cpuinfo:
        lines = cpuinfo.readlines()
    idstrs = [x for x in lines if x.startswith('physical id')]
    ids = [int(x.split(':')[-1].strip()) for x in idstrs]
    return max(ids) + 1


def _numanode_by_pciid(pciid):
    """a function to get numa node pci device is connected to"""
    filename = '/sys/bus/pci/devices/0000:{}/numa_node'.format(pciid)
    with open(filename) as numafile:
        return int(numafile.readline())


def _ifaces():
    """a function to get list of VPP-usable interfaces"""
    retval = []
    for line in subprocess.check_output(['lspci', '-n']).split('\n'):
        for name in KNOWLEDGE_BASE:
            if name in line:
                pciid, _ = line.split(' ', 1)
                pci_bus, rest = pciid.split(':')
                pci_dev, pci_func = rest.split('.')
                retval.append({
                    'queues': KNOWLEDGE_BASE[name][0],
                    'queue_size': KNOWLEDGE_BASE[name][1],
                    'numa_node': _numanode_by_pciid(pciid),
                    'vpp_name': KNOWLEDGE_BASE[name][2].format(
                        '{:x}'.format(int(pci_bus, 16)),
                        '{:x}'.format(int(pci_dev, 16)),
                        int(pci_func)),
                    'pciid': pciid,
                    'module': KNOWLEDGE_BASE[name][3]
                })
    return sorted(retval, lambda l, r: cmp(l['pciid'], r['pciid']))


def _modules():
    ifaces = _ifaces()
    return list({x['module'] for x in ifaces})


def _is_mellanox():
    modules = _modules()
    return 'mlx5_core' in modules or 'mlx4_core' in modules


def main():
    """grain entry point"""
    return {
        'num_sockets': _num_sockets(),
        'yavpp_ifaces': _ifaces(),
        'yavpp_modules': _modules(),
        'is_mellanox': _is_mellanox(),
    }


if __name__ == '__main__':
    print(main())
