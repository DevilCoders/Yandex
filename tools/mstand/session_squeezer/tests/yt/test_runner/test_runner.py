import os

from typing import Dict
from typing import List
from typing import Optional

import mstand_utils.mstand_paths_utils as mstand_upath
import yt.wrapper as yt  # noqa
import yaqutils.misc_helpers as umisc

from experiment_pool import pool_helpers
from mstand_enums.mstand_online_enums import SqueezeResultEnum, TableGroupEnum
from mstand_utils.yt_options_struct import TableBounds
from mstand_utils.yt_options_struct import YtJobOptions
from session_squeezer.squeeze_runner import SqueezeRunner
from session_yt.squeeze_yt import SqueezeBackendYT
from session_yt.squeeze_yt import YTBackendLocalFiles
from session_yt.squeeze_yt import YTBackendStatboxFiles

import yatest.common


class Runner:
    def __init__(self, client: yt.YtClient,
                 services: List[str],
                 pool_path: str,
                 path_to_row_count: Dict[str, int],
                 allow_empty_tables: bool = False,
                 use_filters: bool = False,
                 history: Optional[int] = None,
                 future: Optional[int] = None,
                 squeeze_bin_file: Optional[str] = None,
                 squeeze_path: Optional[str] = None):
        self.client = client
        self.services = services
        self.pool_path = pool_path
        self.path_to_row_count = path_to_row_count
        self.allow_empty_tables = allow_empty_tables
        self.use_filters = use_filters
        self.enable_binary = squeeze_bin_file is not None
        self.squeeze_bin_file = squeeze_bin_file
        self.history = history
        self.future = future

        if squeeze_path is None:
            self.paths_params = mstand_upath.PathsParams()
        else:
            self.paths_params = mstand_upath.PathsParams(squeeze_path=squeeze_path)

        if squeeze_bin_file is None:
            self.local_files = YTBackendLocalFiles(
                services=self.services,
                use_filters=self.use_filters,
                source_dir=yatest.common.work_path("libra_files"),
            )
        else:
            self.local_files = YTBackendStatboxFiles(
                services=self.services,
                use_filters=self.use_filters,
            )

    def __call__(self, yt_lock_enable: bool, replace: bool = False) -> SqueezeRunner:
        pool = pool_helpers.load_pool(self.pool_path)
        pool.init_services(self.services)

        squeeze_backend = SqueezeBackendYT(local_files=self.local_files,
                                           config=self.client.config,
                                           yt_job_options=YtJobOptions(memory_limit=512),
                                           table_bounds=TableBounds(),
                                           yt_lock_enable=yt_lock_enable,
                                           allow_empty_tables=self.allow_empty_tables,
                                           sort_threads=5,
                                           add_acl=False,
                                           squeeze_bin_file=self.squeeze_bin_file,
                                           )

        runner = SqueezeRunner(paths_params=self.paths_params,
                               squeeze_backend=squeeze_backend,
                               services=self.services,
                               replace=replace,
                               history=self.history,
                               future=self.future,
                               use_filters=self.use_filters,
                               enable_binary=self.enable_binary,
                               table_groups=[TableGroupEnum.CLEAN])
        new_pool = runner.squeeze_pool(pool)

        assert pool is new_pool

        for table_path, row_count in self.path_to_row_count.items():
            self.check_path(table_path, row_count)
            if yt_lock_enable:
                self.check_exists("{}.lock".format(table_path))
                self.check_empty("{}.lock/@acquired_process_url".format(table_path))

        return runner

    def check_path(self, path: str, row_count: int) -> None:
        assert self.client.exists(path), "'{}' should exist".format(path)
        actual_count = self.client.get_attribute(path, "row_count")
        assert actual_count == row_count, \
            "Wrong number of rows in {} table, {} ({} required)".format(path, actual_count, row_count)

    def check_exists(self, path: str) -> None:
        assert self.client.exists(path), "'{}' should exist".format(path)

    def check_empty(self, path: str) -> None:
        assert self.client.exists(path), "'{}' should exist".format(path)
        assert self.client.get(path) == "", "File '{}' is supposed to be empty".format(path)


def run_test(client: yt.YtClient,
             services: List[str],
             pool_path: str,
             path_to_row_count: Dict[str, int],
             allow_empty_tables: bool = False,
             use_filters: bool = False,
             history: Optional[int] = None,
             future: Optional[int] = None,
             squeeze_bin_file: Optional[str] = None,
             squeeze_path: Optional[str] = None) -> None:
    upload_libra_files(client)

    run = Runner(
        client=client,
        services=services,
        pool_path=pool_path,
        path_to_row_count=path_to_row_count,
        allow_empty_tables=allow_empty_tables,
        use_filters=use_filters,
        history=history,
        future=future,
        squeeze_bin_file=squeeze_bin_file,
        squeeze_path=squeeze_path,
    )

    results_iter = umisc.par_imap_unordered(run, [False, True, True], max_threads=3, dummy=True)
    results = set()
    for runner in results_iter:
        results |= set(runner.squeeze_results.values())
    assert results == {SqueezeResultEnum.SKIPPED_AFTER_LOCK, SqueezeResultEnum.SQUEEZED}

    runner = run(yt_lock_enable=False, replace=True)
    assert set(runner.squeeze_results.values()) == {SqueezeResultEnum.SQUEEZED}

    runner = run(yt_lock_enable=False, replace=False)
    assert set(runner.squeeze_results.values()) == {SqueezeResultEnum.SKIPPED}


def get_pool_path(tests_path: str, filename: str = "pool.json") -> str:
    test_folder = os.path.basename(os.path.dirname(os.path.realpath(tests_path)))
    return yatest.common.source_path("tools/mstand/session_squeezer/tests/yt/{}/data/{}".format(test_folder, filename))


def upload_libra_files(client: yt.YtClient):
    libra_path = yatest.common.work_path("libra_files")
    resources_path = "//home/mstand/resources"
    client.create("map_node", resources_path, recursive=True, ignore_existing=True)

    for filename in os.listdir(libra_path):
        dst_path = os.path.join(resources_path, filename)
        if not client.exists(dst_path):
            client.write_file(
                destination=dst_path,
                stream=open(os.path.join(libra_path, filename), "rb"),
            )
