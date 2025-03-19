import logging

__author__ = "aarseniev"
__virtualname__ = "ldmerge"

log = logging.getLogger(__name__)


def __virtual__():
    return __virtualname__


def _get_keys(data):
    """
    data = []
    keyslist = []
    Return list of keys from data (if there are any dict's)
    """
    keyslist = []
    for elem in data:
        if isinstance(elem, dict):
            for k in elem:
                keyslist.append(k)
    return keyslist


def _operate(pri, pri_keys, low, low_keys):
    """
    pri, pri_keys, low, low_keys = []
    result = []
    Return merged pri,low list's. Keys for dicts in lists
    """
    result = []
    for pri_elem in pri:
        if isinstance(pri_elem, dict) or pri_elem not in low_keys:
            result.append(pri_elem)
    for low_elem in low:
        if isinstance(low_elem, dict):
            for k in low_elem:
                if k not in pri_keys:
                    result.append(low_elem)
        else:
            if low_elem not in pri and low_elem not in pri_keys:
                result.append(low_elem)
    return result


def run(prior, lower):
    prior_keys = _get_keys(prior)
    lower_keys = _get_keys(lower)
    return _operate(prior, prior_keys, lower, lower_keys)
