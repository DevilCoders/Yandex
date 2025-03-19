import typing
from random import shuffle

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecordBitMarkup,
    BitDataTimeInterval,
    MarkupData,
    MarkupDataVersions,
    MarkupSolutionTranscriptAndType,
    MarkupTranscriptType,
    MarkupSolution,
    MarkupSolutionPlainTranscript,
    MarkupInput,
    MarkupSolutionCheckTranscript,
    AudioURLAndTranscriptInput,
    MarkupStep,
)

"""
Markup cases we want to check:
- majority votes
  - no majority vote
  - majority vote
    - check transcript
      - ok speech
      - ok no speech
    - transcript, no speech
- mono-channel and multi-channel records
- all bits no speech
  - record with one of two channels all bits no speech
  - record with both channels all bits no speech
  - mono-channel record all bits no speech
"""


def create_plain_transcript_markup(
    record_id: str,
    record_bit_id: str,
    markup_index: int,
    text: str,
    misheard: bool = False,
) -> RecordBitMarkup:
    return create_markup(
        record_id=record_id,
        record_bit_id=record_bit_id,
        markup_index=markup_index,
        version=MarkupDataVersions.PLAIN_TRANSCRIPT,
        input=None,
        solution=MarkupSolutionPlainTranscript(text=text, misheard=misheard),
        markup_step=MarkupStep.TRANSCRIPT,
    )


def create_transcript_and_type_markup(
    record_id: str,
    record_bit_id: str,
    markup_index: int,
    text: str,
    type: MarkupTranscriptType = MarkupTranscriptType.SPEECH,
) -> RecordBitMarkup:
    return create_markup(
        record_id=record_id,
        record_bit_id=record_bit_id,
        markup_index=markup_index,
        version=MarkupDataVersions.TRANSCRIPT_AND_TYPE,
        input=None,
        solution=MarkupSolutionTranscriptAndType(text=text, type=type),
        markup_step=MarkupStep.TRANSCRIPT,
    )


# noinspection PyTypeChecker
def create_check_transcript_markup(
    record_id: str,
    record_bit_id: str,
    markup_index: int,
    text: str,
    ok: bool,
    type: MarkupTranscriptType,
) -> RecordBitMarkup:
    return create_markup(
        record_id=record_id,
        record_bit_id=record_bit_id,
        markup_index=markup_index,
        version=MarkupDataVersions.CHECK_TRANSCRIPT,
        input=AudioURLAndTranscriptInput(audio_s3_obj=None, text=text, text_source=None),
        solution=MarkupSolutionCheckTranscript(ok=ok, type=type),
        markup_step=MarkupStep.CHECK_ASR_TRANSCRIPT,
    )


# noinspection PyTypeChecker
def create_markup(
    record_id: str,
    record_bit_id: str,
    markup_index: int,
    version: MarkupDataVersions,
    input: typing.Optional[MarkupInput],
    solution: MarkupSolution,
    markup_step: MarkupStep,
) -> RecordBitMarkup:
    return RecordBitMarkup(
        record_id=record_id,
        bit_id=record_bit_id,
        id=record_bit_id + '/' + str(markup_index),
        audio_obfuscation_data=None,
        audio_params=None,
        markup_id=None,
        markup_step=markup_step,
        pool_id=None,
        assignment_id=None,
        markup_data=MarkupData(
            version=version,
            input=input,
            solution=solution,
            known_solutions=None,
            task_id=None,
            overlap=None,
            raw_data=None,
            created_at=None,
        ),
        validation_data=None,
        received_at=None,
        other=None,
    )


