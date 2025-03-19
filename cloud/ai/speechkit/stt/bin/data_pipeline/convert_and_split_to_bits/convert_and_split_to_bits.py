#!/usr/bin/python3

import os
from functools import partial
from multiprocessing.pool import ThreadPool
import uuid

import nirvana.job_context as nv
import ujson as json
from pydub import AudioSegment

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    AudioEncoding,
    SplitDataFixedLengthOffset,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.files import (
    records_dir,
    bits_dir,
    get_name_for_record_audio_file,
    get_name_for_record_bit_audio_file,
    unpack_and_list_files,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.records_splitting import get_bits_indices

model_native_sample_rate_hertz = 16000
model_native_sample_width = 2  # 16 bit
pydub_cmd = 'pydub-0.25.1'
sox_cmd = 'sox -r {sample_rate} -b {bits} -e signed-integer -c {channels_count} {old_filename} -t wav ' \
          '-r {output_sample_rate} {new_filename} remix {channel}'
sox_audio_container_cmd = 'sox {old_filename} -t wav -r {output_sample_rate} {new_filename} remix {channel}'


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    with open(inputs.get('records.json')) as f:
        records = [Record.from_yson(r) for r in json.load(f)]
    records_files_path = inputs.get('records_files.tar.gz')

    unpack_and_list_files(archive_path=records_files_path, directory_path=records_dir)

    bit_length_ms = params.get('bit-length')
    bit_offset_ms = params.get('bit-offset')

    pool = ThreadPool(processes=16)

    os.system(f'mkdir {bits_dir}')

    records_convert_cmd = pool.map(
        partial(process_record, bit_length_ms=bit_length_ms, bit_offset_ms=bit_offset_ms),
        records,
    )

    convert_data = {
        'sample_rate': model_native_sample_rate_hertz,
        'record_id_to_convert_cmd': {record.id: convert_cmd for record, convert_cmd in zip(records, records_convert_cmd)},
    }

    with open(outputs.get('convert_params.json'), 'w') as f:
        json.dump(convert_data, f, indent=4, ensure_ascii=False)
    with open(outputs.get('split_data.json'), 'w') as f:
        json.dump(
            SplitDataFixedLengthOffset(
                length_ms=bit_length_ms,
                offset_ms=bit_offset_ms,
                split_cmd=pydub_cmd,
            ).to_yson(),
            f, indent=4, ensure_ascii=False)

    records_bits_files_path = outputs.get('records_bits_files.tar.gz')

    os.system(f'tar czf {records_bits_files_path} {bits_dir}')


def process_record(record: Record, bit_length_ms: int, bit_offset_ms: int) -> str:
    record_file_path = os.path.join(records_dir, get_name_for_record_audio_file(record))
    audio_encoding = record.req_params.get_audio_encoding()

    assert record.audio_params.channel_count <= 16

    # try to use sox then possible, because after 48000->16000 downsampling with pydub converted audio sometimes
    # contains "crickets" glitches; by the moment OPUS sox handler is not installed in Nirvana operation (don't know
    # how to install it), so OGG OPUS will be converted by pydub

    if audio_encoding == AudioEncoding.LPCM:
        cmd = sox_cmd
        audio_duration_ms = int(record.audio_params.duration_seconds * 1000)

        conv_audio_channels = []
        for channel in range(record.audio_params.channel_count):
            new_filename = f'{str(uuid.uuid4())}.wav'
            cmd = sox_cmd.format(
                old_filename=record_file_path,
                sample_rate=record.req_params.get_sample_rate_hertz(),
                bits=model_native_sample_width * 8,
                channels_count=record.audio_params.channel_count,
                new_filename=new_filename,
                output_sample_rate=model_native_sample_rate_hertz,
                channel=channel + 1,
            )
            exit_code = os.system(cmd)
            if exit_code != 0:
                raise RuntimeError(f'Command "{cmd}" finished with non-zero code {exit_code}')
            conv_audio_channels.append(AudioSegment.from_wav(new_filename))
    elif audio_encoding == AudioEncoding.OGG_OPUS:
        # source_audio = AudioSegment.from_raw(record_file_path,
        #                                      channels=record.audio_params.channel_count,
        #                                      sample_width=2,
        #                                      frame_rate=record.req_params.get_sample_rate_hertz())
        cmd = pydub_cmd
        source_audio = AudioSegment.from_ogg(record_file_path)

        assert record.audio_params.channel_count == source_audio.channels

        # we could get it from record, but let's do all durations and split stuff by pydub
        audio_duration_ms = source_audio.__len__()

        # Recognition models work with 16KHz sample rate and 16 bit sample width.
        # Records from the cloud must have 16 bit sample width, but for some reason
        # sometimes it is interpreted as 32 bit by ffmpeg/pydub. Records from other
        # sources are not forced to have 16 bit sample width.
        conv_audio = source_audio.set_sample_width(model_native_sample_width).set_frame_rate(
            model_native_sample_rate_hertz)

        conv_audio_channels = conv_audio.split_to_mono()
    else:
        raise ValueError(f'Unexpected record encoding: {audio_encoding}')

    for i, conv_mono_audio in enumerate(conv_audio_channels):
        channel = i + 1
        bits_indices = get_bits_indices(audio_duration_ms, bit_length_ms, bit_offset_ms)
        for left, right in bits_indices:
            bit_audio = conv_mono_audio[left:right]
            bit_filename = get_name_for_record_bit_audio_file(record.id, channel, left, right)
            bit_filepath = os.path.join(bits_dir, bit_filename)
            bit_audio.export(bit_filepath, format='wav')

    return cmd
