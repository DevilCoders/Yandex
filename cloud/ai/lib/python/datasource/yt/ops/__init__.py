import os
import time
import typing
from datetime import datetime
from random import randrange

import yt.wrapper as yt
import yt.yson as yson

from cloud.ai.lib.python.datasource.yt.model import TableMeta, objects_to_rows, get_table_name
from cloud.ai.lib.python.log import get_logger
from cloud.ai.lib.python.serialization import OrderedYsonSerializable

tmp_table_expiration_timeout_ms = 1000 * 60 * 60 * 24 * 30  # 1 month, nearly as lifetime of Nirvana blocks logs

logger = get_logger(__name__)

retry_errors_codes = [
    402,  # cypress transaction lock conflict
    211,  # table has changed between taking input and output locks
]


class Table:
    meta: TableMeta
    name: str
    path: str

    def __init__(self, meta: TableMeta, name: str, client=yt):
        self.meta = meta
        self.name = name
        self.path = f'{self.meta.dir_path}/{name}'
        self.client = client

    def append_objects(self, rows: typing.List[OrderedYsonSerializable]):
        """
        Appends rows to destination YT table via merge of destination and temporary YT tables.
        """
        self.append_rows(objects_to_rows(rows))

    def append_rows(self, rows: typing.List[dict]):
        if len(rows) == 0:
            return

        path_tmp = self.client.create_temp_table(
            attributes=yson.json_to_yson(self.meta.attrs), expiration_timeout=tmp_table_expiration_timeout_ms
        )
        self.client.write_table(path_tmp, rows)
        logger.debug(f'created tmp table {path_tmp}')

        if not self.client.exists(self.path):
            self.client.create(
                'table', self.path, attributes=yson.json_to_yson(self.meta.attrs), recursive=True, ignore_existing=True
            )
            logger.debug(f'created table {self.path}')

        attempts_count = 16
        reattempt_timeout = randrange(1, 10)
        for attempt_index in range(attempts_count):
            try:
                self.client.run_merge(source_table=[path_tmp, self.path], destination_table=self.path)
                break
            except self.client.YtOperationFailedError as e:
                if any(e.find_matching_error(code=code) is not None for code in retry_errors_codes):
                    logger.warn(
                        f'merge tables failed due to concurrent transaction conflict, attempt '
                        f'#{attempt_index + 1}, sleep for {reattempt_timeout} seconds before next attempt'
                    )
                    time.sleep(reattempt_timeout)
                    reattempt_timeout *= 2
                else:
                    raise e

        logger.debug(f'tables ({path_tmp}, {self.path}) are merged into {self.path}')

    def read_rows(self) -> typing.Iterable[typing.Any]:
        return self.client.read_table(self.path)

    def remove(self):
        self.client.remove(self.path)

    @staticmethod
    def get_name(date: datetime) -> str:
        return get_table_name(date)


def configure_yt_wrapper(yt_proxy: str = None):
    if yt_proxy is None:
        yt_proxy = os.environ.get('YT_PROXY')
        if yt_proxy is None:
            yt_proxy = 'hahn'
    yt.config['proxy']['url'] = yt_proxy
    configure_parallelism()
    configure_retries_and_timeouts()


def configure_parallelism(max_thread_count: int = 8):
    yt.config["read_parallel"]["enable"] = True
    yt.config["read_parallel"]["max_thread_count"] = max_thread_count
    yt.config["write_parallel"]["enable"] = True
    yt.config["write_parallel"]["max_thread_count"] = max_thread_count


def configure_retries_and_timeouts():
    yt.config['proxy']['retries']['count'] *= 10
    yt.config['proxy']['connect_timeout'] *= 10
    yt.config['proxy']['request_timeout'] *= 10
    yt.config['proxy']['heavy_request_timeout'] *= 10
