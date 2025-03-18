import logging

import mstand_utils.stat_helpers as ustat
import numpy
import random


# noinspection PyClassHasNoInit
class TestStatFunctions:
    def test_avg_sq_dev(self):
        assert ustat.average_squared_deviation([1, 1, 1]) == 0.0
        assert round(ustat.average_squared_deviation([1, 2, 3]), 3) == 0.577

    def test_compare_via_ttest_rel(self):
        size = 1000
        sample_a = numpy.random.normal(size=size)
        # sample_b = numpy.random.normal(size=size)
        sample_b = [x + (random.random() - 0.5) for x in sample_a]

        pvalue = ustat.compare_via_ttest_rel(sample_a, sample_b)
        logging.info("pvalue = %s", pvalue)
        assert pvalue > 0.0
