from pydub import AudioSegment


def make_audio_segment(record, s3, output_sampling_rate):
    assert record['spec']['audio_encoding'] == 1 or record['spec']['audio_encoding'] == 2, 'expected PCM/OGG format'

    n_channels = record['spec']['audio_channel_count']
    sampling_rate = record['spec']['sample_rate_hertz']

    s3_object = s3.get_object(Bucket=record['s3_obj']['bucket'], Key=record['s3_obj']['key'])
    if record['spec']['audio_encoding'] == 1:
        data = AudioSegment.from_raw(s3_object['Body'], channels=n_channels, sample_width=2, frame_rate=sampling_rate)
    else:
        data = AudioSegment.from_ogg(s3_object['Body'])

    if sampling_rate != output_sampling_rate:
        data = data.set_frame_rate(output_sampling_rate)

    return data
