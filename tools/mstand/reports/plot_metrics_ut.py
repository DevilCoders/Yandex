import os

import experiment_pool.pool_helpers as pool_helpers
import reports.plot_metrics as p_m


class TestMetricPairPlotter(object):
    def test_simple(self, data_path):
        pool = pool_helpers.load_pool(os.path.join(data_path, "corr_test_pool.json"))
        metrics = list(pool_helpers.enumerate_all_metrics(pool).keys())
        assert p_m.plot_metric_pair_html(pool, metrics[0], metrics[1], threshold=0.05)
