# coding=utf-8

import collections
import logging
import math

from scipy.special import erfinv

import reports.report_helpers as rhelp
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.math_helpers as umath
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
from experiment_pool import MetricColor
from reports import MetricResultPair

"""
observations - список наблюдений;
    date_from - с какого дня наблюдение
    date_to - по какой день наблюдение
    observation_id - id наблюдения
    control/experiments[] - информация о контроле/экспериментах
        date_from - с какого дня активен тестид
        date_to - по какой день активен тестид
        metrics - список прокрасок метрик
            metric_key
                name
            values
                average - среднее значение метрики
                data_type - ?
                sum - ?
            criterias - ?
            extra_data - детальная информация
                val - среднее значение
                mwtest - значение mw-теста
                ttest - значение t-теста
                prec - СКО значения
                delta_val - значение дельты
                delta_prec - СКО дельты
                delta1 - какая дельта нужна для значимости 99
                delta05 - какая дельта нужна для значимости 99.5
                delta01 - какая дельта нужна для значимости 99.9
    invalid_days - есть ли дни, которые надо исключать
    extra_data - доп. информация о наблюдении
        aspect_count - сколько экспериментов с таким аспектом
        parent_aspect - общий аспект
        child_aspect - название специального аспекта
        name - название эксперимента
        ticket - имя тикета
        verdict - экспертный вердикт (0 - ХЗ, 1 - почти ужас, 2 - ужас, 3 - ужас-ужас)
        medal - медалька
        platform - платформа
"""


class ScoreCalculatorParams(object):
    def __init__(
            self,
            score_mapping,
            wrong_penalty_func,
            sensitivity_func,
            aspect_penalty_func,
    ):
        """`
        :type score_mapping: dict[int, float]
        :type wrong_penalty_func: callable
        :type sensitivity_func: callable
        :type aspect_penalty_func: callable
        """
        self.score_mapping = score_mapping
        self.wrong_penalty_func = wrong_penalty_func
        self.sensitivity_func = sensitivity_func
        self.aspect_penalty_func = aspect_penalty_func


def calculate_score(
        verdict,
        delta_val,
        delta_prec,
        pvalue,
        aspect_count,
        red_sign,
        params,
):
    """
    :type verdict: int
    :type delta_val: float
    :type delta_prec: float | None
    :type pvalue: float
    :type aspect_count: int
    :type red_sign: float
    :type params: ScoreCalculatorParams
    :rtype: (float, int)
    """

    assert aspect_count != 0

    # определяем, смотрит ли метрика в ту же сторону, что и эксперт
    if verdict > 0:
        if delta_val * red_sign > 0:
            looks_same_way = +1
        else:
            looks_same_way = -1
    else:
        looks_same_way = 0

    # сколько баллов за (не)совпадение с мнением эксперта
    score = params.score_mapping.get(verdict, 0)
    if looks_same_way == -1:
        score = params.wrong_penalty_func(score)

    # поощряем за высокую чувствительность
    score *= params.sensitivity_func(delta_val, delta_prec, pvalue)

    # снижаем score за большое количество экспериментов с этим аспектом в сете
    score *= params.aspect_penalty_func(aspect_count)

    return score, looks_same_way


# noinspection PyUnusedLocal
def sensitivity_func_log(delta_val, delta_prec, pvalue):
    return math.log(abs(delta_val) / delta_prec)


# noinspection PyUnusedLocal
def sensitivity_func_sqrt(delta_val, delta_prec, pvalue):
    return math.log(abs(delta_val) / delta_prec)


# noinspection PyUnusedLocal
def sensitivity_func_linear(delta_val, delta_prec, pvalue):
    return abs(delta_val) / delta_prec


# noinspection PyUnusedLocal
def sensitivity_func_const(delta_val, delta_prec, pvalue):
    return 1.


# noinspection PyUnusedLocal
def sensitivity_func_erfinv_cap5(delta_val, delta_prec, pvalue):
    return min(erfinv(1 - pvalue), 5)


