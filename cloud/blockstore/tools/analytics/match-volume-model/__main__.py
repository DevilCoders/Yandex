import argparse
import json
import logging
import sys

import cloud.blockstore.tools.nbsapi as nbs
import cloud.blockstore.tools.kikimrapi as kikimr

from cloud.blockstore.public.sdk.python.client import CreateClient


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


def fetch_channel_count(host, port, hive_tablet_id, partitions):
    channel_count = 0

    for partition in partitions:
        groups_and_channels = kikimr.fetch_groups(
            host,
            port,
            hive_tablet_id,
            int(partition["TabletId"])
        )

        channels = set()
        for channel, _ in groups_and_channels:
            channels.add(channel)

        if len(channels) > 3:
            channel_count += len(channels) - 3
        else:
            logging.error("bad channel count for tablet %s: %s" % (partition["TabletId"], len(channels)))
            return None

    return channel_count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='server', default='localhost')
    parser.add_argument('--nbs-port', help='nbs grpc port', default=9766)
    parser.add_argument('--monport', help='kikimr monitoring port', default=8765)
    parser.add_argument('--hive-tablet-id', help='hive tablet id (bs group info is fetched from hive)', default='72057594037968897')
    parser.add_argument('--volume-info', help='volume info file')
    parser.add_argument('--channel-info', help='channel info file')
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.ERROR

    logging.basicConfig(stream=sys.stderr, level=log_level, format="[%(levelname)s] [%(asctime)s] %(message)s")

    # gathering volume configs

    if args.volume_info is not None:
        with open(args.volume_info) as f:
            volumes = json.load(f)
    else:
        client = CreateClient('{}:{}'.format(args.host, args.nbs_port), log=logging)
        volumes = get_volume_descriptions(client, logging)
        with open("volumeinfo.json", "w") as f:
            json.dump(volumes, f, indent=4)

    volumes.sort(key=lambda x: x["Name"])

    # gathering real channel info

    if args.channel_info is not None:
        with open(args.channel_info) as f:
            volume_channels = json.load(f)
    else:
        volume_channels = []
        for volume in volumes:
            channel_count = fetch_channel_count(
                args.host,
                args.monport,
                args.hive_tablet_id,
                volume["Partitions"]
            )

            if channel_count is not None:
                volume_channels.append((volume["Name"], channel_count))

        with open("channelinfo.json", "w") as f:
            json.dump(volume_channels, f, indent=4)

    volume_channels.sort(key=lambda x: x[0])

    # matching

    i = 0
    j = 0

    unmatched_count = 0
    wrong_ecps_count = 0
    unset_ecps_count = 0
    wrong_real_channels_count = 0
    small_real_channels_count = 0

    while i < len(volumes) and j < len(volume_channels):
        volume = volumes[i]
        channels = volume_channels[j]

        if volume["Name"] < channels[0]:
            i += 1
            logging.warn("unmatched volume: %s" % volume["Name"])
            unmatched_count += 1
        elif volume["Name"] > channels[0]:
            j += 1
            logging.warn("unmatched channels: %s" % channels[0])
            unmatched_count += 1
        else:
            i += 1
            j += 1

            vc = volume["VolumeConfig"]
            ecps = len(vc.get("ExplicitChannelProfiles", []))
            num_channels = vc["NumChannels"]

            if channels[1] == 0:
                # XXX prev version of this script used to cache 0 for deleted volumes
                continue

            if num_channels + 3 != ecps:
                if ecps:
                    wrong_ecps_count += 1
                else:
                    unset_ecps_count += 1
                print >> sys.stderr, "%s: ecps != num_channels + 3, %s != %s + 3" % (volume["Name"], ecps, num_channels)
            if num_channels != channels[1]:
                print >> sys.stderr, "%s: real channel count != num_channels, %s != %s" % (volume["Name"], channels[1], num_channels)
                wrong_real_channels_count += 1
                if num_channels > channels[1]:
                    print >> sys.stderr, "%s: real channel count IS LESS THAN num_channels, %s < %s" % (volume["Name"], channels[1], num_channels)
                    small_real_channels_count += 1

    if i < len(volumes):
        logging.warn("unmatched volumes: %s" % [x["Name"] for x in volumes[i:]])
        unmatched_count += len(volumes) - i
    if j < len(volume_channels):
        logging.warn("unmatched channels: %s" % [x[0] for x in volume_channels[j:]])
        unmatched_count += len(volume_channels) - j

    print "\t".join([
        str(unmatched_count),
        str(wrong_ecps_count),
        str(unset_ecps_count),
        str(wrong_real_channels_count),
        str(small_real_channels_count),
    ])


if __name__ == '__main__':
    sys.exit(main())
