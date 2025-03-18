import logging
import sys
from collections import OrderedDict
from collections import defaultdict

import scipy.stats

import mstand_utils.stat_helpers as ustat
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.math_helpers as umath
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
import yaqutils.template_helpers as utemplate
from experiment_pool import MetricColor
from experiment_pool import Pool  # noqa
from user_plugins import PluginKey  # noqa

import project_root


def diff_color(pvalue, threshold, diff):
    if pvalue < threshold:
        if diff > 0:
            return MetricColor.GREEN
        if diff < 0:
            return MetricColor.RED
    return MetricColor.GRAY


def extract_pvalues(pool, metric_key_id_map):
    metric_pvalues = defaultdict(OrderedDict)
    for observation in pool.observations:
        for experiment in observation.experiments:
            for metric_res in experiment.metric_results:
                metric_id = metric_key_id_map[metric_res.metric_key]
                sub_dict = metric_pvalues[metric_id]
                sub_key = (observation.key_without_experiments(), experiment.key())
                for crit_res in metric_res.criteria_results:
                    if crit_res.pvalue is not None:
                        sub_dict[(sub_key, crit_res.key())] = crit_res.pvalue
    return metric_pvalues


def generate_sensitivity_table(pool, metric_key_id_map, min_pvalue=0.001, threshold=0.001):
    """
    :type pool: Pool
    :type metric_key_id_map: dict[PluginKey, int]
    :type min_pvalue: float
    :type threshold: float
    :return: list[dict]
    """

    logging.info("start generate sensitivity table")
    metric_pvalues_by_exp = extract_pvalues(pool, metric_key_id_map)

    metric_avg_s_values = {}
    metric_s_values_by_exp = {}
    metric_kstest = {}
    for metric_id, pvalue_by_exp in usix.iteritems(metric_pvalues_by_exp):
        s_values_by_exp = OrderedDict((key, umath.s_function(pval, min_pvalue))
                                      for key, pval in usix.iteritems(pvalue_by_exp))
        metric_s_values_by_exp[metric_id] = s_values_by_exp

        pvalues = pvalue_by_exp.values()
        avg_s_value = umath.s_function_avg(pvalues, min_pvalue)
        metric_avg_s_values[metric_id] = avg_s_value

        if pvalues:
            _, kstest_pvalue = scipy.stats.kstest(list(pvalues), "uniform", alternative="greater")
        else:
            kstest_pvalue = None
        metric_kstest[metric_id] = kstest_pvalue

    metric_ids = sorted(metric_key_id_map.values())
    table_size = len(metric_ids)
    logging.info("pair_pvalues dict creation started. table size: %d x %d", table_size, table_size)

    pair_pvalues = {}
    for index, (metric_id1, sens_by_exp1) in enumerate(usix.iteritems(metric_s_values_by_exp)):
        if index % 10 == 0:
            logging.info("computing pair pvalues for metric %s of %s", index + 1, len(metric_s_values_by_exp))
        for metric_id2, sens_by_exp2 in usix.iteritems(metric_s_values_by_exp):
            if metric_id1 != metric_id2:
                common_keys = set(sens_by_exp1) & set(sens_by_exp2)
                sens1 = [sens_by_exp1[key] for key in common_keys]
                sens2 = [sens_by_exp2[key] for key in common_keys]
                pair_pvalues[(metric_id1, metric_id2)] = ustat.compare_via_ttest_rel(sens1, sens2)

    logging.info("pair_pvalues dict creation ended")
    logging.info("start table data generation")

    compare_results = []
    for index, metric_id in enumerate(metric_ids):
        if index % 10 == 0:
            logging.info("processing metric %s of %s", index + 1, len(metric_ids))

        sensitivity = metric_avg_s_values.get(metric_id, 0.0)
        kstest = metric_kstest.get(metric_id)
        row_metrics = build_one_sensitivity_table_row(
            metric_id,
            metric_ids,
            metric_avg_s_values,
            pair_pvalues,
            threshold,
        )
        pvalue_count = len(metric_s_values_by_exp.get(metric_id, {}))
        one_compare_result = {
            "metric_id": metric_id,
            "sensitivity": sensitivity,
            "metrics": row_metrics,
            "kstest": kstest,
            "pvalue_count": pvalue_count,
        }
        compare_results.append(one_compare_result)
    logging.info("sensitivity table generated")
    return compare_results


def build_one_sensitivity_table_row(id_control, metric_ids, metric_avg_s_values, metric_pair_pvalues, threshold):
    row_metrics = {}
    value_control = metric_avg_s_values.get(id_control)

    for row_metric_id in metric_ids:
        if row_metric_id == id_control:
            continue

        value = metric_avg_s_values.get(row_metric_id)
        pvalue = metric_pair_pvalues.get((id_control, row_metric_id))
        if value is not None and value_control is not None:
            diff = value_control - value
            diff_percent = 100.0 * diff / value if value else 0.0
            color = diff_color(pvalue, threshold, diff)
        else:
            diff = None
            diff_percent = None
            color = MetricColor.GRAY

        row_metrics[row_metric_id] = {
            "color": color,
            "pvalue": pvalue,
            "diff": diff,
            "diff_percent": diff_percent,
        }
    return row_metrics


