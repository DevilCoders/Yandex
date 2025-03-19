import logging
import progressbar

import cloud.blockstore.public.sdk.python.protos as protos
from cloud.blockstore.public.sdk.python.client import CreateClient


class CloudStats(object):

    def __init__(self):
        self.used_bytes_count = 0
        self.bytes_count = 0


if __name__ == '__main__':
    client = CreateClient('localhost:9766')

    volume_ids = client.list_volumes()

    bar = progressbar.ProgressBar(max_value=len(volume_ids))
    bar.start()

    clouds = {}
    completed = 0
    total_used_bytes_count = 0
    total_bytes_count = 0

    for volume_id in volume_ids:
        try:
            r = client.stat_volume(volume_id)

            s = r["Stats"]
            v = r["Volume"]

            if v.StorageMediaKind == protos.EStorageMediaKind.Value("STORAGE_MEDIA_SSD"):
                c = clouds.setdefault(v.CloudId, CloudStats())

                used_bytes_count = s.UsedBlocksCount * 4096
                bytes_count = v.BlocksCount * 4096

                total_used_bytes_count += used_bytes_count
                total_bytes_count += bytes_count

                c.used_bytes_count += used_bytes_count
                c.bytes_count += bytes_count

        except Exception as e:
            logging.error("volume: {}, error: {}".format(volume_id, e))

        completed += 1
        bar.update(completed)

    bar.update(completed)

    def key(x):
        return x[1].used_bytes_count

    print("{:20}\t{:16}\t{:16}\t{:16}\t{:16}".format('Cloud Id', 'Used Bytes', 'Bytes', '% Total Used', '% Total Bytes'))
    for cloud_id, c in sorted(clouds.items(), key=key, reverse=True):
        print("{:20}\t{:16}\t{:16}\t{:16.4f}\t{:16.4f}".format(
            cloud_id,
            c.used_bytes_count,
            c.bytes_count,
            c.used_bytes_count * 100.0 / total_used_bytes_count,
            c.bytes_count * 100.0 / total_bytes_count))
