import logging
import os
import time

import numpy
import pandas

try:
    # new pandas (1.1.0+) way
    from pandas.errors import ParserError
    from pandas.errors import EmptyDataError
except ImportError:
    # legacy way
    from pandas.io.common import ParserError
    from pandas.io.common import EmptyDataError

import postprocessing.criteria_utils as crit_utils
from yaqutils import MainUtilsException
import yaqutils.misc_helpers as umisc
from experiment_pool import MetricDataType
from postprocessing.criteria_read_mode import CriteriaReadMode


# noinspection PyClassHasNoInit
class CriteriaReadException(MainUtilsException):
    pass


def load_metric_values_file(base_dir, metric_values, criteria_read_mode):
    """
    :type base_dir: str
    :type metric_values: MetricValues
    :type criteria_read_mode: str
    :rtype: numpy.array | list[float] | dict[str]
    """
    if metric_values.data_file is None:
        raise CriteriaReadException("Data file is not specified, cannot load metric values file. ")

    time_start = time.time()
    full_data_path = os.path.join(base_dir, metric_values.data_file)
    metric_data = load_metric_values_tsv(full_data_path, metric_values.data_type, criteria_read_mode)
    if metric_data is not None:
        umisc.log_elapsed(time_start, "file %s loaded (rows: %d)", metric_values.data_file, len(metric_data))
    return metric_data


def prepare_criteria_input(data_type, control_data, exp_data, read_mode):
    """
    :type data_type: str
    :type control_data: list | dict
    :type exp_data: list | dict
    :type read_mode: str
    :return: (numpy.array, numpy.array)
    """
    if data_type == MetricDataType.VALUES:
        return control_data, exp_data
    elif data_type == MetricDataType.KEY_VALUES:
        common_keys = sorted(set(control_data.keys()) & set(exp_data.keys()))
        logging.info("control size: %d, exp size: %d, common part: %d",
                     len(control_data), len(exp_data), len(common_keys))
        prep_control_data = shape_criteria_results([control_data[key] for key in common_keys], read_mode)
        prep_exp_data = shape_criteria_results([exp_data[key] for key in common_keys], read_mode)
        return prep_control_data, prep_exp_data
    else:
        raise CriteriaReadException("Unsupported metric data type {}".format(data_type))


def shape_criteria_results(values, criteria_read_mode):
    """
    :type values: list
    :type criteria_read_mode: str
    :rtype: numpy.array
    """
    if criteria_read_mode == CriteriaReadMode.FLOAT_1D:
        if not all(len(row) == 1 for row in values):
            raise CriteriaReadException("All rows should have only 1 number")
        return numpy.array(values, dtype=float).ravel()
    elif criteria_read_mode == CriteriaReadMode.FLOAT_2D:
        if values:
            width = len(values[0])
            if not all(len(row) == width for row in values):
                raise CriteriaReadException("All rows should have equal length ({})".format(width))
        return numpy.array(values, dtype=float)
    elif criteria_read_mode == CriteriaReadMode.JSON_LISTS:
        return numpy.array(values, dtype=object)
    else:
        raise CriteriaReadException("Unsupported criteria read mode: '{}'".format(criteria_read_mode))


def load_metric_values_tsv(path, data_type, criteria_read_mode):
    """
    :type path: str
    :type data_type: str
    :type criteria_read_mode: str
    :rtype: numpy.array list[float] dict[str]
    """
    if data_type == MetricDataType.VALUES:
        logging.info("File %s type is 'values', loading metric result using numpy.", path)
        return load_simple_tsv_lines(path, criteria_read_mode)
    elif data_type == MetricDataType.KEY_VALUES:
        logging.info("File %s type is 'key-values', loading metric result manually.", path)
        return load_keyed_tsv_lines(path)
    else:
        raise CriteriaReadException("Unsupported data type: '{}'".format(data_type))


def load_simple_tsv_lines(path, criteria_read_mode):
    """
    :type path: str
    :type criteria_read_mode: str
    :rtype: numpy.array
    """
    if criteria_read_mode == CriteriaReadMode.FLOAT_1D:
        try:
            table = pandas.read_csv(path, dtype=float, sep='\t', header=None, squeeze=True).values
        except EmptyDataError:
            logging.warning("File {} is empty, set pvalue to NaN".format(path))
            return None
        except ParserError as e:
            raise CriteriaReadException("Data should be 1d ({}): {}".format(e, path))
        if len(table.shape) != 1:
            raise CriteriaReadException("Data should be 1d: {}".format(path))
        return table
    elif criteria_read_mode == CriteriaReadMode.FLOAT_2D:
        try:
            table = pandas.read_csv(path, dtype=float, sep='\t', header=None, na_filter=False).values
        except EmptyDataError:
            logging.warning("File {} is empty, set pvalue to NaN".format(path))
            return None
        except ParserError as e:
            raise CriteriaReadException("Data should be 2d ({}): {}".format(e, path))
        if len(table.shape) != 2:
            raise CriteriaReadException("Data should be 2d {}".format(path))
        return table
    else:
        return shape_criteria_results(list(crit_utils.iter_tsv_lines(path)), criteria_read_mode)


def load_keyed_tsv_lines(path):
    """
    :type path: str
    :rtype: dict
    """
    return dict(crit_utils.iter_tsv_lines(path, keyed=True))
