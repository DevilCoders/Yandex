# -*- coding: utf-8 -*-
import functools
import itertools
import logging
import os
import traceback

import experiment_pool.pool_helpers as pool_helpers
import mstand_metric_helpers.online_metric_helpers as online_mhelp
import session_metric.metric_quick_check as mqc
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv
import yaqutils.six_helpers as usix
import yaqutils.time_helpers as utime

from experiment_pool import ExperimentForCalculation
from mstand_enums.mstand_online_enums import ServiceEnum
from mstand_structs.squeeze_versions import SqueezeVersions
from session_metric.metric_calculator import MetricCalculator, MetricSources
from user_plugins import PluginContainer


class MetricRunner(object):
    def __init__(
            self,
            metric_container,
            squeeze_path,
            calc_backend,
            history=None,
            future=None,
            use_buckets=None,
            check_versions=False,
            min_versions=None,
            use_filters=False,
    ):
        """
        :type metric_container: PluginContainer
        :type squeeze_path: str
        :type calc_backend: any
        :type history: int | None
        :type future: int | None
        :type use_buckets: bool
        :type check_versions: bool
        :type min_versions: SqueezeVersions
        :type use_filters: bool
        """
        assert isinstance(metric_container, PluginContainer)
        self.metric_container = metric_container
        self.squeeze_path = squeeze_path

        assert callable(calc_backend.calc_metric_batch)
        assert callable(calc_backend.find_bad_tables)
        assert callable(calc_backend.get_all_versions)
        self.calc_backend = calc_backend

        self.history = history
        if self.history:
            assert self.history > 0
        self.future = future
        if self.future:
            assert self.future > 0

        self.use_buckets = use_buckets
        self.check_versions = check_versions
        self.min_versions = min_versions if min_versions else SqueezeVersions()

        self.detect_buckets()
        self.use_filters = use_filters

    def detect_buckets(self):
        if self.use_buckets is None:  # no need for loop if use_buckets == True|False
            if not self.metric_container.plugin_instances:
                self.use_buckets = False
            else:
                bucket_using_metrics = 0
                for metric_instance in usix.itervalues(self.metric_container.plugin_instances):
                    if online_mhelp.is_bucket_metric(metric_instance):
                        bucket_using_metrics += 1

                if bucket_using_metrics == len(self.metric_container.plugin_instances):
                    logging.warning("Enabling --buckets explicitly because all of the metrics use them")
                    self.use_buckets = True
                elif bucket_using_metrics == 0:
                    logging.warning("Disabling --buckets explicitly because none of the metrics use them")
                    self.use_buckets = False
                else:
                    raise Exception("Use buckets in every metric or in none of them")

    @staticmethod
    def from_cli_args(cli_args, metric_container, calc_backend):
        min_versions_json = ujson.load_from_str(cli_args.min_versions.strip()) if cli_args.min_versions else {}
        min_versions = SqueezeVersions.deserialize(min_versions_json)

        return MetricRunner(
            metric_container=metric_container,
            squeeze_path=cli_args.squeeze_path,
            calc_backend=calc_backend,
            history=cli_args.history,
            future=cli_args.future,
            use_buckets=cli_args.buckets,
            check_versions=cli_args.check_versions,
            min_versions=min_versions,
            use_filters=getattr(cli_args, "use_filters", False),
        )

    def check_metrics_quick(self, services):
        logging.info("Testing metrics on sample data...")

        for metric_id, metric_instance in usix.iteritems(self.metric_container.plugin_instances):
            metric_key = self.metric_container.plugin_key_map[metric_id]
            logging.info("Checking metric %s", metric_key)
            mqc.check_metric_quick(metric_instance, services)

            if self.history or self.future:
                if online_mhelp.is_session_metric(metric_instance):
                    raise Exception("Error in {} \n"
                                    "Cannot use session or request metrics with --history and --future"
                                    .format(metric_key.name))

            logging.info("Metric %s is OK", metric_key)

        logging.info("Metrics self-test passed OK")

    def get_exps_from_obs(self, observation):
        """
        :type observation: Observation
        :rtype: list[ExperimentForCalculation]
        """
        obs_exps = ExperimentForCalculation.from_observation(observation)
        if obs_exps:
            obs_exps = sorted(obs_exps)
        return obs_exps

    def get_exps_from_pool(self, pool):
        """
        :type pool: Pool
        :rtype: list[list[ExperimentForCalculation]]
        """
        experiments_by_obs = []
        for observation in pool.observations:
            obs_exps = self.get_exps_from_obs(observation)
            if obs_exps:
                experiments_by_obs.append(obs_exps)
        return experiments_by_obs

    def calc_for_pool(self, pool, save_to_dir=None, save_to_tar=None,
                      threads=1, batch_min=1, batch_max=1):
        """
        :type pool: Pool
        :type save_to_dir: str | None
        :type save_to_tar: str | None
        :type threads: int
        :type batch_min: int
        :type batch_max: int
        :rtype: dict[ExperimentForCalculation, list[MetricResult]]
        """

        experiments_by_obs = self.get_exps_from_pool(pool)

        if not save_to_dir and save_to_tar:
            save_to_dir = "{}.content".format(save_to_tar)

        experiments = sorted(set(itertools.chain.from_iterable(experiments_by_obs)))
        if save_to_dir:
            ufile.make_dirs(save_to_dir)
            metric_keys = self.metric_container.plugin_key_map
            result_files = generate_metric_result_file_paths(metric_keys, save_to_dir, experiments)
        else:
            result_files = {}

        if not self.metric_container.plugin_keys:
            logging.info("Found no metrics to calculate, skipping YT calculations")
            results = {}
            for expr in experiments:
                results[expr] = []
        else:
            for obs in pool.observations:
                self.check_metrics_quick(obs.services)
                self._check_required_services(obs.services)
            results = self._calc_many(
                experiments_by_obs=experiments_by_obs,
                result_files=result_files,
                threads=threads,
                batch_min=batch_min,
                batch_max=batch_max,
            )

        for observation in pool.observations:
            if observation.control.testid:
                control_key = ExperimentForCalculation(observation.control, observation)
                observation.control.add_metric_results(results[control_key])
            for experiment in observation.experiments:
                if experiment.testid:
                    experiment_key = ExperimentForCalculation(experiment, observation)
                    experiment.add_metric_results(results[experiment_key])

        if save_to_dir:
            pool_filename = "pool.json"
            pool_path = os.path.join(save_to_dir, pool_filename)
            logging.info("Saving result pool to %s", pool_path)
            pool_helpers.dump_pool(pool, pool_path)
            if save_to_tar:
                save_metric_results_to_tar(save_to_dir, save_to_tar)
        return results

    def _calc_many(self, experiments_by_obs, result_files, threads, batch_min, batch_max):
        """
        :type experiments_by_obs: list[list[ExperimentForCalculation]]
        :type result_files: dict[ExperimentForCalculation, dict[int, str]]
        :type threads: int
        :type batch_min: int
        :type batch_max: int
        :rtype: dict[ExperimentForCalculation, list[MetricResult]]
        """
        if not experiments_by_obs:
            return {}

        experiment_sources = self._get_all_sources(experiments_by_obs)
        batches = MetricRunner._split_batches(experiment_sources, batch_min, batch_max)
        batches_count = len(batches)

        calculator = MetricCalculator(self.metric_container, self.use_buckets)
        mp_worker = functools.partial(
            mp_calc_metric_batch,
            result_files=result_files,
            calculator=calculator,
            calc_backend=self.calc_backend,
        )

        logging.info(
            "Will calculate metric for %d experiments: %s",
            len(experiment_sources),
            umisc.to_lines(sorted(experiment_sources)),
        )
        logging.info(
            "Will use %d batches: %s",
            batches_count,
            umisc.to_lines(MetricRunner._batch_str(batch) for batch in batches),
        )
        all_results = {}
        results_iter = umisc.par_imap_unordered(mp_worker, batches, threads)
        for pos, (batch_results, error) in enumerate(results_iter):
            if error is not None:
                raise Exception(error)
            for experiment, result in usix.iteritems(batch_results):
                all_results[experiment] = result
            umisc.log_progress("metric calculation", pos, batches_count)
            unirv.log_nirvana_progress("metric calculation", pos, batches_count)

        return all_results

    def _get_all_sources(self, experiments_by_obs):
        """
        :type experiments_by_obs: list[list[ExperimentForCalculation]]
        :rtype: dict[ExperimentForCalculation, MetricSources]
        """
        sources = {}
        for exps in experiments_by_obs:
            for exp in exps:
                if exp not in sources:
                    sources[exp] = MetricSources.from_experiment(
                        experiment=exp.experiment,
                        observation=exp.observation,
                        services=exp.observation.services,
                        squeeze_path=self.squeeze_path,
                        history=self.history,
                        future=self.future,
                    )
        self._check_sources(sources)
        if self.check_versions:
            self._check_sources_versions(sources, experiments_by_obs)
        self._check_min_versions(sources, experiments_by_obs)
        return sources

    def _check_sources(self, all_sources):
        """
        :type all_sources: dict[ExperimentForCalculation, MetricSources]
        """
        all_tables = sorted(set(itertools.chain.from_iterable(s.all_tables() for s in usix.itervalues(all_sources))))
        logging.info("Will check if %d tables exist: %s", len(all_tables), umisc.to_lines(all_tables))
        bad_tables = self.calc_backend.find_bad_tables(all_tables)
        if bad_tables:
            raise Exception("{} tables do not exist: {}".format(len(bad_tables), umisc.to_lines(bad_tables)))

    def _check_sources_versions(self, sources, experiments_by_obs):
        """
        :type sources: dict[ExperimentForCalculation, MetricSources]
        :type experiments_by_obs: list[list[ExperimentForCalculation]]
        """
        bad_obs = []
        for exps in experiments_by_obs:
            tables = MetricRunner._get_tables_from_experiments(exps, sources)
            logging.info("Will check versions of %d tables: %s", len(tables), umisc.to_lines(tables))
            if self._find_old_tables(tables):
                bad_obs.append(exps)
        if bad_obs:
            raise Exception(
                "{} observations has different versions: {}".format(
                    len(bad_obs),
                    umisc.to_lines(bad_obs),
                )
            )

    def _find_old_tables(self, all_tables):
        """
        :type all_tables: list[str]
        :rtype: list[str]
        """
        all_versions = self.calc_backend.get_all_versions(all_tables)
        newest_versions = SqueezeVersions.get_newest(all_versions.values())
        logging.info("Got squeeze versions: %s", newest_versions)
        old_tables = {tab for tab, ver in usix.iteritems(all_versions)
                      if ver.is_older_than(newest_versions, check_history=False)}
        return sorted(old_tables)

    def _check_min_versions(self, sources, experiments_by_obs):
        """
        :type sources: dict[ExperimentForCalculation, MetricSources]
        :type experiments_by_obs: list[list[ExperimentForCalculation]]
        """
        newest_required_version = self._get_newest_from_required_min_versions()
        if newest_required_version.is_empty():
            return
        logging.info("Min required versions: %s", newest_required_version.serialize())
        bad_obs = []
        for exps in experiments_by_obs:
            tables = MetricRunner._get_tables_from_experiments(exps, sources)
            logging.info("Will check min versions of %d tables: %s", len(tables), umisc.to_lines(tables))
            all_versions = self.calc_backend.get_all_versions(tables)
            oldest_version = SqueezeVersions.get_oldest(all_versions.values())
            if oldest_version.is_older_than(newest_required_version):
                bad_obs.append(exps)
        if bad_obs:
            raise Exception(
                "{} observations do not satisfy minimal required squeeze versions: {}".format(
                    len(bad_obs),
                    umisc.to_lines(bad_obs),
                )
            )

    def _get_newest_from_required_min_versions(self):
        """
        :rtype: SqueezeVersions
        """
        min_versions = []
        if not self.min_versions.is_empty():
            min_versions.append(self.min_versions)
        for metric_instance in self.metric_container.plugin_instances.values():
            metric_min_version = getattr(metric_instance, "min_versions", {})
            if metric_min_version:
                min_versions.append(SqueezeVersions.deserialize(metric_min_version))
        newest = SqueezeVersions.get_newest(min_versions)
        return newest

    @staticmethod
    def _get_tables_from_experiments(exps, sources):
        """
        :type exps: list[ExperimentForCalculation]
        :type sources: dict[ExperimentForCalculation, MetricSources]
        """
        tables = sorted(set(itertools.chain.from_iterable(sources[exp].all_tables() for exp in exps)))
        return tables

    def _check_required_services(self, services):
        """
        :type services: list[str]
        :rtype: None
        """
        missing_req_services = self.get_missing_services(self.metric_container, services)

        if missing_req_services:
            raise Exception("Missing required services: %s", ", ".join(missing_req_services))
        else:
            logging.info("All requirements OK")

    @staticmethod
    def get_missing_services(metric_container, chosen_services):
        """
        :type metric_container: PluginContainer
        :type chosen_services: list[str]
        :rtype: list[str]
        """
        plugin_keys = metric_container.plugin_key_map

        chosen_services = set(chosen_services)
        required_services = set()
        missing_req_services = set()

        for plugin_id, metric_instance in metric_container.plugin_instances.items():
            metric_instance_name = plugin_keys[plugin_id].pretty_name()
            metric_req_services = set(getattr(metric_instance, "required_services", []))
            if metric_req_services:
                logging.info("Metric %s requires services: %s", metric_instance_name,
                             ', '.join(sorted(metric_req_services)))

            required_services.update(metric_req_services)

        if ServiceEnum.WEB & chosen_services:
            required_services.discard(ServiceEnum.WEB_AUTO)
        if ServiceEnum.WEB_EXTENDED & chosen_services:
            required_services.discard(ServiceEnum.WEB_AUTO_EXTENDED)

        missing_req_services.update(required_services - chosen_services)
        return sorted(missing_req_services)

    @staticmethod
    def _split_batches(experiment_sources, batch_min, batch_max):
        """
        :type experiment_sources: dict[ExperimentForCalculation, MetricSources]
        :type batch_min: int
        :type batch_max: int
        :rtype: list[dict[ExperimentForCalculation, MetricSources]]
        """
        batches = []
        for batch in umisc.split_by_chunks_random_iter(sorted(experiment_sources), batch_min, batch_max):
            batches.append({exp: experiment_sources[exp] for exp in batch})
        return batches

    @staticmethod
    def _batch_str(batch, fill="\t\t"):
        """
        :type batch: dict[ExperimentForCalculation, MetricSources]
        :type fill: str
        """
        experiments = umisc.to_lines(sorted(usix.iterkeys(batch)), fill)
        return "batch with {} experiments: {}".format(len(batch), experiments)