def write_json(fd, rows, metric_id_key_map, *_):
    serialized_rows = []
    for row in rows:
        serialized_metrics = {}
        for metric_id, values in usix.iteritems(row["metrics"]):
            values["pvalue"] = umisc.serialize_float(values["pvalue"])
            row_metric_key = metric_id_key_map[metric_id]
            serialized_metrics[row_metric_key.str_key()] = values

        metric_key = metric_id_key_map[row["metric_id"]]
        serialized_row = {
            "metric_key": metric_key.serialize(),
            "sensitivity": row["sensitivity"],
            "metrics": serialized_metrics,
            "kstest": row["kstest"],
            "pvalue_count": row["pvalue_count"],
        }
        serialized_rows.append(serialized_row)
    ujson.dump_to_fd(serialized_rows, fd, sort_keys=True, pretty=True)


def write_tsv(fd, rows, metric_id_key_map, *_):
    # TODO: import csv

    metric_ids = [row['metric_id'] for row in rows]

    metric_names = [metric_id_key_map[mid].pretty_name() for mid in metric_ids]
    header = ["Metric", "Sensitivity", "kstest", "pvalue_count"] + metric_names
    fd.write("\t".join(header))
    fd.write("\n")
    for row in rows:
        metric_key = metric_id_key_map[row["metric_id"]]
        sensitivity_str = "{:.4f}".format(row["sensitivity"])
        kstest = row["kstest"]
        kstest_str = "{:.6f}".format(kstest) if kstest is not None else ""
        current = [
            metric_key.str_key(),
            sensitivity_str,
            kstest_str,
            str(row["pvalue_count"]),
        ]
        for metric_id in metric_ids:
            data = row["metrics"].get(metric_id)
            if not data:
                current.append("")
            else:
                pvalue = data["pvalue"]
                if pvalue:
                    pvalue_str = "{:.4f}".format(pvalue)
                else:
                    pvalue_str = "N/A"
                current.append("{} {} {:.4f} {:.2f}%".format(
                    data["color"],
                    pvalue_str,
                    data["diff"],
                    data["diff_percent"],
                ))
        line = "\t".join(x for x in current)
        fd.write(line)
        fd.write("\n")


def collect_stats(rows):
    """
    :type rows: list[dict]
    :rtype: defaultdict
    """
    stats = defaultdict(lambda: {
        MetricColor.RED: 0,
        MetricColor.GRAY: 0,
        MetricColor.GREEN: 0,
        MetricColor.YELLOW: 0,
        'total': 0
    })
    for row in rows:
        metric_id = row["metric_id"]
        for data in row["metrics"].values():
            stats[metric_id]["total"] += 1
            stats[metric_id][data["color"]] += 1
    return stats


def render_template(name, rows, metric_id_key_map, stats, min_pvalue, threshold, template_dir=None):
    """
    :type name: str
    :type rows: list[dict]
    :type metric_id_key_map: dict[int, PluginKey]
    :type stats: dict[PluginKey, dict]
    :type min_pvalue: float
    :type threshold: float
    :type template_dir: str | None
    :return:
    """
    if template_dir is None:
        template_dir = project_root.get_project_path("templates")
    env = utemplate.get_environment(template_dir)
    return env.get_template(name).render(
        min_pvalue=min_pvalue,
        threshold=threshold,
        rows=rows,
        metric_id_key_map=metric_id_key_map,
        stats=stats
    )


def write_template(fd, name, rows, metric_id_key_map, min_pvalue, threshold):
    """
    :type fd:
    :type name: str
    :type rows: list[dict]
    :type metric_id_key_map: dict[int, PluginKey]
    :type min_pvalue: float
    :type threshold: float
    :rtype:
    """

    stats = collect_stats(rows)
    result = render_template(
        name=name,
        rows=rows,
        metric_id_key_map=metric_id_key_map,
        stats=stats,
        min_pvalue=min_pvalue,
        threshold=threshold
    )
    fd.write(result)


def write_wiki(fd, rows, metric_id_key_map, min_pvalue, threshold):
    write_template(fd, 'wiki_sensitivity.tpl', rows, metric_id_key_map, min_pvalue, threshold)


def write_html(fd, rows, metric_id_key_map, min_pvalue, threshold):
    write_template(fd, 'html_sensitivity.tpl', rows, metric_id_key_map, min_pvalue, threshold)


def dump_sensitivity_table(path, writer_func, rows, metric_id_key_map, min_pvalue, threshold):
    if path is not None:
        if path == "-":
            fd = sys.stdout
        else:
            fd = ufile.fopen_write(path)
        with fd as fd:
            return writer_func(fd, rows, metric_id_key_map, min_pvalue, threshold)
