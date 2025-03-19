import typing
from random import shuffle

from cloud.ai.speechkit.stt.lib.data.model.common.s3_consts import cloud_endpoint, data_bucket
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    S3Object,
    MarkupTranscriptType,
    MarkupDataVersions,
    active_transcript_markup_version,
)
from cloud.ai.speechkit.stt.lib.data.ops.queries import (
    select_transcript_honeypots_candidates,
    TranscriptHoneypotCandidateRow,
)


def generate_honeypots(
    markups_table_name_ge: typing.Optional[str],
    random_sample_fraction: float,
    honeypots_with_speech_count: int,
    text_min_length: int,
    text_min_words: int,
    solution_min_overlap: int,
    max_unique_texts: int,
    from_honeypots: bool,
    words_blacklist: typing.List[str],
    lang: str,
) -> typing.List[dict]:
    print(
        f"""Generating honeypots
    Desired count of honeypots with speech: {honeypots_with_speech_count}
    Source: {'>=' + markups_table_name_ge if markups_table_name_ge else 'ALL'}, sample fraction: {random_sample_fraction}
    Text requirements:
        min length: {text_min_length}
        min words: {text_min_words}
    Solution requirements:
        min overlap: {solution_min_overlap}
        max unique texts: {max_unique_texts}
    From honeypots: {from_honeypots}
    Words blacklist: {words_blacklist}"""
    )

    honeypots = select_transcript_honeypots_candidates(
        markups_table_name_ge=markups_table_name_ge,
        random_sample_fraction=random_sample_fraction,
        text_min_length=text_min_length,
        text_min_words=text_min_words,
        solution_min_overlap=solution_min_overlap,
        max_unique_texts=max_unique_texts,
        from_honeypots=from_honeypots,
        lang=lang,
    )
    print(f'{len(honeypots)} honeypots candidates received from YQL query')

    # We already make shuffle through table samples in YQL, but let's do it again just in case
    shuffle(honeypots)

    honeypots, scanned_count = filter_honeypots_candidates_by_words_blacklist(
        honeypots_candidates=honeypots,
        words_blacklist=words_blacklist,
        limit=honeypots_with_speech_count,
    )
    print(f'Filtered {len(honeypots)} unique and non-blacklisted words texts from {scanned_count} candidates')

    if len(honeypots) < honeypots_with_speech_count:
        raise RuntimeError(
            f'Not enough honeypots: {len(honeypots)}/{honeypots_with_speech_count}. '
            'Try to use more wide markup table name pattern or use larger random sample fraction'
        )

    # Just in case of Toloka mixer glitches
    shuffle(honeypots)

    return [get_honeypot_toloka_data(h.audio_s3_key, h.texts, type=MarkupTranscriptType.SPEECH) for h in honeypots]


def filter_honeypots_candidates_by_words_blacklist(
    honeypots_candidates: typing.List[TranscriptHoneypotCandidateRow],
    words_blacklist: typing.List[str],
    limit: int,
) -> typing.Tuple[typing.List[TranscriptHoneypotCandidateRow], int]:
    words_blacklist = set(words_blacklist)
    result = []
    scanned_count = 0
    for hc in honeypots_candidates:
        scanned_count += 1
        if any(word in words_blacklist for word in ' '.join(hc.texts).split(' ')):
            continue
        result.append(hc)
        if len(result) >= limit:
            break
    return result, scanned_count


def get_honeypot_toloka_data(
    audio_s3_key: str, texts: typing.List[str], type: MarkupTranscriptType,
) -> dict:
    if type != MarkupTranscriptType.SPEECH and len(texts) > 1:
        raise ValueError('Non SPEECH type with multiple texts')
    if type == MarkupTranscriptType.SPEECH and any(text == '' for text in texts):
        raise ValueError('Empty text with SPEECH type')
    res = {
        'input_values': {'url': S3Object(endpoint=cloud_endpoint, bucket=data_bucket, key=audio_s3_key).to_https_url()},
        'known_solutions': [
            {
                'correctness_weight': 1,
                'output_values': {
                    'text': text,
                },
            }
            for text in texts
        ],
    }
    if active_transcript_markup_version == MarkupDataVersions.PLAIN_TRANSCRIPT:
        assert len(texts) == 1
        res['known_solutions'][0]['output_values']['misheard'] = texts[0] == ''
    elif active_transcript_markup_version == MarkupDataVersions.TRANSCRIPT_AND_TYPE:
        for known_solution in res['known_solutions']:
            known_solution['output_values']['cls'] = type.to_toloka_output()
    else:
        raise ValueError(f'Unexpected active markup project: {active_transcript_markup_version}')
    return res
