#!/usr/bin/env python

import argparse
import json
import logging
import re
import sys

import concurrent.futures
import cloud.blockstore.tools.cms.lib.pssh as libpssh

from cloud.blockstore.libs.storage.protos import disk_pb2
from cloud.blockstore.public.api.protos.disk_pb2 import TDescribeDiskRegistryConfigResponse
from google.protobuf.text_format import Parse


DR_TABLET_ID = {
    'prod': {
        'sas': '72075186229350205',
        'vla': '72075186230158522',
        'myt': '72075186228787222'
    },
    'preprod': {
        'sas': '72075186227537969',
        'vla': '72075186227734559',
        'myt': '72075186227185491'
    }
}


def agent_url(cluster, agent_id):
    dc = agent_id[:3]
    tablet = DR_TABLET_ID[cluster][dc]
    return f'http://localhost:8766/tablets/app?action=agent&TabletID={tablet}&AgentID={agent_id}'


def device_url(cluster, dc, uuid):
    tablet = DR_TABLET_ID[cluster][dc]
    return f'http://localhost:8766/tablets/app?action=dev&TabletID={tablet}&DeviceUUID={uuid}'


def prepare_logging(args):
    log_level = logging.ERROR

    if args.silent:
        log_level = logging.INFO
    elif args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))

    logging.basicConfig(
        stream=sys.stderr,
        level=log_level,
        format="[%(levelname)s] [%(asctime)s] %(message)s")


def parse_device_info(html):

    i = 0
    # uuid
    i = html.find('<td>', i)
    if i == -1:
        assert(0)
    i += 4
    j = html.find('</td>', i)
    if j == -1:
        assert(0)
    col = html[i:j]
    assert(col.startswith('<a'))

    uuid = col[col.find('>')+1 : col.find('</a>')]
    assert(uuid)

    # name

    i = html.find('<td>', i)
    if i == -1:
        assert(0)
    i += 4
    j = html.find('</td>', i)
    if j == -1:
        assert(0)
    name = html[i:j]
    assert(name)

    # state
    state = None

    i = html.find('<td>', i)
    if i == -1:
        assert(0)
    i += 4
    j = html.find('</td>', i)
    if j == -1:
        assert(0)
    col = html[i:j]

    if col.find('online') != -1:
        state = 'online'
    elif col.find('error') != -1:
        state = 'error'
    elif col.find('warning') != -1:
        state = 'warning'

    assert(state is not None)

    # skip
    for _ in range(5):
        i = html.find('<td>', i)
        if i == -1:
            assert(0)
        i += 4
        j = html.find('</td>', i)
        if j == -1:
            assert(0)

    # disk_id
    disk_id = None

    i = html.find('<td>', i)
    if i == -1:
        assert(0)
    i += 4
    j = html.find('</td>', i)
    if j == -1:
        assert(0)
    col = html[i:j]
    if col:
        assert(col.startswith('<a'))
    disk_id = col[col.find('>')+1 : col.find('</a>')]

    return (uuid, name, state, disk_id)

# (state, [ (uuid, name, state, disk_id) ])
def parse_agent_info(html):
    state = None
    if html.find('State: <font color=green>online</font>') != -1:
        state = 'online'
    elif html.find('State: <font color=red>error</font>') != -1:
        state = 'error'
    elif html.find('State: warning') != -1:
        state = 'warning'
    elif html.find('State: <font color=red>unavailable</font>') != -1:
        state = 'unavailable'

    assert(state is not None)

    devices = []

    i = 0
    while True:
        i = html.find('<tr>', i)
        if i == -1:
            break
        i += 4
        j = html.find('</tr>', i)
        if j == -1:
            assert(0)
        row = html[i:j]
        if row.startswith('<th>'):
            continue # skip header
        devices.append(parse_device_info(row))

    assert (len(devices) == 15)

    return (state, devices)


def get_control_target(cluster, dc):
    return f"C@cloud_{cluster}_nbs-control_{dc}[0]"


def update_device_state(pssh, uuid, state, cluster, dc):
    data = json.dumps({
        "Message": "cleanup-nrd",
        "ChangeDeviceState": {
            "DeviceUUID": uuid,
            "State": state
        }
    })

    r = pssh.run(
        f"blockstore-client executeaction --config .nbs-client/config.txt --action diskregistrychangestate --input-bytes '{data}'",
        get_control_target(cluster, dc))
    assert(r)


