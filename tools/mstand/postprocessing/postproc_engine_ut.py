import logging
import os
import pytest

import experiment_pool.pool_helpers as pool_helpers
import postprocessing.postproc_engine as pp_engine
import yaqutils.test_helpers as utest
from experiment_pool.filter_metric import MetricFilter
from mstand_structs.squeeze_versions import get_python_version
from postprocessing import PostprocGlobalContext
from user_plugins import PluginContainer
from mstand_structs.squeeze_versions import SqueezeVersions


@pytest.fixture
def src_dir(data_path):
    return os.path.join(data_path, "postproc_engine")


@pytest.fixture
def src_dir_kv(data_path):
    return os.path.join(data_path, "postproc_engine_key_values")


def compare_control_metric_values(pool1, pool2, obs_index, metric_index):
    val1 = pool1.observations[obs_index].control.metric_results[metric_index].metric_values
    val2 = pool2.observations[obs_index].control.metric_results[metric_index].metric_values
    assert val1 == val2


def compare_exp_metric_values(pool1, pool2, obs_index, exp_index, metric_index):
    val1 = pool1.observations[obs_index].experiments[exp_index].metric_results[metric_index].metric_values
    val2 = pool2.observations[obs_index].experiments[exp_index].metric_results[metric_index].metric_values
    assert val1 == val2


