# -*- coding: utf-8 -*-

import json
from collections import Counter

from core.actions import mathstat
from core.models import Estimation, Task


def aggregate_by_assessor(ests, actual_only, reference_est=None):
    # estimations aggregated by assessor contain the following structures:
    # (
    #    estimation object,
    #    list of 3 4-tuples for all criteria:
    #    (
    #        name of criterion,
    #        name of criterion value,
    #        name of Administrator's value,
    #        boolean showing if error is significant
    #    )
    # )
    # values are shuffled!
    return [(est, est.get_correction_value(actual_only, reference_est))
            for est in ests]


def aggregate_by_count(ests, shuffle):
    est_info = [(est, est.digits(shuffle=shuffle)) for est in ests]
    counters = [Counter(crit) for crit in
                zip(*[est_info_item[1] for est_info_item in est_info])]
    for counter in counters:
        counter.update({-1: 0, 0: 0, 1: 0})
    # estimations aggregated by count contain 3 pairs:
    # (
    #    criterion name,
    #    list of 3 pairs for digits -1, 0, 1:
    #    (
    #        digit of criterion value,
    #        count of such digits in all the estimations assessors made
    #    )
    # )
    return zip(ests[0].get_criterions_names() if len(ests) > 0 else Estimation.Criterion.NAMES_MCR,
               [sorted(counter.items()) for counter in counters])


# gets iterable collection of estimations
# and 2 iterable collections of task ids:
# tasks with different and opposite estimations
#
# returns list of following tuples
# for each user who made estimations:
# (
#   user login,
#   result of aggregate_by_count for this user's ests,
#   amount of tasks having different estimations with this user's,
#   amount of tasks having opposite estimations with this user's,
#   list of following tuples for all user's estimations:
#       (
#         estimation,
#         list of 3 1s or 0s showing if this est has different estimations
#               for each criterion,
#         list of 3 1s or 0s showing if this est has opposite estimations
#               for each criterion,
#         sign of having comments in estimations
#       )
# )
def user_aggregated_counts(ests, different, opposite):
    user_ests = {}
    user_different = {}
    user_opposite = {}
    for est in ests:
        digits = est.digits()
        if est.user.login not in user_ests:
            user_ests[est.user.login] = []
            user_different[est.user.login] = []
            user_opposite[est.user.login] = []
        user_ests[est.user.login].append(est)
        user_different[est.user.login].append([
            1 if est.task_id in different[i] else 0 for i in (0, 1, 2)
        ])
        user_opposite[est.user.login].append([
            1 if est.task_id in opposite[i] and digits[i] else 0
            for i in (0, 1, 2)
        ])
    aggregated = []
    for login in user_ests:
        has_comments = False
        for est in user_ests[login]:
            if est.comment:
                has_comments = True
                break
        aggregated.append((
            login, aggregate_by_count(user_ests[login], shuffle=False),
            [sum(counters) for counters in zip(*user_different[login])],
            [sum(counters) for counters in zip(*user_opposite[login])],
            [(user_ests[login][i],
              user_different[login][i],
              user_opposite[login][i])
             for i in range(len(user_ests[login]))],
            has_comments
        ))
    return aggregated


# returns list of following tuples
# for each criterion:
# (
#   criterion name,
#   zip(corrected digits aggregated by value,
#       original digits aggregated by value),
#   wilcoxon test for corrected digits,
#   different,
#   opposite
# ),
# task_count,
# json(different, opposite)
def integral_data(ests):
    crits_names = ests[0].get_criterions_names() if len(ests) > 0 else Estimation.Criterion.NAMES_MCR
    crit_ids = range(len(crits_names))
    # left, both, right for each criterion
    corrected_counters = [[0] * 3 for i in crit_ids]
    digits_counters = [[0] * 3 for i in crit_ids]
    deltas = [[] for i in crit_ids]
    has_line_info = False
    for est in ests:
        corr_digits = est.corrected_digits()
        digits = est.digits()
        first_lines = est.task.first_snippet.data.get('lines', 0)
        second_lines = est.task.second_snippet.data.get('lines', 0)
        if first_lines or second_lines:
            has_line_info = True

        for i in crit_ids:
            corrected_counters[i][corr_digits[i] + 1] += 1
            digits_counters[i][digits[i] + 1] += 1
            deltas[i].append(
                corr_digits[i] - 0.1 * (second_lines - first_lines)
            )
    different, opposite, task_count = consensus_values(ests)
    integral_data = []
    for i, crit in enumerate(crits_names):
        integral_data.append(
            (crit, zip(corrected_counters[i], digits_counters[i]),
             mathstat.wilcoxon_test(deltas[i]),
             different[i], opposite[i])
        )
        different[i] = list(different[i])
        opposite[i] = list(opposite[i])
    tasks_json = json.dumps({
        'different': different,
        'opposite': opposite
    })
    return integral_data, task_count, tasks_json, has_line_info


def consensus_values(ests):
    crits_names = ests[0].get_criterions_names() if len(ests) > 0 else Estimation.Criterion.NAMES_MCR
    crit_ids = range(len(crits_names))
    different = [set() for i in crit_ids]
    opposite = [set() for i in crit_ids]
    values_sets = {}
    for est in ests:
        digits = est.digits()
        if est.task_id not in values_sets:
            values_sets[est.task_id] = [set() for i in crit_ids]
        for i in crit_ids:
            values_sets[est.task_id][i].add(digits[i])
            if len(values_sets[est.task_id][i]) > 1:
                different[i].add(est.task_id)
                if {-1, 1}.issubset(values_sets[est.task_id][i]):
                    opposite[i].add(est.task_id)
    return (different, opposite, len(values_sets))


# gets an iterable of dicts with keys 'task__status' and 'correction__errors'
# returns list of tuples (crit_name | overall, crit | overall precision)
# or just list of precisions if flat=True
def precisions(est_dicts, flat=False):
    res = {}
    corr_len = 0
    res[Estimation.Criterion.OVERALL] = 0
#This works only for multicriterial estimations
    criterion_names = Estimation.Criterion.NAMES_MCR

    for crit in criterion_names:
        res[crit] = 0
    for est in est_dicts:
        if est['task__status'] == Task.Status.COMPLETE:
            corr_len += 1
            err = est['correction__errors']
            if err:
                res[Estimation.Criterion.OVERALL] += 1
                for i, crit in enumerate(criterion_names):
                    if err & (1 << i):
                        res[crit] += 1
    corr_len = float(corr_len) or 1.0
    res = [(crit, 1 - (res[crit] / corr_len))
           for crit in
           criterion_names + (Estimation.Criterion.OVERALL,)]
    if flat:
        return [res_tuple[1] for res_tuple in res]
    else:
        return res
