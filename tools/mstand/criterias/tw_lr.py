# -*- coding: utf-8 -*-
import math

import scipy
import scipy.stats

__author__ = 'adrutsa'


class TWTest(object):
    """
    Tarone-Ware test
    """

    def __init__(self):
        pass

    @staticmethod
    def value(params):
        """
        :type params: CriteriaParamsForAPI
        :rtype: float
        """
        control = params.control_data
        experiment = params.exp_data

        pvalue_calculator = PvalueCalculatorTWLR(control, experiment, "TW")
        return pvalue_calculator.run()

    def __repr__(self):
        return "TWTest()"


class LRTest(object):
    """
    Logrank test
    """

    def __init__(self):
        pass

    @staticmethod
    def value(params):
        """
        :type params: CriteriaParamsForAPI
        :rtype: float
        """
        control = params.control_data
        experiment = params.exp_data

        pvalue_calculator = PvalueCalculatorTWLR(control, experiment, "LR")
        return pvalue_calculator.run()

    def __repr__(self):
        return "LRTest()"


class PvalueCalculatorTWLR(object):
    def __init__(self, l_a, l_b, testtype):
        assert testtype in ["TW", "LR"]

        self.testtype = testtype

        self._sum_mean = 0
        self._sum_var = 0

        self._l_a = l_a
        self._l_b = l_b

        # sort lists
        self._l_a.sort()
        self._l_b.sort()

        self._n_a = len(self._l_a)
        self._n_b = len(self._l_b)

    def run(self):
        self._calculate_sums()
        return self._calculate_pvalue_type_mw()

    def _calculate_sums(self):
        l_a, l_b = self._l_a, self._l_b

        # the number of instances with value < than the current value
        # (in l_a & l_b respectively)
        lower_in_a = 0
        lower_in_b = 0
        # the current position of the pointer
        # (in l_a & l_b respectively)
        cur_a = 0
        cur_b = 0

        while cur_a < self._n_a and cur_b < self._n_b:
            # the number of instances with value == the current value
            # (in l_a / l_b respectively)
            cur_in_a = 0
            cur_in_b = 0

            if l_a[cur_a] < l_b[cur_b]:
                cur_in_a += 1
                cur_a += 1
                while cur_a < self._n_a and l_a[cur_a] == l_a[cur_a - 1]:
                    cur_in_a += 1
                    cur_a += 1

            elif l_a[cur_a] > l_b[cur_b]:
                cur_in_b += 1
                cur_b += 1
                while cur_b < self._n_b and l_b[cur_b] == l_b[cur_b - 1]:
                    cur_in_b += 1
                    cur_b += 1

            else:
                cur_in_a += 1
                cur_a += 1
                while cur_a < self._n_a and l_a[cur_a] == l_a[cur_a - 1]:
                    cur_in_a += 1
                    cur_a += 1
                cur_in_b += 1
                cur_b += 1
                while cur_b < self._n_b and l_b[cur_b] == l_b[cur_b - 1]:
                    cur_in_b += 1
                    cur_b += 1

            # update sums
            self._update_sums(cur_in_a, cur_in_b, lower_in_a, lower_in_b)

            # next step
            lower_in_a += cur_in_a
            lower_in_b += cur_in_b

    def _update_sums(self, cur_in_a, cur_in_b, lower_in_a, lower_in_b):
        # the number of instances with value >= than the current value
        # (in l_a / l_b / l_a+l_b respectively)
        nolower_in_a = self._n_a - lower_in_a
        nolower_in_b = self._n_b - lower_in_b
        nolower_in_ab = nolower_in_a + nolower_in_b
        # the number of instances with value == the current value
        # (in l_a / l_b / l_a+l_b respectively)
        cur_in_ab = cur_in_a + cur_in_b

        # calculate sums
        # calculate mean term
        term_mean = cur_in_b
        term_mean -= nolower_in_b * cur_in_ab / float(nolower_in_ab)

        # calculate var term
        term_var = nolower_in_b * nolower_in_a * cur_in_ab
        term_var *= (nolower_in_ab - cur_in_ab) / float(nolower_in_ab)
        term_var /= float(nolower_in_ab * (nolower_in_ab - 1))

        if self.testtype == "TW":
            self._sum_mean += math.sqrt(nolower_in_ab) * term_mean
            self._sum_var += nolower_in_ab * term_var

        elif self.testtype == "LR":
            self._sum_mean += term_mean
            self._sum_var += term_var

        else:
            assert False

    def _calculate_pvalue_type_mw(self):
        # calculate chi^2
        chi2 = self._sum_mean * self._sum_mean
        chi2 /= float(self._sum_var)

        # calculate p-value
        pvalue = 1 - scipy.stats.chi2(1).cdf(chi2)

        return pvalue
