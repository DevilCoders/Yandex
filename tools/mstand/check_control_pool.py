#!/usr/bin/env python3

import argparse
import itertools
import logging

import experiment_pool.pool_helpers as upool
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.stat_helpers as ustat
import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.template_helpers as utemplate
from experiment_pool import MetricColor
from reports import ExperimentPair
from reports.report_helpers import format_num

import project_root

THRESHOLDS = [
    0.001,
    0.005,
    0.01,
    0.05,
    0.1
]


def group_by_observation_id(observations):
    def get_id(obs):
        return obs.id

    observations = sorted(observations, key=get_id)
    return [list(g) for _, g in itertools.groupby(observations, get_id)]


def sort_by_date(observations):
    return sorted(observations, key=lambda obs: obs.dates.start)


def split_by_length(observations):
    period = []
    one_day = []
    for observation in observations:
        if observation.dates.number_of_days() > 1:
            period.append(observation)
        else:
            one_day.append(observation)

    return sort_by_date(one_day), sort_by_date(period)


def colors_for_thresholds(pair):
    colors = {}
    for threshold in THRESHOLDS:
        pair.colorize(value_threshold=threshold, rows_threshold=None)
        colors[threshold] = pair.color_by_value
    return colors


def lowest_colored_threshold(colors):
    for threshold, color in sorted(colors.items()):
        if color != MetricColor.GRAY:
            return threshold, color
    return 1, MetricColor.GRAY


def format_counts(counts):
    return ", ".join("th={}: {}".format(k, v) for k, v in sorted(counts.items()))


def build_observation_group_data(observations, metric_key):
    data = []

    not_gray_counts = {threshold: 0 for threshold in THRESHOLDS}

    for observation in observations:
        for exp_pair in ExperimentPair.from_observations([observation], fill_result_pairs=True):
            result_pair = exp_pair.result_pairs.get(metric_key)

            if result_pair is None:
                logging.error("    --> observation %s doesn't have metric results", observation)
                continue

            assert result_pair.exp_res.criteria_results, "experiments must have pvalue"

            control_value = result_pair.control_res.metric_values.significant_value
            exp_value = result_pair.exp_res.metric_values.significant_value
            diff = result_pair.get_diff().significant
            pvalue = result_pair.exp_res.criteria_results[0].pvalue
            colors = colors_for_thresholds(result_pair)

            log_level = logging.DEBUG
            for threshold, color in colors.items():
                if color != MetricColor.GRAY:
                    not_gray_counts[threshold] += 1
                    log_level = logging.WARNING

            lowest_color = lowest_colored_threshold(colors)

            data.append({
                "obs_id": observation.id,
                "dates": observation.dates,
                "control_value": control_value,
                "exp_value": exp_value,
                "diff": diff,
                "pvalue": format_num(pvalue, prec=3),
                "color": lowest_color,
                "colors": colors
            })

            logging.log(
                log_level,
                "    --> observation: %s, control: %s, experiment: %s, diff: %s, pvalue: %s, color: %s",
                observation, control_value, exp_value, diff, pvalue, lowest_color
            )

    length = len(data)
    threshold_data = {}
    for threshold, count in not_gray_counts.items():
        left, right = ustat.conf_interval(alpha=0.05, size=length, threshold=threshold)
        threshold_data[threshold] = {
            "left": left,
            "right": right,
            "count": count,
            "in": bool(left <= count <= right)
        }

    logging.info("  --> group total: %s, not gray: %s", length, format_counts(not_gray_counts))

    return {
        "data": data,
        "length": length,
        "threshold_data": threshold_data
    }


def build_control_pool_data(pool):
    all_data = {}

    for metric_key in upool.enumerate_all_metrics(pool):
        logging.info("Checking metric: %s", metric_key)

        one_day, period = split_by_length(pool.observations)

        logging.info("  --> checking long observations")
        long_result = build_observation_group_data(period, metric_key)
        logging.info("  --> checking one day observations")
        one_day_result = build_observation_group_data(one_day, metric_key)

        all_data[metric_key] = {
            "long": long_result,
            "one_day": one_day_result,
        }

    return all_data


def parse_args():
    parser = argparse.ArgumentParser(description="mstand convert pool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    mstand_uargs.add_output_html(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    pool = upool.load_pool(cli_args.input_file)

    template_dir = project_root.get_project_path("templates")
    env = utemplate.get_environment(template_dir)
    env.globals = {"data": build_control_pool_data(pool), "show_all_gray": False}

    html_text = env.get_template("html_control_pool.tpl").render()

    ufile.write_text_file(cli_args.output_html, html_text.encode("utf-8"))


if __name__ == "__main__":
    main()