# record 1, two channels, 6 bits per channel
# channel 1, bit 1: check transcript no majority vote by 1/3, plain transcript majority vote on no speech by 1/1
# channel 1, bit 2: transcript and type no majority vote by 1/2
# channel 1, bit 3: check transcript no majority vote by 1/2, transcript and type no majority vote by 2/4
# channel 1, bit 4: check transcript majority vote ok empty text by 2/3
# channel 1, bit 5: check transcript majority vote ok not empty text by 3/5
# channel 1, bit 6: check transcript no majority vote by 1/3, plain transcript no majority vote by 2/4
# channel 2, bit 1: check transcript majority vote ok not empty text by 1/1
# channel 2, bit 2: check transcript no majority vote by 1/2, transcript and type majority vote on no speech by 2/3
# channel 2, bit 3: transcript and type majority vote no speech by 1/1
# channel 2, bit 4: transcript and type no majority vote by 1/3
# channel 2, bit 5: check transcript majority vote ok not empty text by 2/2
# channel 2, bit 6: check transcript majority vote ok empty text by 2/3
record1_id = 'record1'

record1_channel1_bit1_id, record1_channel1_bit1_data = (
    'record1/1/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=1),
)
record1_channel1_bit2_id, record1_channel1_bit2_data = (
    'record1/1/bit2',
    BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=1),
)
record1_channel1_bit3_id, record1_channel1_bit3_data = (
    'record1/1/bit3',
    BitDataTimeInterval(start_ms=6000, end_ms=15000, index=2, channel=1),
)
record1_channel1_bit4_id, record1_channel1_bit4_data = (
    'record1/1/bit4',
    BitDataTimeInterval(start_ms=9000, end_ms=18000, index=3, channel=1),
)
record1_channel1_bit5_id, record1_channel1_bit5_data = (
    'record1/1/bit5',
    BitDataTimeInterval(start_ms=12000, end_ms=21000, index=4, channel=1),
)
record1_channel1_bit6_id, record1_channel1_bit6_data = (
    'record1/1/bit6',
    BitDataTimeInterval(start_ms=15000, end_ms=22300, index=5, channel=1),
)

record1_channel2_bit1_id, record1_channel2_bit1_data = (
    'record1/2/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=2),
)
record1_channel2_bit2_id, record1_channel2_bit2_data = (
    'record1/2/bit2',
    BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=2),
)
record1_channel2_bit3_id, record1_channel2_bit3_data = (
    'record1/2/bit3',
    BitDataTimeInterval(start_ms=6000, end_ms=15000, index=2, channel=2),
)
record1_channel2_bit4_id, record1_channel2_bit4_data = (
    'record1/2/bit4',
    BitDataTimeInterval(start_ms=9000, end_ms=18000, index=3, channel=2),
)
record1_channel2_bit5_id, record1_channel2_bit5_data = (
    'record1/2/bit5',
    BitDataTimeInterval(start_ms=12000, end_ms=21000, index=4, channel=2),
)
record1_channel2_bit6_id, record1_channel2_bit6_data = (
    'record1/2/bit6',
    BitDataTimeInterval(start_ms=15000, end_ms=22300, index=5, channel=2),
)

record1_channel1_bit1_markup1 = create_check_transcript_markup(
    record1_id, record1_channel1_bit1_id, 1, text='добрый пень', ok=False, type=MarkupTranscriptType.SPEECH,
)
record1_channel1_bit1_markup2 = create_check_transcript_markup(
    record1_id, record1_channel1_bit1_id, 2, text='добрый пень', ok=True, type=MarkupTranscriptType.SPEECH,
)
record1_channel1_bit1_markup3 = create_check_transcript_markup(
    record1_id, record1_channel1_bit1_id, 3, text='добрый пень', ok=False, type=MarkupTranscriptType.UNCLEAR,
)
record1_channel1_bit1_markup4 = create_plain_transcript_markup(
    record1_id, record1_channel1_bit1_id, 4, text='?',
)

record1_channel1_bit2_markup1 = create_transcript_and_type_markup(
    record1_id, record1_channel1_bit2_id, 1, text='у меня такая',
)
record1_channel1_bit2_markup2 = create_transcript_and_type_markup(
    record1_id, record1_channel1_bit2_id, 2, text='', type=MarkupTranscriptType.NO_SPEECH,
)

