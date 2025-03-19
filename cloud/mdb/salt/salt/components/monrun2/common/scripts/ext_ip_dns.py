#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check if external ip and dns are consistent in compute
"""

from __future__ import absolute_import, print_function, unicode_literals

import socket
import sys

import dns.resolver
import requests

DNS_SERVER = '2a02:6b8:0:1::1'


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    try:
        res = requests.get('http://169.254.169.254/latest/meta-data/public-ipv4')
        if res.status_code == 404:
            die()
        elif res.status_code != 200:
            res.raise_for_status()
        else:
            address = res.text.strip()

        fqdn = socket.getfqdn()
        resolver = dns.resolver.Resolver()
        resolver.nameservers = [DNS_SERVER]
        answer = resolver.query(fqdn, 'a')
        if len(answer) > 1:
            die(2, 'Multiple A records: {records}'.format(records=', '.join([x.to_text() for x in answer])))
        elif not answer:
            die(2, 'No A record for external ip address: {address}'.format(address=address))
        else:
            in_record = answer[0].to_text()
            if in_record != address:
                message = ('External ip and dns mismatch: '
                           'expected {address} actual {in_record}'.format(address=address, in_record=in_record))
                die(2, message)
        die()
    except Exception as exc:
        die(1, 'Unable to check: {error}'.format(error=repr(exc)))


if __name__ == '__main__':
    _main()
