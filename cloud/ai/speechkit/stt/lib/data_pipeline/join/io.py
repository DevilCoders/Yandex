import os
import typing
from collections import defaultdict

from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupDataVersions, MarkupData

from .common import RecordMarkupData

text_fetchers: typing.Dict[MarkupDataVersions, typing.Callable[[MarkupData], str]] = {
    MarkupDataVersions.PLAIN_TRANSCRIPT: lambda markup_data: markup_data.solution.text,
    MarkupDataVersions.TRANSCRIPT_AND_TYPE: lambda markup_data: markup_data.solution.text,
    MarkupDataVersions.CHECK_TRANSCRIPT: lambda markup_data: markup_data.input.text,
}


def marshal_input_for_markup_joiner(records_markups_data: typing.List[RecordMarkupData]) -> str:
    """
    Markups joiner executable input format:

    bits_count record_1_id_channel_1
    bit_0_start_ms markup_text_1
    bit_0_start_ms markup_text_2
    bit_0_start_ms markup_text_3
    bit_1_start_ms markup_text_1
    bit_1_start_ms markup_text_2
    ...
    bits_count record_1_id_channel_2
    ...
    bits_count record_2_id_channel_1
    ...

    We make joins for records as well as for records bits.
    """
    result = []
    for record_markup_data in records_markups_data:
        for record_channel_markup_data in record_markup_data.channels:
            record_channel_result = []
            record_channel_markups_count = 0
            for record_bit_markup_data in record_channel_markup_data.bits_markups:
                bit_markups_count = 0
                bit_data = record_bit_markup_data.bit_data
                bit_id = None
                for markups in record_bit_markup_data.version_to_markups.values():
                    bit_id = markups[0].bit_id
                    bit_markups_count += len(markups)

                record_channel_markups_count += bit_markups_count
                result.append(f'{bit_markups_count} {bit_id}')
                for markups in record_bit_markup_data.version_to_markups.values():
                    for markup in markups:
                        markup_data = markup.markup_data
                        if markup_data.version not in text_fetchers:
                            raise ValueError(f'Unsupported markup data: {markup_data}')
                        text = text_fetchers[markup_data.version](markup_data)
                        record_channel_result.append(f'{bit_data.start_ms} {text}')
                        result.append(f'0 {text}')
            result.append(
                f'{record_channel_markups_count} '
                f'{dump_record_with_channel_id(record_markup_data.record_id, record_channel_markup_data.channel)}')
            result += record_channel_result
    return '\n'.join(result)


def unmarshal_output_of_markup_joiner(output: str) -> typing.Dict[str, typing.Dict[int, str]]:
    """
    Markups joiner executable output format:

    record_1_id_channel_1 joined_text
    record_1_id_bit_1_id joined_text
    ...
    record_1_id_channel_2 joined_text
    ...
    record_2_id_channel_1 joined_text
    ...
    """
    entity_id_to_channel_texts = defaultdict(dict)
    for line in output.strip().split('\n'):
        entity_id, join = line.split(' ', 1)
        if entity_id[-4:-2] == '__':  # TODO: fix this ugly trick
            entity_id, channel = load_record_and_channel_id(entity_id)
        else:
            channel = 1
        entity_id_to_channel_texts[entity_id][channel] = join.strip()
    return entity_id_to_channel_texts


def dump_record_with_channel_id(record_id: str, channel: int) -> str:
    return f'{record_id}__{channel:02d}'


def load_record_and_channel_id(record_with_channel_id: str) -> typing.Tuple[str, int]:
    return record_with_channel_id[:-4], int(record_with_channel_id[-2:])


def call_markups_joiner(
    executable_path: str,
    bit_offset: int,
    records_markups_data: typing.List[RecordMarkupData],
) -> typing.Dict[str, typing.Dict[int, str]]:
    input_path = 'joiner_in.txt'
    output_path = 'joiner_out.txt'

    with open(input_path, 'w') as f:
        f.write(marshal_input_for_markup_joiner(records_markups_data))

    exit_code = os.system(f'{executable_path} --input {input_path} --bit_offset {bit_offset} --output {output_path}')

    if exit_code != 0:
        raise RuntimeError(f'Markup joiner executable finished with exit code {exit_code}')

    with open(output_path) as f:
        return unmarshal_output_of_markup_joiner(f.read())
