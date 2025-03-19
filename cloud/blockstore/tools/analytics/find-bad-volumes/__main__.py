import argparse
import concurrent.futures
import json
import logging
import requests
import sys

import progressbar

from cloud.blockstore.public.sdk.python.client import CreateClient
from operator import itemgetter
from tabulate import tabulate

import cloud.blockstore.tools.nbsapi as nbs


def fetch(url):
    r = requests.get(url)
    r.raise_for_status()
    return r.content


def get_tablet_info(mon_port):
    url = "http://localhost:{}/viewer/json/tabletinfo".format(mon_port)
    response = fetch(url)
    return json.loads(response)["TabletStateInfo"]


def get_node_info(mon_port):
    url = "http://localhost:{}/viewer/json/nodelist".format(mon_port)
    response = fetch(url)
    return json.loads(response)


def get_sensors_from_solomon(volume, cluster, since, until=None):
    url = "http://solomon.yandex.net/data-api/get?who=xxx" \
          "&l.project=nbs&l.cluster={cluster}" \
          "&l.host=cluster&l.service=server_volume" \
          "&l.sensor=Errors&l.volume={volume}&b={since}" \
        .format(
            volume=volume,
            cluster=cluster,
            since=since
        )

    if until is not None:
        url += "&e=" + str(until)

    return json.loads(fetch(url))["sensors"]


tablet_to_node = dict()
node_to_host = dict()


def get_volume_info(volume, client, cluster, since, until):
    try:
        description = nbs.describe_volume(client, volume)
    except Exception as e:
        logging.exception('Got exception in nbs.describe_volume: {}'.format(str(e)))
    else:
        partition = description["Partitions"][0]["TabletId"]
        sensors = get_sensors_from_solomon(volume, cluster, since, until)
        errors = 0
        for sensor in sensors:
            for value in sensor["values"]:
                errors += value["value"]

        try:
            node = tablet_to_node[partition]
            host = node_to_host[node]
        except Exception as e:
            logging.error(e)
            host = None

        return [volume, partition, host, errors]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='nbs endpoint', default='localhost')
    parser.add_argument('--mon-port', help='kikimr monitoring port', default=8765)
    parser.add_argument('--nbs-port', help='nbs grpc port', default=9766)
    parser.add_argument('--cluster', help='cluster from solomon', default='yandexcloud_prod')
    parser.add_argument('--since', help="YYYY-MM-DDTHH:MM:SSZ", required=True)
    parser.add_argument('--until', help="YYYY-MM-DDTHH:MM:SSZ", default=None)
    parser.add_argument('--head', type=int, help='first N bad volumes', default=10)
    parser.add_argument('-p', '--parallel', help='number of parallel requests', default=1)

    args = parser.parse_args()

    volumes = nbs.list_volumes(args.host, args.nbs_port)

    global tablet_to_node
    global node_to_host

    for info in get_tablet_info(args.mon_port):
        tablet_to_node[info["TabletId"]] = info["NodeId"]

    for info in get_node_info(args.mon_port):
        node_to_host[info["Id"]] = info["Host"]

    volumes_info = []

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.parallel) as executor:
        completed = 0

        bar = progressbar.ProgressBar(max_value=len(volumes))
        bar.start()

        future_to_volume = dict()

        client = CreateClient('{}:{}'.format(args.host, args.nbs_port), log=logging)

        for volume in volumes:
            future_to_volume[
                executor.submit(
                    get_volume_info,
                    volume,
                    client,
                    args.cluster,
                    args.since,
                    args.until
                )
            ] = volume

            completed += 1
            bar.update(completed)

        bar.finish()

        for future in concurrent.futures.as_completed(future_to_volume):
            volume = future_to_volume[future]

            try:
                volumes_info.append(future.result())
            except Exception as e:
                logging.warn("Skipped volume: %s, e: %s" % (volume, e))

    volumes_info.sort(key=itemgetter(3), reverse=True)

    print(tabulate(volumes_info[:args.head], headers=['VolumeId', 'TabletId', 'Host', 'Errors'], tablefmt="psql"))


if __name__ == '__main__':
    sys.exit(main())
