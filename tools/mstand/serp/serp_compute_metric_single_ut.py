import logging
import os
import pytest

import serp.serp_compute_metric_single as scm_single
import yaqutils.test_helpers as utest
from experiment_pool import Experiment
from experiment_pool import Observation
from serp import MetricDataStorage
from serp import OfflineComputationCtx
from serp import OfflineGlobalCtx
from serp import OfflineMetricCtx
from serp import ParsedSerpDataStorage
from user_plugins import PluginContainer
from user_plugins import PluginBatch
from user_plugins import PluginSource

TEST_METRIC_MODULE = "sample_metrics.offline"


def offline_metric_test_calc(metric_container: PluginContainer, expected_result_subdir, tmpdir, use_external_output, root_path,
                             skip_metric_errors=False, numeric_values_only=False, gzip_external_output=False):
    """
    :type expected_result_subdir: str | None
    :type tmpdir
    :type use_external_output: bool
    :type skip_metric_errors: bool
    :return:
    """
    metric_container = metric_container

    use_cache = False

    src_data_dir = os.path.join(root_path, "debug_data/offline")

    parsed_serp_storage = ParsedSerpDataStorage(cache_subdir=src_data_dir, use_cache=use_cache)

    metric_storage = MetricDataStorage(cache_subdir=str(tmpdir), use_cache=use_cache)

    global_context = OfflineGlobalCtx(use_external_output=use_external_output,
                                      load_urls=True,
                                      skip_metric_errors=skip_metric_errors,
                                      numeric_values_only=numeric_values_only,
                                      gzip_external_output=gzip_external_output)

    metric_context = OfflineMetricCtx(global_ctx=global_context,
                                      plugin_container=metric_container,
                                      parsed_serp_storage=parsed_serp_storage,
                                      metric_storage=metric_storage)

    exp = Experiment(testid=None, serpset_id="100500")
    obs = Observation(obs_id=None, dates=None, control=exp)

    serpset_id = "100500"
    comp_context = OfflineComputationCtx(metric_context, observation=obs, experiment=exp)

    scm_single.compute_metric_by_serpset(comp_context)

    metric_id = list(metric_container.plugin_instances.keys())[0]

    if expected_result_subdir:
        mr_name = metric_storage.rel_metric_by_serpset(metric_id, serpset_id)
        expected_mr_path = os.path.join(src_data_dir, expected_result_subdir, mr_name)
        real_mr_path = metric_storage.metric_by_serpset(metric_id, serpset_id)
        utest.fuzzy_diff_tsv_files(expected_mr_path, real_mr_path)

        # check external output, if any
        ext_mr_name = metric_storage.rel_metric_by_serpset_external(serpset_id)
        expected_ext_mr_path = os.path.join(src_data_dir, expected_result_subdir, ext_mr_name)

        if use_external_output and os.path.exists(expected_ext_mr_path):
            ext_mr_path = metric_storage.metric_by_serpset_external(serpset_id)
            utest.fuzzy_diff_json(expected_ext_mr_path, ext_mr_path)
        else:
            logging.info("No expected external output for metric %s, skipping validation.", metric_id)
    else:
        logging.info("No expected internal output for metric %s, skipping validation.", metric_id)


def create_metric_container(class_name, module_name="sample_metrics.offline.offline_test_metrics", kwargs=None):
    # classes are unique within one module => use class name as alias
    metric_container = PluginContainer.create_single(module_name=module_name,
                                                     class_name=class_name,
                                                     alias=class_name,
                                                     kwargs=kwargs)
    return metric_container


# noinspection PyClassHasNoInit
class TestComputeMetricSingle:
    def test_compute_rel_sum_metric_by_serpset_gzip_external_output(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricRelSum")
        exp_folder = "rel_sum_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True, gzip_external_output=True)

    def test_compute_rel_sum_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricRelSum")
        exp_folder = "rel_sum_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_url_len_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricSumUrlLen")
        exp_folder = "url_len_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_yield_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricYield")
        exp_folder = "yield_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_nothing_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricNothing")
        exp_folder = "nothing_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_not_numbers_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricNotNumbers")
        exp_folder = "not_numbers_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_params_fields_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricParamsFields")
        exp_folder = "params_fields_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_detailed_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricDetailed")
        exp_folder = "detailed_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_detailed_with_depth_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricDetailedWithDepth")
        exp_folder = "detailed_with_depth_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_detailed_with_precompute_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricDetailedWithPrecompute")
        exp_folder = "detailed_with_precompute_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)

    def test_compute_buggy_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container("OfflineTestMetricBuggy")
        offline_metric_test_calc(metric_container, expected_result_subdir=None, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True, skip_metric_errors=True)
        with pytest.raises(Exception):
            offline_metric_test_calc(metric_container, expected_result_subdir=None, tmpdir=tmpdir, root_path=root_path,
                                     use_external_output=True, skip_metric_errors=False)

    def test_compute_judgement_level_metric_by_serpset(self, tmpdir, root_path):
        metric_container = create_metric_container(class_name="JudgementLevel",
                                                   module_name="sample_metrics.offline",
                                                   kwargs={"scales": ["RELEVANCE", "TEST_SCALE"]})
        exp_folder = "judgement_level_metric"
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=True)
        offline_metric_test_calc(metric_container, exp_folder, tmpdir=tmpdir, root_path=root_path,
                                 use_external_output=False)


