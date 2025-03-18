#!/usr/bin/env python3

import argparse
import logging
import math

import bokeh.layouts as bol
import bokeh.plotting as bop

import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
from reports import CorrelationResult


def sen_func0(x):
    return x


def sen_func1(x):
    # arcsin((x - 0.5) * 2) / PI + 0.5
    return math.asin(x * 2.0 - 1.0) / math.pi + 0.5


def sen_func2(x):
    deg = 0.4
    if x <= 0.5:
        return math.pow(x, deg)
    else:
        return 1.0 - math.pow(1.0 - x, deg)


def heatmap_color(x):
    """
    :type x: float
    :rtype: str
    """
    color_int = int((1.0 - x) * 240.0) + 10
    color_hex = "#" + "{:02x}".format(color_int) * 3
    return color_hex


def remove_dups_stable(array):
    unique = set()
    result = []
    for x in array:
        if x not in unique:
            unique.add(x)
            result.append(x)
    return result


def make_heat_map(graph_x, graph_y, colors):
    x_range = remove_dups_stable(graph_x)
    y_range = remove_dups_stable(graph_y)

    logging.info("len x: %d", len(x_range))
    logging.info("len y: %d", len(y_range))

    if len(y_range) == 1:
        logging.info("peforming transpose")
        y_range, x_range = x_range, y_range
        graph_x, graph_y = graph_y, graph_x

    plot_width = 2000
    if len(x_range) == 1:
        plot_width = 500

    heat_map = bop.figure(title="Metric correlations",
                          x_range=x_range,
                          y_range=y_range,
                          toolbar_location=None,
                          plot_width=plot_width,
                          plot_height=2000,
                          )
    heat_map.xaxis.major_label_orientation = math.pi / 4.0

    heat_map.rect(x=graph_x, y=graph_y, color=colors, width=1.0, height=1.0)

    return heat_map


def parse_args():
    parser = argparse.ArgumentParser(description="Build correlation plot")
    uargs.add_verbosity(parser)

    uargs.add_input(parser, required=True)
    uargs.add_output(parser, required=True)

    parser.add_argument(
        "--min-val",
        required=True,
        type=float,
        help="Correlation function min value (for color normalization)"
    )

    parser.add_argument(
        "--max-val",
        required=True,
        type=float,
        help="Correlation function max value (for color normalization)"
    )
    uargs.add_boolean_argument(parser, "sort-by-name", help_message="sort metrics by name (not by correlation value)")
    return parser.parse_args()


def cutoff(x, cutoff_min, cutoff_max):
    if cutoff_max is not None:
        if x > cutoff_max:
            return cutoff_max
    if cutoff_min is not None:
        if x < cutoff_min:
            return cutoff_min
    return x


def normalize(values, min_val, max_val, scale=1.0):
    if not values:
        return values

    real_min = min(values)
    real_max = max(values)

    logging.info("real: min = %s, max = %s", real_min, real_max)

    cut_values = [cutoff(x, min_val, max_val) for x in values]

    delta = abs(max_val - min_val)

    if delta < 1.0e-10:
        return cut_values

    normalized = [(x - min_val) * scale / delta for x in cut_values]

    normalized_min = min(normalized)
    normalized_max = max(normalized)
    logging.info("normalized: min = %s, max = %s", normalized_min, normalized_max)
    return normalized


def name_key(corr_res):
    return corr_res.left_metric.str_key(), corr_res.right_metric.str_key()


def value_key(corr_res):
    return [-1e9, -1e9] if corr_res.corr_value[0] is None else corr_res.corr_value


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose)

    logging.info("Building correlation plot")
    corr_data = ujson.load_from_file(cli_args.input_file)

    graph_x = []
    graph_y = []

    corr_values = []

    corr_results = umisc.deserialize_array(corr_data, CorrelationResult)

    if cli_args.sort_by_name:
        corr_results.sort(key=name_key)
    else:
        corr_results.sort(key=value_key)

    for corr_res in corr_results:
        label_x = corr_res.left_metric.str_key()
        label_y = corr_res.right_metric.str_key()

        if isinstance(corr_res.corr_value, (list, tuple)):
            corr_value = corr_res.corr_value[0]
        else:
            corr_value = corr_res.corr_value

        if corr_value is None or math.isnan(corr_value):
            logging.info("Skipping None/NaN value for %s:%s", label_x, label_y)
            continue

        graph_x.append(label_x)
        graph_y.append(label_y)

        corr_values.append(corr_value)

    normalized_values = normalize(corr_values, min_val=cli_args.min_val, max_val=cli_args.max_val)

    for nv in normalized_values:
        logging.debug("nv = %s", nv)

    colors0 = list(map(heatmap_color, map(sen_func0, normalized_values)))
    colors1 = list(map(heatmap_color, map(sen_func1, normalized_values)))

    logging.info("graph_x: %d", len(graph_x))
    logging.info("graph_y: %d", len(graph_y))

    hm0 = make_heat_map(graph_x, graph_y, colors0)
    hm1 = make_heat_map(graph_x, graph_y, colors1)

    logging.info("Saving heatmap output to %s", cli_args.output_file)
    bop.output_file(cli_args.output_file, title="Metric correlations")
    column = bol.column(hm0, hm1)
    bop.save(column)
    logging.info("All done")


if __name__ == "__main__":
    main()
