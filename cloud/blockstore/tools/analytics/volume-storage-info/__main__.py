import argparse
import json
import logging
import re
import requests
import sys

import cloud.blockstore.tools.nbsapi as nbs

from cloud.blockstore.public.sdk.python.client import CreateClient


CELL_PATTERN = re.compile(r"<td>(.*?)</td>")


def fetch(url):
    logging.info("fetching %s" % url)
    r = requests.get(url)
    r.raise_for_status()
    return r.content


def get_volume_descriptions(client, logging):
    result = []

    volumes = client.list_volumes()

    for volume in volumes:
        try:
            description = nbs.describe_volume(client, volume)
        except Exception as e:
            logging.exception(
                'Got exception in nbs.describe_volume: {}'.format(str(e))
            )
        else:
            result.append(description)

    return result


def build_channel_info_from_config(volume):
    channels = []

    channel = 0
    for ecp in volume["VolumeConfig"]["ExplicitChannelProfiles"]:
        channels.append({
            "Channel": channel,
            "PoolKind": ecp["PoolKind"],
            "DataKind": ecp["DataKind"],
        })

        channel += 1

    return channels


def grep(text, pattern):
    m = pattern.search(text)
    if m is None:
        return None
    return m.group(1)


# TODO
# rewrite this using nbs private api
# see NBS-748
def fetch_channel_info(host, port, partitions):
    channels = []

    for partition in partitions:
        url = "http://{}:{}/tablets/app?TabletID={}#Channels".format(
            host,
            port,
            partition["TabletId"]
        )

        text = fetch(url)
        offset = 0
        while offset < len(text):
            m = CELL_PATTERN.search(text, offset)
            if m is None:
                break

            cell = m.group(1)
            offset = m.end()
            parts = cell.split(": ")

            if len(parts) == 2 and parts[0] == "Channel":
                channel = int(parts[1])
                m = CELL_PATTERN.search(text, offset)
                offset = m.end()
                m = CELL_PATTERN.search(text, offset)
                offset = m.end()
                group = int(m.group(1).split(": ")[1])
                m = CELL_PATTERN.search(text, offset)
                offset = m.end()
                gen = int(m.group(1).split(": ")[1])
                m = CELL_PATTERN.search(text, offset)
                offset = m.end()
                parts = m.group(1).split(": ")
                if len(parts) == 1:
                    pool_kind = "unknown"
                else:
                    pool_kind = parts[1]
                m = CELL_PATTERN.search(text, offset)
                offset = m.end()
                parts = m.group(1).split(": ")
                if len(parts) == 1:
                    data_kind = "unknown"
                else:
                    data_kind = parts[1]

                channels.append({
                    "Channel": channel,
                    "Group": group,
                    "Gen": gen,
                    "PoolKind": pool_kind,
                    "DataKind": data_kind,
                })

    return channels


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='server', default='localhost')
    parser.add_argument('--port', help='nbs grpc port', default=9766)
    parser.add_argument('--monport', help='nbs monitoring port', default=8766)
    parser.add_argument('--volume-info', help='volume info file')
    parser.add_argument('--max-channels', help='channel count filter', type=int)
    parser.add_argument(
        '--use-only-volume-config',
        action='store_true',
        help='don\'t fetch real storage info - use only the config data')
    parser.add_argument(
        '-v', '--verbose', help='verbose mode', default=0, action='count')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.ERROR

    logging.basicConfig(
        stream=sys.stderr,
        level=log_level,
        format="[%(levelname)s] [%(asctime)s] %(message)s"
    )

    # gathering volume configs to build suggested performance profiles

    if args.volume_info is not None:
        with open(args.volume_info) as f:
            volumes = json.load(f)
    else:
        client = CreateClient('{}:{}'.format(
            args.host, args.port), log=logging)
        volumes = get_volume_descriptions(client, logging)
        with open("volumeinfo.json", "w") as f:
            json.dump(volumes, f, indent=4)

    # gathering volume -> bs group mapping to find out real bs group usage

    for volume in volumes:
        if args.use_only_volume_config:
            channels = build_channel_info_from_config(volume)
        else:
            channels = fetch_channel_info(
                args.host, args.monport, volume["Partitions"])

        if args.max_channels is None or len(channels) <= args.max_channels:
            print json.dumps({
                "DiskId": volume["Name"],
                "Channels": channels
            })

            if args.max_channels is not None:
                print >> sys.stderr, volume["Name"]


if __name__ == '__main__':
    sys.exit(main())