# noinspection PyClassHasNoInit
class TestHybridOfflineBatch:
    # tests for MSTAND-1154: hybrid batches
    def test_simple_and_detailed_metric_batch(self, tmpdir, root_path):
        src_simple = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                  class_name="OfflineTestMetricRelSum")
        src_detailed = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                    class_name="OfflineTestMetricDetailed")

        plugin_batch = PluginBatch(plugin_sources=[src_simple, src_detailed])
        metric_container = PluginContainer(plugin_batch=plugin_batch)
        offline_metric_test_calc(metric_container, expected_result_subdir=None,
                                 tmpdir=tmpdir, root_path=root_path, use_external_output=False)

    def test_simple_and_precomputed_metric_batch(self, tmpdir, root_path):
        src_simple = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                  class_name="OfflineTestMetricRelSum")
        src_precomp = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                   class_name="OfflineTestMetricDetailedWithPrecompute")

        plugin_batch = PluginBatch(plugin_sources=[src_simple, src_precomp])
        metric_container = PluginContainer(plugin_batch=plugin_batch)
        offline_metric_test_calc(metric_container, expected_result_subdir=None,
                                 tmpdir=tmpdir, root_path=root_path, use_external_output=False)


# noinspection PyClassHasNoInit
class TestIncompleteMetrics:
    # tests for MSTAND-1142: broken metric classes
    def test_broken_metric_classes(self, tmpdir, root_path):
        src_incomplete = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                      class_name="OfflineTestMetricDetailedIncomplete")

        plugin_batch = PluginBatch(plugin_sources=[src_incomplete])
        metric_container = PluginContainer(plugin_batch=plugin_batch)
        with pytest.raises(Exception):
            offline_metric_test_calc(metric_container, expected_result_subdir=None,
                                     tmpdir=tmpdir, root_path=root_path, use_external_output=False)


# noinspection PyClassHasNoInit
class TestAlwaysNullMetrics:
    # tests for MSTAND-1299: always null metrics (produces no results)
    def test_always_null_metrics(self, tmpdir, root_path):
        src_always_null = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                       class_name="OfflineTestMetricAlwaysNull")

        plugin_batch = PluginBatch(plugin_sources=[src_always_null])
        metric_container = PluginContainer(plugin_batch=plugin_batch)
        offline_metric_test_calc(metric_container, expected_result_subdir=None,
                                 tmpdir=tmpdir, root_path=root_path, use_external_output=False)


# noinspection PyClassHasNoInit
class TestNanInfMetrics:
    # tests for MSTAND-1649: nan/inf metrics
    def test_nan_inf_metrics(self, tmpdir, root_path):
        src_nan_inf = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                   class_name="OfflineTestNanInfMetric")

        plugin_batch = PluginBatch(plugin_sources=[src_nan_inf])
        metric_container = PluginContainer(plugin_batch=plugin_batch)
        offline_metric_test_calc(metric_container, expected_result_subdir=None,
                                 tmpdir=tmpdir, root_path=root_path, use_external_output=False,
                                 skip_metric_errors=True, numeric_values_only=True)

    def test_bool_metrics(self, tmpdir, root_path):
        src_bool = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                class_name="OfflineTestBoolMetric")

        plugin_batch = PluginBatch(plugin_sources=[src_bool])
        metric_container = PluginContainer(plugin_batch=plugin_batch)
        offline_metric_test_calc(metric_container, expected_result_subdir="bool_metric",
                                 tmpdir=tmpdir, root_path=root_path, use_external_output=False,
                                 skip_metric_errors=False, numeric_values_only=True)
