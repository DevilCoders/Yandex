import logging

from experiment_pool import Experiment
from experiment_pool import MetricColor
from experiment_pool import MetricColoring
from experiment_pool import MetricDiff
from experiment_pool import MetricResult  # noqa
from experiment_pool import Observation


class ExperimentPair(object):
    def __init__(self, control, experiment, observation=None, is_first_in_obs=None):
        """
        :type control: Experiment
        :type experiment: Experiment
        """
        self.control = control
        self.experiment = experiment
        self.observation = observation.clone()

        self.is_first_in_obs = is_first_in_obs
        self.result_pairs = None

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "ExpPair(C:{} vs E:{})".format(self.control, self.experiment)

    def __hash__(self):
        raise Exception("ExperimentPair is not hashable")

    def __cmp__(self, other):
        raise Exception("ExperimentPair is not comparable")

    def __eq__(self, other):
        raise Exception("ExperimentPair is not comparable")

    def serialize(self):
        return {
            "control": self.control.serialize(),
            "experiment": self.experiment.serialize(),
        }

    @staticmethod
    def from_pool(pool, fill_result_pairs=False):
        """
        :type pool: Pool
        :type fill_result_pairs: bool
        :rtype: list[ExperimentPair]
        """
        return ExperimentPair.from_observations(pool.observations, fill_result_pairs)

    @staticmethod
    def from_observations(observations, fill_result_pairs=False):
        """
        :type observations: list[Observation] | iter[Observation]
        :type fill_result_pairs: bool
        :rtype: list[ExperimentPair]
        """
        exp_pairs = []
        for observation in observations:
            is_first_in_obs = True
            for experiment in observation.experiments:
                exp_pair = ExperimentPair(observation.control, experiment, observation=observation,
                                          is_first_in_obs=is_first_in_obs)
                is_first_in_obs = False
                if fill_result_pairs:
                    exp_pair.fill_result_pairs()
                exp_pairs.append(exp_pair)

        return exp_pairs

    @staticmethod
    def from_lamps(pool):
        """
        :type pool: Pool
        :rtype: list[ExperimentPair]
        """
        lamps = pool.lamps
        dct = {}
        for lamp in lamps:
            key = lamp.lamp_key
            values = lamp.lamp_values
            if key.control not in dct:
                dates = key.dates
                obs = Observation(control=None,
                                  dates=dates,
                                  obs_id=key.observation)
                obs.control = Experiment(testid=key.control, metric_results=values)
                dct[key.control] = obs
            else:
                dct[key.control].experiments.append(Experiment(testid=key.testid, metric_results=values))

        return ExperimentPair.from_observations(dct.values(), fill_result_pairs=True)

    def fill_result_pairs(self):
        """
        :rtype: None
        """
        self.result_pairs = self.get_result_pairs()

    def get_result_pairs(self):
        """
        :rtype: dict[PluginKey, MetricResultPair]
        """
        result_pairs = {}

        control_res_map = self.control.get_metric_results_map()

        exp_res_map = self.experiment.get_metric_results_map()
        all_keys = set(list(control_res_map.keys()) + list(exp_res_map.keys()))
        for key in all_keys:
            if key in control_res_map and key not in exp_res_map:
                logging.warning("Metric {} in control is missing in experiment {}".format(key, self.experiment))

            control_res = control_res_map.get(key)
            exp_res = exp_res_map.get(key)
            result_pair = MetricResultPair(control_res=control_res, exp_res=exp_res)
            result_pairs[key] = result_pair
        return result_pairs

    def colorize_result_pairs(self, value_threshold, rows_threshold=0.1):
        """
        :type value_threshold: float
        :type rows_threshold: float | None
        :rtype: None
        """
        for res_pair in self.result_pairs.values():
            res_pair.colorize(value_threshold=value_threshold, rows_threshold=rows_threshold)