record1_channel1_bit3_markup1 = create_check_transcript_markup(
    record1_id, record1_channel1_bit3_id, 1, text='такая проблема', ok=True, type=MarkupTranscriptType.SPEECH,
)
record1_channel1_bit3_markup2 = create_check_transcript_markup(
    record1_id, record1_channel1_bit3_id, 2, text='такая проблема', ok=False, type=MarkupTranscriptType.SPEECH,
)
record1_channel1_bit3_markup3 = create_transcript_and_type_markup(
    record1_id, record1_channel1_bit3_id, 3, text='такая проблемка',
)
record1_channel1_bit3_markup4 = create_transcript_and_type_markup(
    record1_id, record1_channel1_bit3_id, 4, text='', type=MarkupTranscriptType.UNCLEAR,
)
record1_channel1_bit3_markup5 = create_transcript_and_type_markup(
    record1_id, record1_channel1_bit3_id, 5, text='', type=MarkupTranscriptType.NO_SPEECH
)
record1_channel1_bit3_markup6 = create_transcript_and_type_markup(
    record1_id, record1_channel1_bit3_id, 6, text='макая ракетку',
)

record1_channel1_bit4_markup1 = create_check_transcript_markup(
    record1_id, record1_channel1_bit4_id, 1, text='', ok=True, type=MarkupTranscriptType.NO_SPEECH
)
record1_channel1_bit4_markup2 = create_check_transcript_markup(
    record1_id, record1_channel1_bit4_id, 2, text='', ok=True, type=MarkupTranscriptType.NO_SPEECH
)
record1_channel1_bit4_markup3 = create_check_transcript_markup(
    record1_id, record1_channel1_bit4_id, 3, text='', ok=False, type=MarkupTranscriptType.SPEECH
)

record1_channel1_bit5_markup1 = create_check_transcript_markup(
    record1_id, record1_channel1_bit5_id, 1, text='с оплатой интернета', ok=True, type=MarkupTranscriptType.SPEECH
)
record1_channel1_bit5_markup2 = create_check_transcript_markup(
    record1_id, record1_channel1_bit5_id, 2, text='с оплатой интернета', ok=False, type=MarkupTranscriptType.UNCLEAR
)
record1_channel1_bit5_markup3 = create_check_transcript_markup(
    record1_id, record1_channel1_bit5_id, 3, text='с оплатой интернета', ok=True, type=MarkupTranscriptType.SPEECH
)
record1_channel1_bit5_markup4 = create_check_transcript_markup(
    record1_id, record1_channel1_bit5_id, 4, text='с оплатой интернета', ok=False, type=MarkupTranscriptType.SPEECH
)
record1_channel1_bit5_markup5 = create_check_transcript_markup(
    record1_id, record1_channel1_bit5_id, 5, text='с оплатой интернета', ok=True, type=MarkupTranscriptType.SPEECH
)

record1_channel1_bit6_markup1 = create_check_transcript_markup(
    record1_id, record1_channel1_bit6_id, 1, text='', ok=False, type=MarkupTranscriptType.SPEECH
)
record1_channel1_bit6_markup2 = create_check_transcript_markup(
    record1_id, record1_channel1_bit6_id, 2, text='', ok=False, type=MarkupTranscriptType.SPEECH
)
record1_channel1_bit6_markup3 = create_check_transcript_markup(
    record1_id, record1_channel1_bit6_id, 3, text='', ok=False, type=MarkupTranscriptType.UNCLEAR
)
record1_channel1_bit6_markup4 = create_plain_transcript_markup(
    record1_id, record1_channel1_bit6_id, 4, text='', misheard=True,
)
record1_channel1_bit6_markup5 = create_plain_transcript_markup(
    record1_id, record1_channel1_bit6_id, 5, text='так',
)
record1_channel1_bit6_markup6 = create_plain_transcript_markup(
    record1_id, record1_channel1_bit6_id, 6, text='-',
)
record1_channel1_bit6_markup7 = create_plain_transcript_markup(
    record1_id, record1_channel1_bit6_id, 7, text='как',
)