def aspect_penalty_sqrt(aspect_count):
    return 1 / math.sqrt(aspect_count)


def aspect_penalty_linear(aspect_count):
    return 1. / aspect_count


# noinspection PyUnusedLocal
def aspect_penalty_const(aspect_count):
    return 1.


def wrong_penalty_same(base_score):
    return -base_score


def wrong_penalty_double(base_score):
    return -2. * base_score


def wrong_penalty_triple(base_score):
    return -3. * base_score


def wrong_penalty_square(base_score):
    return -1. * base_score * base_score


def wrong_penalty_exp(base_score):
    return -1 * math.exp(base_score)


# noinspection PyUnusedLocal
def wrong_penalty_const(base_score):
    return -1000


def safe_div(a, b):
    if not b:
        return None
    return float(a) / float(b)


class GPScoreColoredExperiment(object):
    def __init__(self, observation, control, experiment, delta_val, pvalue, looks_same_way, verdict, verdict_good,
                 score):
        """
        :type observation: experiment_pool.Observation
        :type control: experiment_pool.Experiment
        :type experiment: experiment_pool.Experiment
        :type delta_val: float
        :type pvalue: float
        :type looks_same_way: int
        :type verdict: int
        :type verdict_good: int
        :type score: float
        """
        self.observation_id = observation.id
        self.testid_control = control.testid
        self.testid_experiment = experiment.testid
        self.delta_val = delta_val
        self.pvalue = pvalue
        self.looks_same_way = looks_same_way
        self.verdict = verdict
        self.verdict_good = verdict_good
        self.score = score
        self.parent_aspect, self.child_aspect = rhelp.get_aspects(observation.tags)

    def serialize(self):
        result = {
            "observation_id": self.observation_id,
            "testid_control": self.testid_control,
            "testid_experiment": self.testid_experiment,
            "delta_val": self.delta_val,
            "pvalue": self.pvalue,
            "looks_same_way": self.looks_same_way,
            "verdict": self.verdict,
            "score": self.score,
            "child_aspect": self.child_aspect,
            "parent_aspect": self.parent_aspect,
        }
        if self.verdict_good:
            result["verdict_good"] = self.verdict_good
        return result


class GPScoreStat(object):
    def __init__(self, metric_key, red_sign, min_pvalue):
        """
        :type metric_key: user_plugins.PluginKey
        :type red_sign: int
        :type min_pvalue: float
        """
        assert red_sign in [-1, +1]
        self.metric_key = metric_key
        self.red_sign = red_sign
        self._min_pvalue = min_pvalue

        self.final_score = 0.0

        self.grey_count = 0
        self.colored_count = 0
        self.colored = []
        """:type: list[GPScoreColoredExperiment]"""

        self._sensitivity_sum = 0.0
        self._sensitivity_count = 0

    @property
    def sensitivity(self):
        return safe_div(self._sensitivity_sum, self._sensitivity_count)

    def serialize(self):
        return {
            "metric": self.metric_key.str_key(),
            "red_sign": self.red_sign,
            "final_score": self.final_score,
            "grey_count": self.grey_count,
            "colored_count": self.colored_count,
            "colored": umisc.serialize_array(self.colored),
            "sensitivity": self.sensitivity,
        }

    def add_grey(self):
        self.grey_count += 1

    def add_colored(self, colored):
        """
        :type colored: GPScoreColoredExperiment
        """
        self.final_score += colored.score
        self.colored_count += 1
        self.colored.append(colored)

    def add_pvalue(self, pvalue):
        """
        :type pvalue: float
        """
        if pvalue is not None:
            self._sensitivity_sum += umath.s_function(pvalue, self._min_pvalue)
            self._sensitivity_count += 1