class MetricResultPair(object):
    def __init__(self, control_res, exp_res):
        """
        :type control_res: MetricResult | None
        :type exp_res: MetricResult | None
        """
        if control_res is not None and exp_res is not None:
            # metric keys in pair should always be identical
            assert control_res.metric_key == exp_res.metric_key

        self.control_res = control_res
        self.exp_res = exp_res

        # integral experiment color
        self.color_by_value = None
        # color by row count ratio
        self.color_by_rows = None

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "MRPair(cr={} vs er={})".format(self.control_res, self.exp_res)

    def __hash__(self):
        raise Exception("MetricResultPair is not hashable")

    def __cmp__(self, other):
        raise Exception("MetricResultPair is not comparable")

    def __eq__(self, other):
        raise Exception("MetricResultPair is not comparable")

    def is_complete(self):
        return self.control_res is not None and self.exp_res is not None

    def get_diff(self):
        """
        :rtype: MetricDiff
        """
        assert self.is_complete()
        return MetricDiff(self.control_res.metric_values, self.exp_res.metric_values)

    def get_color_by_value(self):
        assert self.control_res and self.exp_res

        # lamp metric
        if self.exp_res.coloring == MetricColoring.LAMP:
            return self.get_lamp_color()

        # regular metric
        control_value = self.control_res.metric_values.significant_value
        exp_value = self.exp_res.metric_values.significant_value
        if control_value is None or exp_value is None:
            return MetricColor.YELLOW

        if self.exp_res.coloring == MetricColoring.MORE_IS_BETTER:
            return MetricColor.GREEN if exp_value > control_value else MetricColor.RED
        elif self.exp_res.coloring == MetricColoring.LESS_IS_BETTER:
            return MetricColor.GREEN if exp_value < control_value else MetricColor.RED

        return MetricColor.YELLOW

    def get_color_by_rows(self, threshold):
        """
        :type threshold: float | None
        :rtype: str
        """
        assert self.is_complete()

        if threshold is None:
            return MetricColor.GRAY

        control_rows = self.control_res.metric_values.row_count
        exp_rows = self.exp_res.metric_values.row_count

        if control_rows is None or exp_rows is None:
            return MetricColor.GRAY

        if control_rows == 0 and exp_rows == 0:
            return MetricColor.GRAY

        if control_rows == 0 or exp_rows == 0:
            logging.info("Row count discrepancy for metric %s: experiment rows: %s, control rows: %s (threshold=%g)",
                         self.control_res.metric_key, exp_rows, control_rows, threshold)
            return MetricColor.RED

        row_ratio = float(exp_rows) / float(control_rows)
        if abs(row_ratio - 1.0) > threshold:
            logging.info("Row count discrepancy for metric %s: experiment rows: %s, control rows: %s (threshold=%g)",
                         self.control_res.metric_key, exp_rows, control_rows, threshold)
            return MetricColor.RED

        return MetricColor.GRAY

    def colorize(self, value_threshold, rows_threshold):
        """
        :type value_threshold: float
        :type rows_threshold: float | None
        :rtype None
        """
        if self.is_complete():
            self.color_by_rows = self.get_color_by_rows(rows_threshold)
            color_by_value = self.get_color_by_value()
        else:
            self.color_by_rows = MetricColor.YELLOW
            color_by_value = MetricColor.YELLOW

        # initial value - for metric results without criterias
        self.color_by_value = color_by_value

        # recolorize by criterias, if any.
        if self.exp_res:
            for crit_res in self.exp_res.criteria_results:
                if crit_res.pvalue is not None and crit_res.pvalue < value_threshold:
                    crit_res.color = color_by_value
                else:
                    crit_res.color = MetricColor.GRAY
                    # if one of criterias is undecided => all experiment is gray.
                    self.color_by_value = MetricColor.GRAY

    @staticmethod
    def get_lamp_threshold(metric_key):
        """
        :type metric_key: PluginKey
        :rtype: float
        """
        # TODO: fill real thresholds
        thresholds = {
            "SumUserCount": 0.1,
            "SumUserCountWithRequestsWithoutReqids": 0.1
        }
        return thresholds.get(metric_key.name, 0.1)

    def get_lamp_color(self):
        """
        :rtype: str
        """

        exp_value = self.exp_res.metric_values.significant_value
        control_value = self.control_res.metric_values.significant_value

        if exp_value is None or control_value is None:
            return MetricColor.YELLOW

        if abs(control_value) < 1e-7:
            return MetricColor.YELLOW

        value_ratio = float(exp_value) / float(control_value)

        threshold = self.get_lamp_threshold(self.exp_res.metric_key)

        if abs(value_ratio - 1.0) > threshold:
            return MetricColor.RED
        return MetricColor.GRAY