record1_channel2_bit1_markup1 = create_check_transcript_markup(
    record1_id, record1_channel2_bit1_id, 1, text='меня зовут павел чем могу помочь',
    ok=True, type=MarkupTranscriptType.SPEECH,
)

record1_channel2_bit2_markup1 = create_check_transcript_markup(
    record1_id, record1_channel2_bit2_id, 1, text='алиса',
    ok=True, type=MarkupTranscriptType.SPEECH,
)
record1_channel2_bit2_markup2 = create_check_transcript_markup(
    record1_id, record1_channel2_bit2_id, 2, text='',
    ok=False, type=MarkupTranscriptType.UNCLEAR,
)
record1_channel2_bit2_markup3 = create_transcript_and_type_markup(
    record1_id, record1_channel2_bit2_id, 3, text='', type=MarkupTranscriptType.NO_SPEECH,
)
record1_channel2_bit2_markup4 = create_transcript_and_type_markup(
    record1_id, record1_channel2_bit2_id, 4, text='', type=MarkupTranscriptType.UNCLEAR,
)
record1_channel2_bit2_markup5 = create_transcript_and_type_markup(
    record1_id, record1_channel2_bit2_id, 5, text='кек',
)

record1_channel2_bit3_markup1 = create_transcript_and_type_markup(
    record1_id, record1_channel2_bit3_id, 1, text='', type=MarkupTranscriptType.UNCLEAR,
)

record1_channel2_bit4_markup1 = create_transcript_and_type_markup(
    record1_id, record1_channel2_bit4_id, 1, text='', type=MarkupTranscriptType.NO_SPEECH,
)
record1_channel2_bit4_markup2 = create_transcript_and_type_markup(
    record1_id, record1_channel2_bit4_id, 2, text='подскажите номер лицевого лота',
)
record1_channel2_bit4_markup3 = create_transcript_and_type_markup(
    record1_id, record1_channel2_bit4_id, 3, text='подскажите номер ? счета',
)

record1_channel2_bit5_markup1 = create_check_transcript_markup(
    record1_id, record1_channel2_bit5_id, 1, text='счета для проверки', ok=True, type=MarkupTranscriptType.SPEECH,
)
record1_channel2_bit5_markup2 = create_check_transcript_markup(
    record1_id, record1_channel2_bit5_id, 2, text='счета для проверки', ok=True, type=MarkupTranscriptType.SPEECH,
)

record1_channel2_bit6_markup1 = create_check_transcript_markup(
    record1_id, record1_channel2_bit6_id, 1, text='', ok=True, type=MarkupTranscriptType.NO_SPEECH,
)
record1_channel2_bit6_markup2 = create_check_transcript_markup(
    record1_id, record1_channel2_bit6_id, 2, text='', ok=True, type=MarkupTranscriptType.NO_SPEECH,
)
record1_channel2_bit6_markup3 = create_check_transcript_markup(
    record1_id, record1_channel2_bit6_id, 3, text='', ok=False, type=MarkupTranscriptType.SPEECH,
)

# record 2, one channel, 1 bit
# channel 1, bit 1: transcript and type no majority vote by 0/1
record2_id = 'record2'

record2_channel1_bit1_id, record2_channel1_bit1_data = (
    'record2/1/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=3100, index=0, channel=1),
)

record2_channel1_bit1_markup1 = create_transcript_and_type_markup(
    record2_id, record2_channel1_bit1_id, 1, text='спасибо не надо',
)

# record 3, two channels, 3 bits per channel, first channel no speech all bits
# channel 1, bit 1: transcript and type majority vote no speech by 2/3
# channel 1, bit 2: check transcript majority vote not ok no speech by 2/2
# channel 1, bit 3: plain transcript majority vote no speech by 3/4
# channel 2, bit 1: check transcript no majority vote by 1/2, transcript and type no majority vote by 0/1
# channel 2, bit 2: transcript and type no majority vote by 0/2
# channel 2, bit 3: check transcript majority vote ok no speech by 1/1

