from abc import ABC
import sys
from typing import (
    Callable,
    Dict,
    List,
    Optional,
    Sequence,
    Union
)

from torch.utils.data import Dataset, DataLoader, IterableDataset

from core.data.datasource import create_data_source, DataSource
from core.data.reader import WeightedMultiReader
from core.data.sample import TrainSample, TrainSampleBuilder
from core.data.utils import workers_info
from core.utils import get_rank


class TtsDatasetBase(ABC):
    def __init__(
        self,
        data_sources: Sequence[DataSource],
        sample_builder: TrainSampleBuilder,
        min_mel_length: Optional[int],
        max_mel_length: Optional[int]
    ):
        self.data_sources = data_sources
        self.sample_builder = sample_builder
        self.min_mel_length = float("-inf") if min_mel_length is None else min_mel_length
        self.max_mel_length = float("inf") if max_mel_length is None else max_mel_length

        self._weights = [data_source.weight for data_source in data_sources]
        self._reader = None

    def _init_reader(self):
        readers = [iter(data_source) for data_source in self.data_sources]
        self._reader = WeightedMultiReader(readers, self._weights)

    def _next_sample(self) -> Optional[TrainSample]:
        if self._reader is None:
            self._init_reader()
        while True:
            raw_sample = next(self._reader, None)
            if raw_sample is None:
                return
            mel_length = raw_sample.mel.shape[1]
            while mel_length < self.min_mel_length or mel_length > self.max_mel_length:
                sys.stderr.write(f"skipping sample with mel length {mel_length}\n")
                raw_sample = next(self._reader)
                mel_length = raw_sample.mel.shape[1]
            try:
                train_sample = self.sample_builder(raw_sample)
                return train_sample
            except Exception as e:
                # raise e
                sys.stderr.write(f"building sample error: {e}\n")


class TtsInMemoryDataset(TtsDatasetBase, Dataset):
    def __init__(
        self,
        tables: Sequence[Dict],
        sample_builder: TrainSampleBuilder,
        max_num_samples: int,
        min_mel_length: Optional[int],
        max_mel_length: Optional[int]
    ):
        data_sources = [create_data_source(table, False) for table in tables]
        super().__init__(
            data_sources,
            sample_builder,
            min_mel_length,
            max_mel_length
        )
        self._samples = []
        sample = self._next_sample()
        while sample is not None and len(self._samples) < max_num_samples:
            self._samples.append(sample)
            sample = self._next_sample()
        sys.stderr.write(f"#{get_rank()}: loaded {len(self._samples)} test samples\n")
        self._reader = None

    def __getitem__(self, index: int) -> TrainSample:
        return self._samples[index]

    def __len__(self):
        return len(self._samples)


class TtsDataset(TtsDatasetBase, IterableDataset):
    def __init__(
        self,
        tables: Sequence[Dict],
        sample_builder: TrainSampleBuilder,
        min_mel_length: Optional[int],
        max_mel_length: Optional[int],
        dynamic_batch_config: List[Dict[str, int]] = None,
        sample_group_key: Callable[[TrainSample], int] = None
    ):
        data_sources = [create_data_source(table, True) for table in tables]
        super().__init__(
            data_sources,
            sample_builder,
            min_mel_length,
            max_mel_length
        )
        self.sample_group_key = sample_group_key
        self._sample_key_bounds = self._batch_sizes = self._batches = None
        if dynamic_batch_config is not None:
            dynamic_batch_config.sort(key=lambda x: x["group_key"])
            self._sample_key_bounds = [item["group_key"] for item in dynamic_batch_config]
            self._batch_sizes = [item["batch_size"] for item in dynamic_batch_config]
            self._batches = [[] for _ in range(len(self._batch_sizes))]
        self.dynamic_batch_config = dynamic_batch_config

    def __iter__(self):
        self._init_reader()
        return self

    def __next__(self) -> Union[TrainSample, List[TrainSample]]:
        sample = self._next_sample()

        if sample is None:
            if self._batches is None:
                raise StopIteration
            # return remaining batches
            for i in range(len(self._batches)):
                if len(self._batches[i]) > 0:
                    batch = list(self._batches[i])
                    self._batches[i] = []
                    return batch
            raise StopIteration

        if self._batches is None:
            return sample

        while True:
            sample_key = self.sample_group_key(sample)
            group_idx = -1
            for i, bound in enumerate(self._sample_key_bounds):
                if sample_key < bound:
                    group_idx = i
                    break
            if group_idx != -1:
                self._batches[group_idx].append(sample)
                if len(self._batches[group_idx]) >= self._batch_sizes[group_idx]:
                    batch = list(self._batches[group_idx])
                    self._batches[group_idx] = []
                    return batch
            else:
                sys.stderr.write(f"skipping sample with key {sample_key}\n")
            sample = self._next_sample()
            if sample is None:
                raise StopIteration


class TtsDataModule:
    def __init__(
        self,
        data_config: Dict,
        sample_builder: TrainSampleBuilder,
        collate_fn: Callable,
        num_workers: int,
        pin_memory: bool
    ):
        # validation dataset
        val_data_config = data_config.get("val")
        if val_data_config:
            _, total_workers = workers_info()
            num_samples = val_data_config["max_num_samples"] // total_workers
            self.val_dataset = TtsInMemoryDataset(
                tables=val_data_config["tables"],
                sample_builder=sample_builder,
                max_num_samples=num_samples,
                min_mel_length=val_data_config.get("min_mel_length"),
                max_mel_length=val_data_config.get("max_mel_length")
            )
            self.val_batch_size = val_data_config["batch_size"]
        else:
            self.val_dataset = None

        # train dataset
        train_data_config = data_config["train"]
        self.train_dataset = TtsDataset(
            tables=train_data_config["tables"],
            sample_builder=sample_builder,
            min_mel_length=train_data_config.get("min_mel_length"),
            max_mel_length=train_data_config.get("max_mel_length"),
            dynamic_batch_config=train_data_config.get("dynamic_batch"),
            sample_group_key=lambda sample: sample.group_key
        )
        self.train_batch_size = 1
        if self.train_dataset.dynamic_batch_config is None:
            self.train_batch_size = train_data_config["batch_size"]
        self.prefetch_factor = train_data_config.get("prefetch_factor", 2)

        self.collate_fn = collate_fn
        self.num_workers = num_workers
        self.pin_memory = pin_memory

    def train_dataloader(self):
        return DataLoader(
            self.train_dataset,
            batch_size=self.train_batch_size,
            collate_fn=self.collate_fn,
            num_workers=self.num_workers,
            pin_memory=self.pin_memory,
            prefetch_factor=self.prefetch_factor
        )

    def val_dataloader(self):
        if self.val_dataset is not None:
            return DataLoader(
                self.val_dataset,
                batch_size=self.val_batch_size,
                collate_fn=self.collate_fn,
                pin_memory=self.pin_memory
            )
