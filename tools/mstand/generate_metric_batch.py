#!/usr/bin/env python2.7

from collections import OrderedDict
import argparse

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.json_helpers as ujson


def parse_args():
    parser = argparse.ArgumentParser(description="Generate metric batch")
    uargs.add_verbosity(parser)
    uargs.add_output(parser)
    parser.add_argument(
        "--param-name",
        required=True,
        help="parameter name",
    )
    parser.add_argument(
        "--min",
        type=float,
        required=True,
        help="min param value",
    )
    parser.add_argument(
        "--max",
        type=float,
        required=True,
        help="max param value",
    )
    parser.add_argument(
        "--step",
        type=float,
        default=1.0,
        help="param increase step",
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    params = []
    param_name = cli_args.param_name
    param_min = cli_args.min
    param_max = cli_args.max
    param_step = cli_args.step

    if param_step <= 0.0:
        raise Exception("Incorrect step value, expected to be positive number")
    param = param_min
    while param <= param_max:
        params.append(param)
        param += param_step

    kwargs_list = [{
                       "name": "{}={:07.2f}".format(param_name, param),
                       "kwargs": {
                           param_name: param,
                       }
                   } for param in params]

    plugin_desc = OrderedDict()
    plugin_desc["alias"] = "MetricAlias"
    plugin_desc["class"] = "MetricName"
    plugin_desc["module"] = "sample_metrics.online"
    plugin_desc["kwargs_list"] = kwargs_list

    plugins = {
        "plugins": [plugin_desc]
    }

    # cannot use order from OrderedDict in ujson https://github.com/esnme/ultrajson/issues/55
    ujson.dump_to_file(plugins, cli_args.output_file)


if __name__ == "__main__":
    main()
