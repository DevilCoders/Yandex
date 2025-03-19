import os
from dataclasses import dataclass
from io import BytesIO
from typing import Optional, Any

from pydub import AudioSegment
from scipy.io import wavfile

from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id


@dataclass
class RecordFileData:
    record_id: str
    raw_data: bytes
    duration_seconds: float
    sample_rate_hertz: int
    channel_count: int


def process_file(record_file_path: str, record_file_io: Optional[BytesIO] = None) -> RecordFileData:
    _, ext = os.path.splitext(record_file_path)
    if record_file_io is not None:
        f = record_file_io
    else:
        f = record_file_path
    if ext == '.wav':
        return process_file_wav(f)
    else:
        return process_file_not_wav(f, ext)


def validate_audio_data(sample_width: int, frame_rate: int):
    valid_sample_rates = [8000, 16000, 48000]
    assert sample_width == 2, 'sample width must be 2'
    assert frame_rate in valid_sample_rates, (
        f'sample rate must be equal to one of the values: ' f'{", ".join(str(r) for r in valid_sample_rates)}'
    )


def create_audio_data(raw_data: bytes, duration: float, frame_rate: int, channels: int) -> RecordFileData:
    return RecordFileData(
        record_id=generate_id(),
        raw_data=raw_data,
        duration_seconds=duration,
        sample_rate_hertz=frame_rate,
        channel_count=channels,
    )


def process_file_wav(file: Any) -> RecordFileData:
    frame_rate, data = wavfile.read(file)  # faster than pydub, because it does not call ffmpeg
    channels = 1 if len(data.shape) == 1 else data.shape[-1]

    validate_audio_data(data.dtype.itemsize, frame_rate)
    return create_audio_data(
        raw_data=data.tobytes(),
        duration=data.shape[0] / frame_rate,
        frame_rate=frame_rate,
        channels=channels
    )


def process_file_not_wav(file: Any, ext: str) -> RecordFileData:
    if ext == '.mp3':
        audio = AudioSegment.from_mp3(file)
    else:
        audio = AudioSegment.from_file(file)

    validate_audio_data(audio.sample_width, audio.frame_rate)
    return create_audio_data(
        raw_data=audio.raw_data,
        duration=audio.duration_seconds,
        frame_rate=audio.frame_rate,
        channels=audio.channels
    )
