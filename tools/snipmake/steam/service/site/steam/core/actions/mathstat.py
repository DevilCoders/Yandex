# -*- coding: utf-8 -*-

import math


# phi-1(1 - 0.95 / 2)
T_CRITICAL = 1.96


def wilcoxon_test(delta):
    # remove zeros
    delta = filter(bool, delta)
    N = len(delta)
    if not N:
        return (0, 0, False, 0, T_CRITICAL)
    delta = sorted(delta, key=abs)
    rank = []
    for (b, e) in series(delta):
        # average rank for one series
        rank += [(b + 1 + e) / 2.0] * (e - b)
    R_second = sum(rank[i] for i in range(N) if delta[i] > 0)
    T = (R_second - N * (N + 1) / 4.0) / math.sqrt(
        (
            N * (N + 1) * (2 * N + 1) -
            sum(
                (t - 1) * t * (t + 1)
                for t in map(lambda x: x[1] - x[0],
                             series(delta))
            ) / 2.0
        ) / 24.0
    )
    is_right = (T > 0)
    return {'size': N, 'T': T, 'is_right': is_right,
            'T_abs': abs(T), 'T_CRITICAL': T_CRITICAL}


def series(l):
    if l:
        first = 0
        cur_val = abs(l[0])
        for cur in range(1, len(l)):
            if abs(l[cur]) != cur_val:
                yield (first, cur)
                first = cur
                cur_val = abs(l[cur])
        yield (first, len(l))


def filtered_sample_parameters(sample):
    if not sample:
        return (None,) * 4
    count = len(sample)
    sample = [el for el in sample if el]
    if not sample:
        return (0,) * 4
    sample = sorted(sample)
    return (
        sum(sample) / float(count),
        sorted_median(sample),
        sample[0],
        sample[-1]
    )


def median(sample):
    return sorted_median(sorted(sample))


def sorted_median(sample):
    if sample:
        count = len(sample)
        return sample[count >> 1] if count & 1 else (
            sample[count >> 1] + sample[(count >> 1) - 1]
        ) / 2.0
