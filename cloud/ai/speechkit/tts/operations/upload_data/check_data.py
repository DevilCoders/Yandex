from functools import partial
import io
import logging
from multiprocessing import Pool
import os
import re
import sys
import typing as tp
import zipfile

from Levenshtein import distance as levenshtein_distance
import librosa
import numpy as np

from stt import Streamer
import wavfile

logging.basicConfig(level=logging.DEBUG,
                    stream=sys.stdout,
                    format="%(levelname)s: %(asctime)s %(filename)s:%(lineno)d     %(message)s")
logger = logging.getLogger()


class Sample:
    def __init__(self,
                 audio_name: str,
                 raw_text: str = None,
                 text: tp.Optional[str] = None,
                 audio: tp.Optional[np.ndarray] = None,
                 audio_sample_rate: tp.Optional[int] = None,
                 audio_duration: tp.Optional[float] = None,
                 recognition: tp.Optional[str] = None,
                 levenshtein: tp.Optional[float] = None,
                 error_type: tp.Optional[str] = None,
                 error_description: tp.Optional[str] = None):
        self.audio_name = audio_name
        self.raw_text = raw_text
        self.text = text
        self.audio = audio
        self.audio_sample_rate = audio_sample_rate
        self.audio_duration = audio_duration
        self.recognition = recognition
        self.levenshtein = levenshtein
        self.error_type = error_type
        self.error_description = error_description


def check_text(text: str, language: str) -> tp.Set[str]:
    regexps = {
        'ru': re.compile(r'[a-zа-яё,.:\-;!? ()+{}_\*"]*'),
        'kk': re.compile(r'[a-zа-яёіғқңүұһәө,.:\-;!? ()+{}_\*"]*'),
        'de': re.compile(r'[a-zäöüß,.:\-;!? ()+{}_\*"\']*'),
        'fr': re.compile(r'[a-zàâäèéêëîïôœùûüÿç,.:\-;!?‘’ ()+{}_\*"\']*'),
        'he': re.compile(r'[\u0590-\u05fe,.:\-;!? ()+{}_\*"\']*')
    }
    bad_symbols = []
    if regexps[language].fullmatch(text.lower()) is None:
        for symbol in text.lower():
            if regexps[language].fullmatch(symbol) is None:
                bad_symbols.append(symbol)
    return set(bad_symbols)


def expand_template_text(text: str) -> str:
    return re.sub(r'{[a-zA-Zа-яёА-ЯЁ0-9_\-]+\s*=\s*(.+?)}', r'\1', text)


def check_tsv_zip(texts_tsv_path: str,
                  audio_archive_path: str,
                  language: str,
                  min_sample_rate: tp.Optional[int]) -> tp.Tuple[tp.List[Sample], tp.List[Sample]]:
    logger.info('reading data')
    data_dict = {}
    with open(texts_tsv_path) as f:
        for i, line in enumerate(f):
            line = line.strip()
            if not line:
                continue
            row = line.split('\t')
            audio_name, raw_text = row[0], row[1]
            if i == 0 and audio_name == 'recording' and raw_text == 'text':
                continue
            data_dict[audio_name] = Sample(audio_name=audio_name, raw_text=raw_text)

    with zipfile.ZipFile(audio_archive_path) as zip:
        for audio_path in zip.namelist():
            audio_name = os.path.basename(audio_path)
            if not audio_path.endswith('.wav') or audio_path.startswith('__MACOSX'):
                logger.debug(f'skipping {audio_path}')
                continue

            wav_exception = None
            try:
                sample_rate, audio = wavfile.read(io.BytesIO(zip.read(audio_path)))
            except Exception as e:
                wav_exception = e

            if audio_name not in data_dict or not data_dict[audio_name].raw_text.strip():
                data_dict[audio_name] = Sample(audio_name=audio_name,
                                               audio=audio if wav_exception is None else None,
                                               audio_sample_rate=sample_rate if wav_exception is None else None,
                                               audio_duration=len(
                                                   audio) / sample_rate if wav_exception is None else None,
                                               error_type='NO_TEXT',
                                               error_description='no text for audio')
                continue

            if wav_exception is not None:
                logger.debug(f'bad audio {audio_name}\n{repr(wav_exception)}')
                data_dict[audio_name].error_type = 'BAD_AUDIO'
                data_dict[audio_name].error_description = repr(wav_exception)
                continue

            data_dict[audio_name].audio = audio
            data_dict[audio_name].audio_sample_rate = sample_rate
            data_dict[audio_name].audio_duration = len(audio) / sample_rate

            if min_sample_rate and sample_rate < min_sample_rate:
                logger.debug(f'audio "{audio_name}" sampling rate is {sample_rate}, minimum {min_sample_rate} is required')
                data_dict[audio_name].error_type = 'BAD_AUDIO'
                data_dict[audio_name].error_description = f'audio sampling rate is {sample_rate}, minimum {min_sample_rate} is required'
                continue

            num_channels = len(np.array(audio).shape)
            if num_channels > 1 and np.array(audio).shape[1] > 1:
                logger.debug(f'audio "{audio_name}" contains {num_channels} channels')
                data_dict[audio_name].error_type = 'BAD_AUDIO'
                data_dict[audio_name].error_description = f'audio contains {num_channels} channels'
                continue

            if np.max(np.abs(audio)) < 1e-4:
                logger.debug(f'empty audio "{audio_name}"')
                data_dict[audio_name].error_type = 'BAD_AUDIO'
                data_dict[audio_name].error_description = 'empty audio'
                continue

    for audio_name, sample in data_dict.items():
        if sample.error_type is not None:
            continue

        sample.text = expand_template_text(sample.raw_text)

        if sample.audio is None:
            logger.debug(f'no audio "{audio_name}"')
            data_dict[audio_name].error_type = 'NO_AUDIO'
            data_dict[audio_name].error_description = 'no audio for text'
            continue

        bad_symbols = check_text(sample.text, language)
        if bad_symbols:
            logger.debug(f'bad symbols in text for "{audio_name}: {bad_symbols}')
            data_dict[audio_name].error_type = 'BAD_TEXT'
            data_dict[audio_name].error_description = f'bad symbols: {bad_symbols}'
            continue

    good_samples = [sample for sample in data_dict.values() if sample.error_type is None]
    bad_samples = [sample for sample in data_dict.values() if sample.error_type is not None]

    logger.info(f'got {len(good_samples)} good samples after data check')
    logger.info(f'got {len(bad_samples)} bad samples after data check')

    return good_samples, bad_samples


