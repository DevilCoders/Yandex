import functools
import logging
import multiprocessing

import numpy.random
import pytest

import criterias
import mstand_utils.stat_helpers as ustat
import yaqutils.six_helpers as usix
from postprocessing import CriteriaParamsForAPI


@pytest.fixture(scope='session')
def random_state(request):
    seed = request.config.getoption("--seed")
    if seed is None:
        seed = numpy.random.randint(MAX_UINT32)
    logging.info('random seed: %d', seed)
    return numpy.random.RandomState(seed)


def parse_pvalue(raw_criteria_result):
    """
    :type raw_criteria_result: float | list | tuple
    :rtype: float
    """
    # TODO: reuse parse_raw_criteria_result function
    if isinstance(raw_criteria_result, (list, tuple)):
        if not raw_criteria_result:
            return None
        return raw_criteria_result[0]
    else:
        return raw_criteria_result


def generate_pvalue(criteria, generator, seed):
    """
    :type criteria: callable
    :type generator: PairGenerator
    :type seed: int
    """
    rnd_gen = numpy.random.RandomState(seed=seed)
    a, b = generator.generate_sample_data(rnd_gen)

    # this is a dirty hack to pass this flag through test routines.
    is_related = False
    if hasattr(criteria, "related"):
        is_related = criteria.related

    # XXX: current usual criterias are not using *_result fields
    params = CriteriaParamsForAPI(
        control_data=a,
        exp_data=b,
        control_result=None,
        exp_result=None,
        is_related=is_related
    )
    result = criteria.value(params)
    pvalue = parse_pvalue(result)
    return pvalue


class PairGenerator(object):
    def __init__(self, rnd_gen):
        self.size_1 = PairGenerator.random_size(rnd_gen)
        self.size_2 = PairGenerator.random_size(rnd_gen)

    def generate_sample_data(self, rnd_gen):
        raise NotImplementedError()

    @staticmethod
    def random_size(rnd_gen):
        return rnd_gen.randint(1000, 10000)


class NormalGenerator(PairGenerator):
    def __init__(self, rnd_gen, std_dev_same=False, row_length=None):
        super(NormalGenerator, self).__init__(rnd_gen)

        self.mean = NormalGenerator.random_mean(rnd_gen)

        self.std_dev_1 = NormalGenerator.random_std_dev(rnd_gen)
        if std_dev_same:
            self.std_dev_2 = self.std_dev_1
        else:
            self.std_dev_2 = NormalGenerator.random_std_dev(rnd_gen)

        self.row_length = row_length

    def generate_sample_data(self, rnd_gen):
        if not self.row_length:
            sample_1 = rnd_gen.normal(self.mean, self.std_dev_1, self.size_1)
            sample_2 = rnd_gen.normal(self.mean, self.std_dev_2, self.size_2)
        else:
            sample_1 = rnd_gen.normal(self.mean, self.std_dev_1, (self.size_1, self.row_length))
            sample_2 = rnd_gen.normal(self.mean, self.std_dev_2, (self.size_2, self.row_length))
            NormalGenerator.make_related(rnd_gen, sample_1)
            NormalGenerator.make_related(rnd_gen, sample_2)

        return sample_1, sample_2

    def __repr__(self):
        return "NormalGenerator(mean={}, std_dev=[{}, {}], size=[{}, {}])".format(
            self.mean,
            self.std_dev_1,
            self.std_dev_2,
            self.size_1,
            self.size_2
        )

    @staticmethod
    def random_mean(rnd_gen):
        return float(rnd_gen.uniform(-10, 10))

    @staticmethod
    def random_std_dev(rnd_gen):
        return float(rnd_gen.uniform(0.1, 5.0))

    @staticmethod
    def make_related(rnd_gen, matrix):
        for row in matrix:
            for i in usix.xrange(1, len(row)):
                row[i] = row[0] * float(rnd_gen.uniform(0.5, 2.0))


class RelatedGenerator(PairGenerator):
    def __init__(self, rnd_gen):
        super(RelatedGenerator, self).__init__(rnd_gen)
        self.mean = NormalGenerator.random_mean(rnd_gen)
        self.std_dev = NormalGenerator.random_std_dev(rnd_gen)

    def __repr__(self):
        return "RelatedGenerator(mean={}, std_dev={}, size={})".format(
            self.mean,
            self.std_dev,
            self.size_1
        )

    def generate_sample_data(self, rnd_gen):
        base = rnd_gen.normal(self.mean, self.std_dev, self.size_1)
        noise = rnd_gen.uniform(-1, 1, self.size_1)
        return (
            base,
            base + noise
        )


