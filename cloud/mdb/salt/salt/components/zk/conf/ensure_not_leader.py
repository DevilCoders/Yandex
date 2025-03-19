#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Ensure that the local ZooKeeper is not a leader.
"""
import argparse
import functools
import math
import os
import re
import socket
import sys
import time


DEFAULT_CLIENT_PORT = 2181
DEFAULT_CONFIG_PATH = '/etc/zookeeper/conf.yandex/zoo.cfg'


def parse_config(config_path, result):
    """
    Load existing config file.
    """
    if os.path.exists(config_path):
        with open(config_path) as f:
            for line in f.readlines():
                line = line.split('#', 1)[0]  # Allow comments
                if line.strip():
                    # DigestAuthenticationProvider.superDigest is base64 and contians symbols '='
                    key, value = [token.strip() for token in line.split('=', 1)]
                    result[key] = value


@functools.lru_cache(maxsize=None)
def load_config(config_path):
    """
    Load ZooKeeper config.
    """
    result = {}
    parse_config(config_path, result)
    if 'dynamicConfigFile' in result:
        parse_config(result['dynamicConfigFile'], result)

    return result


def get_client_port(config_path):
    """
    Get ZooKeeper client port from config or default.
    """
    config = load_config(config_path)
    return config['clientPort'] if 'clientPort' in config else DEFAULT_CLIENT_PORT


def zk_command(command, port=DEFAULT_CLIENT_PORT):
    """
    Execute ZooKeeper 4-letter command.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(3)
        s.connect(('localhost', port))

        s.sendall(command.encode())
        return s.makefile().read(-1)


def zk_mntr(port=DEFAULT_CLIENT_PORT, retries=3):
    """
    Execute ZooKeeper mntr command and parse its output.
    """
    result = {}
    while retries > 0:
        try:
            response = zk_command('mntr', port)
            for line in response.split('\n'):
                key_value = re.split(r'\s+', line, 1)
                if len(key_value) == 2:
                    result[key_value[0]] = key_value[1]

            if not len(result) > 1:
                raise RuntimeError(f'Too short response: {response.strip()}')

            break

        except Exception as e:
            retries -= 1
            if retries <= 0:
                raise RuntimeError(f'Unable to get ZooKeeper status: {e!r}')
            time.sleep(1)

    return result


def count_servers(config_path):
    """
    Count servers in zookeeper cluster.
    """
    servers = 0
    config = load_config(config_path)
    for key in config:
        if key.startswith('server.'):
            servers += 1
    return servers


def ensure_not_leader(port, attempts, config_path):
    """
    Ensure that the local host is not a leader.
    """
    mntr = zk_mntr(port)
    if 'zk_server_state' not in mntr:
        raise RuntimeError('Can\'t get server state')
    if mntr['zk_server_state'] == 'follower':
        return
    servers = count_servers(config_path)
    if servers < 3:
        raise RuntimeError(f'Not enough servers in cluster, found {servers}, required at least 3')
    min_quorum = math.ceil((servers + 1) / 2)
    for _ in range(attempts):
        followers = int(mntr['zk_synced_followers'])
        if followers < min_quorum:
            raise RuntimeError(
                f'Can\'t reach quorum without local server, required {min_quorum}, alive and synchronized {followers} followers'
            )
        os.system('service zookeeper restart')
        time.sleep(1)
        mntr = zk_mntr(port, 60)
        if 'zk_server_state' not in mntr:
            raise RuntimeError('Can\'t get server state')
        if mntr['zk_server_state'] == 'follower':
            return
    raise RuntimeError(f'Reach maximum {attempts} attempts, switching leader unsuccess')


def _dry_run(port):
    mntr = zk_mntr(port)
    sys.exit(mntr['zk_server_state'] == 'leader')


def _ensure_not_leader(dry_run, attempts, config_path):
    port = get_client_port(config_path)
    if dry_run:
        _dry_run(port)
    ensure_not_leader(port, attempts, config_path)


def _get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--dry-run', action="store_true", default=False, help='Test run feature')
    parser.add_argument('-a', '--attempts', type=int, default=10, help='Attempts to make reelection')
    parser.add_argument('-c', '--config', default=DEFAULT_CONFIG_PATH, help='Path to zoo.cfg')
    args = parser.parse_args()
    return args.dry_run, args.attempts, args.config


if __name__ == '__main__':
    try:
        dry_run, attempts, config_path = _get_args()
        _ensure_not_leader(dry_run, attempts, config_path)
    except Exception as e:
        print(f'Can\'t reelect new leader: {e}')
        sys.exit(1)
