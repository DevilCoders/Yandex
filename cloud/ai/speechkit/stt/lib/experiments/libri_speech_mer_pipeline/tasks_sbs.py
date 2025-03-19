import typing
from collections import defaultdict
from dataclasses import dataclass
from random import shuffle, choice, randrange

import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from .model import Recognition, MarkupSbS
from .tables import table_recognitions_meta, table_markups_sbs_meta, table_name


@dataclass
class RecordDescriptor:
    dataset: str
    record: str

    def __eq__(self, other):
        return self.dataset == other.dataset and self.record == other.record

    def __hash__(self):
        return hash(self.dataset) ^ hash(self.record)


def generate_tasks_and_honeypots_sbs(
    tasks_count: int,
    honeypots_count: int,
    reference_length_limit: typing.Optional[int],
) -> typing.Tuple[
    typing.List[typing.Tuple[Recognition, Recognition, str]],
    typing.List[typing.Tuple[Recognition, Recognition, str]],
]:
    table_recognitions = Table(meta=table_recognitions_meta, name=table_name)
    recognitions = []
    for row in yt.read_table(table_recognitions.path):
        recognitions.append(Recognition.from_yson(row))

    table_markups = Table(meta=table_markups_sbs_meta, name=table_name)
    markups = []
    for row in yt.read_table(table_markups.path):
        markups.append(MarkupSbS.from_yson(row))

    # Recognitions selection strategy for task:
    # - select recognitions without any markup
    # - build recognitions pairs for some fixed (dataset, record, model) for comparison,
    #   each recognition corresponds to some (noise_level, variant). Using noise levels
    #   difference we get comparison prior.
    # - do not get reference and hypotheses which equal to each other
    # - pool with contain only unique references to match pool output

    compared_recognitions_pairs = set()
    for markup in markups:
        compared_recognitions_pairs.add((markup.recognition_id_left, markup.recognition_id_right))
        compared_recognitions_pairs.add((markup.recognition_id_right, markup.recognition_id_left))

    references = set()

    tasks = build_comparison_tasks(recognitions, references, reference_length_limit, compared_recognitions_pairs,
                                   tasks_count, False)

    # Recognitions selection strategy for honeypot is the same as for real task,
    # but noise level difference is much more to get more reliable prior which
    # we will use as expected solution.

    honeypots = build_comparison_tasks(recognitions, references, reference_length_limit, compared_recognitions_pairs,
                                       honeypots_count, True)

    return tasks, honeypots


def build_comparison_tasks(
    recognitions: typing.List[Recognition],
    references: typing.Set[str],
    reference_length_limit: typing.Optional[int],
    compared_recognitions_pairs: typing.Set[typing.Tuple[str, str]],
    tasks_count: int,
    honeypots: bool,
) -> typing.List[
    typing.Tuple[Recognition, Recognition, str],
]:
    record_to_recognitions = defaultdict(list)
    for recognition in recognitions:
        record_to_recognitions[RecordDescriptor(
            dataset=recognition.dataset,
            record=recognition.record,
        )].append(recognition)

    shuffled_records = list(record_to_recognitions.keys())
    shuffle(shuffled_records)

    tasks = []
    for record in shuffled_records:
        less_noise_recognition, more_noise_recognition, reference = select_recognitions_pair(
            record_to_recognitions[record], for_honeypot=honeypots,
        )

        if reference_length_limit is not None and len(reference) > reference_length_limit:
            continue
        if any(len(t) == 0 for t in (reference, less_noise_recognition.hypothesis, more_noise_recognition.hypothesis)):
            continue
        if reference in references:
            continue
        # if len({reference, less_noise_recognition.hypothesis, more_noise_recognition.hypothesis}) != 3:
        if less_noise_recognition.hypothesis == more_noise_recognition.hypothesis:
            # trivial comparisons, then one of hypothesis is equal to reference, is possible in this situation
            continue
        if not honeypots and (less_noise_recognition.id, more_noise_recognition.id) in compared_recognitions_pairs:
            continue

        references.add(reference)

        tasks.append((less_noise_recognition, more_noise_recognition, reference))
        if len(tasks) == tasks_count:
            break

    assert len(tasks) == tasks_count, 'insufficient tasks'

    return tasks


#  0      1           160
# [0.025, 0.050, ..., 0.400]
noise_level_step = 0.0025
noise_levels_count = 160
noise_levels_idx = list(range(noise_levels_count))
noise_levels = [(i + 1) * noise_level_step for i in noise_levels_idx]

variants_per_noise_level = 5


def select_recognitions_pair(recognitions: typing.List[Recognition], for_honeypot: bool) -> typing.Tuple[
    Recognition, Recognition, str,
]:
    # in this function we consider 0 noise level index as no noise case

    if for_honeypot:
        # compare not noisy hypothesis to some very noisy hypothesis for reliable prior
        less_noise_level = 0
        noise_diffs_range = list(range(100, 140 + 1, 20))
    else:
        # compare noise levels around 10% to 10%:
        # 0%, 2.5%, 5%, ..., 15%
        less_noise_level = 40
        noise_diffs_range = [20]  # list(range(-40, 20, 10))

    more_noise_level = less_noise_level + choice(noise_diffs_range)

    assert 0 <= more_noise_level < len(noise_levels_idx)

    if less_noise_level > more_noise_level:
        # in case of negative diff value
        less_noise_level, more_noise_level = more_noise_level, less_noise_level

    model = choice(('Jasper10x5Dr-En', 'QuartzNet15x5NR-En'))

    less_noise_variant = randrange(variants_per_noise_level)
    more_noise_variant = randrange(variants_per_noise_level)

    def find_recognition(m, nl, v):
        for r in recognitions:
            if nl == 0:
                if r.model == m and r.noise is None:
                    return r
            else:
                # nl - 1 because first element in our index array is "no noise"
                if r.model == m and r.noise is not None and r.noise.level_idx == nl - 1 and r.noise.variant == v:
                    return r
        raise RuntimeError(f'Recognition for model {m}, noise level {nl}, variant {v}, not found among {recognitions}')

    less_noise_recognition = find_recognition(model, less_noise_level, less_noise_variant)
    more_noise_recognition = find_recognition(model, more_noise_level, more_noise_variant)

    reference = less_noise_recognition.reference  # references in all recognitions are equal indeed

    return less_noise_recognition, more_noise_recognition, reference


def generate_task_sbs_json(
    data: typing.Tuple[Recognition, Recognition, str],
    for_honeypot: bool,
) -> dict:
    less_noise_recognition, more_noise_recognition, reference = data
    task = {
        'input_values': {
            'orig_phrase': reference,
            'left_phrase': less_noise_recognition.hypothesis,
            'right_phrase': more_noise_recognition.hypothesis,
        },
    }
    overlap = 5
    if for_honeypot:
        task['known_solutions'] = [
            {
                'correctness_weight': 1,
                'output_values': {
                    'result': 'LEFT',
                },
            },
        ]
        overlap = 10000
    task['overlap'] = overlap
    return task


def print_task(data: typing.Tuple[Recognition, Recognition, str]):
    less_noise_recognition, more_noise_recognition, reference = data
    print(f"""
reference:  {reference}
less noise: {less_noise_recognition.hypothesis}
more noise: {more_noise_recognition.hypothesis}
""".strip() + '\n')