class DiscreteGenerator(PairGenerator):
    def __init__(self, rnd_gen):
        super(DiscreteGenerator, self).__init__(rnd_gen)
        self.min = rnd_gen.randint(-1000, 0)
        self.max = rnd_gen.randint(1, 1001)

    def __repr__(self):
        return "DiscreteGenerator(min={}, max={}, size=[{}, {}])".format(
            self.min,
            self.max,
            self.size_1,
            self.size_2
        )

    def generate_sample_data(self, rnd_gen):
        return (
            rnd_gen.randint(self.min, self.max + 1, self.size_1),
            rnd_gen.randint(self.min, self.max + 1, self.size_2)
        )


MAX_UINT32 = (1 << 32) - 1


def generate_pvalues(criteria, iterations, generator, rnd_gen):
    """
    :type criteria: callable
    :type iterations: int
    :type generator: PairGenerator
    :type rnd_gen: numpy.random.RandomState
    """
    logging.info("criteria: %s", criteria)
    logging.info("iterations: %d", iterations)
    logging.info("data generator: %s", generator)

    pool = multiprocessing.Pool()
    gen_pv = functools.partial(generate_pvalue, criteria, generator)

    return pool.map(gen_pv, [rnd_gen.randint(MAX_UINT32) for _ in usix.xrange(iterations)])


LEVELS = [
    0.001,
    0.005,
    0.01,
    0.05,
    0.1,
    0.15,
]


def count_levels(criteria, iterations, generator, rnd_gen):
    """
    :type criteria: callable
    :type iterations: int
    :type generator: PairGenerator
    :type rnd_gen: numpy.random.RandomState
    """
    levels = {level: 0 for level in LEVELS}

    logging.debug('Testing criteria %s on %d runs', criteria, iterations)
    for pvalue in generate_pvalues(criteria, iterations, generator, rnd_gen):
        for level in levels:
            if pvalue < level:
                levels[level] += 1

    logging.debug('Done!')
    return levels


def get_bounds(level, n):
    """
    :type level: float
    :type n: int
    """
    return ustat.conf_interval(alpha=0.001, size=n, threshold=level)


def check_thresholds(criteria, iterations, generator, rnd_gen):
    """
    :type criteria: callable
    :type iterations: int
    :type generator: PairGenerator
    :type rnd_gen: numpy.random.RandomState
    """
    assert iterations > 0
    levels = count_levels(criteria, iterations, generator, rnd_gen)
    bounds = {level: get_bounds(level, iterations) for level in levels}
    messages = []
    bad_levels = []
    for level in sorted(levels):
        count = levels[level]
        lower, upper = bounds[level]
        if lower <= count <= upper:
            messages.append("\t{}:\t{} in [{}; {}]".format(level, count, lower, upper))
        else:
            messages.append("\t{}:\t{} not in [{}; {}]".format(level, count, lower, upper))
            bad_levels.append(level)
    logging.info("%s levels:\n%s", criteria, "\n".join(messages))
    assert not bad_levels


# noinspection PyClassHasNoInit
class TestCriterias:
    criteria_iterations = 10000

    # noinspection PyShadowingNames
    def test_bootstrap(self, random_state):
        # bootstrap is extremely slow, give it less iterations
        bootstrap_iterations = max(1, self.criteria_iterations // 100)
        check_thresholds(
            criterias.Bootstrap(),
            bootstrap_iterations,
            NormalGenerator(random_state, row_length=3),
            random_state
        )

    # noinspection PyShadowingNames
    def test_mwu(self, random_state):
        check_thresholds(
            criterias.MWUTest(),
            self.criteria_iterations,
            NormalGenerator(random_state, std_dev_same=True),
            random_state
        )

    # noinspection PyShadowingNames
    def test_mwu_discrete(self, random_state):
        check_thresholds(
            criterias.MWUTest(),
            self.criteria_iterations,
            DiscreteGenerator(random_state),
            random_state
        )

    # noinspection PyShadowingNames
    def test_tw(self, random_state):
        check_thresholds(
            criterias.TWTest(),
            self.criteria_iterations,
            NormalGenerator(random_state, std_dev_same=True),
            random_state
        )

    # noinspection PyShadowingNames
    def test_lr(self, random_state):
        check_thresholds(
            criterias.LRTest(),
            self.criteria_iterations,
            NormalGenerator(random_state, std_dev_same=True),
            random_state
        )

    # noinspection PyShadowingNames
    def test_ranksums(self, random_state):
        check_thresholds(
            criterias.WilcoxonRankSumsTest(),
            self.criteria_iterations,
            NormalGenerator(random_state, std_dev_same=True),
            random_state
        )

    # noinspection PyShadowingNames
    def test_ttest(self, random_state):
        check_thresholds(
            criterias.TTest(),
            self.criteria_iterations,
            NormalGenerator(random_state),
            random_state
        )

        criteria = criterias.TTest()
        criteria.related = True
        check_thresholds(
            criteria,
            self.criteria_iterations,
            RelatedGenerator(random_state),
            random_state
        )