def preprocess_audio(audio: np.ndarray, sample_rate: int) -> np.ndarray:
    audio = np.array(audio.copy(), dtype=np.float32)
    audio = librosa.resample(audio, orig_sr=sample_rate, target_sr=16000)
    audio = audio / np.max(np.abs(audio)) * 0.73
    audio = (audio * 32768).astype(np.int16)
    return audio


def normalize_text(text: str, language: str):
    text = text.lower().replace('ё', 'е')
    if language == 'ru':
        text = re.sub(r'[^а-я ]', ' ', text)  # leave only letters and spaces
    elif language == 'kk':
        text = re.sub(r'[^а-яіғқңүұһәө ]', ' ', text)  # leave only letters and spaces
    elif language == 'de':
        text = re.sub(r'[^a-zäöüß ]', ' ', text)  # leave only letters and spaces
    elif language == 'fr':
        text = re.sub(r'[^a-zàâäèéêëîïôœùûüÿç ]', ' ', text)  # leave only letters and spaces
    else:
        assert False, f'unknown language: {language}'
    text = re.sub(r' +', ' ', text)  # merge repeated spaces
    return text.strip()


def recognize(samples: tp.List[Sample], language: str, stt_api_key: str) -> tp.List[Sample]:
    streamer = Streamer(language, stt_api_key)
    result = []
    for sample in samples:
        try:
            audio = preprocess_audio(sample.audio, sample.audio_sample_rate)
            recognition = streamer.recognize_streaming(audio, 16000)
        except Exception as e:
            logger.debug(f'got an exception when recognizing "{sample.audio_name}"\n{e}')
            sample.error_type = 'ASR_ERROR'
            sample.error_description = repr(e)
            result.append(sample)
            continue

        recognition_norm = normalize_text(recognition, language)
        text_norm = normalize_text(sample.text, language)
        levenshtein = levenshtein_distance(recognition_norm, text_norm) / max(len(recognition_norm), len(text_norm))

        sample.recognition = recognition
        sample.levenshtein = levenshtein
        result.append(sample)

    return result


def check_text_audio_match(samples: tp.List[Sample],
                           language: str,
                           levenshtein_threshold: float,
                           stt_api_key: str) -> tp.Tuple[tp.List[Sample], tp.List[Sample]]:
    logger.info('starting recognition')

    recognize_partial = partial(recognize, language=language, stt_api_key=stt_api_key)

    num_processes = os.cpu_count()
    chunk_size = max(int(len(samples) / float(num_processes)), 1)
    chunks = [samples[i: i + chunk_size] for i in range(0, len(samples), chunk_size)]
    with Pool(num_processes) as p:
        recognized_chunks = p.map(recognize_partial, chunks)

    good_samples, bad_samples = [], []
    for chunk in recognized_chunks:
        for sample in chunk:
            if sample.levenshtein > levenshtein_threshold:
                logger.debug(f'text audio mismatch for {sample.audio_name}')
                sample.error_type = 'TEXT_AUDIO_MISMATCH'
                sample.error_description = f'levenshtein distance is greater than {levenshtein_threshold}'
            if sample.error_type is None:
                good_samples.append(sample)
            else:
                bad_samples.append(sample)

    logger.info(f'got {len(good_samples)} good samples after recognition')
    logger.info(f'got {len(bad_samples)} bad samples after recognition')

    return good_samples, bad_samples
