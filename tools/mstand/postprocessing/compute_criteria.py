import itertools
import logging
import math
import traceback
from collections import defaultdict

import mstand_utils.stat_helpers as ustat
import postprocessing.compute_criteria_single as comp_crit_single
import reports.metrics_compare as rmc
import yaqutils.math_helpers as umath
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv
import yaqutils.six_helpers as usix
from experiment_pool import ColoringInfo
from experiment_pool import ExperimentForCalculation
from experiment_pool import MetricDataType
from experiment_pool import Observation
from experiment_pool import Pool
from experiment_pool import SyntheticSummary
from postprocessing import CriteriaContext
from postprocessing.criteria_read_mode import CriteriaReadMode
from user_plugins import PluginKey  # noqa


def run_criteria(arguments):
    """
    :type arguments: CriteriaContext
    :rtype: Observation
    """
    try:
        if arguments.synth_count is not None:
            return comp_crit_single.calc_synthetic_criteria_on_observation(arguments)
        else:
            return comp_crit_single.calc_criteria_on_observation(arguments)
    except Exception as exc:
        logging.error("Run_criteria on observation %s failed: %s, %s", arguments.observation, exc,
                      traceback.format_exc())
        raise


def calc_criteria(criteria, criteria_key, pool, base_dir, data_type, synthetic=None, threads=None):
    """
    :type criteria: callable
    :type criteria_key: PluginKey
    :type pool: Pool
    :type base_dir: str
    :type data_type: str
    :type synthetic: int | None
    :type threads: int
    :rtype: Pool
    """

    obs_count = len(pool.observations)
    logging.info("computing criteria for %d observations", obs_count)

    if synthetic and obs_count:
        synth_count = int(math.ceil(float(synthetic) / obs_count))
        logging.info("will generate %d * %d = %d synthetic controls", synth_count, obs_count, synth_count * obs_count)
    else:
        synth_count = None

    read_mode = CriteriaReadMode.from_criteria(criteria)
    args = []
    for obs in pool.observations:
        one_arg = CriteriaContext(criteria=criteria,
                                  criteria_key=criteria_key,
                                  observation=obs,
                                  base_dir=base_dir,
                                  data_type=data_type,
                                  read_mode=read_mode,
                                  synth_count=synth_count)
        args.append(one_arg)

    result_observations = []
    for pos, observation in enumerate(umisc.par_imap(run_criteria, args, threads)):
        umisc.log_progress("criteria calculation", pos, obs_count)
        unirv.log_nirvana_progress("criteria calculation", pos, obs_count)
        result_observations.append(observation)
    return Pool(result_observations, meta=pool.meta)


def build_synthetic_pool(pool, data_type):
    """
    :type pool: Pool
    :type data_type: str
    :rtype: Pool
    """

    if data_type == MetricDataType.VALUES:
        logging.info("Building synthetic pool for 'values' data type")
        unique_exps = ExperimentForCalculation.from_pool(pool, experiment_results=True)
        synth_observations = []
        for exp_for_calc in unique_exps:
            one_obs = Observation(control=exp_for_calc.experiment,
                                  experiments=[],
                                  dates=exp_for_calc.dates,
                                  obs_id=exp_for_calc.observation.id)
            synth_observations.append(one_obs)
        return Pool(synth_observations)
    elif data_type == MetricDataType.KEY_VALUES:
        logging.info("Building synthetic pool for 'key-values' data type")
        pairs = []
        for obs in pool.observations:
            for exp in obs.experiments:
                one_pair = Observation(control=obs.control,
                                       experiments=[exp],
                                       dates=obs.dates,
                                       obs_id=obs.id)
                pairs.append(one_pair)
        return Pool(pairs)
    else:
        raise Exception("Unsupported metric data type: {}".format(data_type))


def fill_colorings(colored_counts, total_count):
    """
    :type colored_counts: dict[float, int]
    :type total_count: int
    :return:
    """
    colorings = []
    for threshold in sorted(colored_counts.keys()):
        colored_count = colored_counts[threshold]
        count_relative = float(colored_count) / total_count
        coloring_for_threshold = {
            "threshold": threshold,
            "colored_count": colored_count,
            "count_relative": count_relative,
            "conf_status": ustat.check_confidence_interval(total_count, colored_count, threshold)
        }
        colorings.append(coloring_for_threshold)
    return colorings


def calc_summary_single(pvalues):
    """
    :type pvalues: list[float]
    :rtype: ColoringInfo
    """

    def count_while(func, values):
        return sum(1 for _ in itertools.takewhile(func, values))

    total_count = len(pvalues)

    if any([math.isnan(x) or math.isinf(x) for x in pvalues]):
        logging.warning("pvalues array has NaN/inf values: %s", pvalues)

    colored_counts = {threshold: count_while(lambda p: p < threshold, sorted(pvalues))
                      for threshold in [0.001, 0.005, 0.01, 0.05, 0.1]}

    colorings = fill_colorings(colored_counts, total_count)

    sensitivity = umath.s_function_avg(pvalues)
    return ColoringInfo(total_count, colorings, sensitivity)


def calc_synthetic_summaries(observations, data_type):
    """
    :type observations: list[Observation]
    :type data_type: str
    :rtype: list[SyntheticSummary]
    """
    pvalues_map = defaultdict(list)

    for observation in observations:
        if data_type == MetricDataType.VALUES:
            assert not observation.experiments
        else:
            assert len(observation.experiments) == 1

        for metric_result in observation.control.metric_results:
            for criteria_result in metric_result.criteria_results:
                pair_key = metric_result.metric_key, criteria_result.criteria_key
                pvalues_map[pair_key].append(criteria_result.pvalue)

    results = []
    for pair_key, pvalues in usix.iteritems(pvalues_map):
        metric_key, criteria_key = pair_key
        coloring_info = calc_summary_single(pvalues)
        one_result = SyntheticSummary(metric_key, criteria_key, coloring_info)
        results.append(one_result)
    return results


def save_summary(pool, output_path, session):
    threshold = 0.01

    try:
        session.preload_testids(pool.all_testids())

        env = rmc.create_template_environment(
            pool=pool,
            session=session,
            validation_result=None,
            threshold=threshold,
            show_all_gray=True,
            show_same_color=True,
        )
        output_html = env.get_template("html_metrics_vertical.tpl").render()
    except Exception as exc:
        message = "Cannot create summary table:\n{}\n\n{}".format(exc, traceback.format_exc())
        logging.error("%s", message)
        output_html = message

    rmc.write_markup(output_path, output_html)
