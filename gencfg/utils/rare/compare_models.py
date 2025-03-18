#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Compare various cpu models")
    parser.add_argument("-m", "--main-model", dest="main_model", type=argparse_types.machinemodel, required=True,
                        help="Obligatory. Main model (others to be compared with this one): one of %s" % ','.join(
                            CURDB.cpumodels.models.keys()))
    parser.add_argument("-o", "--other-models", dest="other_models", type=argparse_types.machinemodels, required=True,
                        help="Obligatory. Other models (to be compared with main)")
    parser.add_argument("-c", "--csv", dest="csv", action="store_true",
                        help="Optional. Generate report in csv format")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def constructline(descr, data, csv):
    if csv:
        result = "{};".format(descr)
    else:
        result = "{0:30}".format(descr)
    for elem in data:
        if csv:
            result += "{};".format(str(elem))
        else:
            result += "{0:15}".format(str(elem))
    return result


def _cons(model, load):
    if len(model.consumption.notbcons) == 0:
        return 0., 0.
    N = int(load * model.ncpu)
    N = max(min(N, model.ncpu), 0)
    return model.consumption.notbcons[N], model.consumption.tbcons[N]


def main(options):
    print constructline("Model", [options.main_model.model] + map(lambda x: x.model, options.other_models), options.csv)
    print constructline("Per machine qps", [1.0] + map(lambda x: round(x.power / float(options.main_model.power), 2),
                                                       options.other_models), options.csv)
    print constructline("Per core qps", [1.0] + map(
        lambda x: round(x.power * options.main_model.ncpu / float(options.main_model.power * x.ncpu), 2),
        options.other_models), options.csv)
    print constructline("Per tact qps", [1.0] + map(lambda x: round(
        x.power * options.main_model.ncpu * options.main_model.tbfreq / float(
            options.main_model.power * x.ncpu * x.tbfreq), 2), options.other_models), options.csv)
    print constructline("Per machine 50% cons",
                        ["%s/%s" % _cons(options.main_model, 0.5)] + map(lambda x: "%s/%s" % _cons(x, 0.5),
                                                                         options.other_models), options.csv)
    print constructline("Per machine 100% cons",
                        ["%s/%s" % _cons(options.main_model, 1.0)] + map(lambda x: "%s/%s" % _cons(x, 1.0),
                                                                         options.other_models), options.csv)
    main_model_eff = options.main_model.power / _cons(options.main_model, 1.0)[1]
    print constructline("Per watt qps", [1.0] + map(lambda x: round(x.power / _cons(x, 1.0)[1] / main_model_eff, 2),
                                                    options.other_models), options.csv)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