record3_id = 'record3'

record3_channel1_bit1_id, record3_channel1_bit1_data = (
    'record3/1/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=1),
)
record3_channel1_bit2_id, record3_channel1_bit2_data = (
    'record3/1/bit2',
    BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=1),
)
record3_channel1_bit3_id, record3_channel1_bit3_data = (
    'record3/1/bit3',
    BitDataTimeInterval(start_ms=6000, end_ms=12300, index=2, channel=1),
)

record3_channel2_bit1_id, record3_channel2_bit1_data = (
    'record3/2/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=2),
)
record3_channel2_bit2_id, record3_channel2_bit2_data = (
    'record3/2/bit2',
    BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=2),
)
record3_channel2_bit3_id, record3_channel2_bit3_data = (
    'record3/2/bit3',
    BitDataTimeInterval(start_ms=6000, end_ms=12300, index=2, channel=2),
)

record3_channel1_bit1_markup1 = create_transcript_and_type_markup(
    record3_id, record3_channel1_bit1_id, 1, text='что',
)
record3_channel1_bit1_markup2 = create_transcript_and_type_markup(
    record3_id, record3_channel1_bit1_id, 2, text='', type=MarkupTranscriptType.UNCLEAR,
)
record3_channel1_bit1_markup3 = create_transcript_and_type_markup(
    record3_id, record3_channel1_bit1_id, 3, text='', type=MarkupTranscriptType.NO_SPEECH,
)

record3_channel1_bit2_markup1 = create_check_transcript_markup(
    record3_id, record3_channel1_bit2_id, 1, text='алиса', ok=False, type=MarkupTranscriptType.NO_SPEECH,
)
record3_channel1_bit2_markup2 = create_check_transcript_markup(
    record3_id, record3_channel1_bit2_id, 2, text='алиса', ok=False, type=MarkupTranscriptType.UNCLEAR,
)

record3_channel1_bit3_markup1 = create_plain_transcript_markup(
    record3_id, record3_channel1_bit3_id, 1, text='',
)
record3_channel1_bit3_markup2 = create_plain_transcript_markup(
    record3_id, record3_channel1_bit3_id, 2, text='но',
)
record3_channel1_bit3_markup3 = create_plain_transcript_markup(
    record3_id, record3_channel1_bit3_id, 3, text='', misheard=True,
)
record3_channel1_bit3_markup4 = create_plain_transcript_markup(
    record3_id, record3_channel1_bit3_id, 4, text='-',
)

record3_channel2_bit1_markup1 = create_check_transcript_markup(
    record3_id, record3_channel2_bit1_id, 1, text='потолок', ok=True, type=MarkupTranscriptType.SPEECH,
)
record3_channel2_bit1_markup2 = create_check_transcript_markup(
    record3_id, record3_channel2_bit1_id, 2, text='потолок', ok=False, type=MarkupTranscriptType.UNCLEAR,
)
record3_channel2_bit1_markup3 = create_transcript_and_type_markup(
    record3_id, record3_channel2_bit1_id, 3, text='алло',
)

record3_channel2_bit2_markup1 = create_transcript_and_type_markup(
    record3_id, record3_channel2_bit2_id, 1, text='скажите пожалуйста',
)
record3_channel2_bit2_markup2 = create_transcript_and_type_markup(
    record3_id, record3_channel2_bit2_id, 2, text='покажите ?',
)

record3_channel2_bit3_markup1 = create_check_transcript_markup(
    record3_id, record3_channel2_bit3_id, 1, text='', ok=True, type=MarkupTranscriptType.NO_SPEECH,
)

# record 4, two channels, 1 bit per channel, both channels no speech all bits
# channel 1, bit 1: check transcript majority vote ok no speech by 2/3
# channel 2, bit 1: check transcript no by 1/2, transcript and type majority vote on no speech by 2/3

record4_id = 'record4'

record4_channel1_bit1_id, record4_channel1_bit1_data = (
    'record4/1/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=4400, index=0, channel=1),
)
record4_channel2_bit1_id, record4_channel2_bit1_data = (
    'record4/2/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=4400, index=0, channel=2),
)

