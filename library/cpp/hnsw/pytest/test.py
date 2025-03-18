import yatest.common
from yatest.common import ExecutionTimeoutError

import filecmp
import os
import timeit

HNSW_BUILD_INDEX_PATH = yatest.common.binary_path("library/cpp/hnsw/tools/build_dense_vector_index/build_dense_vector_index")

DATA_FOLDER = yatest.common.source_path("library/cpp/hnsw/pytest/data")
FLOATS_60000 = yatest.common.source_path(os.path.join(DATA_FOLDER, "floats_60000"))


def test_snapshots():
    def run_with_timeout(cmd, timeout):
        try:
            yatest.common.execute(cmd, timeout=timeout)
        except ExecutionTimeoutError:
            return True
        return False

    cmd = [
        HNSW_BUILD_INDEX_PATH,
        '-v', FLOATS_60000,
        '-t', 'float',
        '-d', '10',
        '-D', 'dot_product',
        '-T', '1',
        '-m', '32',
        '-b', '1000',
        '-s', '1500',
        '-e', '500',
        '-l', '2'
    ]

    index_1 = yatest.common.test_output_path('index_1')
    cmd_1 = cmd + ['-o', index_1]
    exec_time = timeit.timeit(lambda: yatest.common.execute(cmd_1), number=1)

    index_2 = yatest.common.test_output_path('index_2')
    snapshot_file = yatest.common.test_output_path('hnsw.snapshot')
    cmd_2 = cmd + ['-o', index_2,
                   '--snapshot-file', snapshot_file,
                   '--snapshot-interval', '1']

    was_timeout = False
    while run_with_timeout(cmd_2, exec_time / 2):
        was_timeout = True
    assert was_timeout
    assert filecmp.cmp(index_1, index_2)
