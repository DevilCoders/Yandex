import scipy
import scipy.stats


# calculates WilcoxonRankSumsTest from SciPy
class WilcoxonRankSumsTest(object):
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

        _, pvalue = scipy.stats.ranksums(control, experiment)
        return float(pvalue)

    def __repr__(self):
        return "WilcoxonRankSumsTest()"
