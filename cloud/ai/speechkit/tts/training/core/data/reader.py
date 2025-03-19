import sys
from abc import ABC, abstractmethod
import io
import logging
import os
import pickle
from typing import (
    BinaryIO,
    Dict,
    List,
    Sequence,
    TextIO
)

import numpy as np
import yt.wrapper as yt

from core.data.sample import RawTtsSample

logging.getLogger("yt.packages.requests").setLevel(logging.WARNING)
logging.getLogger("yt.packages.urllib3").setLevel(logging.WARNING)


class Reader(ABC):
    @abstractmethod
    def __next__(self) -> RawTtsSample:
        pass


class WeightedMultiReader(Reader):
    def __init__(self, readers: List[Reader], weights: Sequence[float]):
        assert len(readers) == len(weights)
        self._readers = readers
        self._weights = weights
        self._exhausted = {i: False for i in range(len(readers))}

    def __iter__(self):
        return self

    def __next__(self):
        while True:
            reader_ids = [i for i in self._exhausted if not self._exhausted[i]]
            if not reader_ids:
                raise StopIteration
            weights = np.array([self._weights[i] for i in reader_ids], dtype=np.float)
            weights /= weights.sum()
            reader_idx = np.random.choice(reader_ids, p=weights)
            sample = next(self._readers[reader_idx], None)
            if sample is None:
                self._exhausted[reader_idx] = True
            else:
                return sample


class FsReader(Reader):
    def __init__(self,
                 table_file: BinaryIO,
                 meta_file: TextIO,
                 offset: int,
                 infinite: bool):
        self._table_file = table_file
        self._meta_file = meta_file
        self.infinite = infinite

        table_offset = None
        self._start_offset = meta_offset = 0
        for i, line in enumerate(self._meta_file):
            # first line contains total number of samples
            if i == offset + 1:
                _, table_offset = map(int, line.split())
                break
            meta_offset += len(line.encode())
            if i == 0:
                # save first line offset with the number of samples
                self._start_offset = meta_offset
        assert table_offset is not None and self._start_offset

        self._meta_file.seek(meta_offset)
        self._table_file.seek(table_offset)

    def __next__(self) -> RawTtsSample:
        while True:
            line = self._meta_file.readline()
            if not line:
                if self.infinite:
                    self._meta_file.seek(self._start_offset)
                    self._table_file.seek(0)
                    line = self._meta_file.readline()
                else:
                    self._table_file.close()
                    self._meta_file.close()
                    raise StopIteration
            sample_size, offset = map(int, line.split())
            sample_bytes = self._table_file.read(sample_size)
            if not sample_bytes:
                sys.stderr.write("skipping empty sample\n")
                continue
            sample = pickle.load(io.BytesIO(sample_bytes))
            return sample


class YtReader(Reader):
    def __init__(self,
                 cluster: str,
                 table: str,
                 offset: int,
                 infinite: bool):
        self._cluster = cluster
        self._table = table
        self._client = yt.YtClient(proxy=cluster, token=os.environ["YT_TOKEN"])
        self._infinite = infinite
        self._reset_reader(offset)

    def _reset_reader(self, offset: int = 0):
        table = yt.TablePath(name=self._table, start_index=offset, client=self._client)
        self._reader = yt.read_table(table=table, format=yt.YsonFormat(encoding=None), client=self._client)

    def __next__(self) -> RawTtsSample:
        while True:
            row = next(self._reader, None)
            if row is None:
                if self._infinite:
                    self._reset_reader()
                    row = next(self._reader)
                    assert row is not None
                else:
                    raise StopIteration
            sample = self._parse_yt_row(row)
            return sample

    def _parse_yt_row(self, row: Dict) -> RawTtsSample:
        sample_id = row[b"ID"].decode()
        # self._decode_bytes handle None
        utterance = self._decode_bytes(row.get(b"utterance"))
        speaker = row[b"speaker"].decode()
        text = row[b"text"].decode()

        accented_text = row.get(b"accented_text")
        accented_text = accented_text.decode() if accented_text is not None else None

        template_text = row.get(b"template_text")
        template_text = template_text.decode() if template_text is not None else None

        text_variables = row.get(b"text_variables")
        text_variables = self._decode_bytes(text_variables) if text_variables is not None else None

        audio_variables = row.get(b"audio_variables")
        audio_variables = self._decode_bytes(audio_variables) if audio_variables is not None else None

        wav = row[b"pcm__wav"]
        mel = np.load(io.BytesIO(row[b"mel__npy"]))
        pitch = np.load(io.BytesIO(row[b"pitch__npy"]))

        sample = RawTtsSample(
            id=sample_id,
            lang="ru",
            utterance=utterance,
            speaker=speaker,
            text=text,
            accented_text=accented_text,
            template_text=template_text,
            text_variables=text_variables,
            audio_variables=audio_variables,
            wav=wav,
            mel=mel,
            pitch=pitch
        )

        return sample

    def _decode_bytes(self, data):
        if isinstance(data, bytes):
            return data.decode()
        if isinstance(data, dict):
            return dict(map(self._decode_bytes, data.items()))
        if isinstance(data, tuple):
            return tuple(map(self._decode_bytes, data))
        if isinstance(data, list):
            return list(map(self._decode_bytes, data))
        return data
