#!/usr/bin/env python3.6

import socket
import argparse
import requests
import sys
import time

# Jinja here is used to avoid exposing passwords in command line
USER = "{{ salt.pillar.get('data:elasticsearch:users:mdb_admin:name') }}"
PASSWORD = "{{ salt.pillar.get('data:elasticsearch:users:mdb_admin:password') }}"
ENABLED = {{ salt.pillar.get('data:elasticsearch:kibana:enabled', False) }}


def parse_args():
    """
    Parse arguments from command line
    """
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '-n', '--number',
        type=int,
        default=3,
        help='The max number of retries')
    args_parser.add_argument(
        '-u', '--user',
        type=str,
        default=USER,
        help='Username')
    args_parser.add_argument(
        '-p', '--password',
        type=str,
        default=PASSWORD,
        help='Password')

    return args_parser.parse_args()


def main():
    """
    Program entry point.
    """
    if not ENABLED:
        die(0, 'service disabled')

    args = parse_args()
    error = ''
    for n in range(args.number):
        try:
            ping(args.user, args.password)
            die(0, 'OK')
        except Exception as e:
            error = repr(e)

        time.sleep(1)

    die(2, f'service is dead {": " + error if error else ""}')


def ping(user, password):
    hostname = socket.gethostname()
    url = 'https://'+hostname+':443/api/saved_objects/_find'
    params = {
        'type': 'index-pattern', 'search_fields': 'title', 'search': 'my*'
    }
    headers = {
        'kbn-xsrf': 'true'
    }
    response = requests.get(
        url, 
        timeout=5, 
        params=params, 
        headers=headers, 
        auth=(user, password), 
        verify='/etc/ssl/certs/ca-certificates.crt',
    )
    response.raise_for_status()


def die(status, message):
    """
    Emit status and exit.
    """
    print('%s;%s' % (status, message))
    sys.exit(0)


if __name__ == '__main__':
    main()
