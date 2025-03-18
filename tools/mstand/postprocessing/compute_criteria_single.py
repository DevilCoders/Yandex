import logging
import math
import random
import time

import numpy

import mstand_utils.stat_helpers as ustat
import postprocessing.criteria_inputs as crit_inputs
import postprocessing.criteria_utils as crit_utils
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
from experiment_pool import CriteriaResult
from experiment_pool import Deviations
from experiment_pool import MetricDataType
from experiment_pool import MetricValueType
from postprocessing import CriteriaContext  # noqa
from postprocessing import CriteriaParamsForAPI


def parse_raw_criteria_result(criteria_key, raw_criteria_result, synthetic):
    """
    :type criteria_key: PluginKey
    :type raw_criteria_result: float | list | tuple | CriteriaResult
    :type synthetic: bool
    :rtype: CriteriaResult
    """

    if isinstance(raw_criteria_result, CriteriaResult):
        # XXX: criteria doesn't have information about synthetic at its context
        raw_criteria_result.synthetic = synthetic
        return raw_criteria_result

    # TODO: use different method to set extra_data (params.set_extra_data for example)
    if isinstance(raw_criteria_result, (list, tuple)):
        if not raw_criteria_result:
            pvalue = None
            extra_data = None
        elif len(raw_criteria_result) == 1:
            pvalue = raw_criteria_result[0]
            extra_data = None
        else:
            assert len(raw_criteria_result) == 2
            pvalue = raw_criteria_result[0]
            extra_data = raw_criteria_result[1]
    else:
        pvalue = raw_criteria_result
        extra_data = None

    return CriteriaResult(criteria_key=criteria_key,
                          pvalue=pvalue,
                          extra_data=extra_data,
                          synthetic=synthetic)


def calc_synthetic_criteria_on_observation(arguments):
    """
    :type arguments: CriteriaContext
    :rtype: Observation
    """
    observation = arguments.observation

    # in synthetic mode, we write results in control's metric results

    if arguments.data_type == MetricDataType.VALUES:
        if observation.experiments:
            raise Exception("Synthetic mode pool (value mode) should not contain experiments")
        for control_mr in observation.control.metric_results:
            synthetic_results = calc_synthetic_criteria_in_value_mode(arguments, control_mr)
            control_mr.criteria_results.extend(synthetic_results)
    else:
        if len(observation.experiments) != 1:
            raise Exception("Synthetic mode pool (key-value mode) should contain exactly one experiment")

        if not observation.all_metric_keys():
            raise Exception("Observation {} has no metrics. Cannot calc synthetic criteria.".format(observation))

        common_metric_keys = observation.common_metric_keys()
        if not common_metric_keys:
            raise Exception("Observation {} has no common metrics. Cannot calc synthetic criteria.".format(observation))

        control_mr_map = observation.control.get_metric_results_map()
        exp_mr_map = observation.experiments[0].get_metric_results_map()
        for metric_key in common_metric_keys:
            control_mr = control_mr_map[metric_key]
            exp_mr = exp_mr_map[metric_key]

            synthetic_results = calc_synthetic_criteria_in_key_value_mode(arguments, control_mr, exp_mr)
            control_mr.criteria_results.extend(synthetic_results)

    return observation