def cleanup_agent(
    pssh,
    agent_id,
    free_devices,
    skip_devices,
    cluster,
    dry_run):

    dc = agent_id[:3]
    url = agent_url(cluster, agent_id)
    html = pssh.run(f'curl -s "{url}"', get_control_target(cluster, dc))
    assert(html)
    state, devices = parse_agent_info(html)
    if state != 'online':
        logging.info(f"skip agent {agent_id} by state {state}")
        return

    logging.debug(f"agent devices: {devices}")

    devices_to_cleanup = []
    for device in devices:
        if device[0] not in skip_devices:
            devices_to_cleanup.append(device)
        else:
            logging.debug(f"skip device {agent_id}:{device[0]} by filter")

    logging.info(f"start cleanup for agent {agent_id}")
    logging.debug(f"devices to cleanup: {devices}")

    for uuid, name, state, disk_id in devices:
        if disk_id:
            logging.debug(f"skip cleanup for allocated device {agent_id}:{uuid}")
            continue
        if uuid not in free_devices:
            logging.warning(f"skip cleanup for allocated device {agent_id}:{uuid} (was free)")
            continue

        if state != 'online':
            logging.debug(f"skip device {agent_id}:{uuid} by state {state}")
            continue

        logging.info(f"switch device {agent_id}:{uuid} to WARNING")
        update_device_state(pssh, uuid, 1, cluster, dc)

        html = pssh.run(f'curl -s "{device_url(cluster, dc, uuid)}"', get_control_target(cluster, dc))
        assert(html)
        if html.find('Disk: ') != -1:
            logging.debug(f"device {agent_id}:{uuid} was allocated")
            logging.info(f"switch device {agent_id}:{uuid} to ONLINE")
            update_device_state(pssh, uuid, 0, cluster, dc)
            continue

        if html.find('<div>State: warning</div>') == -1:
            logging.warning(f"device {agent_id}:{uuid} in wrong state")
            continue

        if name.find("NBS") == -1:
            logging.error(f"unexpected path {agent_id}:{uuid} {name}")

        logging.info(f"start cleanup device {agent_id}:{uuid} {name} ...")
        if not dry_run:
            r = pssh.run(f'sudo dd if=/dev/zero oflag=direct,sync of={name} bs=4M 2>&1 || true', agent_id)
            if r.find('No space left on device') == -1:
                logging.warn(f"fail to cleanup device {agent_id}:{uuid}")
        logging.info(f"cleanup {agent_id}:{uuid} {name} done")

        logging.info(f"switch device {agent_id}:{uuid} to ONLINE")
        update_device_state(pssh, uuid, 0, cluster, dc)

    logging.info(f"cleanup for agent {agent_id} finished")


def load_disks(pssh, tablet_id, cluster, dc):
    url = f"http://localhost:8766/tablets/app?TabletID={tablet_id}"
    lines = pssh.run(
        f'curl -s "{url}" | grep -Eo "DiskID=\w+" | grep -Eo "\\w+$" | sort -u',
        get_control_target(cluster, dc))
    assert(lines)

    return [line for line in lines.split('\n') if len(line) != 0]


def load_devices(pssh, cluster, dc):
    devices = set()
    text = pssh.run(
        f"blockstore-client describediskregistryconfig --config .nbs-client/config.txt --proto --verbose error",
        get_control_target(cluster, dc))

    assert(text)

    r = Parse(text, TDescribeDiskRegistryConfigResponse())
    for agent in r.KnownAgents:
        for device in agent.Devices:
            devices.add(device)
    return devices


def get_volume_devices(pssh, disk_id, cluster, dc):
    lines = pssh.run(
        f"blockstore-client describevolume --disk-id {disk_id} --config .nbs-client/config.txt --verbose error",
        get_control_target(cluster, dc))

    if lines is None:
        return []

    devices = []

    lines = lines.split('\n')
    for line in lines:
        m = re.search(r'DeviceUUID: \"([a-z0-9]+)\"', line)
        if m:
            devices.append(m.group(1))

    return devices


def find_free_devices(pssh, cluster):
    all_free_devices = {}
    for dc, tablet_id in DR_TABLET_ID[cluster].items():
        logging.debug(f"find free devices for {dc}...")

        allocated_count = 0
        devices = load_devices(pssh, cluster, dc)
        disks = load_disks(pssh, tablet_id, cluster, dc)

        with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
            future_to_disk = dict()

            for disk_id in disks:
                future_to_disk[executor.submit(
                    get_volume_devices,
                    pssh,
                    disk_id,
                    cluster,
                    dc)] = disk_id

            for future in concurrent.futures.as_completed(future_to_disk):
                disk_id = future_to_disk[future]
                allocated_devices = future.result()
                logging.debug(f"devices of disk {disk_id}: {allocated_devices}")
                allocated_count += len(allocated_devices)
                for uuid in allocated_devices:
                    devices.remove(uuid)

        logging.debug(f"total free devices for {dc}: {len(devices)}. Allocated: {allocated_count}")

        all_free_devices[dc] = devices

    return all_free_devices


def load_list(p):
    items = []
    with open(p) as f:
        for line in f.readlines():
            line = line.strip()
            if line:
                items.append(line)
    return items


def cleanup(pssh, args):
    agents = load_list(args.agents)

    skip_devices = set()
    if args.skip_devices:
        skip_devices = set(load_list(args.skip_devices))

    logging.debug(f"Agents {len(agents)}")

    free_devices = find_free_devices(pssh, args.cluster)

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.parallelism) as executor:
        future_to_agent = dict()

        for agent_id in agents:
            dc = agent_id[:3]

            future_to_agent[executor.submit(
                cleanup_agent,
                pssh,
                agent_id,
                free_devices[dc],
                skip_devices,
                args.cluster,
                args.dry_run)] = agent_id

        for future in concurrent.futures.as_completed(future_to_agent):
            agent_id = future_to_agent[future]
            try:
                future.result()
            except Exception as e:
                logging.error(f'Error for {agent_id} : {e}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", '--silent', help="silent mode", default=0, action='count')
    parser.add_argument("-v", '--verbose', help="verbose mode", default=0, action='count')
    parser.add_argument("--agents", type=str, required=True)
    parser.add_argument("--skip-devices", type=str)
    parser.add_argument("--cluster", type=str, choices=['prod', 'preprod'], required=True)
    parser.add_argument("-p", "--parallelism", type=int, default=1)
    parser.add_argument('--dry-run', action='store_true', help='dry run')

    args = parser.parse_args()

    prepare_logging(args)

    pssh = libpssh.Pssh()

    cleanup(pssh, args)

    return 0


if __name__ == '__main__':
    sys.exit(main())
