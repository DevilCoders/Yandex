import threading

import cloud.blockstore.tools.nbsapi as nbs


class BlobIdCache(object):

    def __init__(self):
        self.__cache = dict()
        self.__lock = threading.Lock()
        self.__hits = 0
        self.__reqs = 0

    def __make_key(self, blobId):
        return blobId["RawX1"] + blobId["RawX2"] + blobId["RawX3"]

    def get(self, blobId):
        key = self.__make_key(blobId)

        with self.__lock:
            self.__reqs += 1

            data = self.__cache.get(key)
            if data is not None:
                self.__hits += 1
            return data

    def add(self, blobId, data):
        key = self.__make_key(blobId)
        with self.__lock:
            self.__cache[key] = data

    @property
    def hits(self):
        with self.__lock:
            return self.__hits

    @property
    def reqs(self):
        with self.__lock:
            return self.__reqs


def find_no_data_ranges(
    client,
    disk_id,
    start_index,
    blocks_count,
    checkpoint_id='',
    cache=None,
    index_only=True,
):

    response = nbs.describe_blocks(client, disk_id, start_index, blocks_count, checkpoint_id)

    if 'BlobPieces' not in response.keys():
        return []

    ranges = []
    for p in response['BlobPieces']:
        if cache is None:
            res = nbs.check_blob(client, p['BlobId'], p['BSGroupId'], index_only)
        else:
            res = cache.get(p['BlobId'])
            if res is None:
                res = nbs.check_blob(client, p['BlobId'], p['BSGroupId'], index_only)
                cache.add(p['BlobId'], res)

        if res['Status'] == 'NODATA' or res['Status'] == 'ERROR':
            for r in p['Ranges']:
                block_index = int(r['BlockIndex']) if 'BlockIndex' in r else 0
                ranges += [(block_index, int(r['BlocksCount']))]
        elif res['Status'] != 'OK':
            raise Exception('Invalid status %s for blob %s' % (res['Status'], p))

    return ranges