record4_channel1_bit1_markup1 = create_check_transcript_markup(
    record4_id, record4_channel1_bit1_id, 1, text='', ok=True, type=MarkupTranscriptType.NO_SPEECH,
)
record4_channel1_bit1_markup2 = create_check_transcript_markup(
    record4_id, record4_channel1_bit1_id, 2, text='', ok=False, type=MarkupTranscriptType.SPEECH,
)
record4_channel1_bit1_markup3 = create_check_transcript_markup(
    record4_id, record4_channel1_bit1_id, 3, text='', ok=True, type=MarkupTranscriptType.NO_SPEECH,
)

record4_channel2_bit1_markup1 = create_check_transcript_markup(
    record4_id, record4_channel2_bit1_id, 1, text='про', ok=False, type=MarkupTranscriptType.SPEECH,
)
record4_channel2_bit1_markup2 = create_check_transcript_markup(
    record4_id, record4_channel2_bit1_id, 2, text='про', ok=True, type=MarkupTranscriptType.SPEECH,
)
record4_channel2_bit1_markup3 = create_transcript_and_type_markup(
    record4_id, record4_channel2_bit1_id, 3, text='', type=MarkupTranscriptType.UNCLEAR,
)
record4_channel2_bit1_markup4 = create_transcript_and_type_markup(
    record4_id, record4_channel2_bit1_id, 4, text='', type=MarkupTranscriptType.UNCLEAR,
)
record4_channel2_bit1_markup5 = create_transcript_and_type_markup(
    record4_id, record4_channel2_bit1_id, 5, text='пфф', type=MarkupTranscriptType.SPEECH,
)

# record 5, one channel, 1 bit per channel, no speech all bits
# channel 1, bit 1: plain transcript majority vote no speech by 2/2

record5_id = 'record5'

record5_channel1_bit1_id, record5_channel1_bit1_data = (
    'record5/1/bit1',
    BitDataTimeInterval(start_ms=0, end_ms=1200, index=0, channel=1),
)

record5_channel1_bit1_markup1 = create_plain_transcript_markup(
    record5_id, record5_channel1_bit1_id, 1, text='', misheard=True,
)
record5_channel1_bit1_markup2 = create_plain_transcript_markup(
    record5_id, record5_channel1_bit1_id, 1, text='-',
)

