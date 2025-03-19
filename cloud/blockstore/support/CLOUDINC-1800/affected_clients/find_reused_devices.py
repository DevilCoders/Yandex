#!/usr/bin/env python3

import argparse
import datetime
import json
import sys

from collections import defaultdict


# see https://yql.yandex-team.ru/Operations/YR_LcPMBw0edgDqU1-14YXQqQ6czRNX2MYergPhMg_0=
# to find out how the disk_id - cloud_id mapping was built

_NBS_TESTS_CLOUD_ID = "b1grjf2o6a6f1fmqeu6j"


class Event(object):

    def __init__(self, device_id, disk_id, timestamp):
        self.device_id = device_id
        self.disk_id = disk_id
        self.timestamp = timestamp


class CloudInfo(object):

    def __init__(self, billing_id, client_info, is_trial):
        self.billing_id = billing_id
        self.client_info = client_info
        self.is_trial = is_trial


def print_cloud_info(cloud_info):
    return "BillingId=%s,ClientInfo=%s%s" % (
        cloud_info.billing_id,
        cloud_info.client_info,
        ",TRIAL" if cloud_info.is_trial else ""
    ) if cloud_info is not None else "NO_CLOUD_INFO"


class CompromiseInfo(object):

    def __init__(self, compromised_disk_id, device_id, compromised_by_disk_id, compromised_by_cloud_id):
        self.compromised_disk_id = compromised_disk_id
        self.device_id = device_id
        self.compromised_by_disk_id = compromised_by_disk_id
        self.compromised_by_cloud_id = compromised_by_cloud_id


def print_compromise_info(compromise_info):
    return "CompromisedDiskId=%s,DeviceId=%s,CompromisedByDiskId=%s,CompromisedByCloudId=%s" % (
        compromise_info.compromised_disk_id,
        compromise_info.device_id,
        compromise_info.compromised_by_disk_id,
        compromise_info.compromised_by_cloud_id
    )


def process_events(
        tag,
        events,
        test_disk_ids,
        disk_id2cloud_id,
        cloud_id2info,
        existing_disk_ids,
        disk_ids_from_images,
        compromised_by_existing):
    device2events = defaultdict(list)

    all_disk_ids = set()
    all_device_ids = set()

    for event in events:
        # print("%s, %s, %s" % (event.disk_id, event.device_id, event.timestamp))
        device2events[event.device_id].append(event)
        if event.disk_id not in test_disk_ids and event.disk_id is not None:
            all_disk_ids.add(event.disk_id)
        all_device_ids.add(event.device_id)

    compromised = defaultdict(list)
    compromised_disks = set()

    print("========== %s ==========" % tag)
    print("========== %s ==========" % tag, file=sys.stderr)

    for device_id, device_events in device2events.items():
        assert len(device_events) > 0

        if len(device_events) == 1:
            continue

        class Context(object):

            def __init__(self):
                self.current_chain = []

        def flush_chain(context):
            if len(context.current_chain) == 0:
                return

            last_cloud_id = None
            sub_chain = []
            all_sub_chains = []
            for event in context.current_chain:
                disk_id = event.disk_id
                cloud_id = disk_id2cloud_id.get(disk_id)
                if cloud_id is None:
                    print("WARNING: failed to find cloud_id for disk %s, generating cloud_id from disk_id" % disk_id)
                    cloud_id = "UNRESOLVED_CLOUD_FOR_DISK_%s" % disk_id
                    disk_id2cloud_id[disk_id] = cloud_id
                if last_cloud_id is not None and last_cloud_id != cloud_id:
                    all_sub_chains.append((last_cloud_id, sub_chain))
                    sub_chain = []

                last_cloud_id = cloud_id
                sub_chain.append(event)

            assert last_cloud_id is not None
            assert len(sub_chain) > 0
            all_sub_chains.append((last_cloud_id, sub_chain))

            for i in range(len(all_sub_chains) - 1):
                cloud_id = all_sub_chains[i][0]
                sub_chain = all_sub_chains[i][1]
                next_sub_chain = all_sub_chains[i + 1][1]

                next_disk_id = next_sub_chain[-1].disk_id

                if compromised_by_existing and next_disk_id not in existing_disk_ids:
                    continue

                if next_disk_id in disk_ids_from_images:
                    continue

                for event in sub_chain:
                    compromised[cloud_id].append(CompromiseInfo(
                        event.disk_id,
                        device_id,
                        next_disk_id,
                        disk_id2cloud_id[next_disk_id]
                    ))
                    compromised_disks.add(event.disk_id)

            context.current_chain = []

        context = Context()

        for event in device_events:
            if event.disk_id in test_disk_ids or disk_id2cloud_id.get(event.disk_id) == _NBS_TESTS_CLOUD_ID:
                # nbs tests break device transfer chains since nbs tests fill devices with random data
                flush_chain(context)
            elif event.disk_id is None:
                # the device was a migration target => it got completely rewritten, device transfer chain broken
                flush_chain(context)
            else:
                context.current_chain.append(event)

        flush_chain(context)

        print("device_transfers=%s,%s" % (device_id, [x.disk_id for x in device_events]), file=sys.stderr)

    def get_cloud_info(cloud_id):
        return cloud_id2info.get(cloud_id)

    cloud_id2compromised_by_cloud_ids = defaultdict(set)
    disks_containing_alien_data = dict()

    compromised_disk_id2cloud_id = dict()

    for cloud_id, compromise_infos in sorted(compromised.items()):
        cloud_info = get_cloud_info(cloud_id)

        print("compromised_cloud\tCloudId=%s\t%s\tDiskDevicePairs=%s" % (
            cloud_id,
            print_cloud_info(cloud_info),
            [print_compromise_info(x) for x in compromise_infos]
        ), file=sys.stderr)
        for compromise_info in compromise_infos:
            assert compromise_info.compromised_disk_id not in test_disk_ids
            assert compromise_info.compromised_by_disk_id not in test_disk_ids
            cloud_id2compromised_by_cloud_ids[cloud_id].add(compromise_info.compromised_by_cloud_id)

            if compromise_info.compromised_by_disk_id not in disks_containing_alien_data \
                    and compromise_info.compromised_by_disk_id in existing_disk_ids:
                disks_containing_alien_data[compromise_info.compromised_by_disk_id] = compromise_info.compromised_by_cloud_id

            compromised_disk_id2cloud_id[compromise_info.compromised_disk_id] = cloud_id

    for disk_id, cloud_id in sorted(compromised_disk_id2cloud_id.items()):
        print("compromised_disk\tDiskId=%s\tCloudId=%s\t%s" % (
            disk_id,
            cloud_id,
            print_cloud_info(get_cloud_info(cloud_id))
        ), file=sys.stderr)

    for cloud_id, compromised_by_cloud_ids in sorted(cloud_id2compromised_by_cloud_ids.items()):
        cloud_info = get_cloud_info(cloud_id)

        for compromised_by_cloud_id in sorted(list(compromised_by_cloud_ids)):
            compromised_by_cloud_info = get_cloud_info(compromised_by_cloud_id)
            print("compromised_by_cloud\tCloudId=%s\t%s\tBY\tCloudId=%s\t%s" % (
                cloud_id,
                print_cloud_info(cloud_info),
                compromised_by_cloud_id,
                print_cloud_info(compromised_by_cloud_info)
            ), file=sys.stderr)

    for disk_id, cloud_id in sorted(disks_containing_alien_data.items()):
        cloud_info = get_cloud_info(cloud_id)

        print("disk_with_alien_data\tDiskId=%s\tCloudId=%s\t%s" % (
            disk_id,
            cloud_id,
            print_cloud_info(cloud_info)
        ), file=sys.stderr)

    print("compromised_cloud_count=", len(compromised))
    print("compromised_disk_count=", len(compromised_disks))
    print("disks_containing_alien_data_count=", len(disks_containing_alien_data))
    print("total_disk_count=", len(all_disk_ids))
    print("total_device_count=", len(all_device_ids))


