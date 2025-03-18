import logging

import numpy
import scipy.stats

from postprocessing import CriteriaParamsForAPI  # noqa


class TTest(object):
    # flatten mode (izubr@ request): see MSTAND-738
    def __init__(self, flatten_mode=False):
        self.flatten_mode = flatten_mode
        if flatten_mode:
            self.read_mode = "lists"
            logging.info("Using 'flatten mode' in TTest")
        else:
            self.read_mode = "1d"

    def value(self, params):
        """
        :type params: CriteriaParamsForAPI
        :rtype: float
        """

        if self.flatten_mode:
            control = numpy.concatenate(params.control_data) if len(params.control_data) > 0 else numpy.array([])
            experiment = numpy.concatenate(params.exp_data) if len(params.exp_data) > 0 else numpy.array([])
        else:
            control = params.control_data
            experiment = params.exp_data

        if params.is_related:
            statistic, pvalue = scipy.stats.ttest_rel(control, experiment)
        else:
            statistic, pvalue = scipy.stats.ttest_ind(control, experiment, equal_var=False)
        if numpy.isnan(statistic):
            extra_data = {"t-statistic": None}
        else:
            extra_data = {"t-statistic": float(statistic)}
        return float(pvalue), extra_data

    def __repr__(self):
        return "TTest(flatten_mode={!r})".format(self.flatten_mode)
