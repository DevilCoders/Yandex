import math
import numpy
import logging
import scipy.stats


def normalize_pvalue(pvalue, min_pvalue=0.0000001):
    return max(pvalue, min_pvalue)


def conf_interval(alpha, size, threshold):
    interval_half = alpha / 2.0

    left = scipy.stats.binom.ppf(interval_half, size, threshold)
    right = scipy.stats.binom.ppf(1.0 - interval_half, size, threshold)
    return left, right


def check_confidence_interval(total_count, colored_count, threshold):
    left, right = conf_interval(alpha=0.01, size=total_count, threshold=threshold)
    logging.info("Interval: [%s:%s], colored: %s", left, right, colored_count)
    # cast numpy.bool_ to regular bool (numpy.bool_ breaks json serialization)
    return bool(left <= colored_count <= right)


def average_squared_deviation(arr, is_sum=False):
    """
    :type arr: list[float] | numpy.array
    :type is_sum: bool
    :rtype: float
    """
    if len(arr) == 0:
        return None
    std = numpy.std(arr, ddof=1)

    if is_sum:
        return std * math.sqrt(len(arr))
    else:
        return std / math.sqrt(len(arr))


# for compare sensitivity
def compare_via_ttest_rel(baseline, test):
    try:
        _, pvalue = scipy.stats.ttest_rel(baseline, test)
        return pvalue
    except ValueError as exc:
        logging.warning("Exception in compare_via_ttest_rel: {}", exc)
        return None
