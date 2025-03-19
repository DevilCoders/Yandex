#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check host FQDN is valid
"""

import socket
import subprocess
import sys


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def get_file_hostname():
    fqdn = subprocess.check_output(['sudo', '/bin/cat', '/etc/hostname'])
    fqdn = fqdn.decode('utf-8').strip()
    return fqdn


def _main():
    try:
        hostname = socket.gethostname()
        if '.' not in hostname:
            die(2, 'hostname is not fully qualified') 
        fqdn = socket.getfqdn()
        hostname2 = get_file_hostname()
        if fqdn != hostname or hostname != hostname2:
            die(2, 'hostname mismatch: hostname: {hostname}, getfqdn: {fqdn}, /etc/hostname: {hostname2}'.format(
                hostname=hostname, fqdn=fqdn, hostname2=hostname2))
        die()
    except Exception as exc:
        die(1, 'Unable to check: {error}'.format(error=repr(exc)))


if __name__ == '__main__':
    _main()
