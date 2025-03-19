import argparse
import concurrent.futures
import json
import logging
import operator
import progressbar

from tabulate import tabulate

from cloud.blockstore.tools.analytics.common import http_get
from cloud.blockstore.public.sdk.python.client import CreateClient


def _get_service_sensor(cluster, volume_id, sensor_name, since, until):
    url = "http://solomon.yandex.net/data-api/get?who=xxx" \
          "&l.project=nbs&l.cluster={cluster}" \
          "&l.host=cluster&l.service=service_volume" \
          "&l.sensor={sensor_name}&l.volume={volume_id}" \
          "&b={since}&e={until}"\
        .format(
            cluster=cluster,
            volume_id=volume_id,
            sensor_name=sensor_name,
            since=since,
            until=until
        )
    sensors = json.loads(http_get(url))["sensors"]
    if not len(sensors):
        return 0, 0

    values = sensors[0]["values"]
    if not len(values):
        return 0, 0

    return int(values[0]["value"]), int(values[-1]["value"])


def _get_mixed_plus_merged_count(cluster, volume_id, since, until):
    return tuple(map(
        operator.add,
        _get_service_sensor(cluster, volume_id, "MixedBytesCount", since, until),
        _get_service_sensor(cluster, volume_id, "MergedBytesCount", since, until)
    ))


def _get_used_count(cluster, volume_id, since, until):
    return _get_service_sensor(cluster, volume_id, "UsedBytesCount", since, until)


def _get_size(cluster, volume_id, since, until):
    return _get_service_sensor(cluster, volume_id, "BytesCount", since, until)


def _analyze_volume_voracity(volume_id, media_kind, client, cluster, since, until):
    volume = client.describe_volume(volume_id)

    if media_kind is not None and volume.StorageMediaKind != media_kind:
        return {
            "mm": [volume.DiskId, volume.CloudId, 0, 0, 0],
            "ubc": [volume.DiskId, volume.CloudId, 0, 0, 0],
            "sz": [volume.DiskId, volume.CloudId, 0, 0, 0],
            "storage_overhead": [volume.DiskId, volume.CloudId, 0, 0, 0],
        }

    mmc_old, mmc_new = _get_mixed_plus_merged_count(cluster, volume_id, since, until)
    mmc_delta = mmc_new - mmc_old

    ubc_old, ubc_new = _get_used_count(cluster, volume_id, since, until)
    ubc_delta = ubc_new - ubc_old

    size_old, size_new = _get_size(cluster, volume_id, since, until)
    size_delta = size_new - size_old

    return {
        "mm": [volume.DiskId, volume.CloudId, mmc_old, mmc_new, mmc_delta],
        "ubc": [volume.DiskId, volume.CloudId, ubc_old, ubc_new, ubc_delta],
        "sz": [volume.DiskId, volume.CloudId, size_old, size_new, size_delta],
        "storage_overhead": [volume.DiskId, volume.CloudId, size_new, mmc_new, mmc_new / float(max(size_new, 1))],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--endpoint", help="endpoint", default="localhost:9766")
    parser.add_argument("--cluster", help="cluster", default="yandexcloud_preprod")
    parser.add_argument("--media-kind", help="media kind", type=int)
    parser.add_argument("--since", help="YYYY-MM-DDTHH:MM:SSZ", required=True)
    parser.add_argument("--until", help="YYYY-MM-DDTHH:MM:SSZ", required=True)
    parser.add_argument("--batch", default=64, type=int)
    parser.add_argument("--top", default=100, type=int)

    args = parser.parse_args()

    client = CreateClient(args.endpoint)
    volume_ids = client.list_volumes()

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.batch) as executor:
        bar = progressbar.ProgressBar(max_value=2 * len(volume_ids))
        bar.start()

        completed = 0
        future2volume_id = {}

        for volume_id in volume_ids:
            future = executor.submit(
                _analyze_volume_voracity,
                volume_id,
                args.media_kind,
                client,
                args.cluster,
                args.since,
                args.until
            )
            future2volume_id[future] = volume_id

            completed += 1
            bar.update(completed)

        mm_results = []
        ubc_results = []
        sz_results = []
        storage_overhead_results = []

        for future in concurrent.futures.as_completed(future2volume_id):
            volume_id = future2volume_id[future]
            try:
                result = future.result()
                mm_results.append(result["mm"])
                ubc_results.append(result["ubc"])
                sz_results.append(result["sz"])
                storage_overhead_results.append(result["storage_overhead"])
            except Exception as e:
                logging.error("volume: %s, error: %s" % (volume_id, e))

            completed += 1
            bar.update(completed)

        bar.finish()

        mm_results.sort(key=operator.itemgetter(4), reverse=True)
        ubc_results.sort(key=operator.itemgetter(4), reverse=True)
        sz_results.sort(key=operator.itemgetter(4), reverse=True)
        storage_overhead_results.sort(key=operator.itemgetter(4), reverse=True)

        print(tabulate(mm_results[:args.top],
                       headers=[
                           "disk_id",
                           "cloud_id",
                           "mixed+merged_old",
                           "mixed+merged_new",
                           "mixed+merged_delta"],
                       tablefmt="psql")
              )
        print("\n")
        print(tabulate(ubc_results[:args.top],
                       headers=[
                           "disk_id",
                           "cloud_id",
                           "used_bytes_count_old",
                           "used_bytes_count_new",
                           "used_bytes_count_delta"],
                       tablefmt="psql")
              )
        print("\n")
        print(tabulate(sz_results[:args.top],
                       headers=[
                           "disk_id",
                           "cloud_id",
                           "bytes_count_old",
                           "bytes_count_new",
                           "bytes_count_delta"],
                       tablefmt="psql")
              )
        print("\n")
        print(tabulate(storage_overhead_results[:args.top],
                       headers=[
                           "disk_id",
                           "cloud_id",
                           "size",
                           "mixed+merged",
                           "mixed+merged/size"],
                       tablefmt="psql")
              )
