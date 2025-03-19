#!/usr/bin/env python3

import os
import json
import sys
import subprocess
import shlex


CONTRAIL_PORTS = "/var/lib/contrail/ports"
ENUMERATE_COMMAND = "yc-compute-enumerate-port-ips --api-server 127.0.0.1 --api-port 8082"


def main():
    try:
        ports = os.listdir(CONTRAIL_PORTS)
    except FileNotFoundError:
        return 0

    for port in ports:
        port_data = None
        try:
            with open(os.path.join(CONTRAIL_PORTS, port)) as f:
                try:
                    port_data = json.load(f)
                except json.decoder.JSONDecodeError:
                    pass
        except FileNotFoundError:
            continue

        if port_data:
            # ignore non-hbf ports
            if not port_data.get('system-name', '').startswith('qvb'):
                continue
            # ignore "::" and "0.0.0.0"
            addr = port_data.get('ip6-address')
            if addr and addr != '::':
                print(addr)
            addr = port_data.get('ip-address')
            if addr and addr != '0.0.0.0':
                print(addr)
        else:
            try:
                proc = subprocess.Popen(shlex.split(ENUMERATE_COMMAND + " " + port),
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
            except FileNotFoundError:
                continue
            stdout, _ = proc.communicate()
            if proc.returncode:
                continue
            for line in stdout.decode().split('\n'):
                if line:
                    print(line)
    return 0


if __name__ == '__main__':
    sys.exit(main())
