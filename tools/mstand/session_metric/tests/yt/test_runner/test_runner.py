import os

import yaqutils.json_helpers as ujson

from experiment_pool import pool_helpers

from session_metric.metric_runner import MetricRunner
from mstand_utils.yt_options_struct import YtJobOptions
from mstand_utils.yt_options_struct import ResultOptions
from user_plugins import PluginContainer, PluginBatch
from session_yt.metric_yt import MetricBackendYT

import yatest.common


class Runner:
    def __init__(self, client):
        self.client = client

    def __call__(self, batch_file, pool_file, services=None, history=None, future=None,
                 yt_job_options=None, result_options=None, use_buckets=False, split_metric_results=False):
        if services is None:
            services = []
        if yt_job_options is None:
            yt_job_options = YtJobOptions(memory_limit=512)
        if result_options is None:
            result_options = ResultOptions()
        batch_path = yatest.common.test_source_path(os.path.join("data", batch_file))
        pool_path = yatest.common.test_source_path(os.path.join("data", pool_file))
        pool = pool_helpers.load_pool(pool_path)
        pool.init_services(services)
        backend = MetricBackendYT(config=self.client.config,
                                  yt_job_options=yt_job_options,
                                  result_options=result_options,
                                  download_threads=5,
                                  split_metric_results=split_metric_results,
                                  add_acl=False)
        batch = PluginBatch.deserialize(ujson.load_from_file(batch_path))
        metric_cont = PluginContainer(plugin_batch=batch)
        runner = MetricRunner(metric_container=metric_cont,
                              squeeze_path="//home/mstand/squeeze",
                              calc_backend=backend,
                              use_buckets=use_buckets,
                              history=history,
                              future=future)
        results = runner.calc_for_pool(pool, save_to_tar=yatest.common.work_path("tar_result.tgz"),
                                       threads=5, batch_min=3, batch_max=10)
        return results

    def check_path(self, path, row_count=None):
        assert self.client.exists(path), "'{}' should exist".format(path)
        if row_count:
            actual_count = self.client.get_attribute(path, "row_count")
            assert actual_count == row_count, \
                "Wrong number of rows, {} ({} required)".format(actual_count, row_count)

    def check_exists(self, path):
        assert self.client.exists(path), "'{}' should exist".format(path)

    def check_empty(self, path):
        assert self.client.exists(path), "'{}' should exist".format(path)
        assert self.client.get(path) == "", "File '{}' is supposed to be empty".format(path)

    def check_result_archive(self):
        path = yatest.common.work_path("tar_result.tgz")
        assert os.path.exists(path), "lacks result archive '{}'".format(path)

    def check_result_dir(self):
        path = yatest.common.work_path("tar_result.tgz.content")
        assert os.path.exists(path), "lacks result dir '{}'".format(path)

    def get_result_dir(self):
        return yatest.common.work_path("tar_result.tgz.content")
