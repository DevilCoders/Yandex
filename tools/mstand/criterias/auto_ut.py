# -*- coding: utf-8 -*-

import os.path

import pytest

import criterias.auto
import experiment_pool.pool_helpers
import postprocessing.compute_criteria
import postprocessing.criteria_utils
import user_plugins.plugin_key
import yaqutils.six_helpers as usix


@pytest.fixture
def root_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand"))
    except:
        return str(request.config.rootdir)


class CallCounter(object):
    pvalue = 0.5

    def __init__(self):
        self.counter = 0

    def __call__(self, params):
        self.counter += 1
        return self.pvalue


class TestAutoCriteria(object):
    expected_criterias = {'criterias.mannwhitneyu.MWUTest', 'criterias.ttest.TTest'}
    base_dir, pool, data_type, criteria, calls = None, None, None, None, None

    @pytest.fixture(autouse=True, scope='function')
    def setup(self, root_path):
        self.base_dir = os.path.join(root_path, 'criterias/tests/ut/data/auto')
        self.pool = experiment_pool.pool_helpers.load_pool(
            os.path.join(self.base_dir, 'pool.json')
        )
        self.data_type = postprocessing.criteria_utils.get_pool_data_type(self.pool)
        self.criteria = criterias.auto.AutoCriteria()

    @pytest.fixture(autouse=True, scope='function')
    def patch_criterias(self, monkeypatch):
        self.calls = {}
        for criteria in self.expected_criterias:
            counter = self.calls[criteria] = CallCounter()

            monkeypatch.setattr(criteria + '.value', counter)

    def run_criteria(self, synthetic, expected_calls):
        for counter in usix.itervalues(self.calls):
            assert counter.counter == 0

        result = postprocessing.compute_criteria.calc_criteria(
            criteria=self.criteria,
            criteria_key=user_plugins.plugin_key.PluginKey('TestAutoCriteria'),
            pool=self.pool,
            base_dir=self.base_dir,
            data_type=self.data_type,
            synthetic=synthetic
        )

        for counter in usix.itervalues(self.calls):
            assert counter.counter == expected_calls

        return result

    def test_criteria_keys(self):
        result = self.run_criteria(synthetic=False, expected_calls=1)

        criteria_names = set()

        for observation in result.observations:
            for experiment in observation.experiments:
                for metric_result in experiment.metric_results:
                    for criteria_result in metric_result.criteria_results:
                        criteria_key = criteria_result.criteria_key
                        criteria_names.add(criteria_key.name)

                        assert criteria_result.pvalue == CallCounter.pvalue

        assert criteria_names == self.expected_criterias

    def test_synthetic(self):
        self.pool.observations[0].experiments = []
        self.run_criteria(synthetic=10, expected_calls=10)