def calc_criteria_on_observation(arguments):
    """
    :type arguments: CriteriaContext
    :rtype: Observation
    """
    observation = arguments.observation
    logging.info("Processing observation %s", observation)
    logging.info("--> Control: %s", observation.control)

    control_metric_results_map = observation.control.get_metric_results_map()
    # outer loop is over metrics (not over experiments) - to load control data for each metric once.
    for metric_key, control_result in usix.iteritems(control_metric_results_map):
        if control_result.metric_values.data_file is None:
            raise Exception("Result for metric {} in {} of observation {} has no data_file".format(
                metric_key, observation.control, observation))
        control_data = crit_inputs.load_metric_values_file(arguments.base_dir,
                                                           control_result.metric_values,
                                                           arguments.read_mode)

        if not observation.experiments:
            logging.warning("Observation has no experiments: %s", observation)

        for experiment in observation.experiments:
            logging.info("--> Processing experiment %s", experiment)
            exp_metric_results_map = experiment.get_metric_results_map()
            if metric_key in exp_metric_results_map:
                exp_result = exp_metric_results_map[metric_key]
                if exp_result.metric_values.data_file is None:
                    raise Exception("Result for metric {} in {} of observation {} has no data_file".format(
                        metric_key, experiment, observation))

                calc_criteria_on_experiment(arguments, control_result, exp_result, control_data)
            else:
                # experiment may have no result due to missing serpset_id, serpset fetch error, etc.
                logging.warning("--> No result in experiment %s for metric %s in obs %s, skipping it",
                                experiment, metric_key, observation)

    # observation is enhanced by criteria results
    return observation


def compute_deviations(data_type, metric_value_type, control_data, exp_data):
    """
    :type data_type: str
    :type metric_value_type: str
    :type control_data: numpy.array list[float]
    :type exp_data: numpy.array list[float]
    :rtype: Deviations
    """
    # use len because of numpy.array
    if len(control_data) == 0 or len(exp_data) == 0:
        return Deviations()

    control_flatten = crit_utils.flatten_array(control_data)
    exp_flatten = crit_utils.flatten_array(exp_data)

    is_sum = (metric_value_type == MetricValueType.SUM)

    std_control = ustat.average_squared_deviation(control_flatten, is_sum)
    std_exp = ustat.average_squared_deviation(exp_flatten, is_sum)

    if data_type == MetricDataType.KEY_VALUES:
        np_control = numpy.array(control_flatten)
        np_exp = numpy.array(exp_flatten)
        std_diff = ustat.average_squared_deviation(np_exp - np_control, is_sum)
    else:
        std_diff = math.sqrt(math.pow(std_control, 2) + math.pow(std_exp, 2))

    return Deviations(std_control=std_control, std_exp=std_exp, std_diff=std_diff)


def calc_criteria_on_experiment(arguments, control_result, exp_result, control_data):
    """
    :type arguments: CriteriaContext
    :type control_result: MetricResult
    :type exp_result: MetricResult
    :type control_data: list | dict
    :rtype: None
    """

    exp_data = crit_inputs.load_metric_values_file(arguments.base_dir,
                                                   exp_result.metric_values,
                                                   arguments.read_mode)

    if control_data is None or exp_data is None:
        exp_result.add_criteria_result(CriteriaResult(criteria_key=arguments.criteria_key, pvalue=float("nan")))
        return

    prep_control_data, prep_exp_data = crit_inputs.prepare_criteria_input(data_type=exp_result.metric_values.data_type,
                                                                          control_data=control_data,
                                                                          exp_data=exp_data,
                                                                          read_mode=arguments.read_mode)

    deviations = compute_deviations(
        data_type=exp_result.metric_values.data_type,
        metric_value_type=exp_result.metric_values.value_type,
        control_data=prep_control_data,
        exp_data=prep_exp_data
    )

    control_filename = control_result.metric_values.data_file
    exp_filename = exp_result.metric_values.data_file

    computation_info = "{} vs {} for metric {}".format(control_filename, exp_filename, exp_result.metric_key)
    logging.info("%s - start criteria", computation_info)

    time_start = time.time()
    is_related = arguments.data_type == MetricDataType.KEY_VALUES
    criteria_params = CriteriaParamsForAPI(
        control_data=prep_control_data,
        exp_data=prep_exp_data,
        control_result=control_result,
        exp_result=exp_result,
        is_related=is_related
    )
    raw_criteria_result = arguments.criteria.value(criteria_params)

    umisc.log_elapsed(time_start, "Criteria %s done", computation_info)

    criteria_result = parse_raw_criteria_result(arguments.criteria_key, raw_criteria_result, synthetic=False)
    criteria_result.deviations = deviations

    exp_result.add_criteria_result(criteria_result)


