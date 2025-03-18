import os

import mstand_utils.mstand_paths_utils as mstand_upath
import yaqutils.test_helpers as utest

from experiment_pool import pool_helpers

from session_local import SqueezeBackendLocal
from session_squeezer.squeeze_runner import SqueezeRunner


# noinspection PyClassHasNoInit
class TestSqueezeRunner:
    def test_squeeze_pool(self, tmpdir):
        services = ["web"]
        pool = pool_helpers.load_pool(utest.get_source_path("session_squeezer/tests/fat/data/samples_long/pool.json"))
        pool.init_services(services)
        new_pool = SqueezeRunner(
            paths_params=mstand_upath.PathsParams(
                user_sessions_path=utest.get_source_path("session_squeezer/tests/fat/data/samples_long"),
                yuids_path="",
                squeeze_path=str(tmpdir),
            ),
            squeeze_backend=SqueezeBackendLocal(),
            services=services,
            threads=1,
        ).squeeze_pool(pool)

        assert pool is new_pool

        path_30104 = str(tmpdir.join("testids/web/30104/20160827"))

        assert os.path.exists(path_30104)
        assert len(open(path_30104).readlines()) == 2

        path_30105 = str(tmpdir.join("testids/web/30105/20160827"))

        assert os.path.exists(path_30105)
        assert len(open(path_30105).readlines()) == 1

    def test_squeeze_pool_watchlog(self, tmpdir):
        services = ["watchlog"]
        pool = pool_helpers.load_pool(utest.get_source_path("session_squeezer/tests/fat/data/samples_long/pool.json"))
        pool.init_services(services)
        new_pool = SqueezeRunner(
            paths_params=mstand_upath.PathsParams(
                user_sessions_path=utest.get_source_path("session_squeezer/tests/fat/data/samples_long"),
                yuids_path=utest.get_source_path("session_squeezer/tests/fat/data/samples_long/yuid_testids"),
                squeeze_path=str(tmpdir),
            ),
            squeeze_backend=SqueezeBackendLocal(),
            services=services,
            threads=1,
        ).squeeze_pool(pool)

        assert pool is new_pool


        path_30104_watchlog = str(tmpdir.join("testids/watchlog/30104/20160827"))
        assert os.path.exists(path_30104_watchlog)
        assert len(open(path_30104_watchlog).readlines()) == 4


        path_30105_watchlog = str(tmpdir.join("testids/watchlog/30105/20160827"))
        assert os.path.exists(path_30105_watchlog)
        assert len(open(path_30105_watchlog).readlines()) == 1
