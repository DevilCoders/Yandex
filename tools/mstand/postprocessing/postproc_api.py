import logging
import os

import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
import yaqutils.tsv_helpers as utsv
from experiment_pool import Experiment
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricValues
from experiment_pool import Observation
from experiment_pool import Pool  # noqa
from metrics_api import ExperimentForAPI
from metrics_api import MetricKeyForAPI
from metrics_api import ObservationForAPI
from postprocessing import DestTsvFile
from postprocessing import PostprocExperimentContext
from postprocessing import PostprocObservationContext  # noqa
from postprocessing import SourceTsvFile


class PoolForPostprocessAPI(object):
    def __init__(self, pool, source_dir, dest_dir):
        """
        :type pool: Pool
        :type source_dir: str
        :type dest_dir: str
        """
        self.pool = pool
        self.source_dir = source_dir
        self.dest_dir = dest_dir


class ObservationForPostprocessAPI(object):
    def __init__(self, observation_ctx, metric_key):
        """
        :type observation_ctx: PostprocObservationContext
        """
        self.observation = ObservationForAPI(observation_ctx.observation, observation_ctx.unique_str)
        self.metric_key = MetricKeyForAPI(metric_key)

        self._observation_ctx = observation_ctx

        observation = self._observation_ctx.observation

        control_metric_result = observation.control.find_metric_result(metric_key)
        assert control_metric_result is not None

        control_ctx = PostprocExperimentContext(self._observation_ctx,
                                                observation.control,
                                                "control")
        self.control = ExperimentForPostprocessAPI(control_ctx, metric_key, control_metric_result)

        self.experiments = []
        for pos, exp in umisc.aligned_str_enumerate(observation.experiments):
            exp_metric_result = exp.find_metric_result(metric_key)
            assert exp_metric_result is not None
            exp_ctx = PostprocExperimentContext(self._observation_ctx,
                                                exp,
                                                "exp" + pos)
            exp_api = ExperimentForPostprocessAPI(exp_ctx, metric_key, exp_metric_result)
            self.experiments.append(exp_api)

    def add_custom_file(self, file_name):
        return self._observation_ctx.add_custom_file(file_name)

    def all_experiments(self):
        return [self.control] + self.experiments

    def make_new_observation(self, new_metric_key):
        """
        :type new_metric_key: PluginKey
        :rtype: Observation
        """

        # new observation with one metric result in each experiment
        old_observation = self._observation_ctx.observation

        new_control_result = self.control.make_new_metric_result(new_metric_key)
        new_control = self.control.make_new_experiment(new_control_result)

        new_experiments = []
        for exp in self.experiments:
            new_exp_result = exp.make_new_metric_result(new_metric_key)
            new_exp = exp.make_new_experiment(new_exp_result)
            new_experiments.append(new_exp)

        return Observation(obs_id=old_observation.id,
                           dates=old_observation.dates,
                           sbs_ticket=old_observation.sbs_ticket,
                           sbs_workflow_id=old_observation.sbs_workflow_id,
                           sbs_metric_results=old_observation.sbs_metric_results,
                           tags=old_observation.tags,
                           extra_data=self.observation.extra_data,
                           control=new_control,
                           experiments=new_experiments)

    @property
    def pool_extra_data(self):
        return self._observation_ctx.global_ctx.pool_extra_data


