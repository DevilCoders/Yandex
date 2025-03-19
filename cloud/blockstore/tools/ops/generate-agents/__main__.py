#!/usr/bin/env python

import hashlib
import json
import socket
import sys


def ip6addr(host):
    return socket.getaddrinfo(host, None, socket.AF_INET6)[0][4][0]


def main():
    hosts = [line.rstrip() for line in open(sys.argv[1]).readlines()]
    ticket = sys.argv[2]
    dr_conf_file = open(sys.argv[3], "w")
    patch_file = open(sys.argv[4], "w")

    patch = []
    for host in hosts:
        file_devices = []

        for i in range(15):
            no = "%s" % (i + 1) if i < 9 else str(i + 1)
            file_devices.append({
                "Path": "/dev/nvme3n%sp1" % no,
                "BlockSize": 4096,
                "DeviceId": hashlib.md5(("%s-%s" % (host, no)).encode("utf-8")).hexdigest(),
            })

        agent_config = {
            "AgentId": host,
            "Backend": "DISK_AGENT_BACKEND_SPDK",
            "NvmeTarget": {
                "Nqn": "nqn.2018-09.io.spdk:cnode1",
                "TransportIds": [
                    "trtype:RDMA adrfam:IPv6 traddr:%s trsvcid:10010" % ip6addr(host)
                ]
            },
            "Enabled": True,
            "FileDevices": file_devices,
        }

        spdk_config = {
            "CpuMask": "0x300",
            "HugeDir": "/dev/hugepages/nbs",
        }

        patch.append({
            "ticket": ticket,
            "hosts": [host],
            "patch": {
                "DiskAgentConfig": agent_config,
                "SpdkEnvConfig": spdk_config,
            }
        })

        dr_conf_file.write("KnownAgents {\n")
        dr_conf_file.write("  AgentId: \"%s\"\n" % host)
        for dev in file_devices:
            dr_conf_file.write("  Devices: \"%s\"\n" % dev["DeviceId"])
        dr_conf_file.write("}\n")

    json.dump(patch, patch_file, indent=2)
    patch_file.write('\n')


if __name__ == '__main__':
    sys.exit(main())
