import itertools
import logging
import os

import adminka.adminka_helpers as admhelp

import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.math_helpers as umath
import yaqutils.misc_helpers as umisc

from correlations import CorrelationPairForAPI
from correlations import CorrelationParamsForAPI
from correlations import CorrelationValuesForAPI
from correlations import MetricDiffForAPI
from correlations import MetricValuesForAPI

from experiment_pool import CriteriaResult  # noqa
from experiment_pool import MetricDiff
from experiment_pool import MetricResult  # noqa
from experiment_pool import MetricValues  # noqa
from experiment_pool import Pool  # noqa

from reports import CorrOutContext  # noqa
from reports import CorrCalcContext  # noqa
from reports import CorrelationParams
from reports import CorrelationResult
from reports import ExperimentPair
from reports import CriteriaPair

from user_plugins import PluginKey


def make_criteria_pair(criteria_left_name, criteria_right_name):
    criteria_left = PluginKey(name=criteria_left_name) if criteria_left_name else None
    criteria_right = PluginKey(name=criteria_right_name) if criteria_right_name else None

    criteria_pair = CriteriaPair(criteria_left=criteria_left, criteria_right=criteria_right)
    return criteria_pair


def find_criteria_result(metric_result, criteria_key):
    """
    :type metric_result: MetricResult
    :type criteria_key: PluginKey
    :rtype: CriteriaResult
    """
    if criteria_key:
        cr_map = metric_result.get_criteria_results_map()
        return cr_map.get(criteria_key)
    else:
        if not metric_result.criteria_results:
            return None
        elif len(metric_result.criteria_results) > 1:
            raise Exception("Only one criteria expected for metric result %s in auto mode", metric_result)
        return metric_result.criteria_results[0]


def get_diff_for_api(control_values, exp_values):
    """"
    :type control_values: MetricValues
    :type exp_values: MetricValues
    :rtype: MetricDiffForAPI
    """
    metric_diff = MetricDiff(control_values, exp_values)
    return MetricDiffForAPI(metric_diff)


def calc_correlation_for_metric_pair(correlation, metric_pair, criteria_pair, exp_pairs, min_pvalue):
    """
    :type correlation: callable
    :type metric_pair: CorrelationResult
    :type criteria_pair: CriteriaPair
    :type exp_pairs: list[ExperimentPair]
    :type min_pvalue: float
    :rtype: CorrelationResult
    """
    corr_pairs = []
    for exp_pair in exp_pairs:
        corr_params = CorrelationParams(exp_pair, metric_pair, criteria_pair)
        corr_pair = make_one_correlation_pair(corr_params)
        if corr_pair is not None:
            corr_pairs.append(corr_pair)
    if not corr_pairs:
        logging.debug("No correlation pairs for %s and %s, skipping", metric_pair, criteria_pair)
        return metric_pair

    pair_number = len(corr_pairs)
    if pair_number < 10:
        logging.warning("Correlation is computed over rather short array of size %d", pair_number)

    params_for_api = CorrelationParamsForAPI(corr_pairs)
    corr_value = correlation(params_for_api)

    left_sensitivity = umath.s_function_avg((pair.left.pvalue for pair in corr_pairs), min_pvalue)
    right_sensitivity = umath.s_function_avg((pair.right.pvalue for pair in corr_pairs), min_pvalue)

    return CorrelationResult(left_metric=metric_pair.left_metric,
                             right_metric=metric_pair.right_metric,
                             corr_value=corr_value,
                             left_sensitivity=left_sensitivity,
                             right_sensitivity=right_sensitivity,
                             additional_info=params_for_api.additional_info)


