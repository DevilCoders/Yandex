import operator

import numpy as np

from antiadblock.tasks.bypass_uids.lib.util import save_uids_to_file
from antiadblock.tasks.bypass_uids.lib.data import Columns


UIDS = range(1000000000, 2000000000, 1000)


def test_memmap(tmpdir):
    expected_array = np.array(UIDS, dtype=np.uint64)
    f = tmpdir.join("uids_file")
    save_uids_to_file(
        str(f),
        ({Columns.uniqid.name: u} for u in UIDS),
        operator.itemgetter(Columns.uniqid.name),
    )
    memmaped_array = np.memmap(str(f), dtype=np.uint64, mode='r')
    assert np.array_equal(expected_array, memmaped_array)
