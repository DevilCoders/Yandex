import typing
from random import shuffle

import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from .model import Recognition, MarkupCheck
from .tables import table_recognitions_meta, table_markups_check_meta, table_name


def generate_tasks_check(
    tasks_count: int,
    reference_length_limit: typing.Optional[int],
) -> typing.List[Recognition]:
    table_recognitions = Table(meta=table_recognitions_meta, name=table_name)
    recognitions = []
    for row in yt.read_table(table_recognitions.path):
        recognitions.append(Recognition.from_yson(row))

    table_markups = Table(meta=table_markups_check_meta, name=table_name)
    markups = []
    for row in yt.read_table(table_markups.path):
        markups.append(MarkupCheck.from_yson(row))

    checked_recognitions_ids = {markup.recognition_id for markup in markups}

    noise_levels_idx_of_interest = list(range(9, 59 + 1, 10))

    recognitions = [r for r in recognitions if r.id not in checked_recognitions_ids]
    shuffle(recognitions)

    references = set()

    filtered_recognitions = []
    for r in recognitions:
        if len(r.reference) > reference_length_limit:
            continue
        if r.reference in references:
            continue
        # if r.reference == r.hypothesis:
        #     continue
        # trivial meaning check, then hypothesis is equal to reference, is possible in this situation
        references.add(r.reference)
        if r.noise is None:
            filtered_recognitions.append(r)
        else:
            # strange filter for variant is to make no noise and noised recognitions appear equally likely
            if r.noise.variant == 3 and r.noise.level_idx in noise_levels_idx_of_interest:
                filtered_recognitions.append(r)
        if len(filtered_recognitions) >= tasks_count:
            break

    return filtered_recognitions


def generate_task_check_json(
    reference: str,
    hypothesis: str,
    expected_ok: typing.Optional[bool] = None,
) -> dict:
    task = {
        'input_values': {
            'reference': reference,
            'recognised': hypothesis,
        },
    }
    overlap = 5
    if expected_ok is not None:
        if expected_ok:
            task['known_solutions'] = [
                {
                    'correctness_weight': 1,
                    'output_values': {
                        'result': 'OK',
                    },
                },
            ]
        else:
            task['known_solutions'] = [
                {
                    'correctness_weight': 1,
                    'output_values': {
                        'result': 'BAD',
                    },
                },
                {
                    'correctness_weight': 1,
                    'output_values': {
                        'result': 'MIS',
                    },
                },
            ]
        overlap = 10000
    task['overlap'] = overlap
    return task


honeypots_check = [
    (
        'larch himself was a peculiar character',
        'larch himself was a peculiar person',
        True,
    ),
    (
        'the stillness was awful creepy and uncomfortable',
        'stillness was awful creepy and uncomfortable',
        True,
    ),
    (
        'your affectionate aunt adella hunter',
        'your affectionate aunt umbrella potato',
        False,
    ),
    (
        'but what was it',
        'but what what was it',
        True,
    ),
    (
        'you fret me beyond endurance',
        'you fret him beyond endurance',
        False,
    ),
    (
        'we\'ve got our man and that\'s all we want',
        'we\'ve got cameraman that\'s all we got',
        False,
    ),
    (
        'then i must request that you will not make it again very true',
        'then i must request that you will make it again very true',
        False,
    ),
    (
        'i believe poor annie is dreadfully unhappy',
        'i believe poor annie is unhappy',
        True,
    ),
    (
        'i can do that too',
        'i can\'t do that too',
        False,
    ),
    (
        'i shall say that you are in my own cabin so that i can care for you',
        'i shall say that you you are in my own cabin so that i can care for you',
        True,
    ),
    (
        'anna mikhaylovna opened the door',
        'anna mikhaylovna closed the door',
        False,
    ),
    (
        'why polonius some one asked',
        'why palladium some one asked',
        False,
    ),
    (
        'yes i knew it was a little wicked admitted anne',
        'i knew it was a little wicked admitted anne',
        True,
    ),
    (
        'after that every time he came to new york he used to call at sixty five late at night with his violin',
        'after that every time he came to new pork he used to call at sixty five late at night with his violin',
        False,
    ),
    (
        'he is on my pony',
        'well he is on my pony',
        True,
    ),
    (
        'and thus they journeyed onwards a long long way',
        'they journeyed onwards a long way',
        True,
    ),
    (
        'he wants you to forgive him',
        'you want him to forgive you',
        False,
    ),
    (
        'where where is sita',
        'where is sita',
        True,
    ),
    (
        'he was about to embrace his friend but nicholas avoided him',
        'a o o i i i e e e i',
        False,
    ),
    (
        'i saw your sign and i know a boy who needs the job',
        'i saw your sign i know a boy who needs the job',
        True,
    ),
    (
        'i believe in my heart that women are jealous of it as of a rival',
        'i believe that women are jealous of it as of a rival',
        True,
    ),
    (
        'you will do it as speedily as possible',
        'i will do it as speedily as possible',
        False,
    ),
    (
        'why need you conceal it if that is the truth',
        'but why need you conceal it if that is the truth',
        True,
    ),
    (
        'i did not expect you today he added',
        'i expected you today he added',
        False,
    ),
    (
        'nothing can be heard at the bottom of the garden really',
        'nothing can be heard at the bottom of the garden',
        True,
    ),
    (
        'man judges himself',
        'man judges',
        False,
    ),
    (
        'i\'m not coming said annie',
        'i\'m coming said annie',
        False,
    ),
    (
        'the duke says yes',
        'the puke says yes',
        False,
    ),
    (
        'well i admit it',
        'well well i admit it',
        True,
    ),
    (
        'and i say it\'s your picture',
        'and i say it\'s my picture',
        False,
    ),
    (
        'he will pay it at four o\'clock to day',
        'he will pay it at fought o\'clock to day',
        False,
    ),
    (
        'i have money',
        'i hair money',
        False,
    ),
    (
        'good night frank good night',
        'good night frank',
        True,
    ),
]