def save_metric_results_to_tar(save_to_dir, save_to_tar):
    """
    :type save_to_dir: str
    :type save_to_tar: str
    """
    logging.info("Creating tar archive %s from %s", save_to_tar, save_to_dir)
    ufile.tar_directory(path_to_pack=save_to_dir, tar_name=save_to_tar, dir_content_only=True)
    logging.info("Creating tar archive %s done", save_to_tar)


def mp_calc_metric_batch(experiments, result_files, calculator, calc_backend):
    """
    :type experiments: dict[ExperimentForCalculation, MetricSources]
    :type result_files: dict[ExperimentForCalculation, dict[int, dict]]
    :type calculator: MetricCalculator
    :type calc_backend: Any
    """
    try:
        results = calc_backend.calc_metric_batch(
            experiments=experiments,
            calculator=calculator,
            result_files=result_files,
        )
        return results, None
    except BaseException as exc:
        logging.info("Error in mp_calc_metric_batch for %s: %s, %s", experiments, exc, traceback.format_exc())
        return None, str(exc)


def generate_metric_result_file_paths_single(metric_keys, dir_path, experiment, aligned_exp_num):
    """
    :type metric_keys: dict[int, PluginKey]
    :type dir_path: str
    :type experiment: ExperimentForCalculation
    :type aligned_exp_num: str
    :return:
    """
    results = {}
    for metric_id in metric_keys:
        file_name = "{}_exp_{}_{}_{}_metric_{}.tsv".format(
            aligned_exp_num,
            experiment.testid,
            utime.format_date(experiment.dates.start),
            utime.format_date(experiment.dates.end),
            metric_id,
        )
        results[metric_id] = os.path.join(dir_path, file_name)
    return results


def generate_metric_result_file_paths(metric_keys, dir_path, experiments):
    """
    :type metric_keys: dict[int, PluginKey]
    :type dir_path: str
    :type experiments: list
    :rtype: dict[ExperimentForCalculation, dict[int, str]]
    """
    results = {}
    for pos, experiment in umisc.aligned_str_enumerate(experiments):
        result = generate_metric_result_file_paths_single(metric_keys, dir_path, experiment, pos)
        results[experiment] = result
    return results