class GPScoreCalculatorSingle(object):
    def __init__(self, params, metric_key, min_pvalue):
        """
        :type params: ScoreCalculatorParams
        :type metric_key: user_plugins.PluginKey
        :type min_pvalue: float
        """
        self._params = params
        self._metric_key = metric_key
        self._min_pvalue = min_pvalue
        self._metric_scores = {
            -1: GPScoreStat(metric_key, red_sign=-1, min_pvalue=self._min_pvalue),
            +1: GPScoreStat(metric_key, red_sign=+1, min_pvalue=self._min_pvalue),
        }
        self._best_red_sign = None

    def add_experiment(self, observation, control, experiment, control_result, experiment_result, aspect_counts):
        """
        :type observation: experiment_pool.Observation
        :type control: experiment_pool.Experiment
        :type experiment: experiment_pool.Experiment
        :type control_result: experiment_pool.MetricResult
        :type experiment_result: experiment_pool.MetricResult
        :type aspect_counts: dict[str, int]
        """
        logging.info("Will calculate score for %s vs %s", control_result, experiment_result)

        pair = MetricResultPair(control_result, experiment_result)
        pair.colorize(value_threshold=0.01, rows_threshold=None)

        delta_val = pair.get_diff().significant.abs_diff

        assert len(experiment_result.criteria_results) == 1
        crit_res = experiment_result.criteria_results[0]

        if crit_res.deviations is None:
            raise Exception(
                "Criteria {} for metric {} in {} in {} has no deviations, try updating blocks".format(
                    crit_res,
                    experiment_result.metric_key,
                    experiment,
                    observation
                )
            )

        delta_prec = crit_res.deviations.std_diff
        pvalue = crit_res.pvalue

        if pair.color_by_value == MetricColor.GRAY:
            for metric_stat in usix.itervalues(self._metric_scores):
                metric_stat.add_grey()
                metric_stat.add_pvalue(pvalue)
        else:
            verdict = rhelp.get_verdict_by_testid(observation.tags, experiment.testid)

            if observation.extra_data and "verdict" in observation.extra_data:
                verdict_ed = int(observation.extra_data["verdict"])
                if verdict_ed != verdict.bad:
                    logging.warning("Observation {} has wrong verdict in pool: expected {}, got {}".format(
                        observation, verdict, verdict_ed
                    ))

            _, child_aspect = rhelp.get_aspects(observation.tags)
            aspect_count = aspect_counts[child_aspect]

            if observation.extra_data and "aspect_count" in observation.extra_data:
                aspect_count_ed = int(observation.extra_data["aspect_count"])
                if aspect_count_ed != aspect_count:
                    logging.warning("Observation {} has wrong aspect_count in pool: expected {}, got {}".format(
                        observation, aspect_count, aspect_count_ed
                    ))

            for red_sign, metric_stat in usix.iteritems(self._metric_scores):
                score, looks_same_way = calculate_score(
                    verdict=verdict.good if verdict.is_good() else verdict.bad,
                    delta_val=delta_val,
                    delta_prec=delta_prec,
                    pvalue=pvalue,
                    aspect_count=aspect_count,
                    red_sign=-red_sign if verdict.is_good() else red_sign,
                    params=self._params,
                )
                colored = GPScoreColoredExperiment(
                    observation=observation,
                    control=control,
                    experiment=experiment,
                    delta_val=delta_val,
                    pvalue=pvalue,
                    looks_same_way=looks_same_way,
                    verdict=verdict.bad,
                    verdict_good=verdict.good,
                    score=score,
                )
                metric_stat.add_colored(colored)
                metric_stat.add_pvalue(pvalue)

    def best(self):
        """
        :rtype: GPScoreStat
        """
        if self._metric_scores[-1].final_score > self._metric_scores[+1].final_score:
            return self._metric_scores[-1]
        else:
            return self._metric_scores[+1]


