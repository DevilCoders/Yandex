import numpy
import yaqutils.six_helpers as usix

MAX_UINT32 = (1 << 32) - 1


class Bootstrap(object):
    read_mode = "lists"

    def __init__(self, counts=10000):
        self.counts = counts

    def value(self, params):
        control = params.control_data
        experiment = params.exp_data
        return bootstrap(control, experiment, self.counts)

    def __repr__(self):
        return "Bootstrap(counts={!r})".format(self.counts)


def bootstrap(sample_1, sample_2, count):
    assert count > 0

    size_1 = len(sample_1)
    if size_1 == 0:
        return 0.5

    sums_1 = numpy.array([numpy.sum(x) for x in sample_1])
    lens_1 = numpy.array([len(x) for x in sample_1])
    assert len(sums_1) == size_1
    assert len(lens_1) == size_1

    size_2 = len(sample_2)
    if size_2 == 0:
        return 0.5

    sums_2 = numpy.array([numpy.sum(x) for x in sample_2])
    lens_2 = numpy.array([len(x) for x in sample_2])
    assert len(sums_2) == size_2
    assert len(lens_2) == size_2

    # WARNING: sums_rnd_gen and lens_rnd_gen must have same seed
    seed = numpy.random.randint(MAX_UINT32)
    sums_rnd_gen = numpy.random.RandomState(seed)
    lens_rnd_gen = numpy.random.RandomState(seed)

    count_1 = 0
    count_2 = 0
    count_eq = 0

    for _ in usix.xrange(count):
        # WARNING: sums_rnd_gen and lens_rnd_gen must be used exact same way
        rnd_sums_1 = sums_rnd_gen.choice(sums_1, size_1, replace=True)
        rnd_lens_1 = lens_rnd_gen.choice(lens_1, size_1, replace=True)
        rnd_mean_1 = float(numpy.sum(rnd_sums_1)) / float(numpy.sum(rnd_lens_1))

        # WARNING: sums_rnd_gen and lens_rnd_gen must be used exact same way
        rnd_sums_2 = sums_rnd_gen.choice(sums_2, size_2, replace=True)
        rnd_lens_2 = lens_rnd_gen.choice(lens_2, size_2, replace=True)
        rnd_mean_2 = float(numpy.sum(rnd_sums_2)) / float(numpy.sum(rnd_lens_2))

        if rnd_mean_1 < rnd_mean_2:
            count_1 += 1
        elif rnd_mean_2 < rnd_mean_1:
            count_2 += 1
        else:
            count_eq += 1

    count_min = min(count_1, count_2)
    return (2.0 * float(count_min) + float(count_eq)) / count
