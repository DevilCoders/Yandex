import logging
import os
import pytest

import serp.serp_compute_metric as sscm
import serp.serp_compute_metric_single_ut as sscm_ut
import yaqutils.test_helpers as utest
import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import Observation
from experiment_pool import Pool
from serp import MetricDataStorage
from serp import OfflineGlobalCtx
from serp import OfflineMetricCtx
from serp import ParsedSerpDataStorage


def make_pool_by_serpset_id(serpset_id, control):
    """
    :type serpset_id: str
    :type control: bool
    :rtype: Pool
    """
    null_dates = utime.DateRange(start=None, end=None)
    one_exp = Experiment(testid=None, serpset_id=serpset_id)
    if control:
        observation = Observation(obs_id=None, dates=null_dates,
                                  control=one_exp, experiments=[])
    else:
        observation = Observation(obs_id=None, dates=null_dates,
                                  control=None, experiments=[one_exp])

    return Pool([observation])


def offline_metric_test_calc(metric_container, expected_result_subdir, use_external_output, tmpdir, root_path,
                             numeric_values_only=False, skip_metric_errors=False):
    """
    :type metric_container: PluginContainer
    :type expected_result_subdir: str
    :type use_external_output: bool
    :type tmpdir
    :type root_path
    :type numeric_values_only: bool
    :type skip_metric_errors: bool
    :rtype:
    """
    serpset_id = "100500"
    pool_control = make_pool_by_serpset_id(serpset_id, control=True)

    logging.info("numeric_values_only = %s, skip_metric_errors = %s", numeric_values_only, skip_metric_errors)
    metric_values_control = offline_metric_test_calc_on_pool(pool=pool_control,
                                                             metric_container=metric_container,
                                                             expected_result_subdir=expected_result_subdir,
                                                             use_external_output=use_external_output,
                                                             tmpdir=tmpdir,
                                                             root_path=root_path,
                                                             numeric_values_only=numeric_values_only,
                                                             skip_metric_errors=skip_metric_errors)
    pool_exp = make_pool_by_serpset_id(serpset_id, control=False)
    metric_values_exp = offline_metric_test_calc_on_pool(pool=pool_exp,
                                                         metric_container=metric_container,
                                                         expected_result_subdir=expected_result_subdir,
                                                         use_external_output=use_external_output,
                                                         tmpdir=tmpdir,
                                                         root_path=root_path,
                                                         numeric_values_only=numeric_values_only,
                                                         skip_metric_errors=skip_metric_errors)

    assert metric_values_control == metric_values_exp
    return metric_values_control


def offline_metric_test_calc_on_pool(pool, metric_container, expected_result_subdir, use_external_output, tmpdir,
                                     root_path, numeric_values_only=False, skip_metric_errors=False):
    use_cache = False

    serpset_id = pool.all_serpset_ids()[0]

    src_data_dir = os.path.join(root_path, "debug_data/offline")

    parsed_serp_storage = ParsedSerpDataStorage(root_dir=src_data_dir, cache_subdir="", use_cache=use_cache)

    metric_storage = MetricDataStorage(root_dir=str(tmpdir), cache_subdir="", use_cache=use_cache)

    logging.info("numeric_values_only = %s, skip_metric_errors = %s", numeric_values_only, skip_metric_errors)
    global_ctx = OfflineGlobalCtx(use_external_output=use_external_output, load_urls=True,
                                  numeric_values_only=numeric_values_only,
                                  skip_metric_errors=skip_metric_errors)
    metric_ctx = OfflineMetricCtx(global_ctx=global_ctx,
                                  plugin_container=metric_container,
                                  parsed_serp_storage=parsed_serp_storage,
                                  metric_storage=metric_storage)

    sscm.calc_offline_metric_local(pool, metric_ctx, max_processes=1)

    metric_id = list(metric_container.plugin_instances.keys())[0]

    mr_name = metric_storage.rel_metric_by_serpset(metric_id, serpset_id)
    expected_mr_path = os.path.join(src_data_dir, expected_result_subdir, mr_name)
    real_mr_path = metric_storage.metric_by_serpset(metric_id, serpset_id)
    utest.fuzzy_diff_tsv_files(test_file=real_mr_path, expected_file=expected_mr_path)

    # test results extraction
    if use_external_output:
        external_output_file_tar = str(tmpdir.join("external-metric-result-test.tgz"))
        sscm.create_external_output(metric_ctx, external_output_file_tar)

    one_exp = pool.all_experiments()[0]

    if one_exp.metric_results:
        metric_values = one_exp.metric_results[0].metric_values
        logging.info("Metric values: %s", metric_values)
        return metric_values
    else:
        logging.warning("No metric results for metric %s", metric_container.plugin_instances[0])
        return None