def _collect_aspect_counts(pool):
    child_aspects = []
    for observation in pool.observations:
        aspects = rhelp.get_aspects(observation.tags)
        if not aspects:
            raise Exception(
                "Observation {} doesn't have aspect tags. Tags: {}".format(observation, observation.tags)
            )
        _, child_aspect = aspects
        child_aspects.extend([child_aspect] * len(observation.experiments))

    # noinspection PyArgumentList
    aspect_counts = collections.Counter(child_aspects)
    logging.info("Aspect counts in pool:")
    for k, v in aspect_counts.most_common():
        logging.info("--> %s: %s", k, v)
    return aspect_counts


class GPScoreCalculator(object):
    def __init__(self, pool, params, min_pvalue):
        """
        :type pool: experiment_pool.Pool
        :type params: ScoreCalculatorParams
        :type min_pvalue: float
        """
        self._params = params
        self._score_calculators = {}
        """:type : dict[user_plugin.PluginKey, GPScoreCalculatorSingle]"""
        self._min_pvalue = min_pvalue

        logging.info("Will calculate score for %d observations", len(pool.observations))
        self.aspect_counts = _collect_aspect_counts(pool)

        for observation in pool.observations:
            self._add_observation(observation)

    def _add_observation(self, observation):
        """
        :type observation: experiment_pool.Observation
        """
        control = observation.control
        for experiment in observation.experiments:
            self._add_experiments(observation, control, experiment)

    def _add_experiments(self, observation, control, experiment):
        """
        :type observation: experiment_pool.Observation
        :type control: experiment_pool.Experiment
        :type experiment: experiment_pool.Experiment
        """
        logging.info("Will calculate score for %s vs %s", control, experiment)
        control_results = control.get_metric_results_map()
        experiment_results = experiment.get_metric_results_map()
        for metric_key, control_result in usix.iteritems(control_results):
            experiment_result = experiment_results[metric_key]
            calculator = self._score_calculators.get(metric_key)
            if not calculator:
                calculator = GPScoreCalculatorSingle(self._params, metric_key, self._min_pvalue)
                self._score_calculators[metric_key] = calculator
            calculator.add_experiment(
                observation, control, experiment,
                control_result, experiment_result,
                self.aspect_counts
            )

    def best(self):
        """
        :rtype: list[GPScoreStat]
        """
        return [calculator.best() for calculator in usix.itervalues(self._score_calculators)]

    def best_sorted(self):
        """
        :rtype: list[GPScoreStat]
        """
        result = self.best()
        result.sort(key=lambda stat: (-stat.final_score, stat.metric_key))
        return result


def print_metrics_tsv(scores_sorted, output_path):
    """
    :type scores_sorted: list[GPScoreStat]
    :type output_path: str
    """
    with ufile.fopen_write(output_path) as fd:
        for pos, stat in enumerate(scores_sorted):
            name = stat.metric_key.str_key()
            red_sign = stat.red_sign
            score = stat.final_score
            line = "{}. {}\t{}\t{}\n".format(pos + 1, name, red_sign, score)
            fd.write(line)


def print_metrics_json(scores_sorted, output_path):
    """
    :type scores_sorted: list[GPScoreStat]
    :type output_path: str
    """
    json_result = umisc.serialize_array(scores_sorted)
    ujson.dump_to_file(json_result, output_path)


def calculate_scores_main(pool, output_tsv, output_json, min_pvalue=0.001):
    """
    :type pool: experiment_pool.Pool
    :type output_tsv: str
    :type output_json: str
    :type min_pvalue: float
    """
    # 1-2-4-8, штраф*-3, sens, aspect
    score_mapping = {0: 1, 1: 2, 2: 3, 3: 5}
    score_params = ScoreCalculatorParams(
        score_mapping=score_mapping,
        wrong_penalty_func=wrong_penalty_const,
        sensitivity_func=sensitivity_func_erfinv_cap5,
        aspect_penalty_func=aspect_penalty_sqrt,
    )
    result = GPScoreCalculator(pool, score_params, min_pvalue).best_sorted()
    if output_tsv:
        print_metrics_tsv(result, output_tsv)
    if output_json:
        print_metrics_json(result, output_json)
