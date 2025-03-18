import logging
from correlations import CorrelationParamsForAPI  # noqa

import scipy.stats
import math
import numpy as np

import mstand_utils.stat_helpers as ustat


def log_pvalue_func(side, min_pvalue):
    """
    :type side: CorrelationValuesForAPI
    :type min_pvalue: float
    :rtype: float
    """
    sign = np.sign(side.metric_diff.abs_diff)
    pvalue = ustat.normalize_pvalue(side.pvalue, min_pvalue)
    pv_log = math.log(pvalue)

    return sign * pv_log


class CorrelationPearson(object):
    def __init__(self, min_pvalue=0.000001):
        self.min_pvalue = min_pvalue

    def __call__(self, corr_params):
        """
        :type corr_params: CorrelationParamsForAPI
        :rtype: float
        """
        # logging.debug("corr params: %s", corr_params)

        pvalues_left = []
        pvalues_right = []

        for x in corr_params.corr_pairs:
            pvalues_left.append(log_pvalue_func(x.left, self.min_pvalue))
            pvalues_right.append(log_pvalue_func(x.right, self.min_pvalue))

        corr = scipy.stats.pearsonr(pvalues_left, pvalues_right)
        logging.debug("corr result: %s", corr)
        return corr
