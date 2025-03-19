#!/usr/bin/env python
"""
Worker slb checks
"""

import json
import socket
import sys
import urlparse
from contextlib import closing

import requests

requests.packages.urllib3.disable_warnings()


def die(status, message):
    """
    Format output for monrun
    """
    print('{status};{message}'.format(
        status=status, message=message if message else 'OK'))
    sys.exit(0)


def check_slb(address, ca_path):
    """
    Check if slb available
    """
    if ca_path == '/etc/ssl/certs':
        verify = True
    else:
        verify = ca_path
    if address and 'localhost' not in address:
        try:
            if address.startswith('http'):
                session = requests.Session()
                adapter = requests.adapters.HTTPAdapter(max_retries=3)
                session.mount(urlparse.urljoin(address, '/'), adapter)
                session.get(
                    urlparse.urljoin(address, '/'),
                    timeout=(1, 1),
                    verify=verify,
                    allow_redirects=False)
            else:
                socket.setdefaulttimeout(1)
                host, port = address.split(':', 1)
                with closing(socket.create_connection((host, int(port)))):
                    pass
        except requests.exceptions.ReadTimeout:
            pass
        except Exception as exc:
            return ' '.join([address] + repr(exc).split())


def _main():
    status = 0
    message = []
    with open('/etc/dbaas-worker.conf') as conf_file:
        config = json.load(conf_file)

    for options in config.values():
        for option in options:
            if 'url' in option:
                error = check_slb(options[option], options.get(
                    'ca_path', False))
                if error:
                    status = 1
                    message.append(error)

    die(status, '; '.join(message))


if __name__ == '__main__':
    _main()
