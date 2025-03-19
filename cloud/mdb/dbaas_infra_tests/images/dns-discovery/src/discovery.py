#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple python script to update /etc/hosts file with docker-compose services
"""

import logging
import os
import socket
import subprocess
import time

import docker


def _main():
    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)-8s: %(message)s')
    log = logging.getLogger('docker-discovery')
    client = None
    mynet = '.'.join(socket.getfqdn().split('.')[1:])
    while True:
        try:
            if not client:
                client = docker.from_env()
            records = []
            for container in client.containers.list():
                if not container.attrs['State']['Running']:
                    continue
                name = container.attrs['Config']['Hostname']
                if not name.endswith(mynet):
                    continue
                network = container.attrs['NetworkSettings']['Networks'][mynet]
                for addr in ['IPAddress', 'GlobalIPv6Address']:
                    records.append('{addr} {host}.{domain} {host}\n'.format(
                        addr=network[addr], host=name.split('.')[0], domain=mynet))
            current = ''.join(sorted(records))
            with open('/etc/docker-hosts.list') as fobj:
                ondisk = fobj.read()
            if current != ondisk:
                log.info('Updating hosts file')
                with open('/etc/docker-hosts.list.tmp', 'w') as fobj:
                    fobj.write(current)
                os.rename('/etc/docker-hosts.list.tmp', '/etc/docker-hosts.list')
                subprocess.check_call(['/usr/bin/supervisorctl', 'signal', 'HUP', 'dnsmasq'])
        except Exception as exc:
            log.error('Unhandled exception: %s', exc)
            client = None
        time.sleep(1)


if __name__ == '__main__':
    _main()
