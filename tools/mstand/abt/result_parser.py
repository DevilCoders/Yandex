import logging
import pprint

from experiment_pool import CriteriaResult
from experiment_pool import Deviations
from experiment_pool import MetricColoring
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from user_plugins import PluginKey


class ResultParser(object):
    def __init__(self, detailed=False, custom_test=None):
        """
        :type detailed: bool
        :type custom_test: str | None
        """
        self.detailed = detailed
        self.custom_test = custom_test
        if self.custom_test:
            name = "abt({})".format(self.custom_test)
        else:
            name = "abt"
        self.criteria_key = PluginKey(name=name)

    def parse(self, observation, testids, summary, daily, coloring):
        """
        :type observation: experiment_pool.Observation
        :type testids: list[str]
        :type summary: dict[str, list[dict[str]]]
        :type daily: dict[str, list[list[dict[str]]]]
        :type coloring: str
        :rtype: dict[str, dict[str]]
        """
        logging.debug("summary for observation %s: %s", observation, pprint.pformat(summary))
        logging.debug("daily for observation %s: %s", observation, pprint.pformat(daily))
        metric_names = summary.keys()
        for metric_name in metric_names:
            self._parse_one(
                observation,
                testids,
                summary[metric_name],
                daily.get(metric_name),
                metric_name,
                coloring
            )

    def get_metric_values(self, metric_data):
        """
        :type metric_data: list[dict[str, float]]
        :rtype: list[float]
        """
        # (MSTAND-1493) default ab metrics return zero if they don't have data
        return [x.get("value", 0.0) for x in metric_data]

    def is_valid_metric_values(self, metric_data):
        """
        :type metric_data: list[dict[str, float]]
        :rtype: bool
        """
        return all(self.get_metric_values(metric_data))

    def _parse_one(self, observation, testids, summaries, daily, metric_name, coloring):
        """
        :type observation: epool.Observation
        :type testids: list[str]
        :type summaries: list[dict[str, float]]
        :type daily: list[list[dict[str, float]]]
        :type metric_name: str
        :rtype: dict[str]
        """
        if not daily:
            valid_days = None
        else:
            valid_days = sum(1
                             for day in daily
                             if self.is_valid_metric_values(day))
        expected_days = observation.dates.number_of_days()
        if expected_days != float("inf") or valid_days is None:
            invalid_days = None
        elif valid_days > expected_days:
            raise Exception("magic: valid days more than expected")
        else:
            invalid_days = expected_days - valid_days

        values = self.get_metric_values(summaries)

        control_testid = testids[0]
        control_value = values[0]
        control_summary = summaries[0]

        colorings = set()
        results = {}
        logging.debug("%s %s (control) %s value: %s", metric_name, control_testid, observation, control_value)
        for testid, value, summary in zip(testids[1:], values[1:], summaries[1:]):
            logging.debug("%s %s %s value: %s", metric_name, testid, observation, value)
            diff = summary.get("delta_val")
            logging.debug("%s %s %s diff: %s", metric_name, testid, observation, diff)
            # (MSTAND-1493) pvalue of 0.5 is set due to lack of data
            pvalue = summary.get("pvalue", 0.5)
            logging.debug("%s %s %s value: %s", metric_name, testid, observation, pvalue)
            results[testid] = (value, summary)
            abt_color = summary.get("color")
            if abt_color:
                logging.debug("%s %s %s abt color: %s", metric_name, testid, observation, abt_color)
                if pvalue < 0.5:
                    if diff < 0:
                        if abt_color == "green":
                            colorings.add(MetricColoring.LESS_IS_BETTER)
                        elif abt_color == "red":
                            colorings.add(MetricColoring.MORE_IS_BETTER)
                    elif diff > 0:
                        if abt_color == "green":
                            colorings.add(MetricColoring.MORE_IS_BETTER)
                        elif abt_color == "red":
                            colorings.add(MetricColoring.LESS_IS_BETTER)

        logging.debug("%s %s got coloring from abt: %s", metric_name, observation, sorted(colorings))
        if not coloring or coloring == MetricColoring.NONE:
            if len(colorings) == 1:
                coloring = colorings.pop()
            elif len(colorings) > 1:
                logging.error("%s %s got ambiguous coloring from abt: %s", metric_name, observation, sorted(colorings))

        control_metric_values = MetricValues(count_val=None,
                                             sum_val=None,
                                             average_val=control_value,
                                             data_type=MetricDataType.NONE)

        if self.custom_test:
            metric_key = PluginKey(name="{}({})".format(metric_name, self.custom_test))
        else:
            metric_key = PluginKey(name=metric_name)

        control_result = MetricResult(
            metric_key=metric_key,
            metric_values=control_metric_values,
            invalid_days=invalid_days,
            metric_type=MetricType.ONLINE,
            coloring=coloring,
        )

        if self.detailed:
            control_result.extra_data = control_summary
        observation.control.add_metric_result(control_result)

        for experiment in observation.experiments:
            if experiment.testid not in results:
                logging.warning("No results for testid %s in observation %s, skipping", experiment.testid, observation)
            value, summary = results[experiment.testid]
            metric_result = self._construct_abt_metric_result(
                value, summary, metric_key, invalid_days, coloring, control_summary.get("prec")
            )
            experiment.add_metric_result(metric_result)

    def _construct_abt_metric_result(self, average, summary, metric_key, invalid_days, coloring, control_std_dev):

        exp_metric_values = MetricValues(
            count_val=None,
            sum_val=None,
            average_val=average,
            data_type=MetricDataType.NONE,
        )

        metric_result = MetricResult(
            metric_key=metric_key,
            metric_values=exp_metric_values,
            metric_type=MetricType.ONLINE,
            invalid_days=invalid_days,
            coloring=coloring,
        )

        if self.custom_test:
            custom_test_value = summary[self.custom_test]
            pvalue = 1.0 - custom_test_value / 100.0
        else:
            # (MSTAND-1493) pvalue of 0.5 is set due to lack of data
            pvalue = summary.get("pvalue", 0.5)

        std_exp = summary.get("prec")
        std_diff = summary.get("delta_prec")

        crit_res = CriteriaResult(
            criteria_key=self.criteria_key,
            pvalue=pvalue,
            deviations=Deviations(
                std_control=control_std_dev,
                std_exp=std_exp,
                std_diff=std_diff,
            )
        )
        metric_result.criteria_results.append(crit_res)

        if self.detailed:
            metric_result.extra_data = summary
        return metric_result


class SpuResultParser(ResultParser):
    def get_metric_values(self, metric_data):
        """
        :type metric_data: list[dict[str, float]]
        :rtype: list[float]
        """
        return [0.0] + self._get_experiment_values(metric_data)

    def is_valid_metric_values(self, metric_data):
        """
        :type metric_data: list[dict[str, float]]
        :rtype: bool
        """
        return all(self._get_experiment_values(metric_data))

    @staticmethod
    def _get_experiment_values(metric_data):
        """
        :type metric_data: list[dict[str, float]]
        :rtype: list[float]
        """
        # (MSTAND-1493) delta_val considered to be zero if it doesn't exists
        return [x.get("delta_val", 0.0) for x in metric_data[1:]]
