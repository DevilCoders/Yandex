#!/usr/bin/env python3
import argparse
import logging
import re
import socket
import sys
import time
from functools import wraps

from kazoo.client import KazooClient

COMMANDS = {}


def command(name):
    def decorator(fun):
        COMMANDS[name] = fun

    return decorator


def main():
    """
    Program entry point.
    """
    logging.disable(logging.CRITICAL)

    args_parser = argparse.ArgumentParser()
    args_parser.add_argument('command')
    args = args_parser.parse_args()

    COMMANDS[args.command]()


@command('alive')
def alive_command():
    try:
        keeper_mntr()

        client = KazooClient(connection_retry=3, command_retry=3, timeout=1)
        client.start()
        client.get("/")
        client.create(path='/{0}_alive'.format(socket.getfqdn()), ephemeral=True)
        client.stop()

        die(0, 'OK')

    except Exception as e:
        die(2, repr(e))


@command('avg_latency')
def avg_latency_command():
    try:
        die(0, keeper_mntr()['zk_avg_latency'])
    except Exception as e:
        die(1, str(e))


@command('min_latency')
def min_latency_command():
    try:
        die(0, keeper_mntr()['zk_min_latency'])
    except Exception as e:
        die(1, str(e))


@command('max_latency')
def max_latency_command():
    try:
        die(0, keeper_mntr()['zk_max_latency'])
    except Exception as e:
        die(1, str(e))


@command('queue')
def queue_command():
    try:
        die(0, keeper_mntr()['zk_outstanding_requests'])
    except Exception as e:
        die(1, str(e))


@command('descriptors')
def descriptors_command():
    try:
        die(0, keeper_mntr()['zk_open_file_descriptor_count'])
    except Exception as e:
        die(1, str(e))


def retry(attempts, interval=0.5):
    """
    Retry decorator.
    """

    def decorator(fun):
        @wraps(fun)
        def wrapper(*args, **kwargs):
            attempt = 1
            while True:
                try:
                    return fun(*args, **kwargs)
                except Exception:
                    if attempt >= attempts:
                        raise
                    attempt += 1
                    time.sleep(interval)

        return wrapper

    return decorator


@retry(3)
def keeper_mntr():
    """
    Execute Keeper mntr command and parse its output.
    """
    result = {}
    try:
        response = keeper_command('mntr')
        for line in response.split('\n'):
            key_value = re.split('\s+', line, 1)
            if len(key_value) == 2:
                result[key_value[0]] = key_value[1]

        if not len(result) > 1:
            raise RuntimeError(f'Too short response: {response.strip()}')

    except Exception as e:
        raise RuntimeError(f'Unable to get Keeper status: {e!r}')

    return result


def keeper_command(command):
    """
    Execute Keeper 4-letter command.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(3)
        s.connect(("localhost", 2181))

        s.sendall(command.encode())
        return s.makefile().read(-1)


def die(status, message):
    """
    Emit status and exit.
    """
    print(f'{status};{message}')
    sys.exit(0)


if __name__ == '__main__':
    main()