# noinspection PyClassHasNoInit
class TestPostprocessEngine:
    def test_observation_postprocess_engine(self, tmpdir, src_dir):
        dest_dir = str(tmpdir.join("obs_test"))

        postprocessor = PluginContainer.create_single(module_name="postprocessing.scripts.postproc_samples",
                                                      class_name="ObservationPostprocessSample")

        ctx = PostprocGlobalContext(postprocessor, src_dir, dest_dir)
        pp_engine.postprocess_results_core(ctx, max_threads=1)

    def test_postprocess_engine_echo(self, tmpdir, src_dir):
        dest_dir = str(tmpdir.join("echo_test"))

        src_pool_path = os.path.join(src_dir, "pool.json")
        src_pool = pool_helpers.load_pool(src_pool_path)

        postprocessor = PluginContainer.create_single(module_name="postprocessing.scripts.echo",
                                                      class_name="EchoPostprocessor")

        ctx = PostprocGlobalContext(postprocessor, src_dir, dest_dir)

        dest_pool = pp_engine.postprocess_results_core(ctx, max_threads=1)

        dest_control_version = SqueezeVersions(
            service_versions={u"web": 1},
            common=1,
            history=None,
            python_version=get_python_version(),
        )
        assert repr(dest_pool.observations[0].control.metric_results[0].version) == repr(dest_control_version)

        dest_experiment_version = SqueezeVersions(
            service_versions={u"web": 2},
            common=2,
            history=None,
            python_version=get_python_version(),
        )
        assert repr(dest_pool.observations[0].experiments[0].metric_results[0].version) == repr(dest_experiment_version)

        assert not dest_pool.observations[0].experiments[0].metric_results[1].version

        compare_control_metric_values(src_pool, dest_pool, obs_index=0, metric_index=0)
        compare_control_metric_values(src_pool, dest_pool, obs_index=0, metric_index=1)
        compare_control_metric_values(src_pool, dest_pool, obs_index=1, metric_index=0)
        compare_control_metric_values(src_pool, dest_pool, obs_index=1, metric_index=1)

        compare_exp_metric_values(src_pool, dest_pool, obs_index=0, exp_index=0, metric_index=0)
        compare_exp_metric_values(src_pool, dest_pool, obs_index=0, exp_index=1, metric_index=0)
        compare_exp_metric_values(src_pool, dest_pool, obs_index=0, exp_index=1, metric_index=1)

        # dest_pool_path = os.path.join(dest_dir, "pool.json")
        # utest.fuzzy_diff_tsv_files(src_pool_path, dest_pool_path)
        expected1 = os.path.join(src_dir, "testid_1_metric_2.tsv")
        logging.info("expected path1: %s", expected1)
        test_file_1 = os.path.join(dest_dir, "postproc_0_obs0_control.testid_1_metric_1.tsv")
        utest.fuzzy_diff_tsv_files(expected1, test_file_1, fail_on_diff=False)

        expected2 = os.path.join(src_dir, "testid_5_metric_1.tsv")
        logging.info("expected path2: %s", expected2)
        test_file_2 = os.path.join(dest_dir, "postproc_0_obs1_exp0.testid_5_metric_1.tsv")
        utest.fuzzy_diff_tsv_files(expected2, test_file_2, fail_on_diff=False)

    def test_postprocess_engine_echo_key_values(self, tmpdir, src_dir_kv):
        dest_dir = str(tmpdir.join("echo_test"))

        src_pool_path = os.path.join(src_dir_kv, "pool.json")
        src_pool = pool_helpers.load_pool(src_pool_path)

        postprocessor = PluginContainer.create_single(module_name="postprocessing.scripts.echo",
                                                      class_name="EchoPostprocessor")

        ctx = PostprocGlobalContext(postprocessor, src_dir_kv, dest_dir)
        dest_pool = pp_engine.postprocess_results_core(ctx, max_threads=1)
        compare_control_metric_values(src_pool, dest_pool, obs_index=0, metric_index=0)

    def test_experiment_postprocess_engine(self, tmpdir, src_dir):
        dest_dir = str(tmpdir.join("exp_test"))

        src_pool_path = os.path.join(src_dir, "pool.json")

        src_pool = pool_helpers.load_pool(src_pool_path)

        # this postprocessor multiplies each value by approximation of Pi
        postprocessor = PluginContainer.create_single(module_name="postprocessing.scripts.postproc_samples",
                                                      class_name="ExperimentPostprocessSample")

        ctx = PostprocGlobalContext(postprocessor, src_dir, dest_dir)

        dest_pool = pp_engine.postprocess_results_core(ctx, max_threads=1)
        assert len(dest_pool.observations) == len(src_pool.observations)

        average_old = src_pool.observations[0].control.metric_results[1].metric_values.average_val
        average_new = dest_pool.observations[0].control.metric_results[1].metric_values.average_val
        assert average_new == average_old * 3.1415926

        dest_control_version = SqueezeVersions(
            service_versions={u"web": 1},
            common=1,
            history=None,
            python_version=get_python_version(),
        )
        assert repr(dest_pool.observations[0].control.metric_results[0].version) == repr(dest_control_version)

        dest_experiment_version = SqueezeVersions(
            service_versions={u"web": 2},
            common=2,
            history=None,
            python_version=get_python_version(),
        )
        assert repr(dest_pool.observations[0].experiments[0].metric_results[0].version) == repr(dest_experiment_version)

        assert not dest_pool.observations[0].experiments[0].metric_results[1].version

    def test_pool_filtration(self, tmpdir, src_dir):
        dest_dir = str(tmpdir.join("filter_test"))

        postprocessor = PluginContainer.create_single(module_name="postprocessing.scripts.postproc_samples",
                                                      class_name="ObservationPostprocessSample")

        ctx = PostprocGlobalContext(postprocessor, src_dir, dest_dir)

        dest_pool = pp_engine.postprocess_results_core(ctx, max_threads=1)
        for i in range(2):
            for j in range(2):
                assert dest_pool.observations[i].experiments[j].metric_results[0].metric_name()[:7] == "Metric1"
                assert dest_pool.observations[i].experiments[j].metric_results[1].metric_name()[:7] == "Metric2"
                assert len(dest_pool.observations[i].experiments[j].metric_results) == 2

        filter = MetricFilter(metric_whitelist=["Metric1"])
        dest_pool = pp_engine.postprocess_results_core(ctx, filter, max_threads=1)
        for i in range(2):
            for j in range(2):
                assert dest_pool.observations[i].experiments[j].metric_results[0].metric_name()[:7] == "Metric1"
                assert len(dest_pool.observations[i].experiments[j].metric_results) == 1

        filter = MetricFilter(metric_blacklist=["Metric2"])
        dest_pool = pp_engine.postprocess_results_core(ctx, filter, max_threads=1)
        for i in range(2):
            for j in range(2):
                assert dest_pool.observations[i].experiments[j].metric_results[0].metric_name()[:7] == "Metric1"
                assert len(dest_pool.observations[i].experiments[j].metric_results) == 1
