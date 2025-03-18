# coding=utf-8
import datetime
import logging
import os
import pytest

import session_metric.metric_quick_check as mqc
import yaqutils.file_helpers as ufile
import yaqutils.test_helpers as utest
import yaqutils.time_helpers as utime

from experiment_pool import Experiment
from experiment_pool import Observation
from experiment_pool import Pool
from sample_metrics.online import ClicksWithoutRequests
from sample_metrics.online import SPU
from sample_metrics.online import SSPU
from sample_metrics.online import TotalNumberOfClicks
from sample_metrics.online import TotalNumberOfRequests
from sample_metrics.online import WebAbandonment
from sample_metrics.online import WebNoTop3Clicks
from sample_metrics.online.online_test_metrics import NoNumericValues
from sample_metrics.online.online_test_metrics import NoValues
from session_local import MetricBackendLocal
from session_metric import MetricRunner
from mstand_enums.mstand_online_enums import ServiceEnum
from .test_metrics_for_services import MetricRequiredServices, MetricMoreRequiredServices
from .test_metrics_for_versions import MetricMinVersions, MetricStrictMinVersions
from user_plugins import PluginContainer
from user_plugins import PluginKey


def calc_metric_local(metric, tmp_dir, base_sample_dir, expected_file):
    """
    :type metric:
    :type tmp_dir: str
    :type base_sample_dir: str
    :param expected_file:
    :return:
    """
    local_backend = MetricBackendLocal()
    dates = utime.DateRange(datetime.date(2016, 5, 2), datetime.date(2016, 5, 2))
    calc_metric_universal(metric=metric,
                          tmp_dir=tmp_dir,
                          base_sample_dir=base_sample_dir,
                          expected_file=expected_file,
                          expected_avg=None,
                          squeeze_path=base_sample_dir,
                          calc_backend=local_backend,
                          dates=dates)


# noinspection PyShadowingNames
def calc_metric_universal(metric, tmp_dir, base_sample_dir, expected_file, expected_avg, squeeze_path, calc_backend, dates):
    """
    :type metric: callable
    :type tmp_dir: str
    :type base_sample_dir: str
    :type expected_file: str | None
    :type expected_avg: float | None
    :type squeeze_path: str
    :type calc_backend:
    :type dates: utime.DateRange
    :rtype: None
    """

    services = ["web"]
    control = Experiment(testid="all")
    experiment = Experiment(testid="all")
    observation = Observation(obs_id=None, dates=dates,
                              control=control, experiments=[experiment],
                              services=services)
    pool = Pool([observation])

    # derived from session_calc_metric_local:

    metric_name = metric.__class__.__name__
    metric_key = PluginKey(metric_name)
    metric_container = PluginContainer.create_single_direct(plugin_key=metric_key, plugin_instance=metric)

    metric_runner = MetricRunner(
        metric_container=metric_container,
        squeeze_path=squeeze_path,
        calc_backend=calc_backend,
    )

    dest_dir = os.path.join(tmp_dir, "online_metric_ut_results")
    ufile.make_dirs(dest_dir)

    tar_name = os.path.join(tmp_dir, "{}.tgz".format(metric_name))
    results = metric_runner.calc_for_pool(
        pool=pool,
        save_to_dir=dest_dir,
        save_to_tar=tar_name,
    )
    one_exp_results = list(results.values())[0]
    first_metric_result = one_exp_results[0]
    first_vals = first_metric_result.metric_values
    data_file = first_vals.data_file

    logging.info("metric %s average value: %s", metric_name, first_vals.average_val)

    if expected_avg is not None:
        precision = 5
        avg = first_vals.average_val
        if round(avg, precision) != round(expected_avg, precision):
            raise Exception("Metric {} avg mismatch: expected {}, got {}".format(metric_name, expected_avg, avg))
    else:
        logging.info("No expected average specified for metric %s", metric_name)

    if expected_file:
        data_file_full_path = os.path.join(dest_dir, data_file)
        expected_full_path = os.path.join(base_sample_dir, expected_file)
        utest.fuzzy_diff_tsv_files(data_file_full_path, expected_full_path)
    else:
        logging.info("No expected result checking for metric %s", metric_name)

    logging.info("testing calculation without dump")

    # drop existing metric results
    pool.clear_metric_results()
    # and run same calculation without dumping
    metric_runner.calc_for_pool(pool=pool)


# noinspection PyShadowingNames
def check_and_calc_metric_local(metric, tmpdir, base_sample_dir, expected_file=None):
    """
    :type metric:
    :type tmpdir: pytest.LocalPath
    :type base_sample_dir: str
    :type expected_file: str
    :rtype:
    """

    # quick test
    logging.info("quick metric check")
    mqc.check_metric_quick(metric, [ServiceEnum.WEB_DESKTOP])

    # full test
    logging.info("full metric check")
    calc_metric_local(metric, str(tmpdir), base_sample_dir, expected_file=expected_file)


# noinspection PyClassHasNoInit
class TestMetricsSingle:
    # noinspection PyShadowingNames
    def test_clicks_without_requests(self, tmpdir, data_path):
        expected_file = "clicks_without_requests.tsv"
        check_and_calc_metric_local(ClicksWithoutRequests(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_spu_single(self, tmpdir, data_path):
        expected_file = "spu.tsv"
        check_and_calc_metric_local(SPU(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_sspu_single(self, tmpdir, data_path):
        expected_file = "sspu.tsv"
        check_and_calc_metric_local(SSPU(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_total_number_of_clicks_single(self, tmpdir, data_path):
        expected_file = "total_number_or_clicks.tsv"
        check_and_calc_metric_local(TotalNumberOfClicks(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_total_number_of_requests(self, tmpdir, data_path):
        expected_file = "total_number_or_requests.tsv"
        check_and_calc_metric_local(TotalNumberOfRequests(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_web_abandonment(self, tmpdir, data_path):
        expected_file = "web_abandonment.tsv"
        check_and_calc_metric_local(WebAbandonment(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_web_no_top3_clicks(self, tmpdir, data_path):
        expected_file = "web_no_top3_clicks.tsv"
        check_and_calc_metric_local(WebNoTop3Clicks(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_no_numeric_values(self, tmpdir, data_path):
        expected_file = "no_numeric_values.tsv"
        check_and_calc_metric_local(NoNumericValues(), tmpdir, data_path, expected_file)

    # noinspection PyShadowingNames
    def test_no_values(self, tmpdir, data_path):
        expected_file = "no_values.tsv"
        check_and_calc_metric_local(NoValues(), tmpdir, data_path, expected_file)

    def test_required_services(self, tmpdir, data_path):
        expected_file = "all_ones.tsv"
        check_and_calc_metric_local(MetricRequiredServices(), tmpdir, data_path, expected_file)

    def test_required_services_must_fail(self, tmpdir, data_path):
        expected_file = "all_ones.tsv"
        with pytest.raises(Exception):
            check_and_calc_metric_local(MetricMoreRequiredServices(), tmpdir, data_path, expected_file)

    def test_min_versions(self, tmpdir, data_path):
        expected_file = "all_ones.tsv"
        check_and_calc_metric_local(MetricMinVersions(), tmpdir, data_path, expected_file)

    def test_min_versions_must_fail(self, tmpdir, data_path):
        expected_file = "all_ones.tsv"
        with pytest.raises(Exception):
            check_and_calc_metric_local(MetricStrictMinVersions(), tmpdir, data_path, expected_file)
