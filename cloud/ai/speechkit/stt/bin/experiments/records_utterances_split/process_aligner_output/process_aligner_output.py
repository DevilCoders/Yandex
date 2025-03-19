import json
import math
import os
from random import shuffle, uniform

import yt.wrapper as yt
from pydub import AudioSegment

from cloud.ai.speechkit.stt.lib.experiments.records_utterances_split import (
    split_record_by_utterances,
    unmarshal_aligner_record_output,
    apply_padding_to_utterances,
    fetch_bits_no_speech,
    Fragment,
)

aligner_output_path = '/Users/o-gulyaev/tmp/utt_split/aligner_output.json'
bits_output_path = '/Users/o-gulyaev/tmp/utt_split/bits.json'
records_wav_path = '/Users/o-gulyaev/tmp/utt_split/records_wav'
bits_wav_path = '/Users/o-gulyaev/tmp/utt_split/bits'
aligner_output_yt_table_path = '//home/mlcloud/o-gulyaev/ASREXP-881/aligner-output-2020-01-28'


def main():
    # download_aligner_output()

    with open(aligner_output_path) as f:
        records_aligner_data = json.load(f)

    min_ms_between_utterances = 3000
    utterance_audio_padding_ms = 500
    min_utterance_duration_ms = 1000
    max_utterance_duration_ms = 15000
    min_bit_no_speech_duration_ms = 3000
    max_bit_no_speech_duration_ms = 12000
    record_bits_no_speech_fetch_ratio = 0.3

    assert min_ms_between_utterances > utterance_audio_padding_ms

    records_data = []
    export_bits_files = True
    for record_data in records_aligner_data:
        record_id = record_data['record_id']
        duration_seconds = record_data['duration_seconds']
        channel = record_data['channel']
        text = record_data['text']

        duration_ms = int(duration_seconds * 1000)

        utterances = apply_padding_to_utterances(
            utterances=split_record_by_utterances(
                words=unmarshal_aligner_record_output(record_data),
                min_ms_between_utterances=min_ms_between_utterances,
            ),
            padding_ms=utterance_audio_padding_ms,
            record_duration_ms=duration_ms
        )

        if len(utterances) == 0 and min_bit_no_speech_duration_ms <= duration_ms <= max_bit_no_speech_duration_ms:
            if uniform(0, 1) < record_bits_no_speech_fetch_ratio:
                bits_no_speech = [
                    Fragment(
                        text='',
                        start_time_ms=0,
                        end_time_ms=duration_ms,
                    ),
                ]
            else:
                bits_no_speech = []
        else:
            bits_no_speech = fetch_bits_no_speech(
                utterances_with_padding=utterances,
                min_bit_duration_ms=min_bit_no_speech_duration_ms,
                max_bit_duration_ms=max_bit_no_speech_duration_ms,
            )

        # We don't need too much no speech bits, let's get some random sample
        shuffle(bits_no_speech)
        bits_no_speech = bits_no_speech[:math.ceil(len(bits_no_speech) * record_bits_no_speech_fetch_ratio)]

        utterances = [
            u for u in utterances if
            min_utterance_duration_ms <= u.end_time_ms - u.start_time_ms <= max_utterance_duration_ms
        ]

        all_bits = utterances + bits_no_speech

        records_data.append({
            'record_id': record_id,
            'duration_seconds': duration_seconds,
            'channel': channel,
            'text': text,
            'bits': [u.to_yson() for u in all_bits],
        })

        if export_bits_files:
            record_file_path = os.path.join(records_wav_path, f'{record_id}_{channel}.wav')
            record_audio = AudioSegment.from_wav(record_file_path)
            for bit in all_bits:
                bit_audio = record_audio[bit.start_time_ms: bit.end_time_ms]
                bit_path = os.path.join(bits_wav_path,
                                        f'{record_id}_{channel}_{bit.start_time_ms}-{bit.end_time_ms}.wav')
                bit_audio.export(bit_path, format='wav')

    with open(bits_output_path, 'w') as f:
        json.dump(records_data, f, indent=4, ensure_ascii=False)


def download_aligner_output():
    yt.config['proxy']['url'] = 'hahn'

    rows = []
    for row in yt.read_table(aligner_output_yt_table_path, format=yt.YsonFormat(encoding=None)):
        rows.append(row)

    data = []
    for row in rows:
        record_id = row[b'record_id'].decode('utf-8')
        channel = row[b'channel']
        data.append({
            'record_id': record_id,
            'aligned_text': json.loads(row[b'aligned_text'].decode('utf-8')),
            'text': row[b'text'].decode('utf-8'),
            'channel': channel,
            'duration_seconds': row[b'duration_seconds'],
        })
        with open(os.path.join(records_wav_path, f'{record_id}_{channel}.wav'), 'wb') as f:
            f.write(row[b'wav'])

    with open(aligner_output_path, 'w') as f:
        json.dump(data, f, indent=4, ensure_ascii=False)
