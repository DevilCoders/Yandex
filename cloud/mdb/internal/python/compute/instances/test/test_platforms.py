import pytest

from cloud.mdb.internal.python.compute.instances.platforms import (
    nvme_chunk_size,
    UnsupportedPlatform,
    WellKnownPlatforms,
)


class TestPlatform:
    def test_unknown(self):
        with pytest.raises(UnsupportedPlatform):
            nvme_chunk_size({}, 'unknwon')

    def test_not_in_map(self):
        with pytest.raises(UnsupportedPlatform) as exc:
            nvme_chunk_size({WellKnownPlatforms.MDB_V1: 1}, "mdb-v2")
        assert str(exc.value) == '"mdb-v2" is not configured (available: mdb-v1)'

    def test_happy_path(self):
        assert 1 == nvme_chunk_size({WellKnownPlatforms.MDB_V2: 1}, "mdb-v2")
