# -*- coding: utf-8 -*-

import itertools
import logging
import os

import yt.yson as yson

import session_local.mr_merge_local
import session_local.versions_local as versions_local
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.six_helpers as usix
from experiment_pool import ExperimentForCalculation  # noqa
from session_metric import MetricCalculator  # noqa
from session_metric.metric_calculator import MetricSourceInfo
from experiment_pool import MetricResult
from mstand_structs import LampKey
from mstand_structs import SqueezeVersions


class MetricLocalReducer(object):
    def __init__(self, calculator, experiment, observation, info_index):
        self.calculator = calculator
        self.experiment = experiment
        self.observation = observation
        self.info_index = info_index

    def run(self, input_files, output_files):
        """
        :type input_files: list[str]
        :type output_files: dict[int, str]
        :rtype: list[MetricResult]
        """
        return self.calculator.read_metric_results(
            rows=self._reduce_all(input_files),
            experiment_name="{} {}".format(self.experiment.testid, self.observation.dates),
            output_files=output_files,
        )

    def _reduce_all(self, paths):
        rows = session_local.mr_merge_local.merge_iterators(
            [_iter_squeeze(path) for path in paths],
            keys=("yuid", "ts", "action_index"),
        )
        for key, group in itertools.groupby(rows, key=lambda r: r["yuid"]):
            result = self._reduce_user(key, group)
            if result:
                yield result

    def _reduce_user(self, yuid, rows):
        """
        :type yuid: str
        :type rows: __generator[dict[str]]
        :rtype dict[str] | None
        """
        rows = MetricSourceInfo.patch_user_actions(rows, self.info_index)
        result = self.calculator.calc_metric_for_user(
            yuid=yuid,
            rows=rows,
            experiment=self.experiment,
            observation=self.observation,
        )
        return result


def _iter_squeeze(path):
    with ufile.fopen_read(path, is_binary=True) as squeeze_fd:
        for row in yson.load(squeeze_fd, yson_type="list_fragment"):
            yield yson.yson_to_json(row)


def calc_metric_batch(experiments, calculator, result_files):
    """
    :type experiments: dict[ExperimentForCalculation, MetricSources]
    :type calculator: MetricCalculator
    :type result_files: dict[ExperimentForCalculation, dict[int, str]]
    :rtype: dict[ExperimentForCalculation, list[MetricResult]]
    """
    all_results = {}
    for experiment, sources in usix.iteritems(experiments):
        output_files = result_files.get(experiment)
        all_results[experiment] = calc_metric_single(
            experiment=experiment,
            sources=sources,
            calculator=calculator,
            output_files=output_files,
        )
    return all_results


def calc_metric_single(experiment, sources, calculator, output_files):
    """
    :type experiment: ExperimentForCalculation
    :type sources: MetricSources
    :type calculator: MetricCalculator
    :type output_files: dict[int, str]
    :rtype: list[MetricResult]
    """
    reducer = MetricLocalReducer(
        calculator=calculator,
        experiment=experiment.experiment,
        observation=experiment.observation,
        info_index=list(sources.all_tables_info()),
    )
    result = reducer.run(input_files=sources.all_tables(), output_files=output_files)

    paths = sources.all_tables()
    all_versions = versions_local.get_all_versions(paths)
    max_version = SqueezeVersions.get_newest(all_versions.values())

    for res in result:
        res.version = max_version

    return result


# noinspection PyMethodMayBeStatic
class MetricBackendLocal(object):
    ONLINE_LAMPS_BATCH = "sample_metrics/online/online-lamps-batch.json"
    ONLINE_LAMPS_TABLE = "lamps_cache.jsonl"

    def __init__(self):
        pass

    def find_bad_tables(self, tables):
        """
        :type tables: list[str]
        :rtype: list[str]
        """
        return [path for path in tables if not os.path.isfile(path)]

    def get_all_versions(self, paths):
        """
        :type paths: list[str]
        :rtype: dict[str, SqueezeVersions]
        """
        return versions_local.get_all_versions(paths)

    @property
    def require_threads_for_paralleling(self):
        return False

    def calc_metric_batch(self, experiments, calculator, result_files):
        """
        :type experiments: dict[ExperimentForCalculation, MetricSources]
        :type calculator: MetricCalculator
        :type result_files: dict[ExperimentForCalculation, dict[int, str]]
        :rtype: dict[ExperimentForCalculation, list[MetricResult]]
        """
        return calc_metric_batch(experiments, calculator, result_files)

    def _read_lamps_cache(self, path):
        """
        :type path: str
        :rtype: generator
        """
        full_path = os.path.join(path, self.ONLINE_LAMPS_TABLE)
        logging.info("Getting cached lamps from {}".format(full_path))
        if not os.path.exists(full_path):
            logging.warning("Table {} not found, creating one".format(full_path))
            os.mknod(full_path)

        with open(full_path, mode="r") as table:
            return table.readlines()

    def get_lamps_from_cache(self, pool_lamp_keys, path):
        """
        :type path: str
        :type pool_lamp_keys: set[LampKey]
        :rtype: list[tuple[LampKey, list[MetricResult]]]
        """
        table = self._read_lamps_cache(path)
        rows = []

        for row in table:
            lamp = ujson.load_from_str(row)
            values = [MetricResult.deserialize(i) for i in lamp["values"]]
            key = LampKey.deserialize(lamp["key"])
            if key in pool_lamp_keys:
                rows.append((key, values))

        return rows

    def write_lamps_to_cache(self, lamps, path):
        """
        :type path: str
        :type lamps: Pool
        """
        full_path = os.path.join(path, self.ONLINE_LAMPS_TABLE)
        logging.info("Caching lamps, path: {}".format(full_path))
        with open(full_path, "a") as table:
            for lamp in lamps.lamps:
                key = lamp.lamp_key.serialize()
                values = [val.serialize() for val in lamp.lamp_values]
                line = ujson.dump_to_str({"key": key, "values": values}) + "\n"
                table.write(line)
