import logging

import numpy
import scipy.stats
import yaqutils.misc_helpers as umisc

__author__ = 'adrutsa'


# This class is used by mstand v2 (e.g., in Nirvana)

# It calculates MWUTest from SciPy


class MWUTest(object):
    def __init__(self, use_continuity=None, flatten_mode=False):
        self.use_continuity = use_continuity
        self.flatten_mode = flatten_mode
        if flatten_mode:
            logging.info("Using 'flatten mode' in MWUTest")
            self.read_mode = "lists"
        else:
            self.read_mode = "1d"
        if scipy_version() < (0, 17):
            raise Exception("Use at least scipy 0.17 (or install anaconda)")

    def value(self, params):
        """
        :type params: CriteriaParamsForAPI
        :rtype: float
        """
        control = params.control_data
        experiment = params.exp_data

        if self.flatten_mode:
            control = numpy.concatenate(control)
            experiment = numpy.concatenate(experiment)
        uc = self.use_continuity
        try:
            if uc is None:
                _, pvalue = scipy.stats.mannwhitneyu(control, experiment, alternative="two-sided")
            else:
                _, pvalue = scipy.stats.mannwhitneyu(control, experiment, use_continuity=uc, alternative="two-sided")
        except ValueError:
            pvalue = 0.5
        return float(pvalue)

    def __repr__(self):
        return "MWUTest(use_continuity={!r}, flatten_mode={!r})".format(self.use_continuity, self.flatten_mode)


def scipy_version():
    return tuple(umisc.optional_int(p) for p in scipy.__version__.split('.'))
