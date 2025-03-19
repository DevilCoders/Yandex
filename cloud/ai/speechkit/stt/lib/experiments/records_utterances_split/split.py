import re
import typing
from dataclasses import dataclass
from random import uniform


@dataclass
class Fragment:
    text: str
    start_time_ms: int
    end_time_ms: int

    def to_yson(self) -> dict:
        return {
            'text': self.text,
            'start_time_ms': self.start_time_ms,
            'end_time_ms': self.end_time_ms,
        }


def unmarshal_aligner_record_output(data: dict) -> typing.List[Fragment]:
    text = data['aligned_text']['texts'][0]
    if len(text) == 0:
        return []
    return [
        Fragment(
            text=word['word'],
            start_time_ms=word.get('startTimeMs', 0),
            end_time_ms=word.get('startTimeMs', 0) + word['durationMs'],
        )
        for word in text['words']
    ]


def split_record_by_utterances(
    words: typing.List[Fragment],
    min_ms_between_utterances: int,
) -> typing.List[Fragment]:
    if len(words) == 0:
        return []

    # example, words_indexes_with_long_silence_before indicated by *
    #
    # [500ms <word_1> 700ms] [800ms <word_2> 1200ms] * [5000ms <word_3> 5300ms] ...

    for i in range(len(words)):
        if i > 0:
            assert words[i - 1].end_time_ms <= words[i].start_time_ms
        assert re.match('^[а-яё]+$', words[i].text) is not None

    words_indexes_with_long_silence_before = []
    for i in range(len(words) - 1):
        if words[i + 1].start_time_ms - words[i].end_time_ms >= min_ms_between_utterances:
            words_indexes_with_long_silence_before.append(i + 1)

    utterances_words_index_ranges = [
        (0, len(words) - 1 \
            if len(words_indexes_with_long_silence_before) == 0 \
            else words_indexes_with_long_silence_before[0] - 1)
    ]
    for i in range(len(words_indexes_with_long_silence_before)):
        start_index = words_indexes_with_long_silence_before[i]
        end_index = len(words) - 1 \
            if i == len(words_indexes_with_long_silence_before) - 1 \
            else words_indexes_with_long_silence_before[i + 1] - 1
        utterances_words_index_ranges.append((start_index, end_index))

    return [
        Fragment(
            text=' '.join(word.text for word in words[start_index:end_index + 1]),
            start_time_ms=words[start_index].start_time_ms,
            end_time_ms=words[end_index].end_time_ms,
        )
        for start_index, end_index in utterances_words_index_ranges
    ]


def fetch_bits_no_speech(
    utterances_with_padding: typing.List[Fragment],
    min_bit_duration_ms: int,
    max_bit_duration_ms: int,
) -> typing.List[Fragment]:
    bits = []
    for i in range(len(utterances_with_padding) - 1):
        curr_utterance = utterances_with_padding[i]
        next_utterance = utterances_with_padding[i + 1]

        start_time_ms, end_time_ms = curr_utterance.end_time_ms, next_utterance.start_time_ms
        no_speech_duration_ms = end_time_ms - start_time_ms
        if no_speech_duration_ms < min_bit_duration_ms:
            continue
        elif no_speech_duration_ms > max_bit_duration_ms:
            # No speech duration is too large, select some random sub-interval.
            # Get start_time point randomly in [0, 0.5) source interval and adjust max possible end_time
            start_time_ms = start_time_ms + int(no_speech_duration_ms * uniform(0, 1) / 2)
            end_time_ms = min(start_time_ms + max_bit_duration_ms, end_time_ms)
            sub_duration_ms = end_time_ms - start_time_ms
            assert sub_duration_ms <= max_bit_duration_ms
            if sub_duration_ms < min_bit_duration_ms:
                continue

        bits.append(Fragment(
            text='',
            start_time_ms=start_time_ms,
            end_time_ms=end_time_ms,
        ))

    return bits


def apply_padding_to_utterances(
    utterances: typing.List[Fragment],
    padding_ms: int,
    record_duration_ms: int,
) -> typing.List[Fragment]:
    return [
        Fragment(
            text=utterance.text,
            start_time_ms=max(0, utterance.start_time_ms - padding_ms),
            end_time_ms=min(record_duration_ms, utterance.end_time_ms + padding_ms),
        )
        for utterance in utterances
    ]
