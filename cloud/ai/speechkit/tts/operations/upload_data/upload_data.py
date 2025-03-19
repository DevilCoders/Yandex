import hashlib
import io
import json
import logging
import math
import os
import re
import sys
import typing as tp
from uuid import uuid4

import numpy as np
import yt.wrapper as yt

from check_data import Sample
import wavfile

logging.basicConfig(level=logging.DEBUG,
                    stream=sys.stdout,
                    format="%(levelname)s: %(asctime)s %(filename)s:%(lineno)d     %(message)s")
logger = logging.getLogger()


def numpy_to_wav(audio: np.ndarray, sample_rate: int) -> bytes:
    wav_bytes = io.BytesIO()
    wavfile.write(wav_bytes, sample_rate, audio)
    return wav_bytes.read()


def hash_from_wav(data: bytes) -> str:
    hash_id = hashlib.sha256(data).hexdigest()[:32]
    id_regex = re.compile(r'[0-9a-f]{32}')
    assert id_regex.fullmatch(hash_id)
    return hash_id


def write_batch(table_path: str, batch: tp.List[dict]):
    completed = False
    while not completed:
        try:
            yt.write_table(yt.TablePath(table_path, append=True), batch, raw=False)
            completed = True
        except Exception as e:
            logger.info(f'Upload failed\n{e}\nRetry...')
            continue


def upload_data(samples: tp.List[Sample], table_path: str):
    logger.info(f'writing to YT')

    yt.config['proxy']['url'] = 'hahn'
    yt.config['write_parallel']['enable'] = True
    yt.config['write_parallel']['max_thread_count'] = os.cpu_count()

    with open('schema.json', 'r') as f:
        schema = json.load(f)
    yt.create('table', path=table_path, recursive=True, ignore_existing=False, attributes={'schema': schema})

    batch_size = 1024
    for i in range(int(math.ceil(len(samples) / batch_size))):
        batch = []
        for sample in samples[i * batch_size:(i + 1) * batch_size]:
            wav = hash_id = None
            if sample.audio is not None:
                wav = numpy_to_wav(sample.audio, sample.audio_sample_rate)
                hash_id = hash_from_wav(wav)
            batch.append({
                'uuid': str(uuid4()),
                'audio_name': sample.audio_name,
                'text': sample.text,
                'raw_text': sample.raw_text,
                'wav': wav,
                'duration': sample.audio_duration,
                'hash': hash_id,
                'error_type': 'OK' if sample.error_type is None else sample.error_type,
                'error_description': sample.error_description,
                'recognition': sample.recognition,
                'levenshtein': sample.levenshtein
            })
        write_batch(table_path, batch)
