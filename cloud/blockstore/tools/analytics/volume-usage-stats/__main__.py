import argparse
import json
import logging
import os
import requests
import sys

import cloud.blockstore.tools.nbsapi as nbs
import cloud.blockstore.tools.solomonapi as solomon

from cloud.blockstore.public.sdk.python.client import CreateClient


def fetch(url):
    logging.info("fetching %s" % url)
    r = requests.get(url)
    r.raise_for_status()
    logging.debug(r.content)
    return r


def fetch_volume2info(client, logging, disk_ids):
    result = []

    for disk_id in disk_ids:
        try:
            volume_info = nbs.describe_volume(client, disk_id)
        except Exception as e:
            logging.exception('Got exception in nbs.describe_volume: {}'.format(str(e)))
        else:
            result.append(volume_info)

    return result


class Stat(object):

    def __init__(
            self,
            read_bandwidth=None,
            write_bandwidth=None,
            read_iops=None,
            write_iops=None):
        self.read_bandwidth = read_bandwidth
        self.write_bandwidth = write_bandwidth
        self.read_iops = read_iops
        self.write_iops = write_iops

    def ready(self):
        return self.read_bandwidth is not None      \
            and self.write_bandwidth is not None    \
            and self.read_iops is not None          \
            and self.write_iops is not None


PPM = [500, 600, 700, 800, 900, 950, 990, 999, 1000]


def calc_ppms(values, sort_key, x2list):
    values.sort(key=sort_key)

    ppms = []
    for q in PPM:
        if len(values):
            p = values[min(int(q * len(values) / 1000.), len(values) - 1)]
        else:
            p = Stat(0, 0, 0, 0)

        ppms.append([q] + x2list(p))

    return ppms


def fetch_usage_stats(cluster, volume, interval):
    sensors = solomon.fetch(solomon.api_url(cluster, "server_volume", interval, [
        ("volume", volume),
        ("host", "cluster"),
        ("request", "ReadBlocks|WriteBlocks|ZeroBlocks"),
        ("sensor", "RequestBytes|Count"),
    ]))
    logging.debug(sensors)

    return sensors


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='server', default='localhost')
    parser.add_argument('--volume-info-file', help='volume info file', required=True)
    parser.add_argument('--volume-stats-file', help='volume info stats file', required=True)
    parser.add_argument('--disk-ids-file', help='disk id list')
    parser.add_argument('--grpc-port', help='nbs grpc port', default=9766)
    parser.add_argument('--cluster', help='cloud cluster', default='yandexcloud_prod')
    parser.add_argument('--interval', help='stats interval', default='24h')
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')
    parser.add_argument('--no-stat', action='store_true', help='don\'t gather stats')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.ERROR

    logging.basicConfig(stream=sys.stderr, level=log_level, format="[%(levelname)s] [%(asctime)s] %(message)s")
    client = CreateClient('{}:{}'.format(args.host, args.grpc_port), log=logging)

    # gathering volume configs

    volume_info_file = args.volume_info_file

    if os.path.exists(volume_info_file):
        assert os.path.isfile(volume_info_file)

        with open(volume_info_file) as f:
            volume2info = json.load(f)
    else:
        disk_ids = []

        if args.disk_ids_file is None:
            disk_ids = client.list_volumes()
        else:
            with open(args.disk_ids_file) as f:
                for line in f.readlines():
                    disk_ids.append(line.rstrip())

        volume2info = fetch_volume2info(client, logging, disk_ids)
        with open(volume_info_file, "w") as f:
            json.dump(volume2info, f, indent=4)

    if not args.no_stat:
        # gathering volume usage stats

        volume_stats_file = args.volume_stats_file

        if os.path.exists(volume_stats_file):
            assert os.path.isfile(volume_stats_file)

            with open(volume_stats_file) as f:
                volume2stats = json.load(f)
        else:
            volume2stats = dict()
            for volume in volume2info:
                name = volume["Name"]
                volume2stats[name] = fetch_usage_stats(args.cluster, name, args.interval)
            with open(volume_stats_file, "w") as f:
                json.dump(volume2stats, f, indent=4)

    # compiling

    result = {}

    for volume in volume2info:
        volume_usage_stat = {}
        volume_usage_stat["VolumeConfig"] = volume["VolumeConfig"]

        if not args.no_stat:
            stats = volume2stats.get(volume["Name"])

            ts2stats = {}
            for stat in stats:
                sensor_name = stat["labels"]["sensor"]
                request_type = stat["labels"]["request"]
                for val in stat["values"]:
                    ts = val["ts"]
                    if ts not in ts2stats:
                        ts2stats[ts] = Stat()
                    v = val["value"]
                    if sensor_name == "Count":
                        if request_type == "ReadBlocks":
                            ts2stats[ts].read_iops = v
                        else:
                            assert request_type in ["WriteBlocks", "ZeroBlocks"]
                            ts2stats[ts].write_iops = v
                    else:
                        assert sensor_name == "RequestBytes"
                        if request_type == "ReadBlocks":
                            ts2stats[ts].read_bandwidth = v
                        else:
                            assert request_type in ["WriteBlocks", "ZeroBlocks"]
                            ts2stats[ts].write_bandwidth = v

            stat_list = []
            for stat in ts2stats.itervalues():
                if stat.ready():
                    stat_list.append(stat)

            def x2read_stat(x):
                return [x.read_bandwidth, x.read_iops]
            volume_usage_stat["R-stat"] = {
                "B-stat": calc_ppms(stat_list, lambda x: x.read_bandwidth, x2read_stat),
                "I-stat": calc_ppms(stat_list, lambda x: x.read_iops, x2read_stat),
            }

            def x2write_stat(x):
                return [x.write_bandwidth, x.write_iops]
            volume_usage_stat["W-stat"] = {
                "B-stat": calc_ppms(stat_list, lambda x: x.write_bandwidth, x2write_stat),
                "I-stat": calc_ppms(stat_list, lambda x: x.write_iops, x2write_stat),
            }

        result[volume["Name"]] = volume_usage_stat

    json.dump(result, sys.stdout, indent=4)


if __name__ == '__main__':
    sys.exit(main())
