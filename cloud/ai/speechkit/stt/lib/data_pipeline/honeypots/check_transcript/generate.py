from enum import Enum
import json
from random import shuffle, choice, randrange
import typing

from cloud.ai.speechkit.stt.lib.data.model.common.s3_consts import cloud_endpoint, cloud_url, data_bucket
from cloud.ai.speechkit.stt.lib.data.model.dao import S3Object, MarkupTranscriptType
from cloud.ai.speechkit.stt.lib.data.ops.queries import (
    select_check_transcript_honeypots_candidates,
    CheckTranscriptHoneypotCandidateRow,
)

import cloud.ai.lib.python.log as log

from cloud.ai.lib.python.text.mutator import Mutator, MutatedText, MutationType

logger = log.get_logger(__name__)


class CheckTranscriptSolutionType(Enum):
    OK_SPEECH = 'ok-speech'
    OK_NO_SPEECH = 'ok-no-speech'
    NOT_OK_SPEECH = 'not-ok-speech'
    NOT_OK_NO_SPEECH = 'not-ok-no-speech'


solution_type_to_predicate: typing.Dict[
    CheckTranscriptSolutionType, typing.Callable[[CheckTranscriptHoneypotCandidateRow], bool]
] = {
    CheckTranscriptSolutionType.OK_SPEECH: lambda c: c.ok and c.type == MarkupTranscriptType.SPEECH,
    CheckTranscriptSolutionType.OK_NO_SPEECH: lambda c: c.ok and c.type == MarkupTranscriptType.NO_SPEECH,
    CheckTranscriptSolutionType.NOT_OK_SPEECH: lambda c: not c.ok and c.type == MarkupTranscriptType.SPEECH,
    CheckTranscriptSolutionType.NOT_OK_NO_SPEECH: lambda c: not c.ok and c.type == MarkupTranscriptType.NO_SPEECH,
}


def generate_honeypots(
    markups_table_name_ge: typing.Optional[str],
    random_sample_fraction: float,
    solution_type_to_count: typing.Dict[CheckTranscriptSolutionType, int],
    solution_min_overlap: int,
    text_min_length: int,
    text_min_words: int,
    transcript_honeypots_path: str,
    transcript_honeypots_mutated_ratio: float,
    overlap_honeypots_mutated_ratio: float,
    lang: str,
    levenshtein_dict: typing.Optional[typing.Dict[str, typing.List[str]]] = None,
) -> typing.List[dict]:

    logger.info(
        f"""Generating honeypots
    Desired solution type counts: {solution_type_to_count}
    Min solution overlap: {solution_min_overlap}
    Source: {'>=' + markups_table_name_ge if markups_table_name_ge else 'ALL'}, sample fraction: {random_sample_fraction}
    Transcript honeypots: {1. - transcript_honeypots_mutated_ratio} as is for OK, {transcript_honeypots_mutated_ratio} to mutate as not OK
    Overlap honeypots: {1. - overlap_honeypots_mutated_ratio} as is for OK, {overlap_honeypots_mutated_ratio} to mutate as not OK
    Text requirements:
        min length: {text_min_length}
        min words: {text_min_words}"""
    )

    honeypots_candidates = select_check_transcript_honeypots_candidates(
        markups_table_name_ge=markups_table_name_ge,
        random_sample_fraction=random_sample_fraction,
        solution_min_overlap=solution_min_overlap,
        text_min_length=text_min_length,
        text_min_words=text_min_words,
        lang=lang,
    )
    logger.info(f'{len(honeypots_candidates)} honeypots candidates received from YQL query')

    # We already make shuffle through table samples in YQL, but let's do it again just in case
    shuffle(honeypots_candidates)

    honeypots = extract_solution_types(honeypots_candidates, solution_type_to_count)
    speech_ok_honeypots = [
        honeypot for honeypot in honeypots if
        solution_type_to_predicate[CheckTranscriptSolutionType.OK_SPEECH](honeypot) and
        honeypot.ok
    ]

    other_honeypots = [
        honeypot for honeypot in honeypots if not
        (solution_type_to_predicate[CheckTranscriptSolutionType.OK_SPEECH](honeypot) and
         honeypot.ok)
    ]

    overlap_honeypots = other_honeypots + generate_mutated_honeypots(
        speech_ok_honeypots,
        overlap_honeypots_mutated_ratio,
        from_overlap=True,
        lang=lang,
        levenshtein_dict=levenshtein_dict,
    )

    honeypots_repr = '\n'.join(h.__repr__() for h in overlap_honeypots)
    logger.info(f'Check transcript honeypots:\n{honeypots_repr}')

    with open(transcript_honeypots_path) as f:
        raw_transcript_honeypots = [get_check_honeypot_from_toloka_transcript_dict(hd) for hd in json.load(f)]

    transcript_honeypots = generate_mutated_honeypots(
        [h for h in raw_transcript_honeypots if h.type == MarkupTranscriptType.SPEECH],
        transcript_honeypots_mutated_ratio, from_overlap=False, lang=lang, levenshtein_dict=levenshtein_dict,
    )

    transcript_honeypots_repr = '\n'.join(h.__repr__() for h in transcript_honeypots)
    logger.info(f'Transcript honeypots:\n{transcript_honeypots_repr}')

    honeypots_data = [
        get_check_toloka_dict_from_check_honeypot(h) for h in (
            overlap_honeypots + transcript_honeypots
        )
    ]

    # Just in case of Toloka mixer glitches
    shuffle(honeypots_data)

    # Quick & dirty, fix lack of "cls" input field
    for honeypot in honeypots_data:
        input_values = honeypot['input_values']
        if 'cls' in input_values:
            continue
        cls = 'sp' if input_values['text'] else 'si'
        input_values['cls'] = cls

    return honeypots_data


