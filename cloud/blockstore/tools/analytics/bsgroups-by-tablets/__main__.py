import argparse
import json
import logging
import math
import requests
import sys

import cloud.blockstore.tools.nbsapi as nbs
import cloud.blockstore.tools.kikimrapi as kikimr
import cloud.blockstore.tools.solomonapi as solomon

from cloud.blockstore.public.sdk.python.client import CreateClient
from collections import defaultdict


def fetch(url):
    logging.info("fetching %s" % url)
    r = requests.get(url)
    r.raise_for_status()
    logging.debug(r.content)
    return r


def get_volume_descriptions(client, logging):
    result = []

    volumes = client.list_volumes()

    for volume in volumes:
        try:
            description = nbs.describe_volume(client, volume)
        except Exception as e:
            logging.exception('Got exception in nbs.describe_volume: {}'.format(str(e)))
        else:
            result.append(description)

    return result


def fetch_group_info(host, port, hive_tablet_id, partitions):
    groups = []

    for partition in partitions:
        # only data channels are taken into account
        groups += [x[1] for x in kikimr.fetch_groups(host, port, hive_tablet_id, int(partition["TabletId"])) if x[0] > 2]

    return groups


PROBS = [0.5, 0.6, 0.7, 0.8, 0.9, 0.95, 0.99, 0.999, 0.9999, 1]


def calc_percentiles(values):
    values.sort()
    percentiles = []
    for q in PROBS:
        p = values[min(int(q * len(values)), len(values) - 1)] if len(values) else 0
        percentiles.append((100 * q, p))
    return percentiles


def findp(percentiles, x):
    for p, v in percentiles:
        if abs(x - p) < 1e-10:
            return v

    raise Exception("percentile %s not found" % x)


def percentiles_str(percentiles):
    return "\t".join("p%s=%s" % (x[0], round(x[1])) for x in percentiles)


class StatGroup(object):

    def __init__(self, labels):
        self.name = "%s_%s" % (labels["request"], labels["sensor"]) if isinstance(labels, dict) else labels
        self.percentiles = []
        self.values = []

    def recalculate_percentiles(self):
        self.percentiles = calc_percentiles(self.values)


class JSONEncoder(json.JSONEncoder):

    def default(self, o):
        if isinstance(o, StatGroup):
            return {
                "__tag": "StatGroup",
                "name": o.name,
                "values": o.values,
            }

        return json.JSONEncoder.default(self, o)


def json_decoder_hook(j):
    if j.get("__tag") == "StatGroup":
        o = StatGroup(j["name"])
        o.values = j["values"]
        return o

    return j


