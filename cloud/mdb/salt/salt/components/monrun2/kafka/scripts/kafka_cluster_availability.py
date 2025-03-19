#!/usr/bin/python

import sys
import socket

from contextlib import closing


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def create_socket():
    dbaas_vtype =  "{{ salt.pillar.get('data:dbaas:vtype') }}"
    if dbaas_vtype == 'compute':
        sock = socket.socket()
    else:
        sock = socket.socket(socket.AF_INET6)
    return sock


def check_zk_host(host):
    with closing(create_socket()) as sock:
        sock.settimeout(3)
        sock.connect((host, 2181))
        sock.send('ruok')
        resp = sock.recv(4)
        if resp == 'imok':
            return True
        return False


def check_kafka_host(host):
    with closing(create_socket()) as sock:
        sock.settimeout(3)
        result = sock.connect_ex((host, 9091))
        if result == 0:
            return True
        else:
            return False


def check_zk_hosts():
    """
    Check that all other zk hosts is available
    """
    hosts_line = "{{ salt['mdb_kafka.zk_fqdns']() }}"
    hosts = hosts_line.split(',')
    msg_parts = []
    for fqdn in hosts:
        try:
            if not check_zk_host(fqdn):
                msg_parts.append(fqdn)
        except Exception:
            msg_parts.append(fqdn)
    return msg_parts


def check_kafka_hosts():
    """
    Check that all other kafka hosts is available
    """
    this_fqdn = "{{ salt.grains.get('fqdn') }}"
    hosts_line = "{{ salt['mdb_kafka.kafka_fqdns']() }}"
    hosts = hosts_line.split(',')
    msg_parts = []
    for fqdn in hosts:
        if fqdn != this_fqdn:
            try:
                if not check_kafka_host(fqdn):
                    msg_parts.append(fqdn)
            except Exception:
                msg_parts.append(fqdn)
    return msg_parts


def _main():
    zk_msg_parts = check_zk_hosts()
    kafka_msg_parts = check_kafka_hosts()
    msg = ''
    if len(zk_msg_parts) > 0:
        msg = 'Failed to connect to Zookeeper brokers: ' + \
              ','.join(zk_msg_parts) + '.'
    if msg != '' and len(kafka_msg_parts) > 0:
        msg += ' '
    if len(kafka_msg_parts) > 0:
        msg += 'Failed to connect to Kafka brokers: ' + \
               ','.join(kafka_msg_parts) + '.'
    if msg != '':
        die(1, msg)
    die()


if __name__ == '__main__':
    _main()
