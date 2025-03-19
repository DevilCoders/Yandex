from abc import ABC, abstractmethod
import os
from typing import Dict

import yt.wrapper as yt

from core.data.reader import (
    Reader,
    FsReader,
    YtReader
)
from core.data.utils import workers_info


class DataSource(ABC):
    def __init__(self, weight: float):
        self._weight = weight

    @abstractmethod
    def __iter__(self) -> Reader:
        pass

    @property
    def weight(self) -> float:
        return self._weight


class FsDataSource(DataSource):
    def __init__(self, table: Dict, infinite: bool):
        super().__init__(table.get("weight", 1))
        self.table = table
        self.infinite = infinite

    def __iter__(self) -> Reader:
        worker_id, total_workers = workers_info()
        table_file = open(f"{self.table['path']}/table", "rb")
        meta_file = open(f"{self.table['path']}/meta")
        samples_count = int(meta_file.readline())
        meta_file.seek(0)
        offset = samples_count // total_workers * worker_id
        reader = FsReader(table_file, meta_file, offset, self.infinite)
        return reader


class YtDataSource(DataSource):
    def __init__(self, table: Dict, infinite: bool):
        super().__init__(table.get("weight", 1))
        self.table = table
        self.infinite = infinite

    def __iter__(self) -> Reader:
        worker_id, total_workers = workers_info()
        cluster = self.table.get("cluster", "hahn")
        client = yt.YtClient(proxy=cluster, token=os.environ["YT_TOKEN"])
        row_count = client.row_count(self.table["path"])
        reader = YtReader(
            cluster=cluster,
            table=self.table["path"],
            offset=(row_count // total_workers) * worker_id,
            infinite=self.infinite
        )
        return reader


def create_data_source(table: Dict, infinite: bool) -> DataSource:
    if table["source"] == "yt":
        return YtDataSource(table, infinite)
    elif table["source"] == "fs":
        return FsDataSource(table, infinite)
    else:
        raise Exception(f"unknown data source: {table['source']}")
