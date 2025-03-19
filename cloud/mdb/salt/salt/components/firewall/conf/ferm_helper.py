#!/usr/bin/env python3
"""
Ferm address handle helper
"""

import argparse
import ipaddress
import subprocess


def get_address(interface):
    """
    Get global ipv6 address on interface
    """
    ret = subprocess.check_output(['/sbin/ip', '-6', 'add', 'list', 'dev', interface])
    for line in ret.decode('utf8').splitlines():
        if 'scope global' in line:
            return line.strip().split()[1].split('/')[0]
    raise RuntimeError(f'Unable to find global address in {ret}')


def get_project_id(interface):
    """
    Get project_id on interface
    """
    address = ipaddress.ip_address(get_address(interface))
    first96bits = int(address) // 2**32
    project_id = first96bits % 2**32
    return hex(project_id)


CMDS = {'address': get_address, 'project_id': get_project_id}


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=CMDS.keys())
    parser.add_argument('interface', type=str)
    args = parser.parse_args()
    print(CMDS[args.command](args.interface))


if __name__ == '__main__':
    _main()
