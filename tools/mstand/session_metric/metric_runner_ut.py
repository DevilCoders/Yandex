import argparse
import os
import pytest

import mstand_metric_helpers.common_metric_helpers as mhelp

from mstand_enums.mstand_online_enums import ServiceEnum
from session_local import MetricBackendLocal
from session_metric.metric_runner import MetricRunner
from user_plugins import PluginBatch
from user_plugins import PluginContainer


# noinspection PyClassHasNoInit
class TestBuckets(object):
    calc_backend = MetricBackendLocal()

    def test_no_bucket_required(self):
        container = PluginContainer.create_single("sample_metrics.online.online_test_metrics", "MetricForUsers")
        metric_runner = MetricRunner(
            metric_container=container,
            squeeze_path="",
            calc_backend=self.calc_backend
        )

        assert not metric_runner.use_buckets

    def test_bucket_required(self):
        container = PluginContainer.create_single("sample_metrics.online.online_test_metrics", "SessionsWOSplit")
        metric_runner = MetricRunner(
            metric_container=container,
            squeeze_path="",
            calc_backend=self.calc_backend,
        )

        assert metric_runner.use_buckets

    def test_ambiguous_buckets(self, data_path):
        test_file = os.path.join(data_path, "test_bad_metrics_batch.json")
        batch = PluginBatch.load_from_file(test_file)
        container = PluginContainer(batch)
        with pytest.raises(Exception):
            MetricRunner(
                metric_container=container,
                squeeze_path="",
                calc_backend=self.calc_backend,
            )

    def test_nonambiguous_batches(self, data_path):
        test_file = os.path.join(data_path, "test_good_metrics_batch.json")
        batch = PluginBatch.load_from_file(test_file)
        container = PluginContainer(batch)
        metric_runner = MetricRunner(
            metric_container=container,
            squeeze_path="",
            calc_backend=self.calc_backend,
        )

        assert metric_runner.use_buckets


class TestRequiredServices(object):
    cli_args = argparse.Namespace(class_name="", module_name="session_metric.metric_runner_test",
                                  services=[], source=None,
                                  batch=None, set_alias=None,
                                  user_kwargs=None, set_coloring=None, plugin_dir=None)

    def test_no_required_services(self):
        self.cli_args.class_name = "MetricNoRequiredServices"
        self.cli_args.services = ["alice", "taxi", "toloka", "web"]

        missing_services = self._get_missing_services()
        assert not missing_services

    def test_satisfy_required_services(self):
        self.cli_args.class_name = "MetricWithRequiredServices"
        self.cli_args.services = ["alice", "news", "taxi", "toloka", "web"]

        missing_services = self._get_missing_services()
        assert not missing_services

    def test_missing_required_services(self):
        self.cli_args.class_name = "MetricWithRequiredServices"
        self.cli_args.services = ["alice", "taxi", "toloka"]

        missing_services = self._get_missing_services()
        expected_missing_services = ["news", "web"]
        assert missing_services == expected_missing_services

    def _get_missing_services(self):
        metric_container = mhelp.create_container_from_cli_args(self.cli_args)
        services = ServiceEnum.convert_aliases(self.cli_args.services)

        missing_services = MetricRunner.get_missing_services(metric_container, services)
        return missing_services