parser = argparse.ArgumentParser()
parser.add_argument("--disk-device-events", help="device timeout log", required=True)
parser.add_argument("--device-migration-events", help="device migration (source -> target) log", required=True)
parser.add_argument("--test-disk-ids", help="disk ids belonging to nbs tests", required=True)
parser.add_argument("--disk-id-to-cloud-id", help="disk_id to cloud_id mapping", required=True)
parser.add_argument("--cloud-id-to-info", help="cloud_id to cloud info (billing account id, client name, etc.) mapping", required=True)
parser.add_argument("--existing-disk-ids", help="existing disk ids", required=True)
parser.add_argument("--disk-ids-from-images", help="ids of the disks created from images", required=True)
parser.add_argument("--compromised-by-existing", help="consider a disk to be compromised if it's data might reside on an existing disk", action='store_true', default=False)

args = parser.parse_args()

test_disk_ids = set()
for line in open(args.test_disk_ids).readlines():
    test_disk_ids.add(line.rstrip())

disk_id2cloud_id = dict()
for line in open(args.disk_id_to_cloud_id).readlines():
    d = json.loads(line.rstrip())
    disk_id2cloud_id[d["disk_id"]] = d["cloud_id"]

cloud_id2info = dict()
for line in open(args.cloud_id_to_info).readlines():
    if line.startswith(","):
        continue

    parts = line.rstrip().split(",")
    billing_id = parts[1]
    cloud_id = parts[2]
    client_info = parts[4]
    is_trial = parts[7] == "trial"
    cloud_id2info[cloud_id] = CloudInfo(billing_id, client_info, is_trial)

existing_disk_ids = set()
for line in open(args.existing_disk_ids).readlines():
    existing_disk_ids.add(line.rstrip())

vla_events = []
sas_events = []
myt_events = []

for line in open(args.disk_device_events).readlines():
    data = json.loads(line.rstrip())
    device_id = data["device_id"]
    disk_id = data["disk_id"]
    ts_str = data["min_timestamp"]
    ts = datetime.datetime.strptime(ts_str, "%Y-%m-%d %H:%M:%S.%f")
    events = None
    if disk_id.startswith("epd"):
        events = sas_events
    elif disk_id.startswith("fhm"):
        events = vla_events
    else:
        assert disk_id.startswith("ef3")
        events = myt_events
    events.append(Event(device_id, disk_id, ts))

for line in open(args.device_migration_events).readlines():
    data = json.loads(line.rstrip())
    device_id = data["target_id"]
    ts_str = data["min_timestamp"]
    ts = datetime.datetime.strptime(ts_str, "%Y-%m-%d %H:%M:%S.%f")
    for events in [vla_events, sas_events, myt_events]:
        events.append(Event(device_id, None, ts))

disk_ids_from_images = set()
for line in open(args.disk_ids_from_images).readlines():
    parts = line.rstrip().split("\t")
    disk_ids_from_images.add(parts[0])

for tag, events in [("vla", vla_events), ("sas", sas_events), ("myt", myt_events)]:
    events.sort(key=lambda x: x.timestamp)
    process_events(
        tag,
        events,
        test_disk_ids,
        disk_id2cloud_id,
        cloud_id2info,
        existing_disk_ids,
        disk_ids_from_images,
        args.compromised_by_existing
    )
