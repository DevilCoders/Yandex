import csv
import numpy

import yaqutils.file_helpers as ufile
from experiment_pool import MetricDataType


def validate_criteria(criteria):
    if not hasattr(criteria, "value"):
        raise Exception("No method 'value' in criteria. See https://wiki.yandex-team.ru/mstand/criteria-dev for details")


def get_pool_data_type(pool):
    data_types = pool.all_data_types()
    if len(data_types) > 1:
        raise Exception("Cannot compute criteria for mixed data types (values and key-values)")
    if not data_types:
        return MetricDataType.NONE
    else:
        return data_types.pop()


def flatten_array(data):
    if data.dtype == object:
        return numpy.concatenate(data)
    if len(data.shape) > 1:
        return data.ravel()
    return data


# TODO: merge with same code in postprocess (and add json parsing)
def iter_tsv_lines(path, keyed=False):
    with ufile.fopen_read(path, use_unicode=False) as fd:
        for row in csv.reader(fd, delimiter="\t"):
            if keyed:
                key = row[0]
                values = [float(x) for x in row[1:]]
                yield key, values
            else:
                values = [float(x) for x in row]
                yield values