markups_with_bits_data = [
    (record1_channel1_bit1_markup1, record1_channel1_bit1_data),
    (record1_channel1_bit1_markup2, record1_channel1_bit1_data),
    (record1_channel1_bit1_markup3, record1_channel1_bit1_data),
    (record1_channel1_bit1_markup4, record1_channel1_bit1_data),
    (record1_channel1_bit2_markup1, record1_channel1_bit2_data),
    (record1_channel1_bit2_markup2, record1_channel1_bit2_data),
    (record1_channel1_bit3_markup1, record1_channel1_bit3_data),
    (record1_channel1_bit3_markup2, record1_channel1_bit3_data),
    (record1_channel1_bit3_markup3, record1_channel1_bit3_data),
    (record1_channel1_bit3_markup4, record1_channel1_bit3_data),
    (record1_channel1_bit3_markup5, record1_channel1_bit3_data),
    (record1_channel1_bit3_markup6, record1_channel1_bit3_data),
    (record1_channel1_bit4_markup1, record1_channel1_bit4_data),
    (record1_channel1_bit4_markup2, record1_channel1_bit4_data),
    (record1_channel1_bit4_markup3, record1_channel1_bit4_data),
    (record1_channel1_bit5_markup1, record1_channel1_bit5_data),
    (record1_channel1_bit5_markup2, record1_channel1_bit5_data),
    (record1_channel1_bit5_markup3, record1_channel1_bit5_data),
    (record1_channel1_bit5_markup4, record1_channel1_bit5_data),
    (record1_channel1_bit5_markup5, record1_channel1_bit5_data),
    (record1_channel1_bit6_markup1, record1_channel1_bit6_data),
    (record1_channel1_bit6_markup2, record1_channel1_bit6_data),
    (record1_channel1_bit6_markup3, record1_channel1_bit6_data),
    (record1_channel1_bit6_markup4, record1_channel1_bit6_data),
    (record1_channel1_bit6_markup5, record1_channel1_bit6_data),
    (record1_channel1_bit6_markup6, record1_channel1_bit6_data),
    (record1_channel1_bit6_markup7, record1_channel1_bit6_data),
    (record1_channel2_bit1_markup1, record1_channel2_bit1_data),
    (record1_channel2_bit2_markup1, record1_channel2_bit2_data),
    (record1_channel2_bit2_markup2, record1_channel2_bit2_data),
    (record1_channel2_bit2_markup3, record1_channel2_bit2_data),
    (record1_channel2_bit2_markup4, record1_channel2_bit2_data),
    (record1_channel2_bit2_markup5, record1_channel2_bit2_data),
    (record1_channel2_bit3_markup1, record1_channel2_bit3_data),
    (record1_channel2_bit4_markup1, record1_channel2_bit4_data),
    (record1_channel2_bit4_markup2, record1_channel2_bit4_data),
    (record1_channel2_bit4_markup3, record1_channel2_bit4_data),
    (record1_channel2_bit5_markup1, record1_channel2_bit5_data),
    (record1_channel2_bit5_markup2, record1_channel2_bit5_data),
    (record1_channel2_bit6_markup1, record1_channel2_bit6_data),
    (record1_channel2_bit6_markup2, record1_channel2_bit6_data),
    (record1_channel2_bit6_markup3, record1_channel2_bit6_data),
    (record2_channel1_bit1_markup1, record2_channel1_bit1_data),
    (record3_channel1_bit1_markup1, record3_channel1_bit1_data),
    (record3_channel1_bit1_markup2, record3_channel1_bit1_data),
    (record3_channel1_bit1_markup3, record3_channel1_bit1_data),
    (record3_channel1_bit2_markup1, record3_channel1_bit2_data),
    (record3_channel1_bit2_markup2, record3_channel1_bit2_data),
    (record3_channel1_bit3_markup1, record3_channel1_bit3_data),
    (record3_channel1_bit3_markup2, record3_channel1_bit3_data),
    (record3_channel1_bit3_markup3, record3_channel1_bit3_data),
    (record3_channel1_bit3_markup4, record3_channel1_bit3_data),
    (record3_channel2_bit1_markup1, record3_channel2_bit1_data),
    (record3_channel2_bit1_markup2, record3_channel2_bit1_data),
    (record3_channel2_bit1_markup3, record3_channel2_bit1_data),
    (record3_channel2_bit2_markup1, record3_channel2_bit2_data),
    (record3_channel2_bit2_markup2, record3_channel2_bit2_data),
    (record3_channel2_bit3_markup1, record3_channel2_bit3_data),
    (record4_channel1_bit1_markup1, record4_channel1_bit1_data),
    (record4_channel1_bit1_markup2, record4_channel1_bit1_data),
    (record4_channel1_bit1_markup3, record4_channel1_bit1_data),
    (record4_channel2_bit1_markup1, record4_channel2_bit1_data),
    (record4_channel2_bit1_markup2, record4_channel2_bit1_data),
    (record4_channel2_bit1_markup3, record4_channel2_bit1_data),
    (record4_channel2_bit1_markup4, record4_channel2_bit1_data),
    (record4_channel2_bit1_markup5, record4_channel2_bit1_data),
    (record5_channel1_bit1_markup1, record5_channel1_bit1_data),
    (record5_channel1_bit1_markup2, record5_channel1_bit1_data),
]
shuffle(markups_with_bits_data)

check_transcript_markups_with_bits_data = [
    x for x in markups_with_bits_data if x[0].markup_data.version == MarkupDataVersions.CHECK_TRANSCRIPT
]