# noinspection PyClassHasNoInit
@pytest.mark.usefixtures("make_silent_run_command")
class TestComputeMetricPool:
    def test_compute_rel_sum_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricRelSum")

        exp_folder = "rel_sum_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert round(metric_values.average_val, 2) == 727.33
        assert metric_values.sum_val == 2182.0
        assert metric_values.count_val == 3

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_url_len_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricSumUrlLen")

        exp_folder = "url_len_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert metric_values.average_val == 2930.0
        assert metric_values.sum_val == 8790.0
        assert metric_values.count_val == 3

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_yield_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricYield")

        exp_folder = "yield_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert round(metric_values.average_val, 2) == 1255.71
        assert metric_values.sum_val == 8790.0
        assert metric_values.count_val == 7

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_nothing_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricNothing")

        exp_folder = "nothing_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert metric_values.average_val == 12.34
        assert metric_values.sum_val == 12.34
        assert metric_values.count_val == 1

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_not_numbers_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricNotNumbers")

        exp_folder = "not_numbers_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert metric_values.average_val is None
        assert metric_values.sum_val == 0
        assert metric_values.count_val == 0
        assert metric_values.row_count == 3

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_params_fields_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricParamsFields")

        exp_folder = "params_fields_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert metric_values.sum_val == 2115
        assert metric_values.count_val == 57

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_detailed_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricDetailed")

        exp_folder = "detailed_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert round(metric_values.average_val, 2) == 31.59
        assert round(metric_values.sum_val, 2) == 94.76
        assert metric_values.count_val == 3

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_detailed_with_precompute_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricDetailedWithPrecompute")

        exp_folder = "detailed_with_precompute_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert round(metric_values.average_val, 2) == 2.0
        assert round(metric_values.sum_val, 2) == 6
        assert metric_values.count_val == 3

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_detailed_with_depth(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestMetricDetailedWithDepth")

        exp_folder = "detailed_with_depth_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert round(metric_values.average_val, 2) == 2200.0
        assert round(metric_values.sum_val, 2) == 6600.0
        assert metric_values.count_val == 3

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_judgement_level_metric_by_pool(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container(class_name="JudgementLevel",
                                                           module_name="sample_metrics.offline",
                                                           kwargs={"scales": ["RELEVANCE", "TEST_SCALE"]})

        exp_folder = "judgement_level_metric"
        metric_values = offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                                 root_path=root_path)
        assert round(metric_values.average_val, 2) == 0.44
        assert round(metric_values.sum_val, 2) == 1.33
        assert metric_values.count_val == 3

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path)

    def test_compute_nan_inf_metric(self, tmpdir, root_path):
        metric_container = sscm_ut.create_metric_container("OfflineTestNanInfMetric")

        # enable values validation
        numeric_values_only = True
        skip_metric_errors = True
        exp_folder = "nan_inf_metric"
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=False, tmpdir=tmpdir,
                                 root_path=root_path, numeric_values_only=numeric_values_only,
                                 skip_metric_errors=skip_metric_errors)

        # same calculation with external output, just check if it works
        offline_metric_test_calc(metric_container, exp_folder, use_external_output=True, tmpdir=tmpdir,
                                 root_path=root_path, numeric_values_only=numeric_values_only,
                                 skip_metric_errors=skip_metric_errors)