def extract_solution_types(
    honeypots_candidates: typing.List[CheckTranscriptHoneypotCandidateRow],
    solution_type_to_count: typing.Dict[CheckTranscriptSolutionType, int],
) -> typing.List[CheckTranscriptHoneypotCandidateRow]:
    result = []
    for solution_type, count in solution_type_to_count.items():
        matched = 0
        predicate = solution_type_to_predicate[solution_type]
        for hc in honeypots_candidates:
            if predicate(hc):
                result.append(hc)
                matched += 1
            if matched >= count:
                break
        if matched < count:
            raise ValueError(f'Not enough honeypots candidates: {matched}/{count} solutions of type {solution_type}')
    return result


def get_check_honeypot_from_toloka_transcript_dict(honeypot_data: dict) -> CheckTranscriptHoneypotCandidateRow:
    return CheckTranscriptHoneypotCandidateRow(
        audio_s3_key=honeypot_data['input_values']['url'][len(f'{cloud_url}/{data_bucket}/'):],
        ok=True,
        text=honeypot_data['known_solutions'][0]['output_values']['text'],
        type=MarkupTranscriptType.from_toloka_output(honeypot_data['known_solutions'][0]['output_values']['cls']),
    )


def get_check_toloka_dict_from_check_honeypot(honeypot: CheckTranscriptHoneypotCandidateRow) -> dict:
    return {
        'input_values': {
            'url': S3Object(endpoint=cloud_endpoint, bucket=data_bucket, key=honeypot.audio_s3_key).to_https_url(),
            'text': honeypot.text,
        },
        'known_solutions': [
            {
                'correctness_weight': 1,
                'output_values': {
                    'ok': bool(honeypot.ok),
                    'cls': honeypot.type.to_toloka_output(),
                },
            }
        ],
    }


def generate_mutated_honeypots(
    honeypots: typing.List[CheckTranscriptHoneypotCandidateRow],
    mutated_honeypots_ratio: float,
    from_overlap: bool,
    lang: str,
    levenshtein_dict: typing.Optional[typing.Dict[str, typing.List[str]]] = None,
):
    assert 0.0 <= mutated_honeypots_ratio <= 1.0
    not_ok_count = int(len(honeypots) * mutated_honeypots_ratio)

    result = []
    mutated_count = 0
    mutations_kwargs_overrides = {}
    if levenshtein_dict is not None:
        mutations_kwargs_overrides[MutationType.CLOSE_LEV_WORD] = {'vocab': levenshtein_dict}

    mt = Mutator(lang=lang, mutations_kwargs_overrides=mutations_kwargs_overrides)

    for honeypot in honeypots:
        assert honeypot.text != ''
        if '?' in honeypot.text or mutated_count >= not_ok_count:
            result.append(honeypot)
        else:
            mutated_result = mutate_text(mt, honeypot, from_overlap)
            if not mutated_result.changed:
                result.append(honeypot)
            else:
                mutated_count += 1
                result.append(
                    CheckTranscriptHoneypotCandidateRow(
                        audio_s3_key=honeypot.audio_s3_key,
                        ok=False,
                        text=mutated_result.new_text,
                        type=MarkupTranscriptType.SPEECH,
                    )
                )
    logger.info(
        f"""generated honeypots from {'transcription' if from_overlap else 'overlap'}:
            OK: {len(result) - mutated_count} / {len(honeypots) - not_ok_count}
            NOT OK: {mutated_count} / {not_ok_count}"""
    )
    return result


def mutate_text(mt: Mutator, honeypot: CheckTranscriptHoneypotCandidateRow, from_overlap: bool) -> MutatedText:
    result = mt.mutate(honeypot.text)
    if result.changed:
        logger.info(
            f"""
    mutate {'transcription' if from_overlap else 'overlap'} honeypot: audio {honeypot.audio_s3_key}
    original text: {honeypot.text}
    mutated text: {result.new_text}
    mutations: {result.applied_mutations}"""
        )
    return result
