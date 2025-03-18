# -*- coding: utf-8 -*-

import datetime

import sample_metrics.online.metrics_ut as mut
import pytlib.client_yt as yt_cl
import yaqutils.time_helpers as utime

from sample_metrics.online import SPU
from session_yt.metric_yt import MetricBackendYT
from mstand_utils.yt_options_struct import YtJobOptions, ResultOptions


# use ./make_squeeze_from_test_user_sessions.sh to recreate test squeeze data

def calc_metric_yt(metric, tmp_dir, expected_file, expected_avg, yt_job_options):
    yt_config = yt_cl.create_config(server="hahn",
                                    verbose_operations=False,
                                    quiet=False,
                                    filter_so=True)

    yt_backend = MetricBackendYT(config=yt_config,
                                 yt_job_options=yt_job_options,
                                 result_options=ResultOptions())

    date_range = utime.DateRange(datetime.date(2016, 5, 2),
                                 datetime.date(2016, 5, 2))
    mut.calc_metric_universal(metric=metric,
                              tmp_dir=tmp_dir,
                              expected_file=expected_file,
                              expected_avg=expected_avg,
                              squeeze_path="//home/mstand/test_data/squeeze",
                              calc_backend=yt_backend,
                              dates=date_range)


# noinspection PyClassHasNoInit
class TestMetricsYT:
    metric = SPU()
    expected_avg = 2.2839196
    yt_memory_limit = 512

    def test_spu_yt(self, tmpdir):
        calc_metric_yt(metric=self.metric,
                       tmp_dir=str(tmpdir),
                       expected_file=None,
                       expected_avg=self.expected_avg,
                       yt_job_options=YtJobOptions(memory_limit=self.yt_memory_limit))

    def test_spu_yt_with_cloud(self, tmpdir):
        yt_job_options = YtJobOptions(memory_limit=self.yt_memory_limit,
                                      tentative_enable=True,
                                      tentative_sample_count=5,
                                      tentative_duration_ratio=5.0)
        calc_metric_yt(metric=self.metric,
                       tmp_dir=str(tmpdir),
                       expected_file=None,
                       expected_avg=self.expected_avg,
                       yt_job_options=yt_job_options)