class ExperimentForPostprocessAPI(object):
    def __init__(self, experiment_ctx, metric_key, metric_result):
        """
        :type experiment_ctx: PostprocExperimentContext
        :type metric_key: PluginKey
        :type metric_result: MetricResult
        """
        self.metric_key = MetricKeyForAPI(metric_key)
        self.experiment = ExperimentForAPI(experiment_ctx.experiment,
                                           experiment_ctx.unique_str)
        self.observation = ObservationForAPI(experiment_ctx.observation_ctx.observation,
                                             experiment_ctx.observation_ctx.unique_str)

        self._experiment_ctx = experiment_ctx
        self._metric_result = metric_result
        self._metric_type = metric_result.metric_values.data_type

        self.extra_data = self._metric_result.extra_data

        self._source_filename = metric_result.metric_values.data_file
        if not self._source_filename:
            raise Exception("Source filename for metric {} in exp. {} is empty. ".format(metric_key, self.experiment))
        self._dest_filename = ExperimentForPostprocessAPI.choose_dest_filename(self._source_filename, experiment_ctx)

        global_ctx = self._experiment_ctx.observation_ctx.global_ctx
        source_path = os.path.join(global_ctx.source_dir, self._source_filename)
        dest_path = os.path.join(global_ctx.dest_dir, self._dest_filename)

        self._source_tsv = SourceTsvFile(source_path)
        self._dest_tsv = DestTsvFile(dest_path)  # TODO: do not use __del__ in DestTsvFile!
        self._dest_fd = self._dest_tsv.fd

        self._count_val = 0
        self._row_count = 0
        self._sum_val = 0

    @property
    def pool_extra_data(self):
        return self._experiment_ctx.observation_ctx.global_ctx.pool_extra_data

    def significant_metric_value(self):
        return self._metric_result.metric_values.significant_value

    def add_custom_file(self, file_name):
        return self._experiment_ctx.observation_ctx.add_custom_file(file_name)

    def _rows_iter(self):
        with self._source_tsv as source_tsv_reader:
            for row in source_tsv_reader:
                yield [ujson.load_from_str(x) for x in row]

    def __iter__(self):
        return self._rows_iter()

    def write_row(self, row):
        if not row:
            # skip empty rows
            # TODO: add test for empty rows?
            return
        assert isinstance(row, list) or isinstance(row, tuple)

        self._row_count += 1
        values = row
        if self._metric_type == MetricDataType.KEY_VALUES:
            values = row[1:]
        for value in values:
            try:
                self._sum_val += value
                self._count_val += 1
            except TypeError:
                pass
        utsv.dump_row_to_fd(row, self._dest_fd)

    def write_value(self, value):
        self._row_count += 1
        if self._metric_type != MetricDataType.KEY_VALUES:
            try:
                self._sum_val += value
                self._count_val += 1
            except TypeError:
                pass
        ujson.dump_to_fd(value, self._dest_fd)
        self._dest_fd.write("\n")

    # not for public use.
    def make_new_metric_result(self, new_metric_key):
        """
        :type new_metric_key: PluginKey
        :rtype:
        """
        old_metric_values = self._metric_result.metric_values
        old_version = self._metric_result.version
        old_ab_info = self._metric_result.ab_info

        new_row_count = self._row_count
        new_count_val = self._count_val
        new_sum_val = self._sum_val
        new_average_val = float(self._sum_val) / self._count_val if self._count_val else None

        new_metric_values = MetricValues(count_val=new_count_val,
                                         sum_val=new_sum_val,
                                         average_val=new_average_val,
                                         row_count=new_row_count,
                                         data_file=self._dest_filename,
                                         data_type=old_metric_values.data_type,
                                         value_type=old_metric_values.value_type)

        logging.info("Metric values: %s -> %s", old_metric_values, new_metric_values)

        return MetricResult(metric_type=self._metric_result.metric_type,
                            coloring=self._metric_result.coloring,
                            extra_data=self.extra_data,
                            invalid_days=self._metric_result.invalid_days,
                            criteria_results=[],  # old criteria results are no longer valid
                            metric_key=new_metric_key,
                            metric_values=new_metric_values,
                            version=old_version,
                            ab_info=old_ab_info)

    def make_new_experiment(self, new_metric_result):
        """
        :type new_metric_result: MetricResult
        :return:
        """
        old_exp = self._experiment_ctx.experiment
        return Experiment(testid=old_exp.testid,
                          serpset_id=old_exp.serpset_id,
                          sbs_system_id=old_exp.sbs_system_id,
                          extra_data=self.experiment.extra_data,
                          errors=old_exp.errors,
                          metric_results=[new_metric_result])

    @staticmethod
    def choose_dest_filename(source_path, experiment_ctx):
        """
        :type source_path: str
        :type experiment_ctx:
        :rtype:
        """
        source_dir, source_name = os.path.split(source_path)
        dest_name = "{}.{}".format(experiment_ctx.unique_str, source_name)
        return os.path.join(source_dir, dest_name)
