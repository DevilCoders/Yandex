#!/usr/bin/env python3

"""
Script generates and stores base check set for yc-infra-inspector, that
should be run after node SETUP process to check health of base system components
"""

import os
import sys

import yaml
from ycinfra import (
    write_file_content,
)

CONFIG_FILE = "/etc/yc/infra-inspector/config.yaml"
VALID_KERNEL_VERSIONS = [
    "4.14.67-23+yc2",
    "4.14.74-28+yc3",
    "4.14.74-28+yc4",
    "4.14.74-28+yc7",
    "4.14.74-28+yc9",
    "4.14.74-28+yc10",
    "4.14.74-28+yc12",
    "4.14.74-28+yc13",
    "4.19.118+yc3",
    "4.19.185+yc5",
]
ACTUAL_KERNEL_VERSIONS = [
    "4.14.74-28+yc9",
    "4.14.74-28+yc10",
    "4.14.74-28+yc12",
    "4.14.74-28+yc13",
    "4.19.185+yc5",
]
IPV6_PREFIXES = {
    "PRIVATE-GPN-1": "2a0d:d6c0:200",
    "PRIVATE-TESTING": "2a0d:d6c0:200",
    "OTHER": "2a02:6b8:bf00"
}


def _get_ipv6_prefix():
    hostname = os.uname()[1]
    if "gpn" in hostname:
        return IPV6_PREFIXES["PRIVATE-GPN-1"]
    if "private-testing" in hostname:
        return IPV6_PREFIXES["PRIVATE-TESTING"]
    return IPV6_PREFIXES["OTHER"]


def main():
    walle_url = os.environ.get('WALLE_URL', "https://api.wall-e.yandex-team.ru")
    ipv6_prefix = _get_ipv6_prefix()
    checks = [
        {"name": "check all available host IPv6 addresses",
         "ip": {
             "state": "present",
             "ip_prefix": ipv6_prefix
         }},
        {"name": "check that hostname and AAAA are equal",
         "dns": {
             "state": "present"
         }},
        {"name": "check host status in wall-e",
         "wall_e": {
             "state": "ready",
             "api_url": walle_url+"/v1"
         }},
        {"name": "wall-e memory juggler check",
         "juggler": {
             "name": "walle_memory",
             "juggler_state": "ok"
         }},
        {"name": "wall-e disk juggler check",
         "juggler": {
             "name": "walle_disk",
             "juggler_state": "ok"
         }},
        {"name": "check valid linux kernel version",
         "linux_kernel": {
             "version": VALID_KERNEL_VERSIONS,
             "state": "present"
         },
         "severity": "crit"},
        {"name": "check actual kernel version",
         "linux_kernel": {
             "version": ACTUAL_KERNEL_VERSIONS,
             "state": "present"
         },
         "severity": "crit"}
    ]
    try:
        write_file_content(CONFIG_FILE, yaml.safe_dump(checks, default_flow_style=False))
    except OSError as ex:
        print(ex)
        sys.exit(1)


if __name__ == '__main__':
    main()
