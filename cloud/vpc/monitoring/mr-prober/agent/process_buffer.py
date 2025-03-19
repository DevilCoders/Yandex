import io
import os
import select
from typing import IO, List
import fcntl
import unittest.mock

import settings


class ProcessOutputBuffer:
    READ_CHUNK_SIZE = 1024 * 256  # 256 KiB
    BUFFER_SIZE = settings.AGENT_PROBER_MAX_OUTPUT_SIZE

    def __init__(self, source: IO[bytes]):
        self._data: List[bytes] = []
        self._total_read: int = 0
        self._input_sources = [source]

        # Mark file descriptor as non-blocking via calling `fcntl.fcntl()`.
        # NOTE: Unit tests use BytesIO or MagicMock as a `source`, while they have no `.fileno()` whish is required
        # for `fcntl.fcntl()`. At the same time, we don't need to mark BytesIO/MagicMock as non-blocking,
        # so just skip these lines.
        if not isinstance(source, io.BytesIO) and not isinstance(source, unittest.mock.MagicMock):
            fl = fcntl.fcntl(source, fcntl.F_GETFL)
            fcntl.fcntl(source, fcntl.F_SETFL, fl | os.O_NONBLOCK)

    def get_data(self):
        return b"".join(self._data)

    @property
    def total_read(self) -> int:
        return self._total_read

    def _read(self, source: IO):
        """
        The function reads one chunk from the source.

        NOTE: when the buffer is full, the source will be closed.
        """
        chunk_size = self.READ_CHUNK_SIZE
        if chunk_size + self._total_read > self.BUFFER_SIZE:
            chunk_size = self.BUFFER_SIZE - self._total_read

        data = source.read(chunk_size)
        self._total_read += len(data)
        self._data.append(data)

        if self._total_read == self.BUFFER_SIZE:
            # the agent will close the pipe immediately when the buffer is full.
            # The prober process will receive a SIGPIPE and should terminate.
            # NOTE: stdout and stderr are closed independently
            source.close()
            self._input_sources.pop()

    def read_once(self):
        """
        The function checks the source, and tries to read only the source is available for read.

        NOTE: when the buffer is full, the source will be closed.
        """
        readable, _, _ = select.select(self._input_sources, [], [], 0)
        for s in readable:
            self._read(s)
