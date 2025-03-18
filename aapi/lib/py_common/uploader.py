import concurrent.futures
import consts
import os


UPLOAD_CHUNK_SIZE = 32 * 1024 * 1024
UPLOAD_THREADS_COUNT = 32
UPLOAD_RAM_LIMIT = 32 * 32 * 1024 * 1024
assert not UPLOAD_RAM_LIMIT < UPLOAD_CHUNK_SIZE * UPLOAD_THREADS_COUNT


def iter_chunks(nodes_iterable):
    chunk = []
    size = 0

    for sha, _type, path in nodes_iterable:
        chunk.append((sha, _type, path))

        if _type == consts.NODE_SVN_HEAD:
            data = path
            size += len(data)
        else:
            size += os.stat(path).st_size

        if size >= UPLOAD_CHUNK_SIZE:
            yield chunk
            chunk = []
            size = 0

    if chunk:
        yield chunk


class Uploader(object):

    def __init__(self, client, table):
        self._client = client
        self._table = table
        self._pool = concurrent.futures.ThreadPoolExecutor(max_workers=UPLOAD_THREADS_COUNT)

    def upload_chunk(self, chunk):
        rows = []
        for sha, _type, path in chunk:
            with open(path, 'rb') as f:
                data = f.read()
            rows.append({'hash': sha, 'type': _type, 'data': data})
        self._client.insert_rows(self._table, rows, update=True)

    def upload_chunk_future(self, chunk):
        return self._pool.submit(self.upload_chunk, chunk).result
