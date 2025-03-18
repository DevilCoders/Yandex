# coding=utf-8
import logging
import os.path
import random
import shutil
import time
import traceback

import postprocessing.postproc_helpers as pp_helpers
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
from experiment_pool import Experiment
from experiment_pool import MetricDataType
from experiment_pool import Observation
from experiment_pool import Pool
from experiment_pool import pool_helpers
from postprocessing import ExperimentForPostprocessAPI
from postprocessing import ObservationForPostprocessAPI
from postprocessing import PoolForPostprocessAPI
from postprocessing import PostprocExperimentContext
from postprocessing import PostprocGlobalContext
from postprocessing import PostprocObservationContext


def merge_observation_results(observations):
    """
    :type observations: list[Observation]
    :rtype: Observation
    """
    if not observations:
        raise Exception("Observation list for merging is empty")

    res_obs = observations.pop(0)

    for obs in observations:
        assert obs.id == res_obs.id
        pool_helpers.merge_observations(obs, res_obs)
    return res_obs


class ObservationPostprocessor(object):
    def __init__(self, observation_ctx):
        """
        :type observation_ctx: PostprocObservationContext
        """
        self.observation_ctx = observation_ctx

    @property
    def postprocessor(self):
        return self.observation_ctx.postprocessor

    def postprocess_observation(self):
        random.seed(self.observation_ctx.random_seed)

        observation = self.observation_ctx.observation

        if pp_helpers.is_observation_processor(self.postprocessor):

            if not observation.all_metric_keys():
                raise Exception("Observation {} has no metrics. Cannot postprocess it.".format(observation))

            common_metric_keys = observation.common_metric_keys()
            if not common_metric_keys:
                raise Exception("Observation {} has no common metrics. Cannot postprocess it.".format(observation))

            new_observations = []

            for metric_key in common_metric_keys:
                logging.info("Processing metric %s (by observation)", metric_key)
                obs_for_api = ObservationForPostprocessAPI(self.observation_ctx, metric_key)
                self.postprocessor.process_observation(obs_for_api)

                if self.observation_ctx.global_ctx.preserve_original_names:
                    new_metric_key = metric_key
                else:
                    new_metric_key = pp_helpers.make_metric_key(metric_key, self.observation_ctx.pp_key)
                new_observation = obs_for_api.make_new_observation(new_metric_key)
                new_observations.append(new_observation)

            # collect common metric results to one observation back
            return merge_observation_results(new_observations)
        else:
            control_ctx = PostprocExperimentContext(self.observation_ctx,
                                                    observation.control,
                                                    "control")
            control_pp = ExperimentPostprocessor(control_ctx)
            new_control = control_pp.postprocess_experiment()

            new_experiments = []
            for pos, exp in umisc.aligned_str_enumerate(observation.experiments):
                experiment_ctx = PostprocExperimentContext(self.observation_ctx,
                                                           exp,
                                                           "exp" + pos)
                experiment_pp = ExperimentPostprocessor(experiment_ctx)
                new_exp = experiment_pp.postprocess_experiment()
                new_experiments.append(new_exp)

            return Observation(obs_id=observation.id,
                               dates=observation.dates,
                               sbs_ticket=observation.sbs_ticket,
                               sbs_workflow_id=observation.sbs_workflow_id,
                               sbs_metric_results=observation.sbs_metric_results,
                               tags=observation.tags,
                               extra_data=observation.extra_data,
                               control=new_control,
                               experiments=new_experiments)


class ExperimentPostprocessor(object):
    def __init__(self, experiment_ctx):
        """
        :type experiment_ctx: PostprocExperimentContext
        """
        self.experiment_ctx = experiment_ctx

    @property
    def postprocessor(self):
        return self.experiment_ctx.postprocessor

    def postprocess_experiment(self):
        experiment = self.experiment_ctx.experiment
        logging.info("Processing experiment %s", experiment)

        metric_results_map = experiment.get_metric_results_map()

        new_metric_results = []
        for metric_key, metric_result in usix.iteritems(metric_results_map):
            if metric_result.metric_values.value_type == MetricDataType.KEY_VALUES:
                logging.warning("Cannot postprocess metric results with key-value data.")
                continue

            logging.info("Processing metric %s in experiment %s", metric_key, experiment)
            new_metric_result = self.postprocess_metric_result(metric_key, metric_result)
            new_metric_results.append(new_metric_result)

        return Experiment(testid=experiment.testid,
                          serpset_id=experiment.serpset_id,
                          sbs_system_id=experiment.sbs_system_id,
                          extra_data=experiment.extra_data,
                          errors=experiment.errors,
                          metric_results=new_metric_results)

    def postprocess_metric_result(self, metric_key, metric_result):
        if not pp_helpers.is_experiment_processor(self.postprocessor):
            raise Exception("Postprocessor has no process_experiment method.")

        exp_for_api = ExperimentForPostprocessAPI(experiment_ctx=self.experiment_ctx,
                                                  metric_key=metric_key,
                                                  metric_result=metric_result)
        try:
            self.postprocessor.process_experiment(exp_for_api)
        except Exception as exc:
            logging.error("Experiment postprocessing failed: %s", exc)
            raise

        if self.experiment_ctx.observation_ctx.global_ctx.preserve_original_names:
            new_metric_key = metric_key
        else:
            new_metric_key = pp_helpers.make_metric_key(metric_key, self.experiment_ctx.observation_ctx.pp_key)
        return exp_for_api.make_new_metric_result(new_metric_key)