def build_one_side_data(exp_pair, metric_key, criteria_key, side):
    """
    :type exp_pair: ExperimentPair
    :type metric_key: PluginKey
    :type criteria_key: PluginKey
    :type side: str
    :rtype: CorrelationValuesForAPI
    """
    control_mr = exp_pair.control.find_metric_result(metric_key)
    exp_mr = exp_pair.experiment.find_metric_result(metric_key)

    if not all([control_mr, exp_mr]):
        logging.debug("No metric %s in %s [%s], skipping", metric_key, exp_pair, side)
        return None

    crit_res = find_criteria_result(exp_mr, criteria_key)

    if not crit_res:
        logging.debug("No criteria %s for metric %s in %s [%s]", criteria_key, metric_key, exp_pair, side)
        return None

    metric_diff_for_api = get_diff_for_api(control_mr.metric_values, exp_mr.metric_values)
    control_values_for_api = MetricValuesForAPI(control_mr.metric_values)
    exp_values_for_api = MetricValuesForAPI(exp_mr.metric_values)

    corr_vals = CorrelationValuesForAPI(control_values=control_values_for_api,
                                        exp_values=exp_values_for_api,
                                        metric_diff=metric_diff_for_api,
                                        pvalue=crit_res.pvalue,
                                        deviations=crit_res.deviations)
    return corr_vals


def make_one_correlation_pair(corr_params):
    """
    :type corr_params: CorrelationParams
    :rtype: CorrelationPairForAPI
    """
    metric_pair = corr_params.metric_pair
    criteria_pair = corr_params.criteria_pair

    left_corr_values = build_one_side_data(exp_pair=corr_params.exp_pair,
                                           metric_key=metric_pair.left_metric,
                                           criteria_key=criteria_pair.criteria_left,
                                           side="left")

    if not left_corr_values:
        logging.debug("Cannot find 'left' criteria (exp. pair %s, metric %s, criteria %s)",
                      corr_params.exp_pair,
                      metric_pair.left_metric,
                      criteria_pair.criteria_left)
        return None

    right_corr_values = build_one_side_data(exp_pair=corr_params.exp_pair,
                                            metric_key=metric_pair.right_metric,
                                            criteria_key=criteria_pair.criteria_right,
                                            side="right")

    if not right_corr_values:
        logging.debug("Cannot find 'right' criteria (exp. pair %s, metric %s, criteria %s)",
                      corr_params.exp_pair,
                      metric_pair.right_metric,
                      criteria_pair.criteria_right)
        return None

    logging.debug("Pair constructed OK, left: %s, right: %s", left_corr_values, right_corr_values)
    return CorrelationPairForAPI(left_corr_values, right_corr_values)


def calc_correlation(exp_pairs, correlation, metric_keys, corr_calc_ctx):
    """
    :type exp_pairs: list[ExperimentPair]
    :type correlation: callable
    :type metric_keys: set[PluginKey]
    :type corr_calc_ctx: CorrCalcContext
    :rtype: list[CorrelationResult]
    """

    main_metric_key = corr_calc_ctx.main_metric_key
    criteria_pair = corr_calc_ctx.criteria_pair
    min_pvalue = corr_calc_ctx.min_pvalue

    results = []
    for metric_key_left, metric_key_right in itertools.product(metric_keys, repeat=2):
        if main_metric_key and main_metric_key not in (metric_key_left, metric_key_right):
            logging.debug("Skipped pair %s vs %s due to main metric filter", metric_key_left, metric_key_right)
            continue

        logging.info("Computing correlation for %s vs %s", metric_key_left, metric_key_right)
        metric_pair = CorrelationResult(metric_key_left, metric_key_right)
        corr_result = calc_correlation_for_metric_pair(correlation, metric_pair, criteria_pair, exp_pairs, min_pvalue)
        logging.info("Correlation: %s", corr_result)
        results.append(corr_result)
    results.sort()
    return results


