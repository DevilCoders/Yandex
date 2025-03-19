import json
import numpy as np
import os
import pandas as pd

from typing import List, Dict

from tools.data import group_by
from tools.data_interface.quality_evalutation import QualityEvaluationSample, QualityEvaluationPool
from tools.markup import MarkupResult, read_markup_from_json, read_assignments


class Vote:
    SAME = True
    DIFFER = False


class AggregatedVotes:
    def __init__(self, n_same=0, n_differ=0):
        self.raw_same = n_same
        self.raw_differ = n_differ
        self.major_same = 1 if n_same > n_differ else 0
        self.major_differ = 1 if n_differ > n_same else 0

    @staticmethod
    def get_json_fields():
        return [
            'major_same',
            'major_differ',
            'percent_same',
            'percent_same_raw'
        ]

    @property
    def percent_same(self):
        return 100 * self.major_same / (self.major_same + self.major_differ)

    @property
    def percent_same_raw(self):
        return 100 * self.raw_same / (self.raw_same + self.raw_differ)

    def to_json(self):
        return {
            'major_same': self.major_same,
            'major_differ': self.major_differ,
            'percent_same': round(self.percent_same, 2),
            'percent_same_raw': round(self.percent_same_raw, 2)
        }

    def __str__(self):
        return json.dumps(self.to_json(), indent=4, sort_keys=True)

    def add(self, other):
        self.major_same += other.major_same
        self.major_differ += other.major_differ
        self.raw_same += other.raw_same
        self.raw_differ += other.raw_differ

    def majority_vote(self):
        if self.major_same > self.major_differ:
            return Vote.SAME
        else:
            return Vote.DIFFER


def calc_votes(marks):
    n_true = np.count_nonzero(marks)
    n_false = np.count_nonzero(np.logical_not(marks))
    return AggregatedVotes(n_same=n_true, n_differ=n_false)


class Statistics:
    def __init__(self, speaker):
        self.speaker = speaker

        self.common_votes = AggregatedVotes()
        self.data_part = {}
        self.is_template = {}
        self.data_part_per_is_template = {'True': {}, 'False': {}}
        self.distortion_type = {}
        self.bad_sample_ids = set()

    @staticmethod
    def update_dict(statistic: Dict[str, AggregatedVotes], key: str, votes: AggregatedVotes):
        if key not in statistic:
            statistic[key] = AggregatedVotes()

        statistic[key].add(votes)

    def update(self, sample: QualityEvaluationSample, votes: AggregatedVotes):
        if sample.speaker_id != self.speaker:
            return

        if sample.is_distorted:
            Statistics.update_dict(self.distortion_type, sample.distortion_type, votes)
            return

        is_template_str = str(sample.is_template)

        self.common_votes.add(votes)
        Statistics.update_dict(self.data_part, sample.data_part, votes)
        Statistics.update_dict(self.is_template, is_template_str, votes)
        Statistics.update_dict(self.data_part_per_is_template[is_template_str], sample.data_part, votes)

        if votes.raw_differ > votes.raw_same:
            self.bad_sample_ids.add(os.path.basename(sample.path_to_reference))

    def get_result(self):
        return self.common_votes.percent_same


def group_markup_by_tasks(markups):
    group_tasks_markups = group_by(markups, lambda m: (m.audio1, m.audio2))

    for group_id, markups in group_tasks_markups.items():
        audio1, audio2 = group_id
        marks = [markup.mark for markup in markups]

        yield audio1, audio2, marks, markups


def calc_statistics(markups: List[MarkupResult], pool: QualityEvaluationPool, speaker: str):
    stats = Statistics(speaker)

    for audio1, audio2, marks, markups in group_markup_by_tasks(markups):
        sample = pool.find_sample(audio1, audio2)
        aggregated_votes = calc_votes(marks)
        stats.update(sample, aggregated_votes)

    return stats


def calc_speaker_statistic_from_markup(path_to_toloka_folder, speaker):
    files_in_toloka_folder = os.listdir(path_to_toloka_folder)
    if 'assignments.json' in files_in_toloka_folder:
        markups = read_markup_from_json(os.path.join(path_to_toloka_folder, 'assignments.json'))
    elif 'assignments.tsv' in files_in_toloka_folder:
        markups = read_assignments(os.path.join(path_to_toloka_folder, 'assignments.tsv'))
    else:
        raise Exception(f'No assignments in folder {path_to_toloka_folder}')
    pool = QualityEvaluationPool.read_from_tsv(os.path.join(path_to_toloka_folder, 'toloka_pool.tsv'))
    return calc_statistics(markups, pool, speaker)


class StatTable:
    def __init__(self):
        self.keys = []
        self.values = []

    def add_stat(self, key, value):
        self.keys.append(key)
        self.values.append(round(value, 3))

    def get_result_as_df(self):
        return pd.DataFrame(data=self.values, index=self.keys, columns=['percent_same'])


def print_valuable_metrics(statistics, with_absolutes=False):
    stat_table = StatTable()

    if with_absolutes:
        stat_table.add_stat('same_count', statistics.common_votes.major_same)
        stat_table.add_stat('differ_count', statistics.common_votes.major_differ)

    stat_table.add_stat('total', statistics.get_result())

    stat_table.add_stat('TRAIN', statistics.data_part["train"].percent_same)
    stat_table.add_stat('TEST', statistics.data_part["eval"].percent_same)

    if "True" in statistics.is_template:
        stat_table.add_stat('IS_TEMPLATE', statistics.is_template["True"].percent_same)
        stat_table.add_stat('IS_NOT_TEMPLATE', statistics.is_template["False"].percent_same)
    print(stat_table.get_result_as_df())