def mp_postprocess_observation(arg):
    """
    :type arg: PostprocObservationContext
    :rtype: tuple[PostprocObservationContext, Observation] | tuple[PostprocObservationContext, BaseException]
    """
    try:
        observation_ctx = arg
        observation = observation_ctx.observation
        logging.info("Starting worker with %s", observation)

        obs_pp = ObservationPostprocessor(observation_ctx)
        return arg, obs_pp.postprocess_observation()
    except BaseException as exc:
        logging.error("Error occured in worker: %s, details: %s", exc, traceback.format_exc())
        return arg, exc


def postprocess_pool(pool, global_ctx, pp_instance, pp_id, pp_key, max_threads=None):
    """
    :type pp_instance: callable
    :type pp_id: int
    :type pp_key: user_plugins.PluginKey
    :type pool: Pool
    :type global_ctx: PostprocGlobalContext
    :type max_threads: int | None
    :rtype:
    """
    ufile.make_dirs(global_ctx.dest_dir)

    seed = time.time()
    logging.info("Using random seed %s", seed)
    random.seed(seed)

    if pp_helpers.is_pool_processor(pp_instance):
        logging.info("Using totally custom pool postprocessing")

        pool_api = PoolForPostprocessAPI(pool=pool, source_dir=global_ctx.source_dir,
                                         dest_dir=global_ctx.dest_dir)
        result_pool = pp_instance.process_pool(pool_api)

        if not isinstance(result_pool, PoolForPostprocessAPI):
            raise Exception("Postprocess API error: process_pool should return PoolForPostprocessAPI class.")
        return result_pool.pool
    else:
        logging.info("Using observation/experiment postprocessing")
        # fork by observations postprocess
        filename_prefix = "postproc_{}_obs".format(pp_id)
        arg_list = [PostprocObservationContext(global_ctx, obs, filename_prefix + pos, pp_instance, pp_key)
                    for pos, obs in umisc.aligned_str_enumerate(pool.observations)]
        logging.info("Running postprocessing for %d observation contexts", len(arg_list))

        new_observations = []
        custom_files = []

        for (obs_ctx, res) in umisc.par_imap(mp_postprocess_observation, arg_list, max_threads):
            if isinstance(res, BaseException):
                raise res
            if obs_ctx.custom_files:
                logging.info("Files added from %s: %s", obs_ctx.observation, obs_ctx.custom_files)
                custom_files.extend(obs_ctx.custom_files)
            new_observations.append(res)
        global_ctx.custom_files = custom_files
        return Pool(new_observations)


def postprocess_results_core(global_ctx, metrics_filter=None, max_threads=None):
    """
    :type global_ctx: PostprocGlobalContext
    :type metrics_filter: MetricFilter
    :type max_threads: int
    :rtype: Pool
    """
    pool_file_name = "pool.json"
    pool = pool_helpers.load_pool(os.path.join(global_ctx.source_dir, pool_file_name))

    global_ctx.pool_extra_data = pool.extra_data

    if metrics_filter:
        metrics_filter.filter_metric_for_pool(pool)

    postproc_container = global_ctx.postproc_container

    new_pool_arr = []

    for pp_id, pp_instance in usix.iteritems(postproc_container.plugin_instances):
        pp_key = postproc_container.plugin_key_map[pp_id]
        logging.info("Postprocessing %s with id %d", pp_key, pp_id)
        new_pool = postprocess_pool(pool, global_ctx, pp_instance, pp_id, pp_key, max_threads)
        new_pool_arr.append(new_pool)

    new_pool = pool_helpers.merge_pools(new_pool_arr)
    pool_helpers.dump_pool(new_pool, os.path.join(global_ctx.dest_dir, pool_file_name))
    return new_pool


def postprocess_and_repack(input_path, dest_dir, dest_tar, pp_container, threads,
                           metrics_filter=None, preserve_original_names=False):
    """
    :type input_path: str
    :type dest_dir: str
    :type dest_tar: str
    :type pp_container: user_plugins.PluginContainer
    :type threads: int
    :type metrics_filter: experiment_pool.filter_metric.MetricFilter
    :type preserve_original_names: bool
    :rtype: Pool
    """
    if os.path.isdir(input_path):
        logging.info("Input is directory")
        source_dir = input_path
        source_dir_is_tmp = False
    else:
        logging.info("Input is TAR archive")
        source_dir = ufile.extract_tar_to_temp(input_path)
        source_dir_is_tmp = True

    if dest_dir:
        dest_dir_is_tmp = False
        ufile.make_dirs(dest_dir)
    else:
        if not dest_tar:
            raise Exception("Please set --save-to-dir or --save-to-tar option.")

        dest_dir = ufile.create_temp_dir(prefix="postproc_", subdir="mstand")
        dest_dir_is_tmp = True

    start_time = time.time()

    ctx = PostprocGlobalContext(pp_container=pp_container,
                                source_dir=source_dir,
                                dest_dir=dest_dir,
                                preserve_original_names=preserve_original_names)

    if check_disable_threads(pp_container):
        threads = 1

    pool = postprocess_results_core(ctx, metrics_filter=metrics_filter, max_threads=threads)

    umisc.log_elapsed(start_time, "pure postprocessing time")

    if dest_tar:
        ufile.tar_directory(path_to_pack=dest_dir, tar_name=dest_tar, dir_content_only=True)

    if source_dir_is_tmp:
        logging.info("removing tmp src dir: %s", source_dir)
        shutil.rmtree(source_dir)

    if dest_dir_is_tmp:
        logging.info("removing tmp dest dir: %s", dest_dir)
        shutil.rmtree(dest_dir)

    return pool


def check_disable_threads(pp_container):
    disabled_threads = False
    for pp_id, pp_instance in usix.iteritems(pp_container.plugin_instances):
        if getattr(pp_instance, "disable_threads", False):
            logging.info("plugin %s has disable_threads", pp_container.plugin_key_map[pp_id].pretty_name())
            disabled_threads = True
    return disabled_threads