def calc_synthetic_criteria_in_value_mode(crit_arguments, metric_result):
    """
    :type crit_arguments: CriteriaContext
    :type metric_result: MetricResult
    :rtype: list[CriteriaResult]
    """
    assert metric_result.metric_values.data_type == MetricDataType.VALUES

    synth_count = crit_arguments.synth_count

    metric_data = crit_inputs.load_metric_values_file(crit_arguments.base_dir,
                                                      metric_result.metric_values,
                                                      crit_arguments.read_mode)
    data_size = len(metric_data)
    data_file_name = metric_result.metric_values.data_file

    cr_results = []
    if data_size < 2:
        logging.info("%s - skip", data_file_name)
        return
    time_start = time.time()
    for _ in usix.xrange(synth_count):
        numpy.random.shuffle(metric_data)
        control_data = metric_data[:data_size // 2]
        exp_data = metric_data[data_size // 2:]

        criteria_params = CriteriaParamsForAPI(
            control_data=control_data,
            exp_data=exp_data,
            control_result=metric_result,
            exp_result=metric_result,
            is_related=False
        )
        raw_criteria_result = crit_arguments.criteria.value(criteria_params)

        criteria_result = parse_raw_criteria_result(crit_arguments.criteria_key, raw_criteria_result, synthetic=True)
        cr_results.append(criteria_result)
    umisc.log_elapsed(time_start, "synthetic criteria for %s done", data_file_name)
    return cr_results


def calc_synthetic_criteria_in_key_value_mode(crit_arguments, control_metric_result, exp_metric_result):
    """
    :type crit_arguments: CriteriaContext
    :type control_metric_result: MetricResult
    :type exp_metric_result: MetricResult
    :rtype: list[CriteriaResult]
    """

    assert control_metric_result.metric_values.data_type == MetricDataType.KEY_VALUES
    assert exp_metric_result.metric_values.data_type == MetricDataType.KEY_VALUES

    synth_count = crit_arguments.synth_count

    control_data = crit_inputs.load_metric_values_file(crit_arguments.base_dir,
                                                       control_metric_result.metric_values,
                                                       crit_arguments.read_mode)

    exp_data = crit_inputs.load_metric_values_file(crit_arguments.base_dir,
                                                   exp_metric_result.metric_values,
                                                   crit_arguments.read_mode)

    prep_control_data, prep_exp_data = crit_inputs.prepare_criteria_input(data_type=MetricDataType.KEY_VALUES,
                                                                          control_data=control_data,
                                                                          exp_data=exp_data,
                                                                          read_mode=crit_arguments.read_mode)

    assert len(prep_control_data) == len(prep_exp_data)

    control_data_file = control_metric_result.metric_values.data_file
    exp_data_file = exp_metric_result.metric_values.data_file

    logging.info("start synthetic criteria for %s:%s", control_data_file, exp_data_file)

    cr_results = []

    time_start = time.time()
    for _ in usix.xrange(synth_count):
        synth_control = []
        synth_exp = []
        for control_value, exp_value in zip(prep_control_data, prep_exp_data):
            if random.random() < 0.5:
                synth_control.append(control_value)
                synth_exp.append(exp_value)
            else:
                synth_control.append(exp_value)
                synth_exp.append(control_value)

        criteria_params = CriteriaParamsForAPI(
            control_data=synth_control,
            exp_data=synth_exp,
            control_result=control_metric_result,
            exp_result=exp_metric_result,
            is_related=True
        )

        raw_criteria_result = crit_arguments.criteria.value(criteria_params)
        criteria_result = parse_raw_criteria_result(crit_arguments.criteria_key, raw_criteria_result, synthetic=True)
        cr_results.append(criteria_result)

    if any(result.pvalue == 0 for result in cr_results):
        umisc.log_elapsed(time_start, "synthetic criteria for %s:%s skipped", control_data_file, exp_data_file)
    else:
        umisc.log_elapsed(time_start, "synthetic criteria for %s:%s done", control_data_file, exp_data_file)

    return cr_results
