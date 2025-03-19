#!/usr/bin/env python
"""
Duplicate containers monitoring
"""

import argparse
import json
import socket
import sys
import time
import uuid

import requests

DBM_TOKEN = "{{ salt['pillar.get']('data:config:dbm_token', '') }}"
DBM_URL = "{{ salt['pillar.get']('data:config:dbm_url', '') }}"
PREV_RUNS_FILE = '/var/cache/duplicated_containers.json'


def die(status, message):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def get_dom0_containers():
    import porto
    conn = porto.Connection()
    conn.connect(timeout=1)
    containers = []
    stopped_containers = []
    meta_containers = []
    for cont in conn.ListContainers():
        if cont.name == 'fill_page_cache':
            continue
        if cont.GetProperties()['parent'] != '/':
            # Ignore sub containers
            # DBM absent containers: myt-j5uvpuae6mu7hhlh.db.yandex.net/core-3919058
            continue
        if cont.GetProperties()['state'] == 'meta':
            meta_containers.append(cont.name)
        if cont.GetProperties()['state'] == 'running':
            containers.append(cont.name)
        if cont.GetProperties()['state'] == 'stopped':
            stopped_containers.append(cont.name)
    return containers, stopped_containers, meta_containers


def _is_transfer_placeholder(fqdn):
    """
    When container transferring DBM creates a placeholder on destination dom0.
    And use UUID instead of FQDN.
    We ignore such containers, cause they are not running at that moment.
    """
    try:
        _ = uuid.UUID(fqdn)
        return True
    except ValueError:
        return False


def get_dbm_containers(fqdn):
    """
    Call DBM backend via REST API to get containers
    """
    limit = 1000
    url = '{url}/api/v2/containers/'.format(url= DBM_URL)
    headers = {
        'Authorization': 'OAuth {token}'.format(token=DBM_TOKEN),
    }
    query = {'dom0': fqdn, 'limit': limit}
    search_string = ''
    for key, value in query.items():
        search_string += '{key}={value};'.format(key = key, value = value)
    res = requests.get(
        url, headers=headers, verify='/opt/yandex/allCAs.pem',
        params={'query': search_string}, timeout=5)
    if res.status_code != 200:
        raise Exception('Request to DBM failed ({code}: {text})'.format(
            code=res.status_code, text=res.text))

    containers = []
    for container in res.json():
        if container['dom0'] == fqdn:
            container_fqdn = container['fqdn']
            if _is_transfer_placeholder(container_fqdn):
                continue
            containers.append(container_fqdn)
    return containers


def load_prev_run():
    try:
        with open(PREV_RUNS_FILE) as fd:
            return json.load(fd)
    except IOError:
        return {
            'redundant': {},
            'absent': {},
        }


def store_new_run(run):
    with open(PREV_RUNS_FILE, 'w') as fd:
        json.dump(run, fd)


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--warn', type=int, default=5,
        help='a number of minutes after which we fire WARN for a problem container',
    )
    parser.add_argument(
        '--crit', type=int, default=10,
        help='a number of minutes after which we fire CRIT for a problem container',
    )
    args = parser.parse_args()

    try:
        existing, stopped_containers, meta_containers = get_dom0_containers()
    except Exception as exc:
        die(1, 'Porto interaction error: {error}'.format(error=repr(exc)))

    try:
        expected = get_dbm_containers(socket.gethostname())
    except Exception as exc:
        die(1, 'DBM interaction error: {error}'.format(error=repr(exc)))

    previous_runs = load_prev_run()

    class Report:
        """
        Just a namespace. And tiny hack that allows us to change
        primitive is_crit from process_containers function
        """
        containers = []
        is_crit = False

    def process_containers(kind, fqdns):
        notify_fqdns = []
        for container_fqdn in fqdns:
            if container_fqdn not in previous_runs[kind]:
                previous_runs[kind][container_fqdn] = time.time()
            age = (time.time() - previous_runs[kind][container_fqdn]) / 60
            if age < args.warn:
                # for an 'absent case' container may be deleting at that moment
                continue
            if age > args.crit:
                Report.is_crit = True
            notify_fqdns.append(container_fqdn)

        # remove all fqdns that not a problematic from previous_runs
        for prev_fqdn in list(previous_runs[kind]):
            if prev_fqdn not in fqdns:
                del previous_runs[kind][prev_fqdn]

        if notify_fqdns:
            Report.containers.append('%s containers: ' % kind + ', '.join(sorted(notify_fqdns)))

    # containers that exist on dom0 but are not present in DBM
    redundant = set(existing).difference(set(expected))
    process_containers('redundant', redundant)

    # containers that are present in DBM and do not exist on dom0
    absent = set(expected).difference(set(existing + stopped_containers + meta_containers))
    process_containers('absent', absent)

    store_new_run(previous_runs)

    if Report.containers:
        die(2 if Report.is_crit else 1, 'DBM ' + ' ; '.join(Report.containers))
    die(0, 'OK')


if __name__ == '__main__':
    _main()