def calc_correlation_main(correlation, pool, corr_calc_ctx, corr_out_ctx, adminka_session):
    """
    :type correlation: callable
    :type pool: Pool
    :type corr_calc_ctx: CorrCalcContext
    :type corr_out_ctx: CorrOutContext
    :type adminka_session:
    :rtype: None
    """

    check_metrics_and_criterias(pool, corr_calc_ctx)
    exp_pairs = ExperimentPair.from_pool(pool)

    metric_keys = pool.all_metric_keys()

    results = calc_correlation(exp_pairs, correlation, metric_keys, corr_calc_ctx)
    serialized_results = dump_correlation_results(exp_pairs, results, corr_calc_ctx, corr_out_ctx, adminka_session)
    return serialized_results


def dump_correlation_results(exp_pairs, results, corr_calc_ctx, corr_out_ctx, adminka_session):
    if corr_out_ctx.output_tsv:
        dump_correlation_to_tsv(results, corr_calc_ctx.main_metric_key, corr_out_ctx.output_tsv)

    if corr_out_ctx.save_to_dir:
        dump_correlation_to_dir(exp_pairs, results, corr_calc_ctx, corr_out_ctx, adminka_session)

    sorted_results = CorrelationResult.sorted_by_value(results)
    serialized_results = umisc.serialize_array(sorted_results)
    # cannot use order from OrderedDict in ujson https://github.com/esnme/ultrajson/issues/55
    ujson.dump_to_file(serialized_results, corr_out_ctx.output_file)
    return serialized_results


def check_metrics_and_criterias(pool, corr_calc_ctx):
    main_metric_key = corr_calc_ctx.main_metric_key
    crit_pair = corr_calc_ctx.criteria_pair

    all_metric_keys = pool.all_metric_keys()
    if main_metric_key:
        if main_metric_key not in all_metric_keys:
            raise Exception("Main metric %s not found in pool.", main_metric_key)

    if not all_metric_keys:
        raise Exception("No metrics were calculated in pool. Cannot compute correlation.")

    all_criterias = pool.all_criteria_keys()

    if not all_criterias:
        raise Exception("No criterias were calculated in pool. Cannot compute correlation.")

    if crit_pair.criteria_left:
        if crit_pair.criteria_left not in all_criterias:
            raise Exception("Left criteria {} not calculated in pool".format(crit_pair.criteria_left))
    else:
        logging.info("No left criteria specified, taking first criteria for each metric result.")

    if crit_pair.criteria_right:
        if crit_pair.criteria_right not in all_criterias:
            raise Exception("Right criteria {} not calculated in pool".format(crit_pair.criteria_right))
    else:
        logging.info("No right criteria specified, taking first criteria for each metric result.")


def dump_correlation_to_tsv(corr_results, main_metric_key, output_tsv):
    """
    :type corr_results: list[CorrelationResult]
    :type main_metric_key: PluginKey
    :type output_tsv: str
    :rtype:
    """

    if not main_metric_key:
        error_msg = "TSV output without --main-metric is not supported."
        logging.error(error_msg)
        with open(output_tsv, "w") as tsv_fd:
            tsv_fd.write(error_msg + "\n")
        return

    with open(output_tsv, "w") as tsv_fd:
        tsv_fd.write(CorrelationResult.tsv_header() + "\n")

        for corr_result in corr_results:
            if corr_result.left_metric == main_metric_key and corr_result.corr_value is not None:
                tsv_fd.write(corr_result.tsv_line() + "\n")

        for corr_result in corr_results:
            if corr_result.right_metric == main_metric_key and corr_result.corr_value is not None:
                tsv_fd.write(corr_result.tsv_line() + "\n")


