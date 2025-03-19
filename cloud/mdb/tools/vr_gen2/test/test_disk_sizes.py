from cloud.mdb.tools.vr_gen2.internal.disk_size import _range_for_disk_sizes
from cloud.mdb.tools.vr_gen2.internal.models.base import DiskRange


def test_disk_size_range():
    assert list(_range_for_disk_sizes(DiskRange(min='1B', upto='5B', step='2B'))) == [1, 3]
    assert list(_range_for_disk_sizes(DiskRange(min='1B', upto='6B', step='2B'))) == [1, 3, 5, 6]
    assert list(_range_for_disk_sizes(DiskRange(min='2B', upto='5B', step='2B'))) == [2, 4]
    assert list(_range_for_disk_sizes(DiskRange(min='2B', upto='6B', step='2B'))) == [2, 4, 6]