def fetch_usage_stats(cluster, volume, interval):
    sensors = solomon.fetch(solomon.api_url(cluster, "server_volume", interval, [
        ("volume", volume),
        ("host", "cluster"),
        ("request", "ReadBlocks|WriteBlocks|ZeroBlocks"),
        ("sensor", "RequestBytes|Count"),
    ]))

    stats = []
    for sensor in sensors:
        stat_group = StatGroup(sensor["labels"])
        stat_group.values = sorted(v["value"] for v in sensor["values"])

        stats.append(stat_group)

    return stats


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='server', default='localhost')
    parser.add_argument('--nbs-port', help='nbs grpc port', default=9766)
    parser.add_argument('--monport', help='kikimr monitoring port', default=8765)
    parser.add_argument('--cluster', help='cloud cluster', default='yandexcloud_prod')
    parser.add_argument('--interval', help='stats interval', default='24h')
    parser.add_argument('--hive-tablet-id', help='hive tablet id (bs group info is fetched from hive)', default='72057594037968897')
    parser.add_argument('--volume-info', help='volume info file')
    parser.add_argument('--volume-stats', help='volume stats file')
    parser.add_argument('--group-info', help='group info file')
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.ERROR

    logging.basicConfig(stream=sys.stderr, level=log_level, format="[%(levelname)s] [%(asctime)s] %(message)s")

    # gathering volume configs to build suggested performance profiles

    if args.volume_info is not None:
        with open(args.volume_info) as f:
            volumes = json.load(f)
    else:
        client = CreateClient('{}:{}'.format(args.host, args.nbs_port), log=logging)
        volumes = get_volume_descriptions(client, logging)
        with open("volumeinfo.json", "w") as f:
            json.dump(volumes, f, indent=4)

    # gathering volume usage stats to find out real volume usage

    if args.volume_stats is not None:
        with open(args.volume_stats) as f:
            volume2stats = json.load(f, object_hook=json_decoder_hook)
    else:
        volume2stats = dict()
        for volume in volumes:
            name = volume["Name"]
            volume2stats[name] = fetch_usage_stats(args.cluster, name, args.interval)
        with open("volumestats.json", "w") as f:
            json.dump(volume2stats, f, indent=4, cls=JSONEncoder)

    # gathering volume -> bs group mapping to find out real bs group usage

    if args.group_info is not None:
        with open(args.group_info) as f:
            group2volumes = json.load(f)
    else:
        m = defaultdict(list)
        for volume in volumes:
            groups = fetch_group_info(args.host, args.monport, args.hive_tablet_id, volume["Partitions"])
            for group in groups:
                m[group].append(volume["Name"])

        group2volumes = []
        for x, y in m.iteritems():
            group2volumes.append((x, sorted(y)))
        with open("groupinfo.json", "w") as f:
            json.dump(group2volumes, f, indent=4)

    # calculating some volume stats

    volume2au_count = dict()
    for volume in volumes:
        sz = 0
        vc = volume["VolumeConfig"]
        for partition_config in vc["Partitions"]:
            sz += int(partition_config["BlockCount"]) * vc.get("BlockSize", 4096)
        # both hybrid and pure hdd disks are considered to be hdd here
        # index size not taken into account here - only the size visible to the guest is considered
        storage_media_kind = vc.get("StorageMediaKind")
        is_ssd = storage_media_kind == 1 if storage_media_kind is not None else vc.get("ChannelProfileId") == 4
        GB = 2 ** 30
        au_size = (32 if is_ssd else 256) * GB
        au_count = math.ceil(float(sz) / au_size)
        volume2au_count[volume["Name"]] = au_count

    # calculating bs group usage stats and finding out the required capacity
    # for the existing groups in terms of allocation units

    class Average(object):

        def __init__(self):
            self.count = 0
            self.value = 0

        def register(self, value):
            self.count += 1
            self.value += value

        def calc(self):
            return self.value / self.count

    group_stats = defaultdict(list)
    volume2group_count = defaultdict(int)
    for group, volume_names in group2volumes:
        for volume in volume_names:
            volume2group_count[volume] += 1

    volume_counts = []
    group2au_count = []
    for group, volume_names in group2volumes:
        volume_counts.append(len(volume_names))

        gs = defaultdict(Average)
        au_count = 0
        for volume in volume_names:
            vs = volume2stats.get(volume)
            if vs is not None:
                for s in vs:
                    if s.name.endswith("RequestBytes"):
                        for v in s.values:
                            gs[s.name].register(v / volume2group_count[volume])

            au_count += float(volume2au_count[volume]) / volume2group_count[volume]

        group2au_count.append((group, au_count))

        for name, a in gs.iteritems():
            logging.info("Stat=%s\tGroup=%s\t%s" % (name, group, a.calc()))
            group_stats[name].append((group, a.calc()))

    print("Top groups by average used bandwidth")
    for name, sl in group_stats.iteritems():
        sl.sort(key=lambda x: -x[1])
        for i in xrange(0, min(10, len(sl))):
            print("\tStat=%s\tGroup=%s\t%s" % (name, sl[i][0], sl[i][1]))

    group2volumes.sort(key=lambda x: -len(x[1]))
    print("Top groups by volume count")
    for i in xrange(min(10, len(group2volumes))):
        group, volume_names = group2volumes[i]
        print("\tGroup=%s\tVolumeCount=%s" % (group, len(volume_names)))

    print("Volume count percentiles")
    print("\t%s" % percentiles_str(calc_percentiles(volume_counts)))

    group2au_count.sort(key=lambda x: -x[1])
    print("Group -> AU count, sorted in descending order")
    for group, au_count in group2au_count:
        print("\tGroup=%s\tAUCount=%s" % (group, au_count))


if __name__ == '__main__':
    sys.exit(main())