def dump_correlation_to_dir(exp_pairs, corr_results, corr_calc_ctx, corr_out_ctx, adminka_session):
    """
    :type exp_pairs: list[ExperimentPair]
    :type corr_results: list[CorrelationResult]
    :type corr_calc_ctx: CorrCalcContext
    :type corr_out_ctx: CorrOutContext
    :type adminka_session:
    :rtype:
    """

    save_to_dir = corr_out_ctx.save_to_dir
    main_metric_key = corr_calc_ctx.main_metric_key
    criteria_pair = corr_calc_ctx.criteria_pair

    ufile.make_dirs(save_to_dir)
    main_tsv_file = os.path.join(save_to_dir, "correlation.tsv")
    dump_correlation_to_tsv(corr_results, main_metric_key, main_tsv_file)

    if not main_metric_key:
        logging.error("directory output without --main-metric is not supported.")
        return

    details_dir = os.path.join(save_to_dir, "details")
    ufile.make_dirs(details_dir)

    for corr_result in corr_results:
        if corr_result.corr_value is None:
            continue
        metric_left = corr_result.left_metric
        metric_right = corr_result.right_metric
        if metric_left == main_metric_key or metric_right == main_metric_key:
            file_name = "{} vs {}.tsv".format(metric_left.str_key(), metric_right.str_key())
            file_path = os.path.join(details_dir, file_name)
            dump_detailed_to_tsv(exp_pairs, corr_result, criteria_pair, file_path, adminka_session)


def dump_detailed_to_tsv(exp_pairs, metric_pair, criteria_pair, file_path, adminka_session):
    """
    :type exp_pairs: list[ExperimentPair]
    :type metric_pair: CorrelationResult
    :type criteria_pair: CriteriaPair
    :type file_path: str
    :type adminka_session: adminka.ab_cache.AdminkaCachedApi
    :rtype:
    """

    metric_left = metric_pair.left_metric
    metric_right = metric_pair.right_metric

    logging.debug("Getting details for %s vs %s", metric_left, metric_right)

    tsv_header = [
        "observation_id",
        "ticket",
        "ctrl_testid",
        "exp_testid",
        "ctrl_serpset_id",
        "exp_serpset_id",
        "delta_left",
        "std_diff_left",
        "ctrl_left",
        "delta_right",
        "std_diff_right",
        "ctrl_right",
    ]

    with open(file_path, "w") as tsv_fd:
        tsv_fd.write("\t".join(tsv_header) + "\n")

        for exp_pair in exp_pairs:

            corr_params = CorrelationParams(exp_pair, metric_pair, criteria_pair)
            corr_pair = make_one_correlation_pair(corr_params)
            if corr_pair is None:
                logging.warning("No correlation pair for e:%s, m:%s c:%s", exp_pair, metric_pair, criteria_pair)
                continue

            ticket = admhelp.fetch_abt_experiment_field(adminka_session, exp_pair.experiment, "ticket")

            dump_one_corr_pair(corr_pair, corr_params, ticket, tsv_header, tsv_fd)


def dump_one_corr_pair(corr_pair, corr_params, ticket, tsv_header, tsv_fd):
    """
    :type corr_pair: CorrelationPairForAPI
    :type corr_params: CorrelationParams
    :type ticket: str
    :type tsv_header: list[str]
    :type tsv_fd:
    :rtype:
    """
    observation = corr_params.exp_pair.observation
    control = corr_params.exp_pair.control
    experiment = corr_params.exp_pair.experiment

    delta_left = corr_pair.left.metric_diff.abs_diff
    delta_right = corr_pair.right.metric_diff.rel_diff
    std_diff_left = corr_pair.left.deviations.std_diff
    std_diff_right = corr_pair.right.deviations.std_diff
    # significant values
    ctrl_left = corr_pair.left.control_values.value
    ctrl_right = corr_pair.right.control_values.value

    tsv_row = [
        observation.id,
        ticket or "",
        control.testid or "",
        experiment.testid or "",
        control.serpset_id or "",
        experiment.serpset_id or "",
        delta_left,
        std_diff_left,
        ctrl_left,
        delta_right,
        std_diff_right,
        ctrl_right,
    ]
    assert len(tsv_header) == len(tsv_row)
    line = "\t".join(str(x) for x in tsv_row)
    tsv_fd.write(line + "\n")
